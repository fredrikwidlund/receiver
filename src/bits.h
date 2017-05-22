#ifndef BITS_H_INCLUDED
#define BITS_H_INCLUDED

typedef struct bits bits;
struct bits
{
  buffer  data;
  uint8_t consumed;
};

void     bits_construct(bits *);
void     bits_add_bytes(bits *, const char *, size_t);
uint64_t bits_take(bits *, int);
int      bits_invalid(bits *);

#endif /* BITS_H_INCLUDED */
