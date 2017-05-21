#ifndef RTP_H_INCLUDED
#define RTP_H_INCLUDED

typedef struct rtp_header rtp_header;
struct rtp_header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint8_t  csrc_count:4;
  uint8_t  extension:1;
  uint8_t  padding:1;
  uint8_t  version:2;
  uint8_t  payload_type:7;
  uint8_t  marker:1;
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
  uint8_t  version:2;
  uint8_t  padding:1;
  uint8_t  extension:1;
  uint8_t  csrc_count:4;
  uint8_t  marker:1;
  uint8_t  payload_type:7;
#endif
  uint16_t seq;
  uint32_t timestamp;
  uint32_t ssrc;
};

typedef struct rtp rtp;
struct rtp
{
  size_t   count;
  uint16_t seq;
  uint32_t timestamp;
  uint32_t ssrc;
  ts_state ts_state;
};

void rtp_init(rtp *);
int  rtp_frame(rtp *, uint8_t *, size_t);

#endif /* RTP_H_INCLUDED */
