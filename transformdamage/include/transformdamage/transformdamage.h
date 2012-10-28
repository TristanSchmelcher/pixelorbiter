/*
 *
 * Compiz Transform Damage plugin
 *
 * transformdamage.h
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

#ifndef _COMPIZ_TRANSFORMDAMAGE_H
#define _COMPIZ_TRANSFORMDAMAGE_H

#define COMPIZ_TRANSFORMDAMAGE_ABI 1

#include <composite/composite.h>
#include <core/pluginclasshandler.h>
#include <core/screen.h>
#include <core/wrapsystem.h>

class TransformDamageScreen;

class TransformDamageScreenInterface : public WrapableInterface<
    TransformDamageScreen, TransformDamageScreenInterface>
{
    public:
	/**
	 * Hookable function to transform damage after damageRegion calls.
	 * (Ideally transformations would be applied by hooking damageRegion
	 * itself, but that confuses plugins that hook damageRegion later to
	 * figure out what is being invalidated (i.e., unityshell).)
	 */
	virtual void transformDamage (CompRegion &region);
};

class TransformDamageScreen :
    public WrapableHandler <TransformDamageScreenInterface, 1>,
    public PluginClassHandler <TransformDamageScreen, CompScreen,
	COMPIZ_TRANSFORMDAMAGE_ABI>,
    public CompositeScreenInterface
{
    public:
	TransformDamageScreen (CompScreen *screen);
	~TransformDamageScreen ();

	void damageRegion (const CompRegion &region);

	WRAPABLE_HND (0, TransformDamageScreenInterface, void,
	    transformDamage, CompRegion &);

    private:
	CompositeScreen *cScreen;
};

#endif
