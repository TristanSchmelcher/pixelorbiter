/*
 *
 * Compiz Pixel Orbiter plugin
 *
 * pixelorbiter.cpp
 *
 * Copyright : (C) 2012 by Tristan Schmelcher
 * E-mail    : tristan_schmelcher@alumni.uwaterloo.ca
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "pixelorbiter.h"

#include <algorithm>
#include <cassert>

COMPIZ_PLUGIN_20090315 (pixelorbiter, PixelOrbiterPluginVTable);

void
PixelOrbiterScreen::snapAxisOffsetToCursor (int *axisOffset, int axisSize,
					    int axisCursorPos)
{
    int margin = optionGetMouseMargin ();
    int maxAxisPos = axisSize - 1;
    int axisCursorPosNegativeMargin = std::max(axisCursorPos - margin, 0);
    int axisCursorPosPositiveMargin = std::min(axisCursorPos + margin, maxAxisPos);
    if (axisCursorPosNegativeMargin < -(*axisOffset))
    {
	*axisOffset = -axisCursorPosNegativeMargin;
    }
    else if (axisCursorPosPositiveMargin > (maxAxisPos - *axisOffset))
    {
	*axisOffset = maxAxisPos - axisCursorPosPositiveMargin;
    }
}

bool
PixelOrbiterScreen::updateOffsets (const Offsets &offsets)
{
    if (currentOffsets == offsets)
    {
	return false;
    }
    currentOffsets = offsets;
    cScreen->damageScreen ();
    return true;
}

bool
PixelOrbiterScreen::snapOffsetsToCursor (Offsets *offsets)
{
    snapAxisOffsetToCursor (&offsets->x, screen->width (), cursorPos.x ());
    snapAxisOffsetToCursor (&offsets->y, screen->height (), cursorPos.y ());
    return updateOffsets (*offsets);
}

void
PixelOrbiterScreen::positionUpdate (const CompPoint &pos)
{
    damageCursor ();
    cursorPos = poller.getCurrentPosition ();
    damageCursor ();

    switch (optionGetMouseBehaviour ())
    {
    case MouseBehaviourBounce:
	{
	    // Snap current offsets without changing desired offsets.
	    Offsets newOffsets (desiredOffsets);
	    if (snapOffsetsToCursor (&newOffsets))
	    {
		// Restart timer so that the snap back can't happen immediately.
		timer.start ();
	    }
	    break;
	}

    case MouseBehaviourPan:
	// Snap desired and current offsets. (In this mode, current always
	// tracks desired.)
	snapOffsetsToCursor (&desiredOffsets);
	break;

    case MouseBehaviourNone:
	// Nothing to do.
	break;

    default:
	assert (false);
    }
}

void
PixelOrbiterScreen::damageCursor ()
{
    if (!haveCursor)
    {
	return;
    }

    assert (!inDamageCursor);
    inDamageCursor = true;
    cScreen->damageRegion (CompRect (
	cursorPos.x () - cursorHotSpot.x (),
	cursorPos.y () - cursorHotSpot.y (),
	cursorSize.width (),
	cursorSize.height ()));
    inDamageCursor = false;
}

bool
PixelOrbiterScreen::orbit ()
{
    int maxOffset = optionGetMaxOffset ();
    int *desiredOffset;
    switch (phase) {
	case LEFT:
	case RIGHT:
	    desiredOffset = &desiredOffsets.x;
	    break;
	case UP:
	case DOWN:
	    desiredOffset = &desiredOffsets.y;
	    break;
	default:
	    assert (false);
	    return true;
    }
    bool phaseDone;
    switch (phase) {
	case DOWN:
	case RIGHT:
	    (*desiredOffset)++;
	    phaseDone = *desiredOffset >= maxOffset;
	    break;
	case UP:
	case LEFT:
	    (*desiredOffset)--;
	    phaseDone = *desiredOffset <= -maxOffset;
	    break;
	default:
	    assert (false);
	    return true;
    }
    if (phaseDone)
    {
	phase = static_cast<Phase>((phase + 1) % NUM_PHASES);
    }
    updateOffsets (desiredOffsets);
    return true;
}

void
PixelOrbiterScreen::loadCursor ()
{
    assert (fixesSupported);

    damageCursor ();

    haveCursor = false;

    XFixesCursorImage *image = XFixesGetCursorImage (screen->dpy ());
    if (!image) {
	compLogMessage ("pixelorbiter", CompLogLevelWarn,
	    "Failed to get cursor image");
	return;
    }

    cursorSize.setWidth(image->width);
    cursorSize.setHeight(image->height);
    cursorHotSpot.setX(image->xhot);
    cursorHotSpot.setY(image->yhot);

    unsigned char *pixels = new unsigned char[
	cursorSize.width () * cursorSize.height () * 4];
    if (!pixels)
    {
	compLogMessage ("pixelorbiter", CompLogLevelWarn, "OOM");
	XFree (image);
	return;
    }

    for (int i = 0; i < cursorSize.width () * cursorSize.height (); i++)
    {
	unsigned long pix = image->pixels[i];
	pixels[i * 4] = pix & 0xff;
	pixels[(i * 4) + 1] = (pix >> 8) & 0xff;
	pixels[(i * 4) + 2] = (pix >> 16) & 0xff;
	pixels[(i * 4) + 3] = (pix >> 24) & 0xff;
    }

    XFree (image);

    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, cursorTexture);
    // Default is linear filtering and clamp-to-edge, which is what we want.
    glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, cursorSize.width (),
		  cursorSize.height (), 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);

    delete [] pixels;

    haveCursor = true;

    damageCursor ();
}

void
PixelOrbiterScreen::handleEvent (XEvent *event)
{
    if (fixesSupported && event->type == fixesEventBase + XFixesCursorNotify)
    {
	loadCursor ();
    }

    screen->handleEvent (event);
}

static void
expandVertBorderDamage (const CompRegion &origRegion,
			CompRegion &region,
			int borderX,
			int borderHeight,
			int expansionX,
			int expansionWidth)
{
    CompRegion border (origRegion & CompRect(
	borderX, 0, 1, borderHeight));
    foreach (CompRect rect, border.rects ())
    {
	rect.setX (expansionX);
	rect.setWidth (expansionWidth);
	region += rect;
    }
}

static void
expandHorizBorderDamage (const CompRegion &origRegion,
			 CompRegion &region,
			 int borderY,
			 int borderWidth,
			 int expansionY,
			 int expansionHeight)
{
    CompRegion border (origRegion & CompRect(
	0, borderY, borderWidth, 1));
    foreach (CompRect rect, border.rects ())
    {
	rect.setY (expansionY);
	rect.setHeight (expansionHeight);
	region += rect;
    }
}

static void
expandBorderDamage(const CompRegion &origRegion,
		   CompRegion &region,
		   int offset,
		   int borderLength,
		   int axisLength,
		   void (*expand)(const CompRegion &origRegion,
			 	  CompRegion &region,
			 	  int borderPos,
			 	  int borderLength,
			 	  int expansionPos,
			 	  int expansionSize))
{
    if (offset > 0)
    {
	(*expand) (origRegion, region,
	    offset,
	    borderLength,
	    0,
	    offset);
    }
    else if (offset < 0)
    {
	(*expand) (origRegion, region,
	    axisLength - 1 + offset,
	    borderLength,
	    axisLength + offset,
	    -offset);
    }
}

static void
computeCornerInfo (int offset, int axisLength, int *cornerPos,
		   int *expansionPos, int *expansionSize)
{
    if (offset > 0)
    {
	*cornerPos = offset;
	*expansionPos = 0;
	*expansionSize = offset;
    }
    else
    {
	*cornerPos = axisLength - 1 + offset;
	*expansionPos = axisLength + offset;
	*expansionSize = -offset;
    }
}

void
PixelOrbiterScreen::expandDamage (CompRegion &region)
{
    // Backup the translated region.
    CompRegion origRegion (region);

    // Expand the damage on the visible horizontally-shifted vertical border, if
    // any.
    expandBorderDamage (origRegion, region, currentOffsets.x, screen->height (),
	screen->width (), &expandVertBorderDamage);
    // Expand the damage on the visible vertically-shifted horizontal border, if
    // any.
    expandBorderDamage (origRegion, region, currentOffsets.y, screen->width (),
	screen->height (), &expandHorizBorderDamage);

    // Expand the damage on the visible horizontally- and vertically-shifted
    // corner, if any.
    if (currentOffsets.x != 0 && currentOffsets.y != 0)
    {
	int cornerX;
	int expansionX;
	int expansionWidth;
	computeCornerInfo (currentOffsets.x, screen->width (), &cornerX,
	    &expansionX, &expansionWidth);

	int cornerY;
	int expansionY;
	int expansionHeight;
	computeCornerInfo (currentOffsets.y, screen->height (), &cornerY,
	    &expansionY, &expansionHeight);

	if (origRegion.contains (CompPoint (cornerX, cornerY)))
	{
	    region += CompRect (expansionX, expansionY, expansionWidth,
		expansionHeight);
	}
    }
}

void
PixelOrbiterScreen::transformDamage (CompRegion &region)
{
    // Translate by the offset.
    region.translate (currentOffsets.x, currentOffsets.y);
    region &= screen->region ();

    // Since the cursor is drawn post-FBO, there is no texture coordinate
    // clamping going on, so no damage expansion.
    if (!inDamageCursor) {
	// Expand the region to include areas at the borders or corners that will
	// also be modified due to texture coordinate clamping. 
	expandDamage (region);
    }

    tScreen->transformDamage (region);
}

static void rectToPoints(const CompRect &quad, CompPoint points[4])
{
    points[0] = CompPoint (quad.x1 (), quad.y1 ());  // Top-left
    points[1] = CompPoint (quad.x1 (), quad.y2 ());  // Bottom-left
    points[2] = CompPoint (quad.x2 (), quad.y2 ());  // Bottom-right
    points[3] = CompPoint (quad.x2 (), quad.y1 ());  // Top-right
}

static void emitTexturedQuad (const CompRect &screenQuad,
			      const CompRect &texQuad)
{
    CompPoint vertices[4];
    CompPoint texCoords[4];
    rectToPoints (screenQuad, vertices);
    rectToPoints (texQuad, texCoords);
    for (int i = 0; i < 4; i++) {
	glTexCoord2i (texCoords[i].x (), texCoords[i].y ());
	glVertex2i (vertices[i].x (), vertices[i].y ());
    }
}

bool
PixelOrbiterScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				   const GLMatrix	     &transform,
				   const CompRegion	     &region,
				   CompOutput	             *output,
				   unsigned int	             mask)
{
    CompRegion r (region);
    r.translate (-currentOffsets.x, -currentOffsets.y);
    r &= screen->region ();

    GLint last_fbo = 0;
    glGetIntegerv (GL_FRAMEBUFFER_BINDING, &last_fbo);

    (*GL::bindFramebuffer) (GL_FRAMEBUFFER_EXT, screenFbo);

    if (screen->width () != lastWidth || screen->height () != lastHeight)
    {
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screenTexture);
	glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, screen->width (),
	    screen->height (), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	(*GL::framebufferTexture2D) (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
	    GL_TEXTURE_RECTANGLE_ARB, screenTexture, 0);
	GLenum status = (*GL::checkFramebufferStatus) (GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
	    compLogMessage ("pixelorbiter", CompLogLevelError, "FBO incomplete! status = %d",
		status);
	}
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
	lastWidth = screen->width ();
	lastHeight = screen->height ();
    }

    bool status = gScreen->glPaintOutput (attrib, transform, r, output, mask);

    (*GL::bindFramebuffer) (GL_FRAMEBUFFER_EXT, last_fbo);

    GLMatrix sTransform = transform;
    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
    glPushMatrix ();
    glLoadMatrixf (sTransform.getMatrix ());
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screenTexture);
    glBegin (GL_QUADS);
    foreach (CompRect rect, region.rects ())
    {
	// GL puts the origin for the framebuffer texture at the bottom-left,
	// so the texture Y-coordinates here are inverted.
	emitTexturedQuad (rect, CompRect (
	    -currentOffsets.x + rect.x (),
	    screen->height () + currentOffsets.y - rect.y (),
	    rect.width (),
	    -rect.height ()));
    }
    glEnd ();

    if (haveCursor)
    {
	// Draw the cursor.
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, cursorTexture);
	glEnable (GL_BLEND);
	glBegin (GL_QUADS);
	emitTexturedQuad (
	    CompRect (
		cursorPos.x () + currentOffsets.x - cursorHotSpot.x (),
		cursorPos.y () + currentOffsets.y - cursorHotSpot.y (),
		cursorSize.width (),
		cursorSize.height ()),
	    CompRect (0, 0, cursorSize.width (), cursorSize.height ()));
	glEnd ();
	glDisable (GL_BLEND);
    }
    else
    {
	compLogMessage ("pixelorbiter", CompLogLevelWarn, "No cursor to draw!");
    }

    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);
    glPopMatrix ();

    return status;
}

void
PixelOrbiterScreen::intervalChanged ()
{
    int interval = optionGetInterval ();
    timer.setTimes (interval, interval * 1.5);
}

PixelOrbiterScreen::PixelOrbiterScreen (CompScreen *s) :
    PluginClassHandler <PixelOrbiterScreen, CompScreen> (s),
    screen (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    tScreen (TransformDamageScreen::get (screen)),
    lastWidth (0),
    lastHeight (0),
    fixesSupported (false),
    fixesEventBase (0),
    canHideCursor (false),
    screenFbo (0),
    screenTexture (0),
    cursorTexture (0),
    haveCursor (false),
    inDamageCursor (false),
    phase (LEFT)
{
    if (!GL::fboSupported)
    {
	compLogMessage ("pixelorbiter", CompLogLevelError, 
	    "FBO support is required");
	return;
    }

    ScreenInterface::setHandler (screen, false);
    GLScreenInterface::setHandler (gScreen, false);
    TransformDamageScreenInterface::setHandler (tScreen, false);

    int unused, major, minor;
    fixesSupported = XFixesQueryExtension(screen->dpy (), &fixesEventBase,
	&unused) && XFixesQueryVersion (screen->dpy (), &major, &minor);
    canHideCursor = fixesSupported && major >= 4;

    if (fixesSupported) {
	XFixesSelectCursorInput (screen->dpy (), screen->root (),
	    XFixesDisplayCursorNotifyMask);
    }

    if (canHideCursor) {
	XFixesHideCursor (screen->dpy (), screen->root ());
    }

    poller.setCallback (boost::bind (&PixelOrbiterScreen::positionUpdate, this,
	_1));
    poller.start ();
    cursorPos = poller.getCurrentPosition ();

    (*GL::genFramebuffers) (1, &screenFbo);
    GLuint textures[2];
    glGenTextures (2, textures);
    screenTexture = textures[0];
    cursorTexture = textures[1];

    screen->handleEventSetEnabled (this, true);
    gScreen->glPaintOutputSetEnabled (this, true);
    tScreen->transformDamageSetEnabled (this, true);

    loadCursor ();

    optionSetIntervalNotify (boost::bind (&PixelOrbiterScreen::intervalChanged,
	this));

    intervalChanged ();
    timer.setCallback (boost::bind (&PixelOrbiterScreen::orbit, this));
    timer.start ();
}

PixelOrbiterScreen::~PixelOrbiterScreen ()
{
    if (!GL::fboSupported)
    {
	return;
    }

    timer.stop ();

    GLuint textures[2];
    textures[0] = screenTexture;
    textures[1] = cursorTexture;
    glDeleteTextures (2, textures);
    (*GL::deleteFramebuffers) (1, &screenFbo);

    poller.stop ();

    if (canHideCursor) {
	XFixesShowCursor (screen->dpy (), screen->root ());
    }

    if (fixesSupported) {
	// This can conflict with ezoom. The SelectCursorInput calls should be
	// factored out into a utility plugin.
	XFixesSelectCursorInput (screen->dpy (), screen->root (), 0);
    }

    if (currentOffsets.x != 0 || currentOffsets.y != 0)
    {
	cScreen->damageScreen ();
    }
}

bool
PixelOrbiterPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI) ||
	!CompPlugin::checkPluginABI ("transformdamage",
	    COMPIZ_TRANSFORMDAMAGE_ABI))
	return false;

    return true;
}
