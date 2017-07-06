#ifndef TS_DEMUX_ADTS_H_INCLUDED
#define TS_DEMUX_ADTS_H_INCLUDED

typedef struct ts_demux_adts_state ts_demux_adts_state;
struct ts_demux_adts_state
{
  uint64_t  pts;
  uint8_t  *data;
  size_t    size;
};

void ts_demux_adts_construct(ts_demux_adts_state *, uint8_t *, size_t, uint64_t);
int  ts_demux_adts_parse(ts_demux_adts_state *, uint8_t **, size_t *, uint64_t *, uint64_t *, int *);

#endif /* TS_DEMUX_ADTS_H_INCLUDED */
