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
};

typedef struct receiver_rtp receiver_rtp;
struct receiver_rtp
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  u_int8_t csrc_count:4;
  u_int8_t extension:1;
  u_int8_t padding:1;
  u_int8_t version:2;
  u_int8_t payload_type:7;
  u_int8_t marker:1;
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
  u_int8_t version:2;
  u_int8_t padding:1;
  u_int8_t extension:1;
  u_int8_t csrc_count:4;
  u_int8_t marker:1;
  u_int8_t payload_type:7;
#endif
  u_int16_t seq;
  u_int32_t timestamp;
  u_int32_t ssrc;
  u_int32_t csrc[1];
};

void receiver_open(receiver *, reactor_user_callback *, void *, char *, char *);

#endif /* RECEIVER_H_INCLUDED */
