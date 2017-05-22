#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

void reactor_rtp_open(reactor_rtp *rtp, reactor_user_callback *callback, void *state)
{
  *rtp = (reactor_rtp) {0};
  reactor_user_construct(&rtp->user, callback, state);
}

void reactor_rtp_data(reactor_rtp *rtp, reactor_memory *m)
{
  bits bits;
  uint32_t payload_type, seq;
  int error;

  error = 0;
  bits_construct(&bits);
  bits_add_bytes(&bits, m->base, m->size);
  printf("%lu\n", bits_take(&bits, 2));
  if (bits_take(&bits, 2) != 2 ||
      bits_take(&bits, 1) != 0 ||
      bits_take(&bits, 1) != 0 ||
      bits_take(&bits, 4) != 0)
    error ++;
  payload_type = bits_take(&bits, 7);
  seq = bits_take(&bits, 32);

  printf("%u %u\n", payload_type, seq);
  if (error || bits_invalid(&bits))
    reactor_rtp_error(rtp);
}

/*
int rtp_frame(rtp *rtp, uint8_t *data, size_t size)
{
  rtp_header *h;
  int e;

  if (size < sizeof *h)
    return -1;
  h = (rtp_header *) data;

  rtp->count ++;
  if (rtp->count == 1)
    {
      rtp->seq = ntohs(h->seq) - 1;
      rtp->timestamp = ntohs(h->timestamp);
      rtp->ssrc = htonl(h->ssrc);
    }

  if (ntohs(h->seq) != (uint16_t) (rtp->seq + 1) ||
      h->version != 2 ||
      h->padding != 0 ||
      h->extension != 0 ||
      h->csrc_count != 0 ||
      h->payload_type != 33 ||
      ntohl(h->ssrc) != rtp->ssrc)
    return -1;

  rtp->seq = ntohs(h->seq);

  data += sizeof *h;
  size -= sizeof *h;

  if (size % 188 != 0)
    return -1;

  for (; size >= 188; data += 188, size -= 188)
    {
      e = ts(&rtp->ts_state, data, 188);
      if (e == -1)
        return -1;
    }

  if (size != 0)
    return -1;

  return 0;
}

*/
