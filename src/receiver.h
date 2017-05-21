#ifndef RECEIVER_H_INCLUDED
#define RECEIVER_H_INCLUDED

enum receiver_state
{
  RECEIVER_STATE_ERROR,
  RECEIVER_STATE_OPEN
};

enum receiver_event
{
  RECEIVER_EVENT_ERROR
};

typedef struct receiver receiver;
struct receiver
{
  int              state;
  reactor_user     user;
  char            *host;
  char            *port;
  int              socket;
  rtp              rtp;
};

void receiver_open(receiver *, reactor_user_callback *, void *, char *, char *);

#endif /* RECEIVER_H_INCLUDED */
