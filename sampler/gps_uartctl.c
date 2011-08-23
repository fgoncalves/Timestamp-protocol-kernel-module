/*
 * gps_uartctl.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
#include <unistd.h>

#include "gps_uartctl.h"
#include "gps_time.h"


/* Currently set to read data from /dev/pts/1
 * For this we must first run:
 *   xuartctl -d -p 0 -o 8o1 -s 9600
 */

#define GPS_DEVICE "/dev/pts/1"
#define ERROR_BUF_SIZE 1000
#define GPS_BUF_SIZE 100

char error_buf[ ERROR_BUF_SIZE];
unsigned char gps_buf[ GPS_BUF_SIZE];
int gps_device_fd;
FILE *error_f = NULL;
pthread_t read_thread;

void* uart_read_thread( void *);

/* Opens de device for communicating with the GPS and initializes
 * the GPS state machine.
 * */
void uart_init( char indoor, FILE *status_output) {
	error_f = status_output;
	gps_device_fd = open( GPS_DEVICE, O_RDWR);
	if( gps_device_fd == -1) {
		fprintf( error_f, "gps_uartctl.c: Error while opening GPS device: %s.\n", GPS_DEVICE);
		fprintf( error_f, "Error %d: %s\n", errno, strerror_r( errno, error_buf, ERROR_BUF_SIZE) == 0 ? error_buf : "");
		exit( 1);
	}
	// start thread to read messages from GPS device
	pthread_create( &read_thread, NULL, uart_read_thread, NULL);

	// give some time to consume all the messages, which will be ignored, before initializing GPS module
	sleep( 1);
	init_gps( indoor, uart_write, status_output);
}

/* Sends a message to the GPS device.
 */
void uart_write( char *msg, int msg_len) {
	int written_b = write( gps_device_fd, msg, msg_len);
	if( written_b != msg_len) {
		fprintf( error_f, "gps_uartctl.c: Error while writing to GPS device. Only wrote %d of %d B\n", written_b, msg_len);
		exit(1);
	}
	else
		fprintf( error_f, "Sent %dB to GPS.\n", written_b);
}

/* Creates a new thread responsible for reading data from the
 * uart device and providing it to the gps state machine */
void* uart_read_thread( void *unused) {
	size_t read_count;

	while( 1) {
		read_count = read( gps_device_fd, gps_buf, GPS_BUF_SIZE);
		if( read_count == 0) {
			fprintf( error_f, "gps_uartctl.c: Unable to read from GPS\n");
			exit(1);
		}
		output_from_gps( gps_buf, read_count);
	}
}
