#ifndef TS_DEMUX_H_INCLUDED
#define TS_DEMUX_H_INCLUDED

typedef struct ts_demux_stream_unit ts_demux_stream_unit;
typedef struct ts_demux_stream ts_demux_stream;
typedef struct ts_demux ts_demux;

enum ts_demux_flags
{
  TS_DEMUX_FLAG_PAT = 0x01
};

enum ts_demux_stream_type
{
  TS_DEMUX_STREAM_TYPE_UNKNOWN = 0,
  TS_DEMUX_STREAM_TYPE_PAT,
  TS_DEMUX_STREAM_TYPE_PMT
};

struct ts_demux_stream_unit
{
  int                                                          complete;
  int                                                          rai;
  int                                                          id;
  uint64_t                                                     pts;
  uint64_t                                                     dts;
  buffer                                                       data;
  TAILQ_ENTRY(ts_demux_stream_unit)                            entries;
};

struct ts_demux_stream
{
  int                                                          type;
  int                                                          empty;
  uint16_t                                                     pid;
  uint8_t                                                      cc;
  TAILQ_HEAD(ts_demux_stream_units_head, ts_demux_stream_unit) units;
};

struct ts_demux
{
  int                                                          flags;
  vector                                                       streams;
};

void             ts_demux_stream_debug(ts_demux_stream *);
int              ts_demux_stream_write(ts_demux_stream *, int, int, int, uint8_t *, size_t);
void             ts_demux_stream_write_eof(ts_demux_stream *);
void             ts_demux_construct(ts_demux *);
int              ts_demux_parse(ts_demux *, uint8_t *, size_t);
int              ts_demux_write(ts_demux *, void *, size_t);
void             ts_demux_write_eof(ts_demux *);
size_t           ts_demux_streams_size(ts_demux *);
ts_demux_stream *ts_demux_streams_lookup(ts_demux *, int, int);
ts_demux_stream *ts_demux_streams_index(ts_demux *, size_t);

#endif /* TS_DEMUX_H_INCLUDED */
