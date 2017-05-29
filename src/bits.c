#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/param.h>

#include "bits.h"

void bits_construct(bits *b, uint8_t *data, size_t size)
{
  b->data = data;
  b->size = 8 * size;
  b->valid = 1;
}

uint64_t bits_take(bits *b, int count)
{
  uint64_t value;
  int n;

  if (count > b->size)
    {
      b->valid = 0;
      return 0;
    }

  value = 0;
  while (count)
    {
      n = MIN(count, (b->size - 1) % 8 + 1);
      value <<= n;
      value += b->data[0] >> (8 - n);
      b->data[0] <<= n;

      count -= n;
      b->size -= n;
      if (b->size % 8 == 0)
        b->data ++;
    }

  return value;
}

uint8_t *bits_take_bytes(bits *b, size_t count)
{
  uint8_t *bytes;

  if (b->size % 8 != 0 || count * 8 > b->size)
    {
      b->valid = 0;
      return NULL;
    }

  bytes = b->data;
  b->data += count;
  b->size -= count * 8;
  return bytes;
}

int bits_valid(bits *b)
{
  return b->valid;
}
