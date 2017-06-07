#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "bitstream.h"

int main()
{
  bitstream s;

  bitstream_construct(&s, (uint8_t []) {0xab, 0x12, 0x34}, 3 * 8);
  assert(bitstream_read(&s, 0, 4) == 0x0a);
  assert(bitstream_read(&s, 4, 4) == 0x0b);
  assert(bitstream_read(&s, 8, 4) == 0x01);
  assert(bitstream_read(&s, 12, 4) == 0x02);
  assert(bitstream_read(&s, 0, 8) == 0xab);
  assert(bitstream_read(&s, 0, 12) == 0xab1);
  assert(bitstream_read(&s, 0, 16) == 0xab12);
  assert(bitstream_read(&s, 4, 16) == 0xb123);

  assert(bitstream_read(&s, 7, 1) == 0x01);
  assert(bitstream_read(&s, 7, 3) == 0x04);
  assert(bitstream_read(&s, 7, 4) == 0x08);
  assert(bitstream_read(&s, 7, 5) == 0x11);

  bitstream_construct(&s, (uint8_t []) {0xa0,0xa0,0xf0,0x55,0x12,0x34,0x56,0x78}, 8 * 8);
  assert(bitstream_read(&s, 16 + 2, 4) == 0x0c);
  assert(bitstream_read(&s, 16 - 2, 4) == 0x03);
  assert(bitstream_read(&s, 0, 16) == 0xa0a0);
  assert(bitstream_read(&s, 0, 32) == 0xa0a0f055);
  assert(bitstream_read(&s, 0, 64) == 0xa0a0f05512345678);
  assert(bitstream_read(&s, 32, 8) == 0x12);
  assert(bitstream_read(&s, 32, 0) == 0x00);
  assert(bitstream_read(&s, 56, 8) == 0x78);
  assert(bitstream_read(&s, 52, 8) == 0x67);

 bitstream_construct(&s, (uint8_t []) {0x12, 0x34, 0x56}, 3 * 8);
 assert(*bitstream_consume_bytes(&s, 1) == 0x12);
 assert(bitstream_consume_bits(&s, 4) == 0x03);
 assert(bitstream_consume_bits(&s, 4) == 0x04);
 assert(bitstream_consume_bits(&s, 2) == 0x01);
 assert(bitstream_consume_bits(&s, 2) == 0x01);
 assert(bitstream_consume_bits(&s, 1) == 0x00);
 assert(bitstream_consume_bits(&s, 1) == 0x01);
 assert(bitstream_consume_bits(&s, 1) == 0x01);
 assert(bitstream_consume_bits(&s, 1) == 0x00);
 assert(bitstream_consume_bytes(&s, 1) == NULL);
 assert(!bitstream_valid(&s));
}
