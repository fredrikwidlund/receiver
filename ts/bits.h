#ifndef BITS_H_INCLUDED
#define BITS_H_INCLUDED

typedef struct bits bits;
struct bits
{
  uint8_t *data;
  size_t   size;
  size_t   offset;
  int      valid;
};

void     bits_clear(bits *);
void     bits_set_data(bits *, uint8_t *, size_t);
uint64_t bits_get(bits *, size_t, size_t);
uint64_t bits_read(bits *, size_t);
void     bits_read_data(bits *, uint8_t **, size_t);
void     bits_get_data(bits *, uint8_t **, size_t *);
int      bits_valid(bits *);

#endif /* BITS_H_INCLUDED */
