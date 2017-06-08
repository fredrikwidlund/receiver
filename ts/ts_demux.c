#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <err.h>

#include <dynamic.h>

#include "bits.h"
#include "ts_demux.h"

void ts_demux_construct(ts_demux *d)
{
  vector_construct(&d->streams, sizeof (ts_demux_stream));
}

int ts_demux_stream_write(ts_demux *d, int pid, int pusi, int cc, uint8_t *data, size_t size)
{
  ts_demux_stream *s = vector_data(&d->streams), *last = vector_back(&d->streams);

  for (; s && s <= last; s ++)
    if (s->pid == pid)
      break;
  if (!s || s > last)
    {
      vector_push_back(&d->streams, (ts_demux_stream []){{0}});
      s = vector_back(&d->streams);
      s->pid = pid;
      s->cc = (cc - 1) & 0x0f;
      printf("pid %u cc %u s->cc %u\n", pid, cc, s->cc);
      buffer_construct(&s->data);
    }

  if (((s->cc + 1) & 0x0f ) != cc)
    return -1;
  buffer_insert(&s->data, buffer_size(&s->data), data, size);
  s->cc = cc;

  return 0;
}

int ts_demux_parse(ts_demux *d, uint8_t *data, size_t size)
{
  bits s;
  uint8_t sync, tei, pusi, tp, tsc, afc, cc, len, flags, has_pcr;
  uint16_t pid;
  double pcr_base, pcr_ext, pcr;
  int e;

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
  if (afc & 0x02)
    {
      len = bits_read(&s, 8);
      if (len)
        {
          flags = bits_read(&s, 8);
          len --;
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
  printf("pid %u, pusi %u, cc %u, size %lu\n", pid, pusi, cc, size);
  if (has_pcr)
    printf("pid %u, pcr %f\n", pid, pcr);


  e = ts_demux_stream_write(d, pid, pusi, cc, data, size);
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
