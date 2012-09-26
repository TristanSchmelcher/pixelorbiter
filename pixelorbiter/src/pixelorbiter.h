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
#include <core/serialization.h>
#include <core/timer.h>
#include <mousepoll/mousepoll.h>
#include <opengl/opengl.h>

#include "pixelorbiter_options.h"

class PixelOrbiterScreen :
    public PluginClassHandler <PixelOrbiterScreen, CompScreen>,
    public PluginStateWriter <PixelOrbiterScreen>,
    public PixelorbiterOptions,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface
{
    public:
	PixelOrbiterScreen (CompScreen *screen);
	~PixelOrbiterScreen ();
	
	template <class Archive>
	void serialize (Archive &ar, const unsigned int version)
	{
	    // TODO
	}
	
	void
	postLoad ();

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

	GLuint screenTexture;
	GLuint cursorTexture;
	GLenum target;

	int cursorWidth;
	int cursorHeight;
	int cursorHotX;
	int cursorHotY;
	bool haveCursor;

	int cursorPosX;
	int cursorPosY;

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
