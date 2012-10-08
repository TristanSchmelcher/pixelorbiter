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

#include <cassert>

COMPIZ_PLUGIN_20090315 (pixelorbiter, PixelOrbiterPluginVTable);

// TODO: Use options.
#define TIMEOUT 60000
#define MAX_OFFSET 10

void
PixelOrbiterScreen::positionUpdate (const CompPoint &pos)
{
    damageCursor ();
    cursorPosX = poller.getCurrentPosition ().x ();
    cursorPosY = poller.getCurrentPosition ().y ();
    damageCursor ();

    if (cursorPosX < -offsetX)
    {
	offsetX = -cursorPosX;
	cScreen->damageScreen ();
    }
    else if (cursorPosX > (screen->width () - 1 - offsetX))
    {
	offsetX = screen->width () - 1 - cursorPosX;
	cScreen->damageScreen ();
    }

    if (cursorPosY < -offsetY)
    {
	offsetY = -cursorPosY;
	cScreen->damageScreen ();
    }
    else if (cursorPosY > (screen->height () - 1 - offsetY))
    {
	offsetY = screen->height () - 1 - cursorPosY;
	cScreen->damageScreen ();
    }
}

void
PixelOrbiterScreen::damageCursor ()
{
    if (!haveCursor)
    {
	return;
    }

    cScreen->damageRegionSetEnabled (this, false);
    cScreen->damageRegion (CompRect (
	cursorPosX + offsetX - cursorHotX, cursorPosY + offsetY - cursorHotY,
	cursorWidth, cursorHeight));
    cScreen->damageRegionSetEnabled (this, true);
}

bool
PixelOrbiterScreen::orbit ()
{
    int *offset;
    switch (phase) {
	case LEFT:
	case RIGHT:
	    offset = &offsetX;
	    break;
	case UP:
	case DOWN:
	    offset = &offsetY;
	    break;
	default:
	    assert (false);
    }
    bool phaseDone;
    switch (phase) {
	case DOWN:
	case RIGHT:
	    (*offset)++;
	    phaseDone = *offset >= MAX_OFFSET;
	    break;
	case UP:
	case LEFT:
	    (*offset)--;
	    phaseDone = *offset <= -MAX_OFFSET;
	    break;
	default:
	    assert (false);
    }
    if (phaseDone)
    {
	phase = static_cast<Phase>((phase + 1) % NUM_PHASES);
    }
    cScreen->damageScreen ();
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

    cursorWidth = image->width;
    cursorHeight = image->height;
    cursorHotX = image->xhot;
    cursorHotY = image->yhot;

    unsigned char *pixels = new unsigned char[cursorWidth * cursorHeight * 4];
    if (!pixels)
    {
	compLogMessage ("pixelorbiter", CompLogLevelWarn, "OOM");
	XFree (image);
	return;
    }

    for (int i = 0; i < cursorWidth * cursorHeight; i++)
    {
	unsigned long pix = image->pixels[i];
	pixels[i * 4] = pix & 0xff;
	pixels[(i * 4) + 1] = (pix >> 8) & 0xff;
	pixels[(i * 4) + 2] = (pix >> 16) & 0xff;
	pixels[(i * 4) + 3] = (pix >> 24) & 0xff;
    }

    XFree (image);

    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, cursorTexture);
    // Default is linear filtering and clamp-to-edge, which is what we want.
    glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, cursorWidth,
		  cursorHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);

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

