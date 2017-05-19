#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/queue.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dynamic.h>
#include <reactor.h>

#include "receiver.h"

static void receiver_error(receiver *r)
{
  r->state = RECEIVER_STATE_ERROR;
  reactor_user_dispatch(&r->user, RECEIVER_EVENT_ERROR, NULL);
}

static void receiver_read_rtp(receiver *r, char *data, size_t n)
{
  receiver_rtp *rtp;
  int i;

  if (n < sizeof *rtp)
    {
      receiver_error(r);
      return;
    }
  rtp = (receiver_rtp *) data;
  rtp->seq = ntohs(rtp->seq);
  rtp->timestamp = ntohl(rtp->timestamp);
  (void) fprintf(stderr, "seq %d, version %d, padding %d, extension %d, payload type %d, csrc count %d, timestamp %u, ssrc %u\n",
                 rtp->seq, rtp->version, rtp->padding, rtp->extension, rtp->payload_type, rtp->csrc_count, rtp->timestamp, rtp->ssrc);
}

static void receiver_event(void *state, int type, void *data)
{
  receiver *r = state;
  struct pollfd *pollfd;
  char buffer[65536];
  ssize_t n;

  switch (type)
    {
    case REACTOR_CORE_EVENT_FD:
      pollfd = data;
      if (pollfd->revents != POLLIN)
        {
          receiver_error(r);
          break;
        }
      n = read(r->socket, buffer, sizeof buffer);
      if (n <= 0)
        receiver_error(r);
      else
        receiver_read_rtp(r, buffer, n);
      break;
    default:
      receiver_error(r);
      break;
    }
}

static void receiver_connect(receiver *r)
{
  struct sockaddr_in sin;
  int e, port;
  in_addr_t addr;

  addr = inet_addr(r->host);
  port = strtoul(r->port, NULL, 10);

  r->socket = socket(AF_INET, SOCK_DGRAM, PF_UNSPEC);
  if (r->socket == -1)
    {
      receiver_error(r);
      return;
    }

  sin = (struct sockaddr_in) {
    .sin_family = AF_INET,
    .sin_addr.s_addr = htonl(INADDR_ANY),
    .sin_port = htons(port)
  };
  e = bind(r->socket, (struct sockaddr *) &sin, sizeof sin);
  if (e == -1)
    {
      receiver_error(r);
      return;
    }

  e = setsockopt(r->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (struct ip_mreq[]){{.imr_multiaddr.s_addr = addr, .imr_interface.s_addr = htonl(INADDR_ANY)}},
                 sizeof(struct ip_mreq));
  if (e == -1)
    {
      receiver_error(r);
      return;
    }

  reactor_core_fd_register(r->socket, receiver_event, r, POLLIN);
}

void receiver_open(receiver *r, reactor_user_callback *callback, void *state, char *host, char *port)
{
  *r = (receiver) {0};
  r->socket = -1;
  r->state = RECEIVER_STATE_OPEN;
  r->host = strdup(host);
  r->port = strdup(port);
  reactor_user_construct(&r->user, callback, state);
  receiver_connect(r);
}
