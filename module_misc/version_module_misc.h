/***********************************************************************
 * This file is part of module_misc.
 *
 * module_misc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * module_misc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with module_misc.  If not, see <http://www.gnu.org/licenses/>
 * 
 * 
 * Copyright (C) 2015, Dominik Rueß; info@dominik-ruess.de
 **********************************************************************/

#ifndef VERSION_MODULE_MISC_H
#define VERSION_MODULE_MISC_H

#include "versioning.h"
#include "patchnumber.h"

template<int major, int minor, int patch>
struct VersionNumberMisc : public VersionNumber<major, minor, patch>
{};

extern VersionNumberMisc<1, 2, DR_PATCH_NUMBER> version_module_misc;

#endif // VERSION_MODULE_MISC_H
