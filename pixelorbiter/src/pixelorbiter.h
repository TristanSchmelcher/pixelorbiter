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
#include <transformdamage/transformdamage.h>

#include "pixelorbiter_options.h"

class PixelOrbiterScreen :
    public PluginClassHandler <PixelOrbiterScreen, CompScreen>,
    public PixelorbiterOptions,
    public ScreenInterface,
    public GLScreenInterface,
    public TransformDamageScreenInterface
{
    public:
	PixelOrbiterScreen (CompScreen *screen);
	~PixelOrbiterScreen ();

	void handleEvent (XEvent *event);

	void transformDamage (CompRegion &region);

	bool glPaintOutput (const GLScreenPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    CompOutput		      *output,
			    unsigned int	      mask);

    private:
	enum Phase
	{
	    LEFT,
	    UP,
	    RIGHT,
	    DOWN,
	    NUM_PHASES
	};

	struct Offsets
	{
	    Offsets () : x(0), y(0) {}

	    bool operator!= (const Offsets &that) const
	    {
		return x != that.x || y != that.y;
	    }

	    bool operator== (const Offsets &that) const
	    {
		return x == that.x && y == that.y;
	    }

	    int x;
	    int y;
	};

	void snapAxisOffsetToCursor (int *axisOffset, int axisSize,
				     int axisCursorPos);

	bool updateOffsets (const Offsets &offsets);

	bool snapOffsetsToCursor (Offsets *offsets);

	void positionUpdate (const CompPoint &pos);

	void damageCursor ();

	bool orbit ();

	void loadCursor ();

	void expandDamage (CompRegion &region);

	void intervalChanged ();

	CompScreen *screen;
	CompositeScreen *cScreen;
	GLScreen *gScreen;
	TransformDamageScreen *tScreen;

	int lastWidth;
	int lastHeight;

	bool fixesSupported;
	int fixesEventBase;
	bool canHideCursor;

	GLuint screenFbo;
	GLuint screenTexture;
	GLuint cursorTexture;

	CompSize cursorSize;
	// Cursor hot-spot point relative to cursor image origin.
	CompPoint cursorHotSpot;
	bool haveCursor;
	bool inDamageCursor;

	CompPoint cursorPos;

	CompTimer timer;
	Phase phase;
	Offsets desiredOffsets;
	Offsets currentOffsets;

	MousePoller poller;
};

class PixelOrbiterPluginVTable :
    public CompPlugin::VTableForScreen <PixelOrbiterScreen>
{
    public:
	bool init ();
};
