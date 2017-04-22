# raw_socket_recv.c  tables.c  tables.h  wrapper.h
CC := gcc
CFLAGS := -c -Wall -O0 -g

OBJS := routing.o tables.o

router : $(OBJS)
	$(CC) -o router $(OBJS)
routing.o : routing.c tables.h wrapper.h
	$(CC) $(CFLAGS) routing.c
tables.o : tables.c
	$(CC) $(CFLAGS) tables.c


clean :
	rm -rf $(OBJS) router

.PHONY: clean
