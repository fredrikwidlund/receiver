#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <err.h>

#include <dynamic.h>

#include "ts.h"

void ts_state_init(ts_state *s)
{
  *s = (ts_state) {0};
  buffer_construct(&s->aac);
  buffer_construct(&s->h264);
  s->aac_out = open("out.aac", O_WRONLY | O_CREAT, 0664);
  if (s->aac_out == -1)
    err(1, "open");
  s->h264_out = open("out.h264", O_WRONLY | O_CREAT, 0664);
  if (s->h264_out == -1)
    err(1, "open");
}

static int ts_pcr(ts_state *s, uint8_t *data, size_t size, double *pcr)
{
  uint64_t base, ext;

  if (size < 6)
    return -1;

  base = data[0];
  base <<= 8;
  base += data[1];
  base <<= 8;
  base += data[2];
  base <<= 8;
  base += data[3];
  base <<= 1;
  base += data[4] >> 7;
  ext = data[4] & 0x01;
  ext <<= 8;
  ext += data[5];
  *pcr = (double) base / 90000 + (double) ext / 27000000;
  return 6;
}

struct ts_header {uint32_t cc:4, afc:2, tsc:2, pid:13, tp:1, pusi:1, tei:1, sync:8;};
static int ts_header(ts_state *s, uint8_t *data, size_t size, struct ts_header *h)
{
  uint32_t w;

  if (size < sizeof *h)
    return -1;
  w = *(uint32_t *) data;
  w = __bswap_32(w);
  *h = *(struct ts_header *) &w;
  if (h->sync != 'G' || h->tei || h->tp)
    return -1;
  return sizeof w;
}

struct ts_adaptation_field {uint32_t afef:1, tpdf:1, spf:1, opcrf:1, pcrf:1, espi:1, rai:1, di:1;};
static int ts_adaptation_field(ts_state *s, uint8_t *data, size_t size, struct ts_adaptation_field *af)
{
  int len, n;
  double pcr;

  if (!size)
    return -1;
  len = data[0];
  if (size < 1 + len)
    return -1;
  *af = (struct ts_adaptation_field) {0};
  if (len)
    {
      *af = *(struct ts_adaptation_field *) &data[1];
      if (af->di || af->espi || af->spf || af->tpdf || af->afef)
        return -1;
      if (af->pcrf)
        {
          n = ts_pcr(s, data + 2, size - 2, &pcr);
          if (n == -1)
            return -1;
          if (pcr < s->pcr && fabs(s->pcr - pcr) < 24 * 3600)
            return -1;
          s->pcr = pcr;
        }
    }
  return 1 + len;
}

struct ts_psi
{
  uint8_t   tid;
  uint8_t   flags;
  uint16_t  tid_ext;
  uint8_t   version;
  uint8_t   current;
  uint8_t   section;
  uint8_t   section_last;
  uint8_t  *data;
  size_t    size;
};
int ts_psi(ts_state *s, uint8_t *data, size_t size, struct ts_psi *psi)
{
  uint16_t len;

  if (size < 3)
    return -1;

  *psi = (struct ts_psi) {0};
  psi->tid = data[0];
  if (psi->tid == 0xff)
    return size;
  psi->flags = data[1] >> 2;
  len = data[1] & 0x03;
  len <<= 8;
  len += data[2];
  if (size < 3 + len)
    return -1;
  data += 3;
  size -= 3;

  if (len < 9)
    return -1;
  psi->tid_ext = *(uint16_t *) data;
  psi->tid_ext = __bswap_16(psi->tid_ext);

  //printf("tid ext %u %02x %u %u\n", psi->tid_ext, data[2], data[3], data[4]);

  psi->data = &data[5];
  psi->size = len - 9;
  return 3 + len;
}

static int ts_psi_pointer(ts_state *s, uint8_t *data, size_t size)
{
  if (!size)
    return -1;
  if (size < data[0])
    return -1;
  return 1 + data[0];
}

static int ts_pat(ts_state *s, uint8_t *data, size_t size)
{
  struct ts_psi psi;
  int consumed;
  int n;
  uint32_t w;

  consumed = 0;
  n = ts_psi_pointer(s, data, size);
  if (n == -1)
    return -1;
  data += n;
  size -= n;
  consumed += n;

  while (size)
    {
      n = ts_psi(s, data, size, &psi);
      if (n == -1)
        return -1;
      if (psi.tid != 0xff)
        {
          if (psi.tid != 0 || psi.flags != 0x2c)
            return -1;
          w = *(uint32_t *) psi.data;
          w = __bswap_32(w);
          if (w >> 16 != 1) // program number
            return -1;
          w &= 0xffff;
          if (w >> 13 != 0x07) // reserved 3 set bits
            return -1;
          w &= 0x1fff; // program map pid
          if (!s->pmt_pid)
            s->pmt_pid = w;
          if (s->pmt_pid != w)
            return -1;
        }
      data += n;
      size -= n;
      consumed += n;
    }

  return consumed;
}

