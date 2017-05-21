#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <dynamic.h>

#include "ts.h"
#include "rtp.h"

void rtp_init(rtp *rtp)
{
  *rtp = (struct rtp) {0};
  ts_state_init(&rtp->ts_state);
}

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

  /*
  (void) fprintf(stderr, "[%lu] seq %d, version %d, padding %d, extension %d, payload type %d, csrc count %d, timestamp %u, ssrc %08x\n",
                 rtp->count, ntohs(h->seq), h->version, h->padding, h->extension, h->payload_type, h->csrc_count, ntohl(h->timestamp), ntohl(h->ssrc));
  */

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
