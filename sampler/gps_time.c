/*
 * gps_uartctl.c
 *
 *  Created on: Jun 13, 2011
 *      Author: ricardo
 */

#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gps_time.h"

#define DLE 0x10
#define ETX 0x03

#define GPS_CMD_RESET 0x25
#define GPS_CMD_AUTOMATIC_POS_REPORT 0x35
#define GPS_CMD_ENHANCED_SENSITIVITY 0x6D

//#define GPS_MSG_ 0x41
#define GPS_MSG_SW_VERSION 0x45
#define GPS_MSG_RECEIVER_HEALTH 0x46
//#define GPS_MSG_ 0x4B
//#define GPS_MSG_ 0x6D
//#define GPS_MSG_ 0x82


gps_fault_t gps_fault = GPS_OK;

char reset_msg[] = { DLE, GPS_CMD_RESET, DLE, ETX };
char disable_auto_report_msg[] = { DLE, GPS_CMD_AUTOMATIC_POS_REPORT, 0, 0, DLE,
				   ETX };
char enhance_sensitivity_on_msg[] = { DLE, GPS_CMD_ENHANCED_SENSITIVITY, 1, 0, DLE,
				      ETX };
char enhance_sensitivity_off_msg[] = { DLE, GPS_CMD_ENHANCED_SENSITIVITY, 0, 0, DLE,
				       ETX };

int num_satellites = 0;

#define LHEX(x) (x & 0x0F)
#define HHEX(x) (x >> 4)
#define BIT(x,y) ( (x >> y) & 0x01)

/* enables debug messages */
#define GPS_DEBUG

#define MSG_BUF_SIZE 1024

/** circular buffer for storing messages from the GPS for processing */
unsigned char msg_buf[MSG_BUF_SIZE];
/** first (inclusive) and last (exclusive) positions of data in the circular buffer */
int buff_start = 0, buff_end = 0;

/* Lock for acessing GPS state and circular buffer */
pthread_mutex_t state_change_m = PTHREAD_MUTEX_INITIALIZER;
/* lock for writting messages to the GPS */
pthread_mutex_t gps_write_m = PTHREAD_MUTEX_INITIALIZER;

/* notify message handling thread that new data is available */
pthread_cond_t data_available_cond = PTHREAD_COND_INITIALIZER;

FILE *debug_f;

pthread_t process_msg_t;

typedef enum {
  GPS_PRE_INIT, // before GPS reset, dump all messages
  GPS_RESET, // after GPS reset, before first post GPS message is received
  GPS_SEARCH, // GPS is reset. Waiting for GPS signal. Yet unable to provide time
  GPS_READY, // was gps signal. Can provide time
  GPS_QUERY
  // sent a query request. Waiting for reply
} gps_state_t;

gps_state_t gps_state = GPS_PRE_INIT;

void (*gps_write)(char *msg, int msg_len);
void* process_msg(void *unused);

void packet_content(char *error_msg, int start, int end) {
  int x;
  if (debug_f == NULL)
    return;

  fprintf(debug_f, "Packet content %d to %d. %s: ", start, end, error_msg);

  for (x = start; x != end; x++)
    fprintf(debug_f, "%hhX ", msg_buf[x% MSG_BUF_SIZE]);

  fprintf(debug_f, "\n");

}

/* searches the buffer for a new message
 * receives the start (inclusive) and end (exclusive) position of data in the buffer
 * If a message is found, start and end will define its first and last position
 * message will be unescaped and shifted towards its end, start will point to its first position
 * and end to its last (exclusive)
 * If a message is not found, start and end will be undefined
 * returns 1 if a message is found, 0 otherwise
 */
char lookup_packet(int *start, int *end) {
  int pos;
  int dle_count = 0;
  char packet_found = 0;
  int origin;

  // "make" circular buffer linear
  if (*end < *start)
    *end += MSG_BUF_SIZE;

  // do we have enough data for a message?
  if (*end - *start < 4)
    return 0;

  // does buffer start with a message?
  if (msg_buf[*start] != DLE) {
    //if (gps_state != GPS_RESET) {
    packet_content(
		   "Packet does not start with DLE. Should never happen rewrite code!",
		   *start, *end);
    exit(1);
    //} else {
    // there may be garbage in the GPS data stream, search for first packet
    //}
  }

  // look for message end
  for (pos = *start; pos < *end && !packet_found; pos++) {
    switch (msg_buf[pos % MSG_BUF_SIZE]) {
    case DLE:
      dle_count++;
      break;
    case ETX:
      if (dle_count % 2)
	packet_found = 1;
    default:
      dle_count = 0;
    }
  }

  if (!packet_found)
    return 0;

  // a packet was found. Let's remove header, trailer and padding
  *end = pos % MSG_BUF_SIZE;
  pos = pos - 1;
  origin = pos - 2; // trailler removed

  while (origin > *start) { // header removed
    // shift message towards the end
    msg_buf[pos % MSG_BUF_SIZE] = msg_buf[origin % MSG_BUF_SIZE];

    // remove padding
    if (msg_buf[origin % MSG_BUF_SIZE] == DLE && msg_buf[(origin - 1)
							 % MSG_BUF_SIZE] == DLE)
      origin -= 2;
    else
      origin--;

    pos--;
  }
  *start = (pos + 1) % MSG_BUF_SIZE;
  return 1;
}

