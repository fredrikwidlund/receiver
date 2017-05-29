#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/queue.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dynamic.h>
#include <reactor.h>

#include "ts.h"
#include "reactor_udp.h"
#include "reactor_rtp.h"
#include "receiver.h"

static void receiver_error(receiver *r, char *error)
{
  r->state = RECEIVER_STATE_ERROR;
  reactor_user_dispatch(&r->user, RECEIVER_EVENT_ERROR, error);
}

static void receiver_rtp_event(void *state, int type, void *data)
{
  receiver *r = state;
  reactor_rtp_frame *f;

  switch (type)
    {
    case REACTOR_RTP_EVENT_FRAME:
      f = data;
      printf("packet %u %u\n", f->seq, f->timestamp);
      break;
    case REACTOR_RTP_EVENT_ERROR:
      receiver_error(r, "rtp error");
      break;
    default:
      (void) fprintf(stderr, "[warning %d]\n", type);
      break;
    }
}

static void receiver_udp_event(void *state, int type, void *data)
{
  receiver *r = state;

  switch (type)
    {
    case REACTOR_UDP_EVENT_READ:
      reactor_rtp_data(&r->rtp, data);
      break;
    case REACTOR_UDP_EVENT_ERROR:
      receiver_error(r, "udp error");
      break;
    }
}

void receiver_open(receiver *r, reactor_user_callback *callback, void *state, char *host, char *port)
{
  *r = (receiver) {0};
  r->socket = -1;
  r->state = RECEIVER_STATE_OPEN;
  r->host = strdup(host);
  r->port = strdup(port);
  reactor_user_construct(&r->user, callback, state);

  reactor_udp_open(&r->udp, receiver_udp_event, r, host, port, REACTOR_UDP_FLAG_SERVER);
  reactor_rtp_open(&r->rtp, receiver_rtp_event, r);
}
