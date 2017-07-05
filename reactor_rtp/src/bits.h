#ifndef BITS_H_INCLUDED
#define BITS_H_INCLUDED

typedef struct bits bits;
struct bits
{
  uint8_t *data;
  int      size;
  int      valid;
};

void      bits_construct(bits *, uint8_t *, size_t);
uint64_t  bits_take(bits *, int);
int       bits_valid(bits *);
uint8_t  *bits_take_bytes(bits *, size_t);

#endif /* BITS_H_INCLUDED */
