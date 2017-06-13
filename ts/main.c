#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/queue.h>
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

int write_file(char *path, void *data, size_t size)
{
  int fd;
  ssize_t n;

  fd = open(path, O_CREAT | O_WRONLY, 0644);
  if (fd == -1)
    return -1;

  while (size)
    {
      n = write(fd, data, size);
      if (n == -1)
        {
          (void) close(fd);
          return -1;
        }
      data = (char *) data + n;
      size -= n;
    }
  (void) close(fd);
  return 0;
}

int main(int argc, char **argv)
{
  ts_demux d;
  ts_demux_stream *s;
  void *data;
  size_t size;
  int e, i;
  char name[256];

  e = read_file(argv[1], &data, &size);
  if (e == -1)
    err(1, "read_file");

  ts_demux_construct(&d);
  e = ts_demux_write(&d, data, size);
  if (e == -1)
    errx(1, "ts_demux_write");
  ts_demux_write_eof(&d);

  for (i = 0; i < ts_demux_streams_size(&d); i ++)
    {
      s = ts_demux_streams_index(&d, i);
      printf("pid %u\n", s->pid);
      
      //ts_demux_stream_debug(s);

      /*
      (void) snprintf(name, sizeof name, "pid-%d.raw", s->pid);
      e = write_file(name, ts_demux_stream_data(s), ts_demux_stream_size(s));
      if (e == -1)
        err(1, "write_file");
      */
    }


  free(data);
}
