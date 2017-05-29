#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <arpa/inet.h>

#include <dynamic.h>
#include <reactor.h>

#include "bits.h"
#include "reactor_rtp.h"

static void reactor_rtp_error(reactor_rtp *rtp)
{
  reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_ERROR, NULL);
}

static reactor_rtp_frame *reactor_rtp_copy(reactor_rtp_frame *frame)
{
  reactor_rtp_frame *copy;

  copy = malloc(sizeof *copy);
  if (!copy)
    return NULL;
  copy->type = frame->type;
  copy->seq = frame->seq;
  copy->timestamp = frame->timestamp;
  copy->data = malloc(frame->size);
  if (!copy->data)
    {
      free(copy);
      return NULL;
    }
  copy->size = frame->size;
  memcpy(copy->data, frame->data, frame->size);
  return copy;
}

static void reactor_rtp_drop(reactor_rtp *rtp, reactor_rtp_frame *frame)
{
  reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_WARNING_DROP, frame);
  free(frame->data);
  free(frame);
}

static void reactor_rtp_queue(reactor_rtp *rtp, reactor_rtp_frame *frame)
{
  int16_t d;

  if (!frame)
    {
      reactor_rtp_error(rtp);
      return;
    }

  d = frame->seq - rtp->seq;
  if (d < 0 || d > 64)
    {
      reactor_user_dispatch(&rtp->user, d < 0 ? REACTOR_RTP_EVENT_WARNING_OLD : REACTOR_RTP_EVENT_WARNING_PREMATURE, frame);
      reactor_rtp_drop(rtp, frame);
      return;
    }
  
  printf("queue %p %p\n", (void *) rtp, (void *) frame);
  /*
      
      TAILQ_FOREACH(i, &rtp->queued_frames, entries)
        {
          if (f->seq == i->seq)
            {
              reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_DUPLICATE, f);
              free(f->data);
              free(f);
              return;
            }

          if (f->seq > n && f->seq < i->seq)
            {
              TAILQ_INSERT_BEFORE(i, f, entries);
              return;
            }
          n = i->seq;
        }
      TAILQ_INSERT_TAIL(&rtp->queued_frame, f, entries);
      
      
    }
  */
}

static void reactor_rtp_update(reactor_rtp *rtp, reactor_rtp_frame *frame)
{
  if (rtp->count)
    {
      if (frame->seq != (uint16_t) (rtp->seq + 1))
        reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_WARNING_DROP, frame);
      if (abs(frame->timestamp - rtp->timestamp) > 100000 || frame->timestamp < rtp->timestamp)
        reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_WARNING_TIMESTAMP, frame);
      if (frame->ssrc != rtp->ssrc)
        reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_WARNING_SSRC, frame);
    }
  rtp->seq = frame->seq;
  rtp->timestamp = frame->timestamp;
  rtp->ssrc = frame->ssrc;
  rtp->count ++;
}

static void reactor_rtp_add(reactor_rtp *rtp, reactor_rtp_frame *frame)
{
  uint16_t n;

  if (!rtp->count)
    {
      rtp->seq = frame->seq - 1;
      rtp->timestamp = frame->timestamp;
      rtp->ssrc = frame->ssrc;
    }
  n = rtp->seq + 1;

  if (frame->seq == n)
    {
      reactor_rtp_update(rtp, frame);
      reactor_user_dispatch(&rtp->user, REACTOR_RTP_EVENT_FRAME, frame);
      return;
    }

  reactor_rtp_queue(rtp, reactor_rtp_copy(frame));
}


void reactor_rtp_open(reactor_rtp *rtp, reactor_user_callback *callback, void *state)
{
  *rtp = (reactor_rtp) {0};
  TAILQ_INIT(&rtp->queue);
  reactor_user_construct(&rtp->user, callback, state);
}

void reactor_rtp_data(reactor_rtp *rtp, reactor_memory *m)
{
  bits bits;
  uint8_t version, padding, extension, csrc_count, marker, payload_type, *data;
  uint16_t seq;
  uint32_t timestamp, ssrc;
  size_t size;

  bits_construct(&bits, (uint8_t *) m->base, m->size);
  version = bits_take(&bits, 2);
  padding = bits_take(&bits, 1);
  extension = bits_take(&bits, 1);
  csrc_count = bits_take(&bits, 4);
  marker = bits_take(&bits, 1);
  payload_type = bits_take(&bits, 7);
  seq = bits_take(&bits, 16);
  timestamp = bits_take(&bits, 32);
  ssrc = bits_take(&bits, 32);

  for (; csrc_count; csrc_count --)
    (void) bits_take(&bits, 32);

  if (extension)
    {
      (void) bits_take(&bits, 16);
      (void) bits_take_bytes(&bits, bits_take(&bits, 16));
    }

  if (version != 2 || !bits_valid(&bits) || bits.size % 8 != 0)
    {
      reactor_rtp_error(rtp);
      return;
    }

  data = bits.data;
  size = bits.size / 8;

  if (padding)
    {
      if (!size || size < data[size - 1])
        {
          reactor_rtp_error(rtp);
          return;
        }
      size -= data[size - 1];
    }

  reactor_rtp_add(rtp, (reactor_rtp_frame[]) {
      {
        .type = payload_type, .marker = marker, .seq = seq,
        .timestamp = timestamp, .ssrc = ssrc, .data = data, .size = size
      }
    });
}
