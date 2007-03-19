/******************************************************************************
* FILE: mnav.c
* DESCRIPTION:
*   
*   
*
* SOURCE: 
* LAST REVISED: 4/05/06 Jung Soon Jang
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "comms/console_link.h"
#include "comms/logging.h"
#include "comms/serial.h"
#include "include/globaldefs.h"
#include "navigation/ahrs.h"
#include "navigation/nav.h"
#include "props/props.hxx"
#include "util/timing.h"

#include "mnav.h"

//
// uNAV packet length definition
//
#define FULL_PACKET_LENGTH	38
#define SENSOR_PACKET_LENGTH    51   
#define GPS_PACKET_LENGTH	35
#define FULL_PACKET_SIZE        86  // scaled mode with sampling less than 100Hz
#define fullspeed		0

#define D2R			0.017453292519940
#define R2D			57.29577951308232
#define g			9.81

//temperature compensation for accel.
//temperature compensation for mag.

char mnav_dev[MAX_MNAV_DEV] = "/dev/ttyS2";

//
// prototype definition
//
bool checksum(uint8_t* buffer, int packet_len);
void decode_imupacket(struct imu *data, uint8_t* buffer);
void decode_gpspacket(struct gps *data, uint8_t* buffer);

//
// global variables
//
static int sPort2;

struct servo servopacket;
bool autopilot_enable = false;
bool autopilot_reinit = false;
int  autopilot_count = 0;
char *cnt_status;

struct imu imupacket;
struct gps gpspacket;
struct nav navpacket;

// imu property nodes
// static SGPropertyNode *theta_node = NULL;
// static SGPropertyNode *phi_node = NULL;
// static SGPropertyNode *psi_node = NULL;
// static SGPropertyNode *Ps_node = NULL;
// static SGPropertyNode *Pt_node = NULL;
// static SGPropertyNode *Ps_filt_node = NULL;
// static SGPropertyNode *Pt_filt_node = NULL;
// static SGPropertyNode *comp_time_node = NULL;

// gps property nodes
// static SGPropertyNode *gps_lat_node = NULL;
// static SGPropertyNode *gps_lon_node = NULL;
// static SGPropertyNode *gps_alt_node = NULL;
// static SGPropertyNode *gps_ve_node = NULL;
// static SGPropertyNode *gps_vn_node = NULL;
// static SGPropertyNode *gps_vd_node = NULL;

// control input nodes
// static SGPropertyNode *servo_chn_node[8];


// open and intialize the MNAV communication channel
void mnav_init()
{
    int		nbytes = 0;
    uint8_t  	SCALED_MODE[11] ={0x55,0x55,0x53,0x46,0x01,0x00,0x03,0x00, 'S',0x00,0xF0};
    uint8_t          CH_BAUD[11]     ={0x55,0x55,0x57,0x46,0x01,0x00,0x02,0x00,0x03,0x00,0xA3};
    uint8_t		CH_SAMP[11]     ={0x55,0x55,0x53,0x46,0x01,0x00,0x01,0x00,0x02,0x00,0x9D};
    uint8_t          CH_SERVO[7]     ={0x55,0x55,0x53,0x50,0x00,0x00,0xA3};
  
    if ( display_on ) {
        printf("[mnav] initialized.\n");
    }
  
    /*********************************************************************
     *Open and configure Serial Port2 (com2)
     *********************************************************************/
    sPort2 = open_serial(SERIAL_PORT2,BAUDRATE_38400,false); 
    // printf("Opened serial port at 38400.\n");
      
    while (nbytes != 11) {
        nbytes = write(sPort2,(char*)CH_BAUD, 11);
        // printf("writing CH_BAUD\n");
    }
    nbytes = 0;  
    close(sPort2);

    sPort2 = open_serial(SERIAL_PORT2,BAUDRATE_57600,false); 
    // printf("Opened serial port at 57600.\n");
  
    
    while (nbytes != 11) {
        nbytes = write(sPort2,(char*)CH_SAMP, 11);
        // printf("writing CH_SAMP\n");
    }
    nbytes = 0;

    while (nbytes != 11) {
        nbytes = write(sPort2,(char*)SCALED_MODE, 11);
        // printf("writing SCALED_MODE\n");
    }
    nbytes = 0;

    while (nbytes !=  7) {
        nbytes = write(sPort2,(char*)CH_SERVO, 7);
        // printf("writing CH_SERVO\n");
    }
    nbytes = 0;  

    // initialize imu property nodes
    // theta_node = fgGetNode("/orientation/pitch-deg", true);
    // phi_node = fgGetNode("/orientation/roll-deg", true);
    // psi_node = fgGetNode("/orientaiton/heading-deg", true);
    // Ps_node = fgGetNode("/position/altitude-pressure-m", true);
    // Pt_node = fgGetNode("/velocities/airspeed-ms", true);
    // Ps_filt_node = fgGetNode("/position/altitude-filtered-m", true);
    // Pt_filt_node = fgGetNode("/velocities/airspeed-filtered-ms", true);
    // comp_time_node = fgGetNode("/time/computer-sec", true);

    // initialize gps property nodes
    // gps_lat_node = fgGetNode("/position/latitude-gps-deg", true);
    // gps_lon_node = fgGetNode("/position/longitude-gps-deg", true);
    // gps_alt_node = fgGetNode("/position/altitude-gps-m", true);
    // gps_ve_node = fgGetNode("/velocities/ve-gps-ms", true);
    // gps_vn_node = fgGetNode("/velocities/vn-gps-ms", true);
    // gps_vd_node = fgGetNode("/velocities/vd-gps-ms", true);

    // initialize control input property nodes
    // for ( int i = 0; i < 8; ++i ) {
    //   servo_chn_node[i] = fgGetNode("/controls/channel", i, true);
    // }
}


