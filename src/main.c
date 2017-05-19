#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "receiver.h"

typedef struct app app;
struct app
{
  receiver receiver;
};

static void usage(void)
{
  extern char *__progname;

  (void) fprintf(stderr, "usage: %s <host> <port>\n", __progname);
  exit(1);
}

static void event(void *state, int type, void *data)
{
  (void) state;
  (void) type;
  (void) data;
  (void) fprintf(stderr, "event\n");
}

int main(int argc, char **argv)
{
  app app;
  char *host, *port;
  int e;

  if (argc != 3)
    usage();
  host = argv[1];
  port = argv[2];

  reactor_core_construct();
  receiver_open(&app.receiver, event, &app, host, port);

  /*
  receiver_construct(&r);
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  receiver_add(&r, "udp://236.0.0.1:2000");
  */
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
