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
  REACTOR_RTP_EVENT_FRAME,
  REACTOR_RTP_EVENT_WARNING_DROP,
  REACTOR_RTP_EVENT_WARNING_TIMESTAMP,
  REACTOR_RTP_EVENT_WARNING_SSRC,
  REACTOR_RTP_EVENT_WARNING_OLD,
  REACTOR_RTP_EVENT_WARNING_PREMATURE
};

typedef struct reactor_rtp reactor_rtp;
struct reactor_rtp
{
  reactor_user                    user;
  size_t                          count;
  uint16_t                        seq;
  uint32_t                        timestamp;
  uint32_t                        ssrc;
  TAILQ_HEAD(, reactor_rtp_frame) queue;
};

typedef struct reactor_rtp_frame reactor_rtp_frame;
struct reactor_rtp_frame
{
  uint8_t                         type;
  uint8_t                         marker;
  uint16_t                        seq;
  uint32_t                        timestamp;
  uint32_t                        ssrc;
  uint8_t                        *data;
  size_t                          size;
  TAILQ_ENTRY(reactor_rtp_frame)  entries;
};

void reactor_rtp_open(reactor_rtp *, reactor_user_callback *, void *);
void reactor_rtp_data(reactor_rtp *, reactor_memory *);

#endif /* REACTOR_RTP_H_INCLUDED */
