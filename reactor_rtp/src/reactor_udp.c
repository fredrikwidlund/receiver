#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "reactor_udp.h"

static void reactor_udp_close_fd(reactor_udp *udp)
{
  if (udp->socket >= 0)
    {
      reactor_core_fd_deregister(udp->socket);
      (void) close(udp->socket);
      udp->socket = -1;
    }
}

static void reactor_udp_error(reactor_udp *udp)
{
  udp->state = REACTOR_UDP_STATE_ERROR;
  reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_ERROR, NULL);
}

static void reactor_udp_event(void *state, int type, void *data)
{
  reactor_udp *udp = state;
  short revents = ((struct pollfd *) data)->revents;
  char buffer[REACTOR_UDP_BLOCK_SIZE];
  reactor_memory memory;
  ssize_t n;

  switch (revents)
    {
    case POLLIN:
      n = read(udp->socket, buffer, sizeof buffer);
      if (n == -1)
        {
          reactor_udp_error(udp);
          break;
        }
      memory = reactor_memory_ref(buffer, n);
      reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_READ, &memory);
      break;
    default:
      reactor_udp_error(udp);
      break;
    }
}

static void reactor_udp_connect(reactor_udp *udp, struct addrinfo *addrinfo)
{
  (void) udp;
  (void) addrinfo;
}

static void reactor_udp_listen(reactor_udp *udp, struct addrinfo *addrinfo)
{
  int e;
  struct sockaddr_in sin;

  if (addrinfo->ai_family != AF_INET)
    {
      reactor_udp_error(udp);
      return;
    }
  sin = *(struct sockaddr_in *) addrinfo->ai_addr;

  udp->socket = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
  if (udp->socket == -1)
    {
      reactor_udp_error(udp);
      return;
    }

  if (IN_MULTICAST(ntohl(sin.sin_addr.s_addr)))
    {
      e = setsockopt(udp->socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     (struct ip_mreq[]){{.imr_multiaddr = sin.sin_addr, .imr_interface.s_addr = htonl(INADDR_ANY)}},
                     sizeof(struct ip_mreq));
      if (e == -1)
        {
          reactor_udp_error(udp);
          return;
        }
      sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }

  e = bind(udp->socket, (struct sockaddr *) &sin, sizeof sin);
  if (e == -1)
    {
      reactor_udp_error(udp);
      return;
    }

  reactor_core_fd_register(udp->socket, reactor_udp_event, udp, POLLIN);
}

static void reactor_udp_resolve_event(void *state, int type, void *data)
{
  reactor_udp *udp = state;

  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_RESULT:
      if (!data)
        reactor_udp_error(udp);
      else if (udp->flags & REACTOR_UDP_FLAG_SERVER)
        reactor_udp_listen(udp, data);
      else
        reactor_udp_connect(udp, data);
      break;
    case REACTOR_RESOLVER_EVENT_ERROR:
      reactor_udp_error(udp);
      break;
    case REACTOR_RESOLVER_EVENT_CLOSE:
      free(udp->resolver);
      udp->resolver = NULL;
      reactor_udp_release(udp);
      break;
    }
}

static void reactor_udp_resolve(reactor_udp *udp, char *node, char *service)
{
  reactor_udp_hold(udp);
  udp->resolver = malloc(sizeof *udp->resolver);
  reactor_resolver_open(udp->resolver, reactor_udp_resolve_event, udp, node, service,
                        (struct addrinfo[]){{.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM}});
}

void reactor_udp_hold(reactor_udp *udp)
{
  udp->ref ++;
}

void reactor_udp_release(reactor_udp *udp)
{
  udp->ref --;
  if (!udp->ref)
    {
      udp->state = REACTOR_UDP_STATE_CLOSED;
      reactor_udp_close_fd(udp);
      reactor_user_dispatch(&udp->user, REACTOR_UDP_EVENT_CLOSE, NULL);
    }
}

void reactor_udp_open(reactor_udp *udp, reactor_user_callback *callback, void *state, char *node, char *service, int flags)
{
  udp->ref = 0;
  udp->state = REACTOR_UDP_STATE_RESOLVING;
  reactor_user_construct(&udp->user, callback, state);
  udp->socket = -1;
  udp->flags = flags;
  reactor_udp_hold(udp);
  reactor_udp_resolve(udp, node, service);
}

void reactor_udp_close(reactor_udp *udp)
{
  if (udp->state & REACTOR_UDP_STATE_CLOSED)
    return;

  udp->state = REACTOR_UDP_STATE_CLOSED;
  reactor_udp_close_fd(udp);
  reactor_udp_release(udp);
}

