/**
 *  \file: imu_vn100.cxx
 *
 * Vectornav.com VN-100 (ascii/uart) driver
 *
 * Copyright Curt Olson curtolson@gmail.com
 *
 * $Id: gpsd.cpp,v 1.7 2009/08/25 15:04:01 curt Exp $
 */

#include <errno.h>		// errno
#include <fcntl.h>		// open()
#include <math.h>		// fabs()
#include <stdio.h>		// printf() et. al.
#include <termios.h>		// tcgetattr() et. al.
#include <unistd.h>		// tcgetattr() et. al.
#include <string.h>		// memset(), strerror()

#include "globaldefs.h"

#include "comms/logging.h"
#include "util/strutils.hxx"
#include "util/timing.h"

#include "imu_vn100.h"


// imu property nodes
static SGPropertyNode *configroot = NULL;

static SGPropertyNode *imu_device_name_node = NULL;

static SGPropertyNode *imu_timestamp_node = NULL;
static SGPropertyNode *imu_p_node = NULL;
static SGPropertyNode *imu_q_node = NULL;
static SGPropertyNode *imu_r_node = NULL;
static SGPropertyNode *imu_ax_node = NULL;
static SGPropertyNode *imu_ay_node = NULL;
static SGPropertyNode *imu_az_node = NULL;
static SGPropertyNode *imu_hx_node = NULL;
static SGPropertyNode *imu_hy_node = NULL;
static SGPropertyNode *imu_hz_node = NULL;
static SGPropertyNode *imu_temp_node = NULL;

static int fd = -1;
static string device_name = "/dev/ttyS0";

// internal filter values
/*static double p_filter = 0.0;
static double q_filter = 0.0;
static double r_filter = 0.0;
static double ax_filter = 0.0;
static double ay_filter = 0.0;
static double az_filter = 0.0;
static double hx_filter = 0.0;
static double hy_filter = 0.0;
static double hz_filter = 0.0;*/


// initialize gpsd input property nodes
static void bind_imu_input( SGPropertyNode *config ) {
    imu_device_name_node = config->getChild("device");
    if ( imu_device_name_node != NULL ) {
	device_name = imu_device_name_node->getStringValue();
    }
    configroot = config;

    // SGPropertyNode *node = NULL;
}


// initialize imu output property nodes 
static void bind_imu_output( string rootname ) {
    SGPropertyNode *outputroot = fgGetNode( rootname.c_str(), true );

    imu_timestamp_node = outputroot->getChild("time-stamp", 0, true);
    imu_p_node = outputroot->getChild("p-rad_sec", 0, true);
    imu_q_node = outputroot->getChild("q-rad_sec", 0, true);
    imu_r_node = outputroot->getChild("r-rad_sec", 0, true);
    imu_ax_node = outputroot->getChild("ax-mps_sec", 0, true);
    imu_ay_node = outputroot->getChild("ay-mps_sec", 0, true);
    imu_az_node = outputroot->getChild("az-mps_sec", 0, true);
    imu_hx_node = outputroot->getChild("hx", 0, true);
    imu_hy_node = outputroot->getChild("hy", 0, true);
    imu_hz_node = outputroot->getChild("hz", 0, true);
    imu_temp_node = outputroot->getChild("temp_C", 0, true);
}


// send our configured init strings to configure gpsd the way we prefer
static bool imu_vn100_open_9600() {
    if ( display_on ) {
	printf("Vectornav.com VN-100 on %s @ 9600\n", device_name.c_str());
    }

    fd = open( device_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK );
    if ( fd < 0 ) {
        fprintf( stderr, "open serial: unable to open %s - %s\n",
                 device_name.c_str(), strerror(errno) );
	return false;
    }

    struct termios config;     // New Serial Port Settings

    memset(&config, 0, sizeof(config));

    // Save Current Serial Port Settings
    // tcgetattr(fd,&oldTio); 

    // Configure New Serial Port Settings
    config.c_cflag     = B9600 | // bps rate
                         CS8	 | // 8n1
                         CLOCAL	 | // local connection, no modem
                         CREAD;	   // enable receiving chars
    config.c_iflag     = IGNPAR;   // ignore parity bits
    config.c_oflag     = 0;
    config.c_lflag     = 0;
    config.c_cc[VTIME] = 0;
    config.c_cc[VMIN]  = 0;	   // block 'read' from returning until at
                                   // least 1 character is received

    // Flush Serial Port I/O buffer
    tcflush(fd, TCIOFLUSH);

    // Set New Serial Port Settings
    int ret = tcsetattr( fd, TCSANOW, &config );
    if ( ret > 0 ) {
        fprintf( stderr, "error configuring device: %s - %s\n",
                 device_name.c_str(), strerror(errno) );
	return false;
    }

    return true;
}