// Main IMU/GPS data aquisition routine
//
// Note this blocks until new IMU/GPS data is available.  The rate at
// which the MNAV sends data dictates the timing and rate of the
// entire ugear program.
//
void mnav_update()
{
    int headerOK = 0;
    int nbytes = 0;
    uint8_t input_buffer[FULL_PACKET_SIZE]={0,};

    static double Ps_filt = 0.0;
    static double Pt_filt = 0.0;

    bool imu_valid_data = false;
    bool gps_valid_data = false;

    // Find start of packet: the heade r (2 bytes) starts with 0x5555
    while ( headerOK != 2 ) {
        while(1!=read(sPort2,input_buffer,1));
        if (input_buffer[0] == 0x55)
            headerOK++;
        else
            headerOK = 0;
    }
     	
    headerOK = 0; while ( 1 != read(sPort2,&input_buffer[2],1) );
    nbytes = 3; 
  
    // Read packet contents
    switch (input_buffer[2]) {
    case 'S':               // IMU packet without GPS
        // printf("no gps data being sent ... :-(\n");
        while ( nbytes < SENSOR_PACKET_LENGTH ) {
            nbytes += read(sPort2, input_buffer+nbytes,
                           SENSOR_PACKET_LENGTH-nbytes); 
        }

        // check checksum
        if ( checksum(input_buffer,SENSOR_PACKET_LENGTH) ) {
            decode_imupacket(&imupacket, input_buffer);
            imu_valid_data = true;
        } else {
            printf("[imu]:checksum error...!\n"); 
            imupacket.err_type = checksum_err; 
        };
        break;

    case 'N':               // IMU packet with GPS
        while ( nbytes < FULL_PACKET_SIZE ) {
            nbytes += read(sPort2, input_buffer+nbytes,
                           FULL_PACKET_SIZE-nbytes); 
        }

        // printf("G P S   D A T A   A V A I L A B L E\n");

        // check checksum
        if ( checksum(input_buffer,FULL_PACKET_SIZE) ) {
            decode_imupacket(&imupacket, input_buffer);
            imu_valid_data = true;
		     
            // check GPS data packet
            if(input_buffer[33]=='G') {
                decode_gpspacket(&gpspacket, input_buffer);
		gps_valid_data = true;
            } else {
               printf("[gps]:data error...!\n");
                gpspacket.err_type = got_invalid;
            } // end if(checksum(input_buffer...
        } else { 
            printf("[imu]:checksum error(gps)...!\n");
            gpspacket.err_type = checksum_err;
            imupacket.err_type = checksum_err; 
        }
        break;

    default : 
        printf("[imu] invalid data packet ... !\n");

    } // end case


    if ( imu_valid_data ) {
        ahrs_update();

	// Do a simple first order low pass filter to remove noise
	Ps_filt = 0.9 * Ps_filt + 0.1 * imupacket.Ps;
	Pt_filt = 0.9 * Pt_filt + 0.1 * imupacket.Pt;

	// publish values to property tree
	// theta_node->setFloatValue( imupacket.the * SG_RADIANS_TO_DEGREES );
	// phi_node->setFloatValue( imupacket.phi * SG_RADIANS_TO_DEGREES );
	// psi_node->setFloatValue( imupacket.psi * SG_RADIANS_TO_DEGREES );
	// Ps_node->setFloatValue( imupacket.Ps );
	// Pt_node->setFloatValue( imupacket.Pt );
	// Ps_filt_node->setFloatValue( Ps_filt );
	// Pt_filt_node->setFloatValue( Pt_filt );
	// comp_time_node->setDoubleValue( imupacket.time );

	// for ( int i = 0; i < 8; ++i ) {
	//   servo_chn_node[i]->setIntValue( servopacket.chn[i] );
	// }

        if ( console_link_on ) {
            console_link_imu( &imupacket );
            console_link_servo( &servopacket );
        }

        if ( log_to_file ) {
            log_imu( &imupacket );
            log_servo( &servopacket );
        }
    }

    if ( gps_valid_data ) {
        // publish values to property tree
        // gps_lat_node->setDoubleValue( gpspacket.lat );
	// gps_lon_node->setDoubleValue( gpspacket.lon );
	// gps_alt_node->setDoubleValue( gpspacket.alt );
	// gps_ve_node->setDoubleValue( gpspacket.ve );
	// gps_vn_node->setDoubleValue( gpspacket.vn );
	// gps_vd_node->setDoubleValue( gpspacket.vd );

        if ( console_link_on ) {
            console_link_gps( &gpspacket );
        }

        if ( log_to_file ) {
            log_gps( &gpspacket );
        }
    }

    //////////////////////////////////////////////////////////////
    // NOTICE: MANUAL OVERRIDE
    //////////////////////////////////////////////////////////////

    // Manual override functionality for the uNAV is hardwired
    // into the unit.  It is hardwired to the gear channel (CH5
    // when counting from 1.)  This is not optional.  When you
    // enter manual override mode, the servo commands you send to
    // the uNAV are ignored and the unit passes through the
    // transmited values directly.
    //
    // So all the code needs to do is monitor the manual override
    // switch and not send autopilot commands in manual override
    // mode (they would be ignored anyway.)  There is no need to
    // copy the data through in software, it should all happen
    // automatically.
    //
    // Also note that I call this a "manual override" and not a
    // "fail safe".  If the uNAV fails for any reason, you will be
    // picking up toothpicks.
    //

    if ( servopacket.chn[4] <= 12000 ) {
        // if the autopilot is enabled, or signal is lost
        if ( !autopilot_enable && display_on ) {
            printf("[CONTROL]: switching to autopilot\n");
	    autopilot_reinit = true;
        }
        autopilot_enable = true;
        autopilot_count  = 15;
        cnt_status = "MNAV in AutoPilot Mode";
    } else if ( servopacket.chn[4] > 12000
                && servopacket.chn[4] < 60000 )
    {
        // add delay on control trigger to minimize mode confusion
        // caused by the transmitter power off
        if ( autopilot_count < 0 ) {
            if ( autopilot_enable && display_on ) {
                printf("[CONTROL]: switching to manual pass through\n");
            }
            autopilot_enable = false;
            cnt_status = "MNAV in Manual Mode";
        } else {
            autopilot_count--;
        }
    }
}


