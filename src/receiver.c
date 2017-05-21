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
#include "rtp.h"
#include "receiver.h"

static void receiver_error(receiver *r, char *error)
{
  r->state = RECEIVER_STATE_ERROR;
  reactor_user_dispatch(&r->user, RECEIVER_EVENT_ERROR, error);
}

static void receiver_event(void *state, int type, void *data)
{
  receiver *r = state;
  uint8_t buffer[65536];
  ssize_t n;
  int e;

  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      n = read(r->socket, buffer, sizeof buffer);
      if (n <= 0)
        {
          receiver_error(r, strerror(errno));
          break;
        }
      e = rtp_frame(&r->rtp, buffer, n);
      if (e == -1)
        receiver_error(r, "invalid rtp frame");
      break;
    default:
      receiver_error(r, "unknown socket event");
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
      receiver_error(r, strerror(errno));
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
      receiver_error(r, strerror(errno));
      return;
    }

  e = setsockopt(r->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 (struct ip_mreq[]){{.imr_multiaddr.s_addr = addr, .imr_interface.s_addr = htonl(INADDR_ANY)}},
                 sizeof(struct ip_mreq));
  if (e == -1)
    {
      receiver_error(r, strerror(errno));
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
  rtp_init(&r->rtp);
  receiver_connect(r);
}
