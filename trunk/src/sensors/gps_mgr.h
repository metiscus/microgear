/**
 * \file: gps_mgr.h
 *
 * Front end management interface for reading GPS data.
 *
 * Copyright (C) 2009 - Curtis L. Olson curtolson@gmail.com
 *
 * $Id: gps_mgr.h,v 1.1 2009/04/20 01:53:02 curt Exp $
 */


#ifndef _UGEAR_GPS_MGR_H
#define _UGEAR_GPS_MGR_H


extern struct gps gpspacket;

enum gps_source_t {
    gpsNone,
    gpsGPSD,
    gpsMNAV,
    gpsUGFile
};

void GPS_init();
bool GPS_update();
void GPS_close();


#endif // _UGEAR_GPS_MGR_H