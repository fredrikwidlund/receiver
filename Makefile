PROG    = receiver
OBJS    = src/ts.o src/rtp.o src/main.o src/receiver.o
CFLAGS  = -Wall -g -Isrc
LDFLAGS = -lreactor -ldynamic -lavformat -lavcodec -lavdevice -lavutil -lm

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS)
