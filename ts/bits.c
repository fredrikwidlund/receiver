#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/param.h>

#include "bits.h"

void bits_clear(bits *b)
{
  *b = (bits) {0};
}

void bits_set_data(bits *b, uint8_t *data, size_t size)
{
  *b = (bits) {.data = data, .size = size * 8, .valid = 1};
}

uint64_t bits_get(bits *b, size_t offset, size_t size)
{
  uint64_t v;
  uint8_t o;
  int n;

  if (size > 64 || offset + size > b->size)
    {
      bits_clear(b);
      return 0;
    }

  v = 0;
  while (size)
    {
      o = b->data[offset / 8] << offset % 8;
      n = MIN(8 - (offset % 8), size);
      v <<= n;
      v += o >> (8 - n);
      size -= n;
      offset += n;
    }

  return v;
}

uint64_t bits_read(bits *b, size_t size)
{
  uint64_t v;

  v = bits_get(b, b->offset, size);
  if (bits_valid(b))
    b->offset += size;
  return v;
}

void bits_read_data(bits *b, uint8_t **data, size_t size)
{
  uint8_t *p;

  if (bits_valid(b) && b->offset % 8 == 0 && size <= (b->size - b->offset) / 8)
    {
      p = b->data + (b->offset / 8);
      b->offset += size * 8;
    }
  else
    {
      p = NULL;
      bits_clear(b);
    }

  if (data)
    *data = p;
}

void bits_get_data(bits *b, uint8_t **data, size_t *size)
{
  *size = (b->size - b->offset) / 8;
  bits_read_data(b, data, *size);
}

int bits_valid(bits *b)
{
  return b->valid;
}

size_t bits_size(bits *b)
{
  return b->size - b->offset;
}

bits bits_subset(bits *b, size_t offset, size_t size)
{
  bits subset = *b;

  if (subset.offset + offset + size > subset.size)
    bits_clear(&subset);
  else
    {
      subset.offset += offset;
      subset.size = subset.offset + size;
    }
  return subset;
}

void bits_flush(bits *b)
{
  b->offset = b->size;
}
