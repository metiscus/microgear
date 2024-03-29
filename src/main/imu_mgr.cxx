//
// imu_mgr.hxx - front end IMU sensor management interface
//
// Written by Curtis Olson, curtolson <at> gmail <dot> com.  Spring 2009.
// This code is released into the public domain.
// 

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "include/ugear_config.h"

#include "comms/logging.h"
#include "comms/remote_link.h"
#include "comms/packetizer.hxx"
#include "include/globaldefs.h"
#include "main/globals.hxx"
#include "props/props.hxx"
#include "util/myprof.h"
#include "util/timing.h"

#include "sensors/imu_fgfs.hxx"
#ifdef ENABLE_MNAV_SENSOR
#  include "sensors/mnav.h"
#endif // ENABLE_MNAV_SENSOR
#include "sensors/imu_sf6DOFv4.h"
#include "sensors/imu_vn100.h"
#include "sensors/ugfile.h"

#include "imu_mgr.hxx"


//
// Global variables
//

static double imu_last_time = -31557600.0; // default to t minus one year old

static SGPropertyNode *imu_timestamp_node = NULL;

// comm property nodes
static SGPropertyNode *imu_console_skip = NULL;
static SGPropertyNode *imu_logging_skip = NULL;

static myprofile debug2a1;
static myprofile debug2a2;
	

void IMU_init() {
    debug2a1.set_name("debug2a1 IMU read");
    debug2a2.set_name("debug2a2 IMU console link");

    imu_timestamp_node = fgGetNode("/sensors/imu/time-stamp", true);

    // initialize comm nodes
    imu_console_skip = fgGetNode("/config/remote-link/imu-skip", true);
    imu_logging_skip = fgGetNode("/config/logging/imu-skip", true);

    // traverse configured modules
    SGPropertyNode *toplevel = fgGetNode("/config/sensors/imu-group", true);
    for ( int i = 0; i < toplevel->nChildren(); ++i ) {
	SGPropertyNode *section = toplevel->getChild(i);
	string name = section->getName();
	if ( name == "imu" ) {
	    string source = section->getChild("source", 0, true)->getStringValue();
            bool enabled = section->getChild("enable", 0, true)->getBoolValue();
            if ( !enabled ) {
                continue;
            }

	    string basename = "/sensors/";
	    basename += section->getDisplayName();
	    printf("i = %d  name = %s source = %s %s\n",
	           i, name.c_str(), source.c_str(), basename.c_str());
	    if ( source == "null" ) {
		// do nothing
	    } else if ( source == "fgfs" ) {
		fgfs_imu_init( basename, section );
	    } else if ( source == "file" ) {
		ugfile_imu_init( basename, section );
#ifdef ENABLE_MNAV_SENSOR
	    } else if ( source == "mnav" ) {
		mnav_imu_init( basename, section );
#endif // ENABLE_MNAV_SENSOR
	    } else if ( source == "sf6DOFv4" ) {
		sf_6DOFv4_imu_init( basename, section );
	    } else if ( source == "vn100" ) {
		imu_vn100_init( basename, section );
	    } else {
		printf("Unknown imu source = '%s' in config file\n",
		       source.c_str());
	    }
	}
    }
}


bool IMU_update() {
    debug2a1.start();

    imu_prof.start();

    bool fresh_data = false;

    // traverse configured modules
    SGPropertyNode *toplevel = fgGetNode("/config/sensors/imu-group", true);
    for ( int i = 0; i < toplevel->nChildren(); ++i ) {
	SGPropertyNode *section = toplevel->getChild(i);
	string name = section->getName();
	if ( name == "imu" ) {
	    string source = section->getChild("source", 0, true)->getStringValue();
            bool enabled = section->getChild("enable", 0, true)->getBoolValue();
            if ( !enabled ) {
                continue;
            }

	    // printf("i = %d  name = %s source = %s\n",
	    //        i, name.c_str(), source.c_str());
	    if ( source == "null" ) {
		// do nothing
	    } else if ( source == "fgfs" ) {
		fresh_data = fgfs_imu_update();
	    } else if ( source == "file" ) {
		ugfile_read();
		fresh_data = ugfile_get_imu();
#ifdef ENABLE_MNAV_SENSOR
	    } else if ( source == "mnav" ) {
		// read IMU until no data available.  This will flush any
		// potential backlog that could accumulate for any reason.
		mnav_start_nonblock_read();
		while ( mnav_read_nonblock() );
		fresh_data = mnav_get_imu();
#endif // ENABLE_MNAV_SENSOR
	    } else if ( source == "sf6DOFv4" ) {
		fresh_data = sf_6DOFv4_get_imu();
	    } else if ( source == "vn100" ) {
		fresh_data = imu_vn100_get();
	    } else {
		printf("Unknown imu source = '%s' in config file\n",
		       source.c_str());
	    }
	}
    }

    imu_prof.stop();
    debug2a1.stop();

    debug2a2.start();

    if ( fresh_data ) {
	// for computing imu data age
	imu_last_time = imu_timestamp_node->getDoubleValue();

	if ( remote_link_on || log_to_file ) {
	    uint8_t buf[256];
	    int size = packetizer->packetize_imu( buf );

	    if ( remote_link_on ) {
		remote_link_imu( buf, size, imu_console_skip->getIntValue() );
	    }

	    if ( log_to_file ) {
		log_imu( buf, size, imu_logging_skip->getIntValue() );
	    }
	}
    }

    debug2a2.stop();

    return fresh_data;
}


void IMU_close() {
    // traverse configured modules
    SGPropertyNode *toplevel = fgGetNode("/config/sensors/imu-group", true);
    for ( int i = 0; i < toplevel->nChildren(); ++i ) {
	SGPropertyNode *section = toplevel->getChild(i);
	string name = section->getName();
	if ( name == "imu" ) {
	    string source = section->getChild("source", 0, true)->getStringValue();
	    printf("i = %d  name = %s source = %s\n",
		   i, name.c_str(), source.c_str());
	    if ( source == "null" ) {
		// do nothing
	    } else if ( source == "fgfs" ) {
		fgfs_imu_close();
	    } else if ( source == "file" ) {
		ugfile_close();
#ifdef ENABLE_MNAV_SENSOR
	    } else if ( source == "mnav" ) {
		// nop
#endif // ENABLE_MNAV_SENSOR
	    } else if ( source == "sf6DOFv4" ) {
		sf_6DOFv4_close();
	    } else if ( source == "vn100" ) {
		imu_vn100_close();
	    } else {
		printf("Unknown imu source = '%s' in config file\n",
		       source.c_str());
	    }
	}
    }
}


double IMU_age() {
    return get_Time() - imu_last_time;
}
