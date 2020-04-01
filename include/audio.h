#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <custom.h>

typedef struct Sound {
  size_t length;
  uint16_t rate;
  int8_t *samples;
} Sound_t;

typedef enum { CH0, CH1, CH2, CH3 } Channel_t;

/* Volume level is 6-bit value from range [0, 64). */
static inline void AudioSetVolume(Channel_t num, uint8_t level) {
  custom.aud[num].ac_vol = (uint16_t)level;
}

/* Maximum play rate is 31150 Hz which corresponds to numer of raster lines
 * displayed each second (assuming PAL signal). */
static inline void AudioSetSampleRate(Channel_t num, uint16_t rate) {
  custom.aud[num].ac_per = (uint16_t)(3579546L / rate);
}

/* Pointer to sound samples must be word-aligned, length must be even. */
static inline void AudioAttachSample(Channel_t num, int8_t *samples,
                                     size_t length) {
  custom.aud[num].ac_ptr = (uint16_t *)samples;
  custom.aud[num].ac_len = (uint16_t)(length >> 1);
}

static inline void AudioAttachSound(Channel_t num, Sound_t *sound) {
  AudioAttachSample(num, sound->samples, sound->length);
  AudioSetSampleRate(num, sound->rate);
}

static inline void AudioPlay(Channel_t num) {
  EnableDMA(1 << (DMAB_AUD0 + num));
}

static inline void AudioStop(Channel_t num) {
  DisableDMA(1 << (DMAB_AUD0 + num));
}

#endif /* !_AUDIO_H_ */