static int ts_pmt_es(ts_state *s, uint8_t *data, size_t size)
{
  uint8_t type;
  uint16_t pid, len;

  if (size < 5)
    return -1;
  type = data[0];
  pid = data[1] & 0x1f;
  pid <<= 8;
  pid += data[2];
  len = data[3] & 0x03;
  len <<= 8;
  len += data[4];
  if (size < 5 + len)
    return -1;

  if (type == 0x0f) // aac stream type
    {
      if (!s->aac_pid)
        s->aac_pid = pid;
      if (s->aac_pid != pid)
        return -1;
    }

  if (type == 0x1b) // h264 stream type
    {
      if (!s->h264_pid)
        s->h264_pid = pid;
      if (s->h264_pid != pid)
        return -1;
    }
  return 5 + len;
}

static int ts_pmt(ts_state *s, uint8_t *data, size_t size)
{
  struct ts_psi psi;
  int consumed, n, n2, len;
  uint32_t w;
  uint16_t pcr_pid;
  uint8_t *es_data;
  size_t es_size;

  consumed = 0;
  n = ts_psi_pointer(s, data, size);
  if (n == -1)
    return -1;
  data += n;
  size -= n;
  consumed += n;

  while (size)
    {
      n = ts_psi(s, data, size, &psi);
      if (n == -1)
        return -1;
      if (psi.tid != 0xff)
        {
          if (psi.tid != 0x02 || psi.size < 4)
            return -1;
          w = *(uint32_t *) psi.data;
          w = __bswap_32(w);
          pcr_pid = (w >> 16) & 0x1fff;
          if (pcr_pid != 0x1fff)
            {
              if (!s->pcr_pid)
                s->pcr_pid = pcr_pid;
              if (pcr_pid != s->pcr_pid)
                return -1;
            }
          len = w & 0x3ff;
          if (sizeof w + len > psi.size)
            return -1;
          es_data = psi.data + sizeof w + len;
          es_size = psi.size - (sizeof w + len);
          while (es_size)
            {
              n2 = ts_pmt_es(s, es_data, es_size);
              if (n2 == -1)
                return -1;
              es_data += n2;
              es_size -= n2;
            }
        }
      data += n;
      size -= n;
      consumed += n;
    }

  return consumed;
}

int ts_aac(ts_state *s, uint8_t *data, size_t size, buffer *b, int fd, char *debug)
{
  uint8_t *pes, id, *aac;
  size_t pes_size, aac_size, n;
  uint32_t len;

  buffer_insert(b, buffer_size(b), data, size);
  while (buffer_size(b))
    {
      pes = buffer_data(b);
      pes_size = buffer_size(b);
      if (pes_size < 6)
        break;
      if (pes[0] != 0 || pes[1] != 0 || pes[2] != 1)
        return -1;
      id = pes[3];
      len = pes[4];
      len <<= 8;
      len += pes[5];
      if (!len)
        {
          aac = memmem(pes + 1, pes_size - 1, pes, 4);
          if (!aac)
            break;
          len = aac - pes - 6;
          pes_size = len + 6;
        }
      if (pes_size < len + 6)
        break;

      aac = pes + 6 + 3 + pes[8];
      aac_size = pes_size - (6 + 3 + pes[8]);

      n = write(fd, aac, aac_size);
      if (n != aac_size)
        err(1, "write");
      buffer_erase(b, 0, len + 6);
    }
  return size;
}

int ts(ts_state *s, uint8_t *data, size_t size)
{
  struct ts_header h;
  struct ts_adaptation_field af;
  int n;

  n = ts_header(s, data, size, &h);
  if (n == -1)
    return -1;
  data += n;
  size -= n;

  if (h.afc & 2)
    {
      n = ts_adaptation_field(s, data, size, &af);
      if (n == -1)
        return -1;
      data += n;
      size -= n;
    }

  if (h.pid == 0)
    {
      n = ts_pat(s, data, size);
      if (n == -1)
        return -1;
      data += n;
      size -= n;
    }

  if (s->pmt_pid && h.pid == s->pmt_pid)
    {
      n = ts_pmt(s, data, size);
      if (n == -1)
        return -1;
      data += n;
      size -= n;
    }

  if (s->aac_pid && h.pid == s->aac_pid)
    {
      if (s->aac_count || h.pusi)
        {
          s->aac_count ++;
          n = ts_aac(s, data, size, &s->aac, s->aac_out, "audio");
          if (n == -1)
            return -1;
        }
      else
        n = size;
      data += n;
      size -= n;
    }

  if (s->h264_pid && h.pid == s->h264_pid)
    {
      if (s->h264_count || h.pusi)
        {
          s->h264_count ++;
          n = ts_aac(s, data, size, &s->h264, s->h264_out, "video");
          if (n == -1)
            return -1;
        }
      else
        n = size;
      data += n;
      size -= n;
    }

  return 188;
}
