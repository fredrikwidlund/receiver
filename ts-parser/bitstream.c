#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/param.h>

#include "bitstream.h"

void bitstream_construct(bitstream *s, const void *data, size_t size)
{
  *s = (bitstream) {.data = data, .size = size};
}

uint64_t bitstream_read_offset(bitstream *s, size_t offset, size_t count)
{
  uint64_t v;
  uint8_t b;
  int n;

  if (!bitstream_valid(s) || count > 64 || count + offset > s->size)
    {
      s->flags |= BITSTREAM_FLAG_INVALID;
      return 0;
    }

  v = 0;
  while (count)
    {
      b = s->data[offset / 8] << offset % 8;
      n = MIN(8 - (offset % 8), count);
      v <<= n;
      v += b >> (8 - n);
      count -= n;
      offset += n;
    }

  return v;
}

uint64_t bitstream_consume_bits(bitstream *s, size_t count)
{
  uint64_t v;

  v = bitstream_read_offset(s, s->offset, count);
  if (bitstream_valid(s))
    s->offset += count;
  return v;
}

uint64_t bitstream_read(bitstream *s, size_t count)
{
  return bitstream_consume_bits(s, count);
}

const uint8_t *bitstream_consume_bytes(bitstream *s, size_t count)
{
  const uint8_t *b;

  if (s->offset % 8 || (8 * count) + s->offset > s->size)
    {
      s->flags |= BITSTREAM_FLAG_INVALID;
      return NULL;
    }

  b = &(s->data[s->offset / 8]);
  s->offset += count * 8;

  return b;
}

void bitstream_discard_bits(bitstream *s, size_t count)
{
  if (count > s->size - s->offset)
    {
      s->flags |= BITSTREAM_FLAG_INVALID;
      return;
    }

  s->size -= count;
}

int bitstream_valid(bitstream *s)
{
  return (s->flags & BITSTREAM_FLAG_INVALID) == 0;
}

size_t bitstream_bits_left(bitstream *s)
{
  return s->size - s->offset;
}
