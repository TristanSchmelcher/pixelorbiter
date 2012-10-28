/*
 *
 * Compiz Transform Damage plugin
 *
 * transformdamage.cpp
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

#include <transformdamage/transformdamage.h>

class TransformDamagePluginVTable :
    public CompPlugin::VTableForScreen <TransformDamageScreen>
{
    public:
	bool init ();
	void fini ();
};

COMPIZ_PLUGIN_20090315 (transformdamage, TransformDamagePluginVTable);

void
TransformDamageScreenInterface::transformDamage (CompRegion &region)
    WRAPABLE_DEF (transformDamage, region)

void
TransformDamageScreen::damageRegion (const CompRegion &region)
{
    CompRegion transformedRegion (region);
    transformDamage (transformedRegion);
    cScreen->damageRegion (transformedRegion);
}

void
TransformDamageScreen::transformDamage (CompRegion &region)
{
    WRAPABLE_HND_FUNCTN (transformDamage, region);
}

TransformDamageScreen::TransformDamageScreen (CompScreen *s) :
    PluginClassHandler <TransformDamageScreen, CompScreen,
	COMPIZ_TRANSFORMDAMAGE_ABI> (s),
    cScreen (CompositeScreen::get (screen))
{
    CompositeScreenInterface::setHandler (cScreen, false);
    cScreen->damageRegionSetEnabled (this, true);
}

TransformDamageScreen::~TransformDamageScreen ()
{
}

bool
TransformDamagePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	return false;

    CompPrivate p;
    p.uval = COMPIZ_TRANSFORMDAMAGE_ABI;
    screen->storeValue ("transformdamage_ABI", p);

    return true;
}

void
TransformDamagePluginVTable::fini ()
{
    screen->eraseValue ("transformdamage_ABI");
}