void mnav_close()
{
    //close the serial port
    close(sPort2);

    //close files
    logging_close();
}


//
// check the checksum of the data packet
//
bool checksum( uint8_t* buffer, int packet_len ) {
    uint16_t i = 0, rcvchecksum = 0;
    uint16_t sum = 0;

    for ( i = 2; i < packet_len - 2; i++ ) sum = sum + buffer[i];
    rcvchecksum = ((rcvchecksum = buffer[packet_len-2]) << 8) | buffer[packet_len-1];

    // if (rcvchecksum == sum%0x10000)
    if ( rcvchecksum == sum ) //&0xFFFF)
	return true;
    else
 	return false;
}


//
// decode the gps data packet
//
void decode_gpspacket( struct gps *data, uint8_t* buffer )
{
    signed long tmp = 0;

    // gps velocity in m/s
    data->vn =(double)((((((tmp = (signed char)buffer[37]<<8)|buffer[36])<<8)|buffer[35])<<8)|buffer[34])*1.0e-2; tmp=0;
    data->ve =(double)((((((tmp = (signed char)buffer[41]<<8)|buffer[40])<<8)|buffer[39])<<8)|buffer[38])*1.0e-2; tmp=0;
    data->vd =(double)((((((tmp = (signed char)buffer[45]<<8)|buffer[44])<<8)|buffer[43])<<8)|buffer[42])*1.0e-2; tmp=0;

    // gps position
    data->lon=(double)((((((tmp = (signed char)buffer[49]<<8)|buffer[48])<<8)|buffer[47])<<8)|buffer[46])*1.0e-7; tmp=0;
    data->lat=(double)((((((tmp = (signed char)buffer[53]<<8)|buffer[52])<<8)|buffer[51])<<8)|buffer[50])*1.0e-7; tmp=0;
    data->alt=(double)((((((tmp = (signed char)buffer[57]<<8)|buffer[56])<<8)|buffer[55])<<8)|buffer[54])*1.0e-3; tmp=0;
   
   
    // gps time
    data->ITOW = ((data->ITOW = buffer[59]) << 8)|buffer[58];
    data->err_type = no_error;
    data->time = get_Time();

    // printf("sizeof gps = %d  time = %.3f\n", sizeof(struct gps), data->time);
}


