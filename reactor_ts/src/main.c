#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "ts_demux.h"

typedef struct state state;
struct state
{
  ts_demux demux;
};

void event(void *data, int type, void *message)
{
  switch (type)
    {
    case TS_DEMUX_EVENT_ERROR:
      break;
    }
}

int main()
{
  state s;
  char data[1024];
  ssize_t n;

  ts_demux_open(&s.demux, event, &s);

  while (1)
    {
      n = read(0, data, sizeof data);
      if (n == -1)
        err(1, "read");
      if (n == 0)
        {
          ts_demux_eof(&s.demux);
          break;
        }
      ts_demux_data(&s.demux, data, n);
    }

  ts_demux_close(&s.demux);
}
