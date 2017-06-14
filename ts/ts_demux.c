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
  ts_demux_stream_unit *u;

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
      if (!u)
        return -1;
      u->rai = rai == 1 ? 1 : 0;
      buffer_construct(&u->data);
      TAILQ_INSERT_TAIL(&s->units, u, entries);
    }
  buffer_insert(&u->data, buffer_size(&u->data), data, size);

  return 0;
}

void ts_demux_stream_write_eof(ts_demux_stream *s)
{
  ts_demux_stream_unit *u;

  u = TAILQ_LAST(&s->units, ts_demux_stream_units_head);
  if (u && !u->complete)
    u->complete = 1;
}

void ts_demux_construct(ts_demux *d)
{
  d->flags = 0;
  vector_construct(&d->streams, sizeof (ts_demux_stream *));
}

void ts_demux_parse_pat_data(ts_demux *d, bits *bits)
{
  ts_demux_stream *s;
  int16_t pmt_pid;

  (void) bits_read(bits, 16);
  (void) bits_read(bits, 3);
  pmt_pid = bits_read(bits, 13);
  if (bits_valid(bits))
    {
      s = ts_demux_streams_lookup(d, pmt_pid, 1);
      s->type = TS_DEMUX_STREAM_TYPE_PMT;
    }
}

void ts_demux_parse_psi_pointer(ts_demux *d, bits *b)
{
  uint16_t len;

  (void) d;
  len = bits_read(b, 8);
  bits_read_data(b, NULL, len);
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

int ts_demux_parse_pat(ts_demux *d, bits *pat)
{
  bits table;
  uint8_t len, tid, ssi, pb, v, current, sn, lsn;
  uint16_t s_len, tie;

  ts_demux_parse_psi_pointer(d, pat);
  while (bits_valid(pat) && bits_size(pat))
    {
      tid = bits_read(pat, 8);
      if (tid == 0xff)
        break;
      ssi = bits_read(pat, 1);
      pb = bits_read(pat, 1);
      (void) bits_read(pat, 2);
      (void) bits_read(pat, 2);
      s_len = bits_read(pat, 10);

      table = bits_subset(pat, 0, s_len * 8 - 32);
      tie = bits_read(&table, 16);
      (void) bits_read(&table, 2);
      v = bits_read(&table, 5);
      current = bits_read(&table, 1);
      sn = bits_read(&table, 8);
      lsn = bits_read(&table, 8);
      printf("tid %u, len %u, tie %u, version %u, current %u, %u/%u\n", tid, s_len, tie, v, current, sn, lsn);

      while (bits_size(&table))
        ts_demux_parse_pat_data(d, &table);
      if (!bits_valid(&table))
        return 0;
      bits_read_data(pat, NULL, s_len);
    }

  return 0;
}

void ts_demux_parse_pmt_stream(ts_demux *d, bits *b)
{
  ts_demux_stream *s;
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


  s = ts_demux_streams_lookup(d, pid, 1);
  s->type = TS_DEMUX_STREAM_TYPE_ES;
  s->stream_type = type;
  printf("type %02x, pid %u, len %u\n", type, pid, len);
}

void ts_demux_parse_pmt_data(ts_demux *d, bits *b)
{
  int16_t pcr_pid, len;

  (void) bits_read(b, 3);
  pcr_pid = bits_read(b, 13);
  (void) bits_read(b, 4);
  (void) bits_read(b, 2);
  len = bits_read(b, 10);
  printf("PMT pcr %u, len %u\n", pcr_pid, len);
  bits_read_data(b, NULL, len);

  /*
  desc = bits_subset(b, 0, len * 8);
  while (bits_size(&desc))
  ts_demux_parse_pmt_stream(d, &desc);*/

  while (bits_size(b))
    ts_demux_parse_pmt_stream(d, b);
}


int ts_demux_parse_pmt(ts_demux *d, bits *b)
{
  uint16_t tid_ext;
  uint8_t end, section, last_section;
  bits data;

  ts_demux_parse_psi_pointer(d, b);
  while (bits_valid(b) && bits_size(b))
    {

      ts_demux_parse_psi_table(d, b, &end, &tid_ext, &section, &last_section, &data);
      if (!bits_valid(b))
        return -1;
      if (end)
        break;
      printf("PMT %u %u/%u\n", tid_ext, section, last_section);
      ts_demux_parse_pmt_data(d, &data);
      if (section == last_section)
        break;
    }

  return 0;
}

int ts_demux_parse(ts_demux *d, uint8_t *data, size_t size)
{
  ts_demux_stream *stream;
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

  (void) tp; (void) cc; (void) pcr; (void) has_pcr; (void) tei; (void) tsc; (void) sync;
  /*
  printf("pid %u, pusi %u, tp %d, cc %u, rai %d, size %lu\n", pid, pusi, tp, cc, rai, size);
  if (has_pcr)
    printf("pid %u, pcr %f\n", pid, pcr);
  */

  stream = ts_demux_streams_lookup(d, pid, 1);
  e = ts_demux_stream_write(stream, cc, rai, pusi, data, size);
  if (e == -1)
    return -1;

  e = ts_demux_stream_analyze(d, stream);
  if (e == -1)
    return -1;

  return 0;
}

int ts_demux_stream_analyze(ts_demux *d, ts_demux_stream *s)
{
  ts_demux_stream_unit *u;
  bits bits;
  int e;

  switch (s->type)
    {
    case TS_DEMUX_STREAM_TYPE_UNKNOWN:
      break;
    case TS_DEMUX_STREAM_TYPE_PAT:
      u = TAILQ_FIRST(&s->units);
      if (!u && u->analyzed)
        break;
      bits_set_data(&bits, buffer_data(&u->data), buffer_size(&u->data));
      e = ts_demux_parse_pat(d, &bits);
      if (e == -1)
        return -1;
      u->analyzed = e;
      u->complete = e;
      break;
    case TS_DEMUX_STREAM_TYPE_PMT:
      u = TAILQ_FIRST(&s->units);
      if (!u && u->analyzed)
        break;
      bits_set_data(&bits, buffer_data(&u->data), buffer_size(&u->data));
      printf("analyze pmt\n");
      e = ts_demux_parse_pmt(d, &bits);
      u->analyzed = e;
      u->complete = e;
      break;
    }

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
  return *(ts_demux_stream **) vector_at(&d->streams, i);
}

ts_demux_stream *ts_demux_streams_lookup(ts_demux *d, int pid, int add)
{
  ts_demux_stream *s;
  size_t i;

  for (i = 0; i < vector_size(&d->streams); i ++)
    {
      s = ts_demux_streams_index(d, i);
      if (s->pid == pid)
        return s;
    }

  if (!add)
    return NULL;

  s = calloc(1, sizeof *s);
  s->type = pid ? TS_DEMUX_STREAM_TYPE_UNKNOWN : TS_DEMUX_STREAM_TYPE_PAT;
  s->empty = 1;
  s->pid = pid;
  TAILQ_INIT(&s->units);
  vector_push_back(&d->streams, &s);

  return s;
}
