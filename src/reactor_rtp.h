#ifndef REACTOR_RTP_H_INCLUDED
#define REACTOR_RTP_H_INCLUDED

enum reactor_rtp_state
{
  REACTOR_RTP_STATE_CLOSED     = 0x01,
  REACTOR_RTP_STATE_RESOLVING  = 0x02,
  REACTOR_RTP_STATE_OPEN       = 0x04,
  REACTOR_RTP_STATE_ERROR      = 0x08
};

enum reactor_rtp_event
{
  REACTOR_RTP_EVENT_ERROR,
  REACTOR_RTP_EVENT_MP2T
};

typedef struct reactor_rtp reactor_rtp;
struct reactor_rtp
{
  reactor_user user;
  size_t       count;
  uint16_t     seq;
  uint32_t     timestamp;
  uint32_t     ssrc;
};

void reactor_rtp_open(reactor_rtp *, reactor_user_callback *, void *);
void reactor_rtp_data(reactor_rtp *, reactor_memory *);

#endif /* REACTOR_RTP_H_INCLUDED */