// send our configured init strings to configure gpsd the way we prefer
static bool imu_vn100_open_115200() {
    if ( display_on ) {
	printf("Vectornav.com VN-100 on %s @ 115,200\n", device_name.c_str());
    }

    fd = open( device_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK );
    if ( fd < 0 ) {
        fprintf( stderr, "open serial: unable to open %s - %s\n",
                 device_name.c_str(), strerror(errno) );
	return false;
    }

    struct termios config; 	// New Serial Port Settings

    memset(&config, 0, sizeof(config));

    // Save Current Serial Port Settings
    // tcgetattr(fd,&oldTio); 

    // Configure New Serial Port Settings
    config.c_cflag     = B115200 | // bps rate
                         CS8	 | // 8n1
                         CLOCAL	 | // local connection, no modem
                         CREAD;	   // enable receiving chars
    config.c_iflag     = IGNPAR;   // ignore parity bits
    config.c_oflag     = 0;
    config.c_lflag     = 0;
    config.c_cc[VTIME] = 0;
    config.c_cc[VMIN]  = 0;	   // block 'read' from returning until at
                                   // least 1 character is received

    // Flush Serial Port I/O buffer
    tcflush(fd, TCIOFLUSH);

    // Set New Serial Port Settings
    int ret = tcsetattr( fd, TCSANOW, &config );
    if ( ret > 0 ) {
        fprintf( stderr, "error configuring device: %s - %s\n",
                 device_name.c_str(), strerror(errno) );
	return false;
    }

    // Enable non-blocking IO (one more time for good measure)
    fcntl(fd, F_SETFL, O_NONBLOCK);

    return true;
}


// calculate the nmea check sum
static char calc_nmea_cksum(const char *sentence) {
    unsigned char sum = 0;
    int i, len;

    // cout << sentence << endl;

    len = strlen(sentence);
    sum = sentence[0];
    for ( i = 1; i < len; i++ ) {
        // cout << sentence[i];
        sum ^= sentence[i];
    }
    // cout << endl;

    // printf("sum = %02x\n", sum);
    return sum;
}


static int imu_vn100_send_cmd( string msg ) {
    char msg_sum[10];
    snprintf( msg_sum, 3, "%02X", calc_nmea_cksum(msg.c_str()) );
    string command = "$";
    command += msg;
    command += "*";
    command += msg_sum;
    if ( display_on ) {
	printf("sending '%s'\n", command.c_str());
    }
    command += "\r\n";
    
    int len = write( fd, command.c_str(), command.length() );

    return len;
}


void imu_vn100_init( string rootname, SGPropertyNode *config ) {
    bind_imu_input( config );
    bind_imu_output( rootname );

    imu_vn100_open_9600();
    sleep(1);
    imu_vn100_send_cmd( "VNWRG,05,115200" ); // switch to 115,200 baud
    sleep(1);
    imu_vn100_close();
    sleep(1);

    imu_vn100_open_115200();
    sleep(1);
    imu_vn100_send_cmd( "VNWRG,06,253" ); // switch to CMV (raw sensor) output which wasn't documented
    imu_vn100_send_cmd( "VNWRG,07,50" ); // switch to 50hz output
}


