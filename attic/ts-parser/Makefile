PROG    = parser
OBJS    = main.o bitstream.o
CFLAGS  = -Wall -g

$(PROG): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS)
