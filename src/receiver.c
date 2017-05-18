#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "receiver.h"

enum receiver_stream_event
{
  RECEIVER_STREAM_STATE_ERROR,
  RECEIVER_STREAM_STATE_START,
  RECEIVER_STREAM_STATE_OPEN
};

typedef struct receiver_stream receiver_stream;
struct receiver_stream
{
  int              state;
  receiver        *receiver;
  char            *url;
  AVFormatContext *format;
  AVPacket         packet;
};

static void receiver_stream_update(receiver_stream *s)
{
  int e;

  if (s->state == RECEIVER_STREAM_STATE_START)
    {
      e = avformat_find_stream_info(s->format, NULL);
      if (e != 0)
        {
          s->state = RECEIVER_STREAM_STATE_ERROR;
          return;
        }

      s->state = RECEIVER_STREAM_STATE_OPEN;
    }

  e = av_read_frame(s->format, &s->packet);
  if (e != 0)
    {
      printf("%s\n", av_err2str(e));

      s->state = RECEIVER_STREAM_STATE_ERROR;
      return;
    }
}

static void receiver_stream_event(void *state, int type, void *data)
{
  receiver_stream *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_POOL_EVENT_CALL:
      receiver_stream_update(s);
      break;
    case REACTOR_POOL_EVENT_RETURN:
      if (s->state != RECEIVER_STREAM_STATE_OPEN)
        break;
      printf("%p frame %ld\n", (void *) s, s->packet.pts);
      av_packet_unref(&s->packet);
      reactor_core_job_register(receiver_stream_event, s);
      break;
    }
}

static void receiver_stream_read(receiver_stream *s)
{
  reactor_core_job_register(receiver_stream_event, s);
}

int receiver_construct(receiver *r)
{
  *r = (receiver) {0};

  av_register_all();
  avformat_network_init();
  av_log_set_level(AV_LOG_QUIET);

  return 0;
}

int receiver_add(receiver *r, char *url)
{
  receiver_stream *s;
  int e;

  s = calloc(1, sizeof *s);
  s->state = RECEIVER_STREAM_STATE_START;
  s->url = strdup(url);
  s->receiver = r;

  e = avformat_open_input(&s->format, s->url, NULL, NULL);
  if (e != 0)
    return -1;

  receiver_stream_read(s);
  return 0;
}