//
// decode the imu data packet
//
void decode_imupacket( struct imu *data, uint8_t* buffer )
{
    signed short tmp = 0;
    unsigned short tmpr = 0;

    /* acceleration in m/s^2 */
    data->ax = (double)(((tmp = (signed char)buffer[ 3])<<8)|buffer[ 4])*5.98755e-04; tmp=0;
    data->ay = (double)(((tmp = (signed char)buffer[ 5])<<8)|buffer[ 6])*5.98755e-04; tmp=0;
    data->az = (double)(((tmp = (signed char)buffer[ 7])<<8)|buffer[ 8])*5.98755e-04; tmp=0;
   
  
    /* angular rate in rad/s */
    data->p  = (double)(((tmp = (signed char)buffer[ 9])<<8)|buffer[10])*1.06526e-04; tmp=0;
    data->q  = (double)(((tmp = (signed char)buffer[11])<<8)|buffer[12])*1.06526e-04; tmp=0;
    data->r  = (double)(((tmp = (signed char)buffer[13])<<8)|buffer[14])*1.06526e-04; tmp=0;
   
    /* magnetic field in Gauss */
    data->hx = (double)(((tmp = (signed char)buffer[15])<<8)|buffer[16])*6.10352e-05; tmp=0;
    data->hy = (double)(((tmp = (signed char)buffer[17])<<8)|buffer[18])*6.10352e-05; tmp=0;
    data->hz = (double)(((tmp = (signed char)buffer[19])<<8)|buffer[20])*6.10352e-05; tmp=0;

    /* temperature in Celcius */
    /*
      data->Tx = (double)(((tmp = (signed char)buffer[21])<<8)|buffer[22])*6.10352e-03; tmp=0;
      data->Ty = (double)(((tmp = (signed char)buffer[23])<<8)|buffer[24])*6.10352e-03; tmp=0;
      data->Tz = (double)(((tmp = (signed char)buffer[25])<<8)|buffer[26])*6.10352e-03; tmp=0;
    */
   
    /* pressure in m and m/s */
    data->Ps = (double)(((tmp = (signed char)buffer[27])<<8)|buffer[28])*3.05176e-01; tmp=0;
    data->Pt = (double)(((tmp = (signed char)buffer[29])<<8)|buffer[30])*2.44141e-03; tmp=0;

    // servo packet
    switch (buffer[2]) {
    case 'S' :   servopacket.status = buffer[32];
        servopacket.chn[0] = ((tmpr = buffer[33]) << 8)|buffer[34]; tmpr = 0;
        servopacket.chn[1] = ((tmpr = buffer[35]) << 8)|buffer[36]; tmpr = 0;
        servopacket.chn[2] = ((tmpr = buffer[37]) << 8)|buffer[38]; tmpr = 0;
        servopacket.chn[3] = ((tmpr = buffer[39]) << 8)|buffer[40]; tmpr = 0;
        servopacket.chn[4] = ((tmpr = buffer[41]) << 8)|buffer[42]; tmpr = 0;
        servopacket.chn[5] = ((tmpr = buffer[43]) << 8)|buffer[44]; tmpr = 0;
        servopacket.chn[6] = ((tmpr = buffer[45]) << 8)|buffer[46]; tmpr = 0;
        servopacket.chn[7] = ((tmpr = buffer[47]) << 8)|buffer[48]; 
        break;
    case 'N' :   servopacket.status = buffer[67];
        servopacket.chn[0] = ((tmpr = buffer[68]) << 8)|buffer[69]; tmpr = 0;
        servopacket.chn[1] = ((tmpr = buffer[70]) << 8)|buffer[71]; tmpr = 0;
        servopacket.chn[2] = ((tmpr = buffer[72]) << 8)|buffer[73]; tmpr = 0;
        servopacket.chn[3] = ((tmpr = buffer[74]) << 8)|buffer[75]; tmpr = 0;
        servopacket.chn[4] = ((tmpr = buffer[76]) << 8)|buffer[77]; tmpr = 0;
        servopacket.chn[5] = ((tmpr = buffer[78]) << 8)|buffer[79]; tmpr = 0;
        servopacket.chn[6] = ((tmpr = buffer[80]) << 8)|buffer[81]; tmpr = 0;
        servopacket.chn[7] = ((tmpr = buffer[82]) << 8)|buffer[83]; 
        break;
    default  :
        printf("[imu]:fail to decode servo packet..!\n");
    }

    data->time = get_Time();
    servopacket.time = data->time;
    data->err_type = no_error;
}


