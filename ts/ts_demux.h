#ifndef TS_DEMUX_H_INCLUDED
#define TS_DEMUX_H_INCLUDED

typedef struct ts_demux_stream ts_demux_stream;
typedef struct ts_demux ts_demux;

struct ts_demux_stream
{
  uint16_t pid;
  uint8_t  cc;
  buffer   data;
};

struct ts_demux
{
  vector streams;
};

void ts_demux_construct(ts_demux *);
int  ts_demux_parse(ts_demux *, uint8_t *, size_t);
int  ts_demux_write(ts_demux *, void *, size_t);

#endif /* TS_DEMUX_H_INCLUDED */
