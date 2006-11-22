#ifndef _UGEAR_CONSOLE_LINK_H
#define _UGEAR_CONSOLE_LINK_H


#define START_OF_MSG0 147
#define START_OF_MSG1 224

enum ugPacketType {
    GPS_PACKET = 0,
    IMU_PACKET = 1,
    NAV_PACKET = 2,
    SERVO_PACKET = 3,
    HEALTH_PACKET = 4
};

extern bool console_link_on;

void console_link_init();
void console_link_gps( struct gps *gpspacket );
void console_link_imu( struct imu *imupacket );
void console_link_nav( struct nav *navpacket );
void console_link_servo( struct servo *servopacket );
void console_link_health( struct health *healthpacket );


#endif // _UGEAR_CONSOLE_LINK_H
