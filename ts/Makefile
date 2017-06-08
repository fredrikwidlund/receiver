PROG    = ts_demux
OBJS    = bits.o ts_demux.o main.o
CFLAGS  = -Wall -g -ldynamic

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS)
