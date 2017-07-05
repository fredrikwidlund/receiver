PROG    = ts_demux
OBJS    = src/main.o src/ts_demux.o src/bits.o
CFLAGS  = -Wall -pedantic -g -Isrc
LDFLAGS = -lreactor -ldynamic

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

test: src/bitstream.o src/test.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS)
