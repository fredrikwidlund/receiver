#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/queue.h>

#include <dynamic.h>
#include <reactor.h>

#include "bits.h"
#include "ts_demux.h"

static void ts_demux_analyze_stream(ts_demux *, ts_demux_stream *);

static void ts_demux_error(ts_demux *d)
{
  (void) fprintf(stderr, "error\n");
}

static ts_demux_stream *ts_demux_streams_new(ts_demux *d, int pid)
{
  ts_demux_stream *s;

  s = calloc(1, sizeof *s);
  s->type = pid ? TS_DEMUX_STREAM_TYPE_UNKNOWN : TS_DEMUX_STREAM_TYPE_PAT;
  s->cc = -1;
  s->pid = pid;
  vector_construct(&s->units, sizeof (ts_demux_stream_unit *));
  vector_push_back(&d->streams, &s);
  return s;
}

static ts_demux_stream *ts_demux_streams_lookup(ts_demux *d, int pid, int create)
{
  ts_demux_stream *s;
  size_t i;

  for (i = 0; i < vector_size(&d->streams); i ++)
    {
      s = *(ts_demux_stream **) vector_at(&d->streams, i);
      if (s->pid == pid)
        return s;
    }

  return ts_demux_streams_new(d, pid);
}

void ts_demux_parse_psi_table(ts_demux *d, bits *b, uint8_t *end, uint16_t *tid_ext, uint8_t *section, uint8_t *last_section, bits *data)
{
  uint16_t tid, ssi, len, current;

  *end = 1;
  tid = bits_read(b, 8);
  if (tid == 0xff)
    {
      bits_flush(b);
      return;
    }
  *end = 0;

  ssi = bits_read(b, 1);
  (void) bits_read(b, 1);
  (void) bits_read(b, 2);
  (void) bits_read(b, 2);
  len = bits_read(b, 10);
  if (bits_valid(b) && !ssi)
    {
      bits_clear(b);
      return;
    }

  *data = bits_subset(b, 0, len * 8 - 32);
  *tid_ext = bits_read(data, 16);
  (void) bits_read(data, 2);
  (void) bits_read(data, 5);
  current = bits_read(data, 1);
  if (bits_valid(data) && !current)
    {
      bits_clear(b);
      return;
    }

  *section = bits_read(data, 8);
  *last_section = bits_read(data, 8);
}

static void ts_demux_parse_psi_pointer(ts_demux *d, bits *b)
{
  uint8_t len;

  len = bits_read(b, 8);
  bits_read_data(b, NULL, len);
}

void ts_demux_parse_pmt_stream(ts_demux *d, bits *b)
{
  ts_demux_stream *stream;
  uint16_t pid, len;
  uint8_t type;

  (void) d;
  type = bits_read(b, 8);
  (void) bits_read(b, 3);
  pid = bits_read(b, 13);
  (void) bits_read(b, 4);
  (void) bits_read(b, 2);
  len =  bits_read(b, 10);
  bits_read_data(b, NULL, len);

  stream = ts_demux_streams_lookup(d, pid, 1);
  stream->type = TS_DEMUX_STREAM_TYPE_PES;
  stream->stream_type = type;
  ts_demux_analyze_stream(d, stream);
}

void ts_demux_parse_pmt_data(ts_demux *d, bits *b)
{
  int16_t len;

  (void) bits_read(b, 3);
  (void) bits_read(b, 13);
  (void) bits_read(b, 4);
  (void) bits_read(b, 2);
  len = bits_read(b, 10);
  bits_read_data(b, NULL, len);

  while (bits_size(b))
    ts_demux_parse_pmt_stream(d, b);
}

static void ts_demux_parse_pmt(ts_demux *d, bits *b)
{
  uint16_t tid_ext;
  uint8_t end, section, last_section;
  bits data;

  ts_demux_parse_psi_pointer(d, b);
  while (bits_valid(b) && bits_size(b))
    {
      ts_demux_parse_psi_table(d, b, &end, &tid_ext, &section, &last_section, &data);
      if (!bits_valid(b) || end)
        break;
      ts_demux_parse_pmt_data(d, &data);
      if (section == last_section)
        {
          bits_flush(b);
          break;
        }
    }
}

