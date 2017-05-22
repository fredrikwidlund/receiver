#ifndef REACTOR_UDP_H_INCLUDED
#define REACTOR_UDP_H_INCLUDED

enum reactor_udp_state
{
  REACTOR_UDP_STATE_CLOSED     = 0x01,
  REACTOR_UDP_STATE_OPEN       = 0x02,
  REACTOR_UDP_STATE_ERROR      = 0x04
};

enum reactor_udp_event
{
  REACTOR_UDP_EVENT_ERROR,
  REACTOR_UDP_EVENT_READ,
  REACTOR_UDP_EVENT_CLOSE
};

enum reactor_udp_flag
{
  REACTOR_UDP_FLAG_SERVER = 0x01
};

typedef struct reactor_udp reactor_udp;
struct reactor_udp
{
  short             ref;
  short             state;
  reactor_user      user;
  int               socket;
  int               flags;
  reactor_resolver *resolver;
};

void reactor_udp_hold(reactor_udp *);
void reactor_udp_release(reactor_udp *);
void reactor_udp_open(reactor_udp *, reactor_user_callback *, void *, char *, char *, int);
void reactor_udp_close(reactor_udp *, reactor_user_callback *, void *, char *, char *, int);

#endif /* REACTOR_UDP_H_INCLUDED */
