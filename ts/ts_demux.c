#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>

#include "bits.h"
#include "ts_demux.h"

void ts_demux_stream_debug(ts_demux_stream *s)
{
  ts_demux_stream_unit *u;
  int n;

  (void) fprintf(stderr, "[pid %u, type %d\n", s->pid, s->type);
  n = 0;
  TAILQ_FOREACH(u, &s->units, entries)
    {
      fprintf(stderr, "size %lu, rai %d, complete %d\n", buffer_size(&u->data), u->rai, u->complete);
      n ++;
    }
  (void) fprintf(stderr, "units %d\n", n);
}


int ts_demux_stream_write(ts_demux_stream *s, int cc, int rai, int pusi, uint8_t *data, size_t size)
{
  ts_demux_stream_unit *u, *u2;

  if (!s->empty && ((s->cc + 1) & 0x0f ) != cc)
    return -1;
  s->empty = 0;
  s->cc = cc;

  u = TAILQ_LAST(&s->units, ts_demux_stream_units_head);
  if (pusi || !u || u->complete)
    {
      if (!pusi)
        return -1;
      if (pusi && u && !u->complete)
        u->complete = 1;
      u = calloc(1, sizeof *u);
      printf("%d -> %p\n", s->pid, u);
      if (!u)
        return -1;
      u->rai = rai == 1 ? 1 : 0;
      buffer_construct(&u->data);
      TAILQ_INSERT_TAIL(&s->units, u, entries);
      printf("pid %d last %p\n", s->pid, TAILQ_LAST(&s->units, ts_demux_stream_units_head));
    }
  buffer_insert(&u->data, buffer_size(&u->data), data, size);

  return 0;
}

void ts_demux_stream_write_eof(ts_demux_stream *s)
{
  ts_demux_stream_unit *u;

  printf("pid %d last %p\n", s->pid, TAILQ_LAST(&s->units, ts_demux_stream_units_head));
  u = TAILQ_LAST(&s->units, ts_demux_stream_units_head);
  if (u && !u->complete)
    u->complete = 1;
}

void ts_demux_construct(ts_demux *d)
{
  vector_construct(&d->streams, sizeof (ts_demux_stream));
}

int ts_demux_parse(ts_demux *d, uint8_t *data, size_t size)
{
  bits s;
  uint8_t sync, tei, pusi, tp, tsc, afc, cc, len, flags, has_pcr;
  uint16_t pid;
  double pcr_base, pcr_ext, pcr;
  int e, rai;

  bits_set_data(&s, data, size);
  sync = bits_read(&s, 8);
  tei = bits_read(&s, 1);
  pusi = bits_read(&s, 1);
  tp = bits_read(&s, 1);
  pid = bits_read(&s, 13);
  tsc = bits_read(&s, 2);
  afc = bits_read(&s, 2);
  cc = bits_read(&s, 4);

  has_pcr = 0;
  rai = -1;
  if (afc & 0x02)
    {
      len = bits_read(&s, 8);
      if (len)
        {
          flags = bits_read(&s, 8);
          len --;
          rai = flags & 0x80 ? 1 : 0;
          if (flags & 0x10)
            {
              has_pcr = 1;
              pcr_base = bits_read(&s, 33);
              (void) bits_read(&s, 6);
              pcr_ext = bits_read(&s, 9);
              pcr = (double) pcr_base / 90000. + (double) pcr_ext / 27000000.;
              len -= 6;
            }
        }
      bits_read_data(&s, NULL, len);
    }

  bits_get_data(&s, &data, &size);
  if (!bits_valid(&s))
    return -1;

  /*
  printf("pid %u, pusi %u, tp %d, cc %u, rai %d, size %lu\n", pid, pusi, tp, cc, rai, size);
  if (has_pcr)
    printf("pid %u, pcr %f\n", pid, pcr);
  */

  e = ts_demux_stream_write(ts_demux_streams_lookup(d, pid), cc, rai, pusi, data, size);
  if (e == -1)
    return -1;

  return 0;
}

int ts_demux_write(ts_demux *d, void *data, size_t size)
{
  int e;
  uint8_t *p = data;

  if (size % 188)
    return -1;

  for (; size; p += 188, size -= 188)
    {
      e = ts_demux_parse(d, p, 188);
      if (e == -1)
        return -1;
    }

  return 0;
}

void ts_demux_write_eof(ts_demux *d)
{
  size_t i;

  for (i = 0; i < ts_demux_streams_size(d); i ++)
    ts_demux_stream_write_eof(ts_demux_streams_index(d, i));
}

size_t ts_demux_streams_size(ts_demux *d)
{
  return vector_size(&d->streams);
}

ts_demux_stream *ts_demux_streams_index(ts_demux *d, size_t i)
{
  if (i >= vector_size(&d->streams))
    return NULL;
  return vector_at(&d->streams, i);
}

ts_demux_stream *ts_demux_streams_lookup(ts_demux *d, int pid)
{
  ts_demux_stream *s = vector_data(&d->streams), *last = vector_back(&d->streams);

  for (; s && s <= last; s ++)
    if (s->pid == pid)
      return s;

  vector_push_back(&d->streams, (ts_demux_stream []){{0}});
  s = vector_back(&d->streams);
  s->type = pid ? TS_DEMUX_STREAM_TYPE_UNKNOWN : TS_DEMUX_STREAM_TYPE_PAT;
  s->empty = 1;
  s->pid = pid;
  TAILQ_INIT(&s->units);
  return s;
}
