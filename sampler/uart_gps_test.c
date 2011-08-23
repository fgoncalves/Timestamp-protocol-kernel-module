/*
 * uart_gps_test.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "gps_time.h"
#include "gps_uartctl.h"

int main( int argc, char **argv) {

	uart_init( 1, stderr);

	while( !is_gps_ready()) {
		sleep( 1);
	}

	printf( "GPS is ready\n");

	return 0;
}