void send_servo_cmd(uint16_t cnt_cmd[9])
{
    // cnt_cmd[1] = ch1:elevator
    // cnt_cmd[0] = ch0:aileron
    // cnt_cmd[2] = ch2:throttle

    uint8_t data[24];
    short i = 0, nbytes = 0;
    uint16_t sum = 0;

    // printf("sending servo data ");
    // for ( i = 0; i < 9; ++i ) printf("%d ", cnt_cmd[i]);
    // printf("\n");

    data[0] = 0x55; 
    data[1] = 0x55;
    data[2] = 0x53;
    data[3] = 0x53;

    for ( i = 0; i < 9; ++i ) {
        data[4+2*i] = (uint8_t)(cnt_cmd[i] >> 8); 
        data[5+2*i] = (uint8_t)cnt_cmd[i];
    }

    // aileron
    // data[4] = (uint8_t)(cnt_cmd[0] >> 8); 
    // data[5] = (uint8_t)cnt_cmd[0];

    // elevator
    // data[6] = (uint8_t)(cnt_cmd[1] >> 8);
    // data[7] = (uint8_t)cnt_cmd[1];

    // throttle
    // data[8] = (uint8_t)(cnt_cmd[2] >> 8);
    // data[9] = (uint8_t)cnt_cmd[2];

    //checksum: need to be verified
    sum = 0xa6;
    for (i = 4; i < 22; i++) sum += data[i];
  
    data[22] = (uint8_t)(sum >> 8);
    data[23] = (uint8_t)sum;

    //sendout the command packet
    while (nbytes != 24) {
        // printf("  writing servos ...\n");
        nbytes = write(sPort2,(char*)data, 24);
    }
}
