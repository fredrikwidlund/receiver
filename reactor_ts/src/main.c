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
  ts_demux_message *m;

  switch (type)
    {
    case TS_DEMUX_EVENT_ERROR:
      break;
    case TS_DEMUX_EVENT_MESSAGE:
      m = message;
      (void) fprintf(stderr, "pid %d, type %02x, id %02x", m->pid, m->type, m->id);
      if (m->has_pts)
        (void) fprintf(stderr, ", pts %lu", m->pts);
      if (m->has_dts)
        (void) fprintf(stderr, ", dts %lu", m->dts);
      (void) fprintf(stderr, "\n");
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
