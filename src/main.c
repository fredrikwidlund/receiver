#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "ts.h"
#include "rtp.h"
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
  char *error;

  (void) state;
  switch (type)
    {
    case RECEIVER_EVENT_ERROR:
      error = data;
      (void) fprintf(stderr, "error: %s\n", error);
      exit(1);
      break;
    }
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
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
