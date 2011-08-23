/*
 * gps_time.h
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#ifndef GPS_TIME_H_
#define GPS_TIME_H_

#include <stdio.h> //FILE structure
#include <sys/time.h> //tz and tv

typedef enum {
	GPS_OK,
	GPS_ANTENA_SHORT,
	GPS_ANTENA_OPEN,
	GPS_NEEDED_RESET,
	GPS_BATTERY_FAULT
} gps_fault_t;


// Used by serial port interface
void init_gps( char indoor, void (*input_for_gps)( char *msg, int msg_len), FILE *output_status_f);

void output_from_gps( unsigned char* msg, int msg_len);



// available for user
char is_gps_ready();

int getgpstimeofday( struct timeval *tv, struct timezone *tz);

int getgpssatellites();

gps_fault_t getgpsfault();


#endif /* GPS_TIME_H_ */
