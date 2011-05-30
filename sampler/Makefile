CFLAGS = -Wall -DDEBUG
CC = gcc
OUT = sampler

all: timestamp.o packet.o statistics.o sampler.o
	$(CC) -o $(OUT) -lrt $(CFLAGS) statistics.o packet.o timestamp.o sampler.o

timestamp.o: timestamp.h timestamp.c
	$(CC) $(CFLAGS) -c timestamp.c

packet.o: packet.h packet.c
	$(CC) $(CFLAGS) -c packet.c

statistics.o: statistics.h statistics.c
	$(CC) $(CFLAGS) -c statistics.c

sampler.o: sampler.c
	$(CC) $(CFLAGS) -c sampler.c

clean:
	rm -rf $(OUT) *~ *.out *.o
