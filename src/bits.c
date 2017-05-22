#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dynamic.h>

#include "bits.h"

void bits_construct(bits *b)
{
  buffer_construct(&b->data);
}

void bits_add_bytes(bits *b, const char *data, size_t size)
{
  buffer_insert(&b->data, buffer_size(&b->data), (char *) data, size);
}

uint64_t bits_take(bits *b, int n)
{
  uint8_t *data = buffer_data(&b->data);
  size_t size = buffer_size(&b->data);
  uint64_t c = b->consumed, v = 0;
  int p, f;

  printf("n %d\n", n);
  while (n)
    {
      f =c / 8;
      p = 8 - (c % 8);
      if (n >= p)
        {
          v <<= p;
          v += data[f] >> (8 - p);
          c += p;
          n -= p;
        }
      else
        {
          v <<= n;
          v += data[f] >> (8 - n);
          c += n;
          n -= n;
        }
    }
  printf("%08x\n", v);
  return v;
}

int bits_invalid(bits *b)
{
  return 0;
}
