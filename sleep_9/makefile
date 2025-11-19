CC = gcc
CFLAGS = -Wall -I.

CORE_SRCS = ppos_core.c ../queue_0/queue.c

ppos: main.c $(CORE_SRCS)
	$(CC) $(CFLAGS) $(CORE_SRCS) main.c -o ppos

clean:
	rm -f ppos