/*
 *
 * Compiz Pixel Orbiter plugin
 *
 * pixelorbiter.h
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

#include <composite/composite.h>
#include <core/core.h>
#include <core/timer.h>
#include <mousepoll/mousepoll.h>
#include <opengl/opengl.h>

#include "pixelorbiter_options.h"

class PixelOrbiterScreen :
    public PluginClassHandler <PixelOrbiterScreen, CompScreen>,
    public PixelorbiterOptions,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface
{
    public:
	PixelOrbiterScreen (CompScreen *screen);
	~PixelOrbiterScreen ();

	void
	handleEvent (XEvent *event);

	void
	damageRegion (const CompRegion &r);

	bool
	glPaintOutput (const GLScreenPaintAttrib &attrib,
		       const GLMatrix	         &transform,
		       const CompRegion	         &region,
		       CompOutput	         *output,
		       unsigned int              mask);

    private:
	enum Phase {
	    LEFT,
	    UP,
	    RIGHT,
	    DOWN,
	    NUM_PHASES
	};

	void
	snapAxisOffsetToCursor (int *axisOffset, int axisSize,
				int axisCursorPos);

	void
	positionUpdate (const CompPoint &pos);

	void
	damageCursor ();

	bool
	orbit ();

	void
	loadCursor ();

	CompScreen *screen;
	CompositeScreen *cScreen;
	GLScreen *gScreen;

	int lastWidth;
	int lastHeight;

	bool fixesSupported;
	int fixesEventBase;
	bool canHideCursor;

	GLuint screenFbo;
	GLuint screenTexture;
	GLuint cursorTexture;
	GLenum target;

	CompSize cursorSize;
	// Cursor hot-spot point relative to cursor image origin.
	CompPoint cursorHotSpot;
	bool haveCursor;

	CompPoint cursorPos;

	CompTimer timer;
	Phase phase;
	int offsetX;
	int offsetY;

	MousePoller poller;
};

class PixelOrbiterPluginVTable :
    public CompPlugin::VTableForScreen <PixelOrbiterScreen>
{
    public:
	bool init ();
};
