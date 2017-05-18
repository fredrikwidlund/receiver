PROG    = receiver
OBJS    = src/main.o src/receiver.o
CFLAGS  = -Wall -g -Isrc
LDFLAGS = -ldynamic -lreactor -lavformat -lavcodec -lavdevice -lavutil

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)
