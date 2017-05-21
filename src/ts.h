#ifndef TS_H_INCLUDED
#define TS_H_INCLUDED

typedef struct ts_state ts_state;
struct ts_state
{
  double pcr;
  int    pmt_pid;
  int    pcr_pid;
  int    aac_pid;
  int    h264_pid;
  size_t aac_count;
  buffer aac;
  int    aac_out;
  size_t h264_count;
  buffer h264;
  int    h264_out;
};

void ts_state_init(ts_state *);
int  ts(ts_state *, uint8_t *, size_t);

#endif /* TS_H_INCLUDED */
