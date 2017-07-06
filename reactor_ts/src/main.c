#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "bits.h"
#include "ts_demux.h"
#include "ts_demux_adts.h"

typedef struct stream stream;
struct stream
{
  int pid;
  int fd;
};

typedef struct state state;
struct state
{
  ts_demux demux;
  vector   streams;
};

void save(state *s, ts_demux_message *m, char *ext)
{
  stream **streams, *stream;
  char path[4096];
  size_t i, n;

  stream = NULL;
  streams = vector_data(&s->streams);
  for (i = 0; i < vector_size(&s->streams); i ++)
    {
      if (streams[i]->pid == m->pid)
        {
          stream = streams[i];
          break;
        }
    }

  if (!stream)
    {
      (void) snprintf(path, sizeof path, "pid-%d.%s", m->pid, ext);
      stream = malloc(sizeof *stream);
      stream->pid = m->pid;
      stream->fd = open(path, O_WRONLY | O_CREAT, 0644);
      if (stream->fd == -1)
        err(1, "open");
      vector_push_back(&s->streams, &stream);
    }

  n = write(stream->fd, m->data, m->size);
  if (n != m->size)
    err(1, "write");
}

void aac(state *s, ts_demux_message *m)
{
  ts_demux_adts_state adts;
  uint8_t *aac;
  size_t size;
  uint64_t pts, duration;
  int freq, e;

  ts_demux_adts_construct(&adts, m->data, m->size, m->pts);
  while (1)
    {
      e = ts_demux_adts_parse(&adts, &aac, &size, &pts, &duration, &freq);
      if (e == -1)
        errx(1, "ts_demux_adts_parse");
      if (e == 0)
        break;
      (void) fprintf(stderr, "[aac] size %lu, pts %lu, duration %lu, frequency %d\n", size, pts, duration, freq);
    }
}

void h264(state *s, ts_demux_message *m)
{
  (void) fprintf(stderr, "[h264] size %lu, keyframe %d", m->size, m->rai);
  if (m->has_pts)
    (void) fprintf(stderr, ", pts %lu", m->pts);
  if (m->has_dts)
    (void) fprintf(stderr, ", dts %lu", m->dts);
  (void) fprintf(stderr, "\n");
}

void event(void *data, int type, void *message)
{
  ts_demux_message *m;
  state *s = data;

  switch (type)
    {
    case TS_DEMUX_EVENT_ERROR:
      errx(1, "demux error");
      break;
    case TS_DEMUX_EVENT_MESSAGE:
      m = message;
      switch (m->type)
        {
        case 0x0f:
          save(s, m, "aac");
          aac(s, m);
          break;
        case 0x1b:
          save(s, m, "h264");
          h264(s, m);
          break;
        default:
          save(s, m, "raw");
          (void) fprintf(stderr, "[unknown]\n");
          break;
        }
      break;
    }
}

int main()
{
  state s;
  char data[1046576];
  ssize_t n;

  s = (state) {0};
  vector_construct(&s.streams, sizeof (stream));
  ts_demux_open(&s.demux, event, &s);

  while (1)
    {
      n = read(0, data, sizeof data);
      if (n == -1)
        err(1, "read");
      if (n == 0)
        break;
      ts_demux_data(&s.demux, data, n);
    }

  ts_demux_eof(&s.demux);
  ts_demux_close(&s.demux);
}
