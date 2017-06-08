#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include <dynamic.h>

#include "ts_demux.h"

int read_file(char *path, void **data, size_t *size)
{
  buffer b;
  int fd;
  char block[1048576];
  ssize_t n;

  buffer_construct(&b);
  fd = open(path, O_RDONLY);
  if (fd == -1)
    return -1;

  while (1)
    {
      n = read(fd, block, sizeof block);
      if (n <= 0)
        break;
      buffer_insert(&b, buffer_size(&b), block, n);
    }
  (void) close(fd);
  if (n == -1)
    {
      buffer_destruct(&b);
      return -1;
    }

  buffer_compact(&b);
  *data = buffer_data(&b);
  *size = buffer_size(&b);
  return 0;
}

int main(int argc, char **argv)
{
  ts_demux d;
  void *data;
  size_t size;
  int e;

  e = read_file(argv[1], &data, &size);
  if (e == -1)
    err(1, "read_file");

  ts_demux_construct(&d);
  e = ts_demux_write(&d, data, size);
  if (e == -1)
    errx(1, "ts_demux_write");

  free(data);
}
