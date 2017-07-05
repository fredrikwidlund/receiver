#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct media media;
struct media
{
  char            *path;
  AVFormatContext *format;
};

void media_init(void)
{
  av_register_all();
}

int media_open(media *m, char *path)
{
  int e;

  *m = (media) {0};
  m->path = strdup(path);
  if (!m->path)
    return -1;

  e = avformat_open_input(&m->format, m->path, NULL, NULL);
  if (e != 0)
    return -1;

  e = avformat_find_stream_info(m->format, NULL);
  if (e != 0)
    return -1;

  return 0;
}

void media_debug(media *m)
{
  av_dump_format(m->format, 0, m->path, 0);
}

int media_read(media *m)
{
  int e, i, n;
  AVPacket p;
  AVStream *s;
  int fd;

  fd = open("gop.h264", O_CREAT | O_WRONLY);
  if (fd == -1)
    return -1;

  (void) fprintf(stderr, "[info] streams %d\n", m->format->nb_streams);
  for(i = 0; i < m->format->nb_streams; i ++)
    {
      s = m->format->streams[i];
      (void) fprintf(stderr, "[stream %d] id %d, start %ld, duration %ld, frames %ld, time unit %d/%d, frame rate %d/%d\n",
                     i, s->id,
                     s->start_time, s->duration, s->nb_frames,
                     s->time_base.num, s->time_base.den,
                     s->r_frame_rate.num, s->r_frame_rate.den);
    }

  for (n = 0;; n ++)
    {
      e = av_read_frame(m->format, &p);
      if (e != 0)
        break;

      n = write(fd, p.data, p.size);
      if (n != p.size)
        err(1, "write");

      s = m->format->streams[p.stream_index];
      (void) fprintf(stderr, "[%d] type %s, codec %s, flags %02x, stream %d, size %d, pts %ld, dts %ld, duration %ld\n",
                     n,
                     av_get_media_type_string(s->codecpar->codec_type),
                     avcodec_descriptor_get(s->codecpar->codec_id)->name,
                     p.flags,
                     p.stream_index, p.size,
                     p.pts, p.dts, p.duration);

      av_packet_unref(&p);
    }

  close(fd);
  return 0;
}

void usage(void)
{
  extern char *__progname;

  (void) fprintf(stderr, "usage: %s <input-file>\n", __progname);
  exit(1);
}

int main(int argc, char **argv)
{
  char *path;
  media m;
  int e;

  if (argc != 2)
    usage();
  path = argv[1];

  media_init();
  e = media_open(&m, path);
  if (e == -1)
    err(1, "media_open");

  //media_debug(&m);

  e = media_read(&m);
  if (e == -1)
    err(1, "media_read");
}