static void ts_demux_parse_pat_data(ts_demux *d, bits *b)
{
  ts_demux_stream *stream;
  int16_t pmt_pid;

  (void) bits_read(b, 16);
  (void) bits_read(b, 3);
  pmt_pid = bits_read(b, 13);
  if (bits_valid(b))
    {
      stream = ts_demux_streams_lookup(d, pmt_pid, 1);
      stream->type = TS_DEMUX_STREAM_TYPE_PMT;
      ts_demux_analyze_stream(d, stream);
    }
}

static void ts_demux_parse_pat(ts_demux *d, bits *b)
{
  uint16_t tid_ext;
  uint8_t end, section, last_section;
  bits data;

  ts_demux_parse_psi_pointer(d, b);
  while (bits_valid(b) && bits_size(b))
    {
      ts_demux_parse_psi_table(d, b, &end, &tid_ext, &section, &last_section, &data);
      if (!bits_valid(b) || end)
        break;
      ts_demux_parse_pat_data(d, &data);
      if (section == last_section)
        {
          bits_flush(b);
          break;
        }
    }
}

static uint64_t ts_demux_parse_pts(bits *b)
{
  uint64_t pts;

  (void) bits_read(b, 4);
  pts = bits_read(b, 3);
  (void) bits_read(b, 1);
  pts <<= 15;
  pts += bits_read(b, 15);
  (void) bits_read(b, 1);
  pts <<= 15;
  pts += bits_read(b, 15);
  (void) bits_read(b, 1);

  return pts;
}

static int ts_demux_parse_pes(ts_demux *d, int type, bits *b)
{
  uint64_t pts, dts;
  uint32_t code;
  uint16_t len;
  uint8_t id, marker, scrambling, priority, dai, has_pts, has_dts, has_crc, remains;

  if (type != 0x0f && type != 0x1b)
      return -1;

  code = bits_read(b, 24);
  if (code != 0x000001)
    return -1;

  id = bits_read(b, 8);
  len = bits_read(b, 16);
  printf("pes code %06x, id %02x, len %d\n", code, id, len);
  if (len >= 3)
    {
      marker = bits_read(b, 2);
      scrambling = bits_read(b, 2);
      priority = bits_read(b, 1);
      dai = bits_read(b, 1);
      (void) bits_read(b, 2);
      has_pts = bits_read(b, 1);
      has_dts = bits_read(b, 1);
      (void) bits_read(b, 4);
      has_crc = bits_read(b, 1);
      (void) bits_read(b, 1);

      remains = bits_read(b, 8);
      if (!bits_valid(b) || marker != 0x02 || scrambling || !dai)
        return -1;
      len -= 3 + remains;
      if (has_pts)
        {
          pts = ts_demux_parse_pts(b);
          remains -= 5;
          printf("pts %lu\n", pts);
        }
      if (has_dts)
        {
          dts = ts_demux_parse_pts(b);
          remains -= 5;
          printf("dts %lu\n", dts);
        }
     bits_read_data(b, NULL, remains);

      printf("optional header %d %d %d %d, crc %d\n", marker, scrambling, priority, dai, has_crc);
    }

  if (!bits_valid(b))
    return -1;

  printf("data %lu, len %u\n", bits_size(b) / 8, len);
  return 0;
}

static void ts_demux_analyze_stream(ts_demux *d, ts_demux_stream *stream)
{
  ts_demux_stream_unit *unit;
  bits b;

  if (!stream || !vector_size(&stream->units) || stream->flags & TS_DEMUX_STREAM_FLAG_DONE)
    return;

  switch (stream->type)
    {
    case TS_DEMUX_STREAM_TYPE_UNKNOWN:
      break;
    case TS_DEMUX_STREAM_TYPE_PAT:
      unit = *(ts_demux_stream_unit **) vector_front(&stream->units);
      bits_set_data(&b, buffer_data(&unit->data), buffer_size(&unit->data));
      ts_demux_parse_pat(d, &b);
      if (!bits_size(&b) && bits_valid(&b))
        stream->flags |= TS_DEMUX_STREAM_FLAG_DONE;
      break;
    case TS_DEMUX_STREAM_TYPE_PMT:
      unit = *(ts_demux_stream_unit **) vector_front(&stream->units);
      bits_set_data(&b, buffer_data(&unit->data), buffer_size(&unit->data));
      ts_demux_parse_pmt(d, &b);
      if (!bits_size(&b) && bits_valid(&b))
        stream->flags |= TS_DEMUX_STREAM_FLAG_DONE;
      break;
    case TS_DEMUX_STREAM_TYPE_PES:
        while (vector_size(&stream->units) > 1 || (d->state == TS_DEMUX_STATE_EOF && vector_size(&stream->units) == 1))
          {
            unit = *(ts_demux_stream_unit **) vector_front(&stream->units);
            bits_set_data(&b, buffer_data(&unit->data), buffer_size(&unit->data));
            ts_demux_parse_pes(d, stream->stream_type, &b);
            vector_erase(&stream->units, 0);
          }
      break;
    }
}

