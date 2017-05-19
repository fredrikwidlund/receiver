PROG    = receiver
OBJS    = src/main.o src/receiver.o
CFLAGS  = -Wall -g -Isrc
LDFLAGS = -lreactor -ldynamic -lavformat -lavcodec -lavdevice -lavutil

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS)
