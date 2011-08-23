CFLAGS = -Wall -DDEBUG -DBASH
CC = gcc
OUT = sampler

all: timestamp.o packet.o statistics.o sampler.o gps_time.o gps_uartctl.o
	$(CC) -o $(OUT) -lrt $(CFLAGS) statistics.o packet.o timestamp.o sampler.o gps_time.o gps_uartctl.o

timestamp.o: timestamp.h timestamp.c
	$(CC) $(CFLAGS) -c timestamp.c

packet.o: packet.h packet.c
	$(CC) $(CFLAGS) -c packet.c

statistics.o: statistics.h statistics.c
	$(CC) $(CFLAGS) -c statistics.c

sampler.o: sampler.c
	$(CC) $(CFLAGS) -c sampler.c

gps_time.o: gps_time.h
	$(CC) $(CFLAGS) -c gps_time.c

gps_uartctl.o: gps_uartctl.h
	$(CC) $(CFLAGS) -c gps_uartctl.c

clean:
	rm -rf $(OUT) *~ *.out *.o