static double ts_demux_parse_pcr(ts_demux *d, bits *b)
{
  double pcr_base, pcr_ext;

  pcr_base = bits_read(b, 33);
  (void) bits_read(b, 6);
  pcr_ext = bits_read(b, 9);
  return pcr_base / 90000. + pcr_ext / 27000000.;
}

static void ts_demux_parse_ts(ts_demux *d, bits *b)
{
  ts_demux_stream *stream;
  ts_demux_stream_unit *unit;
  uint8_t sync, tei, pusi, tsc, cc, len, has_af, has_payload, has_pcr = 0, rai = 0, *data;
  uint16_t pid;
  size_t size;

  sync = bits_read(b, 8);
  tei = bits_read(b, 1);
  pusi = bits_read(b, 1);
  (void) bits_read(b, 1);
  pid = bits_read(b, 13);
  tsc = bits_read(b, 2);
  has_af = bits_read(b, 1);
  has_payload = bits_read(b, 1);
  cc = bits_read(b, 4);
  has_pcr = 0;
  if (has_af)
    {
      len = bits_read(b, 8);
      if (len)
        {
          (void) bits_read(b, 1);
          rai = bits_read(b, 1);
          (void) bits_read(b, 1);
          has_pcr = bits_read(b, 1);
          (void) bits_read(b, 4);
          len --;
          if (has_pcr)
            {
              d->pcr = ts_demux_parse_pcr(d, b);
              len -= 6;
            }
          bits_read_data(b, NULL, len);
        }
    }

  bits_get_data(b, &data, &size);
  if (!bits_valid(b) || sync != 'G' || tei || tsc)
    {
      ts_demux_error(d);
      return;
    }

  stream = ts_demux_streams_lookup(d, pid, 1);
  if (stream->cc >= 0 && ((stream->cc + 1) & 0x0f) != cc)
    {
      ts_demux_error(d);
      return;
    }
  stream->cc = cc;

  if (pusi)
    {
      unit = calloc(1, sizeof *unit);
      unit->rai = rai;
      buffer_construct(&unit->data);
      vector_push_back(&stream->units, &unit);
    }

  if (!vector_size(&stream->units))
    return;

  unit = *(ts_demux_stream_unit **) vector_back(&stream->units);
  if (has_payload)
    buffer_insert(&unit->data, buffer_size(&unit->data), data, size);

  ts_demux_analyze_stream(d, stream);
}

void ts_demux_open(ts_demux *d, reactor_user_callback *callback, void *data)
{
  *d = (ts_demux) {.state = TS_DEMUX_STATE_OPEN, .pcr = -1.};
  reactor_user_construct(&d->user, callback, data);
  buffer_construct(&d->input);
  vector_construct(&d->streams, sizeof (ts_demux_stream *));
}

void ts_demux_close(ts_demux *d)
{
  d->state = TS_DEMUX_STATE_CLOSED;
}

void ts_demux_data(ts_demux *d, void *data, size_t size)
{
  uint8_t *p;
  size_t n;
  bits b;

  buffer_insert(&d->input, buffer_size(&d->input), data, size);
  for (p = buffer_data(&d->input), n = buffer_size(&d->input); n >= 188; p += 188, n -= 188)
    {
      bits_set_data(&b, p, 188);
      ts_demux_parse_ts(d, &b);
    }
  buffer_erase(&d->input, 0, buffer_size(&d->input) - n);
}

void ts_demux_eof(ts_demux *d)
{
  if (d->state == TS_DEMUX_STATE_OPEN)
    d->state = TS_DEMUX_STATE_EOF;
}