void init_gps( char indoor, void(*input_for_gps)(char *msg, int msg_len),
	       FILE *output_status_func) {
  debug_f = output_status_func;
  gps_write = input_for_gps;

  pthread_create(&process_msg_t, NULL, process_msg, NULL);

  //	gps_state = GPS_SEARCH;
  //	return;

  /****************************/

  // enable enhanced sensitivity for indoor operation
  if( indoor)
    gps_write( enhance_sensitivity_on_msg, sizeof(enhance_sensitivity_on_msg));
  else
    gps_write( enhance_sensitivity_off_msg, sizeof(enhance_sensitivity_off_msg));

  // reset gps device
  gps_write(reset_msg, sizeof(reset_msg));
  gps_state = GPS_RESET;
  // disable automatic position and velocity reports
  gps_write(disable_auto_report_msg, sizeof(disable_auto_report_msg));
}

/* places data, read from the GPS in the message buffer for latter processing
 */
void output_from_gps(unsigned char* msg, int msg_len) {
  int end;

  // ignore messages sent before GPS is initialised
  if (gps_state == GPS_PRE_INIT)
    return;

  pthread_mutex_lock(&state_change_m);
  end = buff_end < buff_start ? buff_end + MSG_BUF_SIZE : buff_end;
  if ((MSG_BUF_SIZE - (end - buff_start)) > msg_len) { // we always leave one free position in the buffer
    // he have enough free space for the message
    for (end = 0; end < msg_len; end++) {
      msg_buf[(buff_end + end) % MSG_BUF_SIZE] = msg[end];
    }
    buff_end = (buff_end + end) % MSG_BUF_SIZE;
    pthread_cond_signal(&data_available_cond);
  } else {
    // should never happen. If it ever happens must rewrite to deal with it
    fprintf(debug_f, "No space left on buffer to hold message! Quitting.\n");
    exit(1);
  }
#ifdef GPS_DEBUG
  //	printf("Stored message - Size = %d. Buffer using from %d to %d\n", msg_len,
  //			buff_start, buff_end);
#endif
  pthread_mutex_unlock(&state_change_m);
}

char is_gps_ready() {
  return gps_state == GPS_READY && gps_fault == GPS_OK;
}

/* Get current time from the GPS device
 * Will return 1 if currently unable to read the time
 * Will return 0 if time is read
 * */
int getgpstimeofday(struct timeval *tv, struct timezone *tz) {
  if (gps_state != GPS_READY || gps_fault != GPS_OK)
    return 1;
  else
    return 0;
}

int getgpssatellites() {
  return num_satellites;
}

gps_fault_t getgpsfault() {
  return gps_fault;
}

void* process_msg(void *unused) {
  int start, end;
  char byte;

  pthread_mutex_lock(&state_change_m);
  while (1) {
    start = buff_start;
    end = buff_end;
    while (!lookup_packet(&start, &end)) {
      pthread_cond_wait(&data_available_cond, &state_change_m);
      start = buff_start;
      end = buff_end;
    }

    if (end < start)
      end += MSG_BUF_SIZE;

#ifdef GPS_DEBUG
    printf("Will process new message - Size = %d, from %d to %d\n",
	   end - start, start, end);
    packet_content("found this ", start, end);
#endif

    //handle non-state specific messages
    switch (msg_buf[start]) {
    case GPS_MSG_RECEIVER_HEALTH:
      if (msg_buf[(start + 1) % MSG_BUF_SIZE] == 0x02) {
	gps_fault = GPS_NEEDED_RESET;
	gps_state = GPS_RESET;
	// reset gps device
	gps_write(reset_msg, sizeof(reset_msg));
      }
      else
	gps_fault = GPS_OK;

      byte = msg_buf[(start + 2) % MSG_BUF_SIZE];
      if (HHEX( byte) == 0x03)
	gps_fault = GPS_ANTENA_SHORT;
      else if (HHEX( byte) == 0x01)
	gps_fault = GPS_ANTENA_OPEN;
      else if (BIT( byte, 0))
	gps_fault = GPS_BATTERY_FAULT;
      break;
    }

    // handle state specific messages
    if (gps_state == GPS_RESET) {
      // wait for SW version message after reset.
      if (msg_buf[start] == GPS_MSG_SW_VERSION) {
	gps_state = GPS_SEARCH;
#ifdef GPS_DEBUG
	printf("*\nReset completed, entering satellite search mode.\n");
#endif
      }
    } else if (gps_state == GPS_SEARCH) {
      switch (msg_buf[start]) {
      case GPS_MSG_RECEIVER_HEALTH:
	if (msg_buf[(start + 1) % MSG_BUF_SIZE] == 0) {
	  gps_state = GPS_READY;
#ifdef GPS_DEBUG
	  printf("*\nGPS fix obtainned.\n");
#endif
	}
	break;
      }
    } else if (gps_state == GPS_READY) {
      switch (msg_buf[start]) {
      case GPS_MSG_RECEIVER_HEALTH:
	if (msg_buf[(start + 1) % MSG_BUF_SIZE] != 0) {
	  gps_state = GPS_SEARCH;
#ifdef GPS_DEBUG
	  printf("*\nGPS fix LOST. Searching for satellites.\n");
#endif
	}
	break;
      }
    } else {
      // GPS_QUERY
    }



    buff_start = end % MSG_BUF_SIZE;
  }
}

