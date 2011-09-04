/*
 * uart_gps_test.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "gps_time.h"
#include "gps_uartctl.h"

/* Subtract the `struct timeval' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.  */


int main(int argc, char **argv) {
	int x;
	struct tm *broken_time;
	struct timeval t;

	uart_init(1, stderr);

	while (!is_gps_ready()) {
		sleep(1);
	}

	printf("GPS is ready\n");

	for (x = 0; x < 100; x++) {
		getGPStimeUTC(&t);
		broken_time = localtime(&(t.tv_sec));

		//printf("%ld sec %ld usec\n", t.tv_sec, t.tv_usec);
		printf("Current date: %02d/%02d/%02d %02d:%02d:%02d.%06d\n",
				broken_time->tm_mday, broken_time->tm_mon + 1,
				broken_time->tm_year + 1900, broken_time->tm_hour,
				broken_time->tm_min, broken_time->tm_sec, (int) t.tv_usec);
	}
	return 0;
}
