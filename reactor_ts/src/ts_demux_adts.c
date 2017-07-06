#include <stdlib.h>
#include <stdint.h>

#include "bits.h"
#include "ts_demux_adts.h"

void ts_demux_adts_construct(ts_demux_adts_state *state, uint8_t *data, size_t size, uint64_t pts)
{
  *state = (ts_demux_adts_state) {.pts = pts, .data = data, .size = size};
}

int ts_demux_adts_parse(ts_demux_adts_state *state, uint8_t **data, size_t *size, uint64_t *pts, uint64_t *duration, int *freq)
{
  bits b;
  uint16_t sync, len;
  uint8_t layer, no_crc, profile, freq_index, *aac;
  int freq_table[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

  if (!state->size)
    return 0;

  bits_set_data(&b, state->data, state->size);

  sync = bits_read(&b, 12);
  (void) bits_read(&b, 1);
  layer = bits_read(&b, 2);
  no_crc = bits_read(&b, 1);
  profile = bits_read(&b, 2);
  freq_index = bits_read(&b, 4);
  (void) bits_read(&b, 8);
  len = bits_read(&b, 13);
  (void) bits_read(&b, 13);
  if (!no_crc)
    (void) bits_read(&b, 16);
  bits_read_data(&b, &aac, len - (no_crc ? 7 : 9));

  if (!bits_valid(&b) || sync != 0x0fff || layer || profile != 1 || freq_index > 12)
    return -1;

  *data = state->data;
  *size = len;
  *pts = state->pts;
  *freq = freq_table[freq_index];
  *duration = 1024 * 90000 / *freq;

  state->data += *size;
  state->size -= *size;
  state->pts += *duration;
  return 1;
}
