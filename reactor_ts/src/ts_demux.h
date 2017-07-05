#ifndef TS_DEMUX_H_INCLUDED
#define TS_DEMUX_H_INCLUDED

enum ts_demux_state
{
  TS_DEMUX_STATE_CLOSED     = 0x01,
  TS_DEMUX_STATE_OPEN       = 0x02,
  TS_DEMUX_STATE_EOF        = 0x04,
  TS_DEMUX_STATE_ERROR      = 0x08
};

enum ts_demux_stream_type
{
  TS_DEMUX_STREAM_TYPE_UNKNOWN = 0,
  TS_DEMUX_STREAM_TYPE_PAT,
  TS_DEMUX_STREAM_TYPE_PMT,
  TS_DEMUX_STREAM_TYPE_PES
};

enum ts_demux_stream_flags
{
  TS_DEMUX_STREAM_FLAG_DONE = 0x01
};

enum ts_demux_event
{
  TS_DEMUX_EVENT_ERROR,
  TS_DEMUX_EVENT_MESSAGE
};

typedef struct ts_demux_message ts_demux_message;
struct ts_demux_message
{
  uint16_t  pid;
  uint8_t   type;
  uint8_t   id;
  uint8_t   has_pts;
  uint64_t  pts;
  uint8_t   has_dts;
  uint64_t  dts;
  uint8_t  *data;
  size_t    size;
};

typedef struct ts_demux_stream_unit ts_demux_stream_unit;
struct ts_demux_stream_unit
{
  int          rai;
  buffer       data;
};

typedef struct ts_demux_stream ts_demux_stream;
struct ts_demux_stream
{
  int          type;
  int          flags;
  int          cc;
  uint16_t     pid;
  uint8_t      stream_type;
  vector       units;
};

typedef struct ts_demux ts_demux;
struct ts_demux
{
  int          state;
  reactor_user user;
  buffer       input;
  double       pcr;
  vector       streams;
};

void ts_demux_open(ts_demux *, reactor_user_callback *, void *);
void ts_demux_close(ts_demux *);
void ts_demux_data(ts_demux *, void *, size_t);
void ts_demux_eof(ts_demux *);

#endif /* TS_DEMUX_H_INCLUDED */
