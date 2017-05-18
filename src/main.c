#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

#include "receiver.h"

int main()
{
  receiver r;
  int e;

  reactor_core_construct();
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
  e = reactor_core_run();
  if (e == -1)
    err(1, "reactor_core_run");
  reactor_core_destruct();
}
