CC=gcc
SPECIAL_FLAGS=-lrt -lpthread
CFLAGS=-c -Wall
OUT=sampler

all: main.o macros.h
	$(CC) -o $(OUT) $(SPECIAL_FLAGS) main.o blocking_queue.o socket_list.o thread_functions.o timestamp.o statistics.o

main.o: thread_functions.o main.c
	$(CC) $(CFLAGS) main.c

thread_functions.o: thread_functions.h thread_functions.c statistics.o blocking_queue.o socket_list.o
	$(CC) $(CFLAGS) thread_functions.c

socket_list.o: socket_list.c socket_list.h
	$(CC) $(CFLAGS) socket_list.c

blocking_queue.o: timestamp.o blocking_queue.c blocking_queue.h
	$(CC) $(CFLAGS) blocking_queue.c

timestamp.o: timestamp.h timestamp.c
	$(CC) $(CFLAGS) timestamp.c

statistics.o: statistics.h statistics.c
	$(CC) $(CFLAGS) statistics.c

clean:
	rm -f *.o *~ $(OUT) *.out