static bool imu_vn100_parse_msg( char *msg_buf, int size )
{
    double current_time = get_Time();
    string msg = (string)msg_buf;

    // variables used to compute an intial steady state gyro bias
    static bool bias_ready = false;
    static double start_time  = -1.0;
    static int count = 0;
    static double p_sum = 0.0, q_sum = 0.0, r_sum = 0.0;
    static double p_bias = 0.0, q_bias = 0.0, r_bias = 0.0;

    // validate message
    if ( msg.length() < 9 ) {
        // bogus command
        return false;
    }
    // save sender check sum
    string nmea_sum = msg.substr(msg.length() - 2);
    // trim down to core message
    msg = msg.substr(0, msg.length() - 3);
    char msg_sum[10];
    snprintf( msg_sum, 3, "%02X", calc_nmea_cksum(msg.c_str()) );

    /*
      debug = fopen("/tmp/debug.txt", "a");
      fprintf(debug, " cmd: '%s' nmea: '%s' '%s'\n", cmd.c_str(),
              nmea_sum.c_str(), cmd_sum);
      fclose(debug);
    */

    // printf("%s %s\n", nmea_sum.c_str(), msg_sum);

    if ( nmea_sum.c_str()[0] != msg_sum[0]
         || nmea_sum.c_str()[1] != msg_sum[1])
    {
        // checksum failure
        /*
	  debug = fopen("/tmp/debug.txt", "a");
	  fprintf(debug, "check sum failure\n");
	  fclose(debug);
	*/
        return false;
    }
    vector<string> tokens = split( msg.c_str(), "," );
    if ( tokens[0] == "VNCMV" && tokens.size() == 11 ) {
	double val, p, q, r;
	val = atof( tokens[1].c_str() );
	// hx_filter = 0.75*hx_filter + 0.25*val;
	imu_hx_node->setDoubleValue( val );

	val = atof( tokens[2].c_str() );
	// hy_filter = 0.75*hy_filter + 0.25*val;
	imu_hy_node->setDoubleValue( val );

	val = atof( tokens[3].c_str() );
	// hz_filter = 0.75*hz_filter + 0.25*val;
	imu_hz_node->setDoubleValue( val );

	val = atof( tokens[4].c_str() );
	// ax_filter = 0.75*ax_filter + 0.25*val;
	imu_ax_node->setDoubleValue( val );

	val = atof( tokens[5].c_str() );
	// ay_filter = 0.75*ay_filter + 0.25*val;
	imu_ay_node->setDoubleValue( val );

	val = atof( tokens[6].c_str() );
	// az_filter = 0.75*az_filter + 0.25*val;
	imu_az_node->setDoubleValue( val );

	p = val = atof( tokens[7].c_str() );
	// p_filter = 0.75*p_filter + 0.25*val;
	imu_p_node->setDoubleValue( val - p_bias );

	q = val = atof( tokens[8].c_str() );
	// q_filter = 0.75*q_filter + 0.25*val;
	imu_q_node->setDoubleValue( val - q_bias );

	r = val = atof( tokens[9].c_str() );
	// r_filter = 0.75*r_filter + 0.25*val;
	imu_r_node->setDoubleValue( val - r_bias );

	val = atof( tokens[10].c_str() );
	// r_filter = 0.75*r_filter + 0.25*val;
	imu_temp_node->setDoubleValue( val );

	imu_timestamp_node->setDoubleValue( current_time );

	if ( !bias_ready ) {
	    // average first 15 seconds of steady state gyro values and
	    // use as a global bias.  This should be removed for the
	    // temperature compensated vector nav unit.
	    if ( start_time < 0.0 ) {
		start_time = current_time;
	    }
	    if ( current_time - start_time < 15.0 ) {
		p_sum += p;
		q_sum += q;
		r_sum += r;
		count++;
		p_bias = p_sum / (double)count;
		q_bias = q_sum / (double)count;
		r_bias = r_sum / (double)count;
	    } else {
		bias_ready = true;
		if ( display_on ) {
		    printf("gyro bias: p=%.2f q=%.2f r=%.2f\n",
			   p_bias, q_bias, r_bias);
		}
		// sanity check
		if ( fabs(p_bias) > 1.0 /* 57.3 deg/sec */ ||
		     fabs(q_bias) > 1.0 /* 57.3 deg/sec */ ||
		     fabs(r_bias) > 1.0 /* 57.3 deg/sec */ )
	        {
		    printf("Something is wrong with the gyro, it is outputting bad data!\n");
		    printf("Aborting so you can fix the hardware problem.\n");
		    printf("NOTE: IMU must be still when software is started.\n");
		    exit(-1);
		}
	    }
	}
    } else {
	if ( display_on ) {
	    printf("Unknown message or wrong number of fields: '%s'\n",
		   msg.c_str());
	}
	return false;
    }

    // printf("%s\n", msg.c_str());

    return true;
}


static bool imu_vn100_read() {
    static int state = 0;
    static int counter = 0;
    int len;
    const int max_len = 256;
    uint8_t input[max_len];
    static uint8_t msg[max_len];

    // printf("read vn100, entry state = %d\n", state);

    bool fresh_data = false;

    if ( state == 0 ) {
	counter = 0;
	len = read( fd, input, 1 );
	while ( len > 0 && input[0] != '$' ) {
	    // printf( "%c", input[0] );
	    len = read( fd, input, 1 );
	}
	if ( len > 0 && input[0] == '$' ) {
	    // printf( "read '$'\n" );
	    state = 1;
	}
    }
    if ( state == 1 ) {
	len = read( fd, input, 1 );
	while ( len > 0 && input[0] != '\r' && counter < max_len-1 ) {
	    msg[counter] = input[0];
	    counter++;
	    // printf( "%d ", input[0] );
	    len = read( fd, input, 1 );
	}
	if ( (len > 0 && input[0] == '\r') || counter >= max_len-1 ) {
	    if ( counter > max_len-1 ) {
		counter = max_len - 1;
	    }
	    msg[counter] = 0;
	    fresh_data = imu_vn100_parse_msg( (char *)msg, counter );
	    state = 0;
	}
	// printf("len = %d  counter = %d  input = %d\n", len, counter, input[0]);
    }

    return fresh_data;
}


bool imu_vn100_get() {
    // scan for new messages
    bool imu_data_valid = false;

    while ( imu_vn100_read() ) {
	imu_data_valid = true;
    }

    return imu_data_valid;
 }


void imu_vn100_close() {
    close(fd);
}
