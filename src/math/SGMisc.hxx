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

#ifndef SGMisc_H
#define SGMisc_H

template<typename T>
class SGMisc {
public:
  static T pi() { return T(3.1415926535897932384626433832795029L); }
  static T twopi() { return 2*T(3.1415926535897932384626433832795029L); }

  static T min(const T& a, const T& b)
  { return a < b ? a : b; }
  static T min(const T& a, const T& b, const T& c)
  { return min(min(a, b), c); }
  static T min(const T& a, const T& b, const T& c, const T& d)
  { return min(min(min(a, b), c), d); }
  static T max(const T& a, const T& b)
  { return a > b ? a : b; }
  static T max(const T& a, const T& b, const T& c)
  { return max(max(a, b), c); }
  static T max(const T& a, const T& b, const T& c, const T& d)
  { return max(max(max(a, b), c), d); }

  // clip the value of a to be in the range between and including _min and _max
  static T clip(const T& a, const T& _min, const T& _max)
  { return max(_min, min(_max, a)); }

  static int sign(const T& a)
  {
    if (a < -SGLimits<T>::min())
      return -1;
    else if (SGLimits<T>::min() < a)
      return 1;
    else
      return 0;
  }

  static T rad2deg(const T& val)
  { return val*180/pi(); }
  static T deg2rad(const T& val)
  { return val*pi()/180; }

  // normalize the value to be in a range between [min, max[
  static T
  normalizePeriodic(const T& min, const T& max, const T& value)
  {
    T range = max - min;
    if (range < SGLimits<T>::min())
      return min;
    T normalized = value - range*floor((value - min)/range);
    // two security checks that can only happen due to roundoff
    if (value <= min)
      return min;
    if (max <= normalized)
      return min;
    return normalized;
  }

  // normalize the angle to be in a range between [-pi, pi[
  static T
  normalizeAngle(const T& angle)
  { return normalizePeriodic(-pi(), pi(), angle); }

  // normalize the angle to be in a range between [0, 2pi[
  static T
  normalizeAngle2(const T& angle)
  { return normalizePeriodic(0, twopi(), angle); }

  static T round(const T& v)
  { return floor(v + T(0.5)); }
  static int roundToInt(const T& v)
  { return int(round(v)); }

#ifndef NDEBUG
  /// Returns true if v is a NaN value
  /// Use with care: allways code that you do not need to use that!
  static bool isNaN(const T& v)
  {
#ifdef HAVE_ISNAN
    return isnan(v);
#elif defined HAVE_STD_ISNAN
    return std::isnan(v);
#else
    // Use that every compare involving a NaN returns false
    // But be careful, some usual compiler switches like for example
    // -fast-math from gcc might optimize that expression to v != v which
    // behaves exactly like the opposite ...
    return !(v == v);
#endif
  }
#endif
};

#endif