void
PixelOrbiterScreen::damageRegion (const CompRegion &region)
{
    CompRegion r (region);
    r.translate (offsetX, offsetY);

    if (offsetX > 0)
    {
	CompRegion leftBorder (r & CompRect(
	    offsetX, offsetY, 1, screen->height ()));
	foreach (CompRect rect, leftBorder.rects ())
	{
	    rect.setX (0);
	    rect.setWidth (offsetX);
	    r |= rect;
	}
    }
    else if (offsetX < 0)
    {
	CompRegion rightBorder (r & CompRect(
	    screen->width () - 1 + offsetX, offsetY, 1, screen->height ()));
	foreach (CompRect rect, rightBorder.rects ())
	{
	    rect.setX (screen->width () + offsetX);
	    rect.setWidth (-offsetX);
	    r |= rect;
	}
    }

    if (offsetY > 0)
    {
	CompRegion topBorder (r & CompRect(
	    offsetX, offsetY, screen->width (), 1));
	foreach (CompRect rect, topBorder.rects ())
	{
	    rect.setY (0);
	    rect.setHeight (offsetY);
	    r |= rect;
	}
    }
    else if (offsetY < 0)
    {
	CompRegion bottomBorder (r & CompRect(
	    offsetX, screen->height () - 1 + offsetY, screen->width (), 1));
	foreach (CompRect rect, bottomBorder.rects ())
	{
	    rect.setY (screen->height () + offsetY);
	    rect.setHeight (-offsetY);
	    r |= rect;
	}
    }

    if (offsetX > 0 && offsetY > 0 && r.contains (CompPoint (
	offsetX, offsetY)))
    {
	// Top-left corner.
	r |= CompRect (0, 0, offsetX, offsetY);
    }

    if (offsetX > 0 && offsetY < 0 && r.contains (CompPoint (
	offsetX, screen->height () - 1 + offsetY)))
    {
	// Bottom-left corner.
	r |= CompRect (0, screen->height () + offsetY, offsetX, -offsetY);
    }

    if (offsetX < 0 && offsetY > 0 && r.contains (CompPoint (
	screen->width () - 1 + offsetX, offsetY)))
    {
	// Top-right corner.
	r |= CompRect (screen->width () + offsetX, 0,
	    -offsetX, offsetY);
    }

    if (offsetX < 0 && offsetY < 0 && r.contains (CompPoint (
	screen->width () - 1 + offsetX, screen->height () - 1 + offsetY)))
    {
	// Bottom-right corner.
	r |= CompRect (screen->width () + offsetX, screen->height () + offsetY,
	    -offsetX, -offsetY);
    }

    r &= screen->region ();
    cScreen->damageRegion (r);
}

bool
PixelOrbiterScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				   const GLMatrix	     &transform,
				   const CompRegion	     &region,
				   CompOutput	             *output,
				   unsigned int	             mask)
{
    CompRegion r (region);
    r.translate (-offsetX, -offsetY);
    r &= screen->region ();

    bool status = gScreen->glPaintOutput (attrib, transform, r, output, mask);

    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screenTexture);
    if (screen->width () != lastWidth || screen->height () != lastHeight)
    {
	glCopyTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, 0, 0,
	    screen->width (), screen->height (), 0);
	lastWidth = screen->width ();
	lastHeight = screen->height ();
    }
    else
    {
	foreach (CompRect rect, r.rects ())
	{
	    glCopyTexSubImage2D (GL_TEXTURE_RECTANGLE_ARB, 0,
		rect.x (), screen->height () - rect.y2 (),
		rect.x (), screen->height () - rect.y2 (),
		rect.width (), rect.height ());
	}
    }

    GLMatrix sTransform = transform;
    sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);
    glPushMatrix ();
    glLoadMatrixf (sTransform.getMatrix ());
    // Left
    GLfloat t_x1 = 0.0 - offsetX;
    // Right
    GLfloat t_x2 = screen->width () - offsetX;
    // GL puts the origin for glCopyTexImage2D at the bottom-left, so the
    // texture coordinates are inverted relative to the vertices.
    // Bottom
    GLfloat t_y1 = 0.0 + offsetY;
    // Top
    GLfloat t_y2 = screen->height () + offsetY;
    // Left
    GLfloat v_x1 = 0;
    // Right
    GLfloat v_x2 = screen->width ();
    // Bottom
    GLfloat v_y1 = screen->height ();
    // Top
    GLfloat v_y2 = 0;
    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screenTexture);
    glEnable (GL_SCISSOR_TEST);
    foreach (CompRect rect, region.rects ())
    {
	glScissor (rect.x (), screen->height () - rect.y2 (),
	    rect.width (), rect.height ());
	glBegin (GL_QUADS);
	glTexCoord2f (t_x1, t_y2);  // Top-left
	glVertex2f (v_x1, v_y2);
	glTexCoord2f (t_x1, t_y1);  // Bottom-left
	glVertex2f (v_x1, v_y1);
	glTexCoord2f (t_x2, t_y1);  // Bottom-right
	glVertex2f (v_x2, v_y1);
	glTexCoord2f (t_x2, t_y2);  // Top-right
	glVertex2f (v_x2, v_y2);
	glEnd ();
    }
    glDisable (GL_SCISSOR_TEST);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);

    if (haveCursor)
    {
	// Draw the cursor.
	GLfloat t_x1 = 0.0;
	GLfloat t_x2 = cursorWidth;
	GLfloat t_y1 = cursorHeight;
	GLfloat t_y2 = 0.0;
	GLfloat v_x1 = cursorPosX + offsetX - cursorHotX;
	GLfloat v_x2 = cursorPosX + offsetX - cursorHotX + cursorWidth;
	GLfloat v_y1 = cursorPosY + offsetY - cursorHotY + cursorHeight;
	GLfloat v_y2 = cursorPosY + offsetY - cursorHotY;
	glEnable (GL_TEXTURE_RECTANGLE_ARB);
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, cursorTexture);
	glEnable (GL_BLEND);
	glBegin (GL_QUADS);
	glTexCoord2f (t_x1, t_y2);
	glVertex2f (v_x1, v_y2);
	glTexCoord2f (t_x1, t_y1);
	glVertex2f (v_x1, v_y1);
	glTexCoord2f (t_x2, t_y1);
	glVertex2f (v_x2, v_y1);
	glTexCoord2f (t_x2, t_y2);
	glVertex2f (v_x2, v_y2);
	glEnd ();
	glDisable (GL_BLEND);
	glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
	glDisable (GL_TEXTURE_RECTANGLE_ARB);
    }
    else
    {
	compLogMessage ("pixelorbiter", CompLogLevelWarn, "No cursor to draw!");
    }

    glPopMatrix ();

    return status;
}

