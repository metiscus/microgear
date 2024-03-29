// waypoint.cxx -- Class to hold data and return info relating to a waypoint
//
// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@hfrl.umn.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id: waypoint.cxx,v 1.10 2009/08/25 15:04:01 curt Exp $


#include <stdio.h>

#include <include/globaldefs.h>

#include <comms/logging.h>
#include <util/polar3d.hxx>

#include "waypoint.hxx"


// Constructor
SGWayPoint::SGWayPoint( const double lon, const double lat,
                        const double alt_m, const double agl_m,
                        const double speed_kt,
                        const double heading_deg, const double dist_m,
                        const modetype m, const string& s ) {
    target_lon = lon;
    target_lat = lat;
    target_alt_m = alt_m;
    target_agl_m = agl_m;
    target_speed_kt = speed_kt;
    offset_hdg_deg = heading_deg;
    offset_dist_m = dist_m;
    mode = m;
    id = s;
}

SGWayPoint::SGWayPoint( SGPropertyNode *node ):
    mode( SPHERICAL ),
    target_lon( 0.0 ),
    target_lat( 0.0 ),
    target_alt_m( -9999.9 ),
    target_agl_m( -9999.9 ),
    target_speed_kt( 0.0 ),
    offset_hdg_deg( 0.0 ),
    offset_dist_m( 0.0 ),
    distance( 0.0 ),
    id( "" )
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "id" ) {
            id = cval;
        } else if ( cname == "lon" ) {
            target_lon = child->getDoubleValue();
        } else if ( cname == "lat" ) {
            target_lat = child->getDoubleValue();
        } else if ( cname == "alt-ft" ) {
            target_alt_m = child->getDoubleValue() * SG_FEET_TO_METER;
        } else if ( cname == "agl-ft" ) {
            target_agl_m = child->getDoubleValue() * SG_FEET_TO_METER;
        } else if ( cname == "speed-kt" ) {
            target_speed_kt = child->getDoubleValue();
        } else if ( cname == "offset-heading-deg" ) {
            offset_hdg_deg = child->getDoubleValue();
        } else if ( cname == "offset-dist-m" ) {
            offset_dist_m = child->getDoubleValue();
        } else if ( cname == "mode" ) {
            if ( cval == "cartesian" ) {
                mode = CARTESIAN;
            } else {
                mode = SPHERICAL;
            }
        } else {
            printf("Error in waypoint config logic, " );
            if ( id.length() ) {
                printf("Section = %s", id.c_str() );
            }
        }
    }
    if ( fabs(offset_dist_m) < 0.1 ) {
	printf("WPT: %.6f %.6f %.0f (MSL) %.0f (AGL) %.0f (kts)\n",
	       target_lon, target_lat, target_alt_m, target_agl_m,
	       target_speed_kt);
    } else {
	printf("WPT: %4.0f deg %.0fm %.0f (MSL) %.0f (AGL) %.0f (kts)\n",
	       offset_hdg_deg, offset_dist_m, target_alt_m, target_agl_m,
	       target_speed_kt);
    }
}


SGWayPoint::SGWayPoint():
    mode( SPHERICAL ),
    target_lon( 0.0 ),
    target_lat( 0.0 ),
    target_alt_m( -9999.9 ),
    target_agl_m( -9999.9 ),
    target_speed_kt( 0.0 ),
    offset_hdg_deg( 0.0 ),
    offset_dist_m( 0.0 ),
    distance( 0.0 ),
    id( "" )
{
}


// Destructor
SGWayPoint::~SGWayPoint() {
}


// Calculate course and distances.  For WGS84 and SPHERICAL
// coordinates lat, lon, and course are in degrees, alt and distance
// are in meters.  For CARTESIAN coordinates x = lon, y = lat.  Course
// is in degrees and distance is in what ever units x and y are in.
void SGWayPoint::CourseAndDistance( const double cur_lon,
				    const double cur_lat,
				    const double cur_alt,
				    double *course, double *dist ) const {
    if ( mode == SPHERICAL ) {
	Point3D current( cur_lon * SGD_DEGREES_TO_RADIANS,
                         cur_lat * SGD_DEGREES_TO_RADIANS,
                         0.0 );
	Point3D target( target_lon * SGD_DEGREES_TO_RADIANS,
                        target_lat * SGD_DEGREES_TO_RADIANS,
                        0.0 );
	calc_gc_course_dist( current, target, course, dist );
	*course = 360.0 - *course * SGD_RADIANS_TO_DEGREES;
    } else if ( mode == CARTESIAN ) {
	double dx = target_lon - cur_lon;
	double dy = target_lat - cur_lat;
	*course = -atan2( dy, dx ) * SGD_RADIANS_TO_DEGREES - 90;
	while ( *course < 0 ) {
	    *course += 360.0;
	}
	while ( *course > 360.0 ) {
	    *course -= 360.0;
	}
	*dist = sqrt( dx * dx + dy * dy );
    }
}

// Calculate course and distances between two waypoints
void SGWayPoint::CourseAndDistance( const SGWayPoint &wp,
			double *course, double *dist ) const {
    CourseAndDistance( wp.get_target_lon(),
		       wp.get_target_lat(),
		       0.0 /* wp.get_target_alt_m() */ ,
		       course, dist );
}

/**
 * Update the target_lon and target_lat values of this waypoint
 * based on this waypoint's offset heading and offset distance
 * values.  The new target location is computed relative to the
 * provided reference point and reference heading.
 */
void SGWayPoint::update_relative_pos( const SGWayPoint &ref,
                                      const double ref_heading_deg )
{
     if ( mode == SPHERICAL ) {
         Point3D orig( ref.get_target_lon() * SGD_DEGREES_TO_RADIANS,
                       ref.get_target_lat() * SGD_DEGREES_TO_RADIANS,
                       0.0 );
         double course = ref_heading_deg + offset_hdg_deg;
         if ( course < 0.0 ) { course += 360.0; }
         if ( course > 360.0 ) { course -= 360.0; }
         course = 360.0 - course; // invert to make this routine happy
         Point3D tgt = calc_gc_lon_lat( orig,
                                        course * SGD_DEGREES_TO_RADIANS,
                                        offset_dist_m );
         target_lon = tgt.lon() * SGD_RADIANS_TO_DEGREES;
         target_lat = tgt.lat() * SGD_RADIANS_TO_DEGREES;

	 /*
	   FILE *debug = fopen("/mnt/mmc/debug.txt", "a");
	   fprintf(debug, "ref_hdg = %.1f offset=%.1f course=%.1f dist=%.1f\n",
                   ref_heading_deg, offset_hdg_deg,
                   360.0 - course, offset_dist_m);
           fprintf(debug, "ref = %.6f %.6f  new = %.6f %.6f\n",
                   ref.get_target_lon(), ref.get_target_lat(),
                   target_lon, target_lat);
           fclose(debug);
	 */
     } else if ( mode == CARTESIAN ) {
         // FIXME: update this code to work with cartesian systems too
     }
}
