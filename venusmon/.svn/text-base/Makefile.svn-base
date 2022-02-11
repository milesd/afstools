# Makefile for venusmon

CC=cc
OBJS=vmon.o
SRCS=vmon.c

all: venusmon

venusmon: $(OBJS)
	$(CC) $(CFLAGS) -o venusmon $(OBJS)

clean:
	rm -f vmon.o venusmon

install: venusmon
	install -s venusmon /afs/andrew/usr0/cl0x/bin