void
PixelOrbiterScreen::postLoad ()
{
    // TODO
}

PixelOrbiterScreen::PixelOrbiterScreen (CompScreen *s) :
    PluginClassHandler <PixelOrbiterScreen, CompScreen> (s),
    PluginStateWriter <PixelOrbiterScreen> (this, s->root ()),
    screen (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    lastWidth (0),
    lastHeight (0),
    fixesSupported (false),
    fixesEventBase (0),
    canHideCursor (false),
    screenTexture (0),
    cursorTexture (0),
    cursorWidth (0),
    cursorHeight (0),
    cursorHotX (0),
    cursorHotY (0),
    haveCursor (false),
    cursorPosX (0),
    cursorPosY (0),
    phase (LEFT),
    offsetX (10),
    offsetY (10)
{
    ScreenInterface::setHandler (screen, false);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

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
    cursorPosX = poller.getCurrentPosition ().x ();
    cursorPosY = poller.getCurrentPosition ().y ();

    GLuint textures[2];
    glGenTextures (2, textures);
    screenTexture = textures[0];
    cursorTexture = textures[1];

    glEnable (GL_TEXTURE_RECTANGLE_ARB);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, screenTexture);
    // Default is linear filtering and clamp-to-edge, which is what we want.
    glTexImage2D (GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB, 0, 0, 0,
		  GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture (GL_TEXTURE_RECTANGLE_ARB, 0);
    glDisable (GL_TEXTURE_RECTANGLE_ARB);

    cScreen->damageScreen ();
    screen->handleEventSetEnabled (this, true);
    cScreen->damageRegionSetEnabled (this, true);
    gScreen->glPaintOutputSetEnabled (this, true);

    loadCursor ();

    timer.setTimes (TIMEOUT, TIMEOUT * 1.5);
    timer.setCallback (boost::bind (&PixelOrbiterScreen::orbit, this));
    timer.start ();
}

PixelOrbiterScreen::~PixelOrbiterScreen ()
{
    writeSerializedData ();

    timer.stop ();

    GLuint textures[2];
    textures[0] = screenTexture;
    textures[1] = cursorTexture;
    glDeleteTextures (2, textures);

    poller.stop ();

    if (canHideCursor) {
	XFixesShowCursor (screen->dpy (), screen->root ());
    }

    if (fixesSupported) {
	XFixesSelectCursorInput (screen->dpy (), screen->root (), 0);
    }

    cScreen->damageScreen ();
}

bool
PixelOrbiterPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;

    return true;
}
