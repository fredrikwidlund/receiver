#ifndef BITSTREAM_H_INCLUDED
#define BITSTREAM_H_INCLUDED

enum bitstream_flags
{
  BITSTREAM_FLAG_INVALID = 0x01
};

typedef struct bitstream bitstream;
struct bitstream
{
  const uint8_t *data;
  size_t         size;
  size_t         offset;
  int            flags;
};

void           bitstream_construct(bitstream *, const void *, size_t);
uint64_t       bitstream_read(bitstream *, size_t, size_t);
uint64_t       bitstream_consume_bits(bitstream *, size_t);
const uint8_t *bitstream_consume_bytes(bitstream *, size_t);
void           bitstream_discard_bits(bitstream *, size_t);
int            bitstream_valid(bitstream *);
size_t         bitstream_bits_left(bitstream *);

#endif /* BITSTREAM_H_INCLUDED */
