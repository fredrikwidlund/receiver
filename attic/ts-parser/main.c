#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <err.h>

#include "bitstream.h"

typedef struct ts_parser ts_parser;
struct ts_parser
{
  int x;
};

void ts_parser_construct(ts_parser *ts)
{
  *ts = (ts_parser) {0};
}

static uint64_t ts_parser_pts(bitstream *s)
{
  uint64_t pts;

  (void) bitstream_read(s, 4);
  pts = bitstream_read(s, 3);
  (void) bitstream_read(s, 1);
  pts <<= 15;
  pts += bitstream_read(s, 15);
  (void) bitstream_read(s, 1);
  pts <<= 15;
  pts += bitstream_read(s, 15);
  (void) bitstream_read(s, 1);
  return pts;
}

int ts_parser_audio(ts_parser *ts, void *data, size_t size)
{
  bitstream s;
  uint32_t sync;
  uint8_t id, layer, prot;

  bitstream_construct(&s, data, 8 * size);
  sync = bitstream_read(&s, 12);
  id = bitstream_read(&s, 1);
  layer = bitstream_read(&s, 2);
  prot = bitstream_read(&s, 1);

  printf("[audio valid %d, %03x, %u, %u, %u]\n", bitstream_valid(&s), sync, id, layer, prot);
  return 0;
}

int ts_parser_video(ts_parser *ts, void *data, size_t size)
{
  bitstream s;
  uint32_t code;
  uint8_t id;

  bitstream_construct(&s, data, 8 * size);
  code = bitstream_read(&s, 32);
  id = bitstream_read(&s, 8);

  printf("[video valid %d, %08x, %02x]\n", bitstream_valid(&s), code, id);
  return 0;
}

int ts_parser_pes(ts_parser *ts, void *data, size_t size)
{
  bitstream s;
  uint32_t code;
  uint16_t len, flags, opt_len;
  uint8_t id;
  uint64_t pts, dts;

  bitstream_construct(&s, data, 8 * size);
  code = bitstream_read(&s, 24);
  id = bitstream_read(&s, 8);
  len = bitstream_read(&s, 16);

  flags = 0;
  pts = 0;
  dts = 0;
  if (id >= 0xc0 && id <= 0xef)
    {
      (void) bitstream_read(&s, 8);
      flags = bitstream_read(&s, 8);
      opt_len = bitstream_read(&s, 8);
      if (flags & 0x80)
        {
          pts = ts_parser_pts(&s);
          opt_len -= 5;
        }
      if (flags & 0x40)
        {
          dts = ts_parser_pts(&s);
          opt_len -= 5;
        }
      (void) bitstream_consume_bytes(&s, opt_len);
    }

  printf("[PES size %lu, code %06x, id %02x, len %u, flags %04x, data %lu]\n", size, code, id, len, flags, (s.size - s.offset) / 8);
  if (pts)
    printf("[PTS %lu, id %02x]\n", pts, id);
  if (dts)
    printf("[DTS %lu, id %02x]\n", dts, id);

  if (id == 0xe0)
    return ts_parser_video(ts, (void *) &s.data[s.offset / 8], (s.size - s.offset) / 8);
  if (id == 0xc0)
    return ts_parser_audio(ts, (void *) &s.data[s.offset / 8], (s.size - s.offset) / 8);
  return 0;
}

int ts_parser_data(ts_parser *ts, void *data, size_t size)
{
  bitstream s;
  uint8_t sync, tei, pusi, tp, tsc, afc, cc, af_len, af_flags;
  uint16_t pid;
  uint32_t pcr_base, pcr_ext;
  double pcr = 0;

  bitstream_construct(&s, data, 8 * size);
  sync = bitstream_read(&s, 8);
  tei = bitstream_read(&s, 1);
  pusi = bitstream_read(&s, 1);
  tp = bitstream_read(&s, 1);
  pid = bitstream_read(&s, 13);
  tsc = bitstream_read(&s, 2);
  afc = bitstream_read(&s, 2);
  cc = bitstream_read(&s, 4);

  if (afc & 0x02)
    {
      af_len = bitstream_read(&s, 8);
      af_flags = bitstream_read(&s, 8);
      af_len --;
      if (af_flags & 0x10)
        {
          pcr_base = bitstream_read(&s, 33);
          (void) bitstream_read(&s, 6);
          pcr_ext = bitstream_read(&s, 9);
          pcr = (double) pcr_base / 90000. + (double) pcr_ext / 27000000.;
          af_len -= 6;
        }
      (void) bitstream_consume_bytes(&s, af_len);
    }

  printf("[%c %u %u %u %u %u %u %u]\n", sync, tei, pusi, tp, pid, tsc, afc, cc);
  if (pcr > 0)
    printf("[PCR %f]\n", pcr);

  if (sync != 'G' || !bitstream_valid(&s) || s.offset % 8)
    return -1;

  if (pusi)
    return ts_parser_pes(ts, (void *) &s.data[s.offset / 8], (s.size - s.offset) / 8);

  return 0;
}

int main()
{
  ts_parser ts;
  char packet[188];
  ssize_t n;
  int e;

  ts_parser_construct(&ts);
  while (1)
    {
      n = read(0, packet, sizeof packet);
      if (n == 0)
        break;

      if (n == -1 || n != 188)
        err(1, "read");

      e = ts_parser_data(&ts, packet, n);
      if (e == -1)
        errx(1, "ts_parser_data");
    }
}
