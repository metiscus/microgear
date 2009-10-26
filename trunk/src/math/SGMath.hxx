// Copyright (C) 2006  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SGMath_H
#define SGMath_H

/// Just include them all

#include <iosfwd>
// FIXME, make it compile on IRIX

//#include <osg/GL>
//#undef GLUT_APIENTRY_DEFINED // GL/glut.h undef APIENTRY when this symbol is defined. osg/GL defines it (?).
                             // This probably would work if we didn't use plib/pu.h that include GL/glut.h 
                             //  on its side.

#include <cmath>

#include "SGMathFwd.hxx"

#include "SGLimits.hxx"
#include "SGMisc.hxx"
#include "SGGeodesy.hxx"
#include "SGVec2.hxx"
#include "SGVec3.hxx"
#include "SGVec4.hxx"
#include "SGGeoc.hxx"
#include "SGGeod.hxx"
#include "SGQuat.hxx"
#include "SGMatrix.hxx"

#endif
