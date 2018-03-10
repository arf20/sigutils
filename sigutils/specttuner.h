/*

  Copyright (C) 2018 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef _SIGUTILS_SPECTTUNER_H
#define _SIGUTILS_SPECTTUNER_H

#include "types.h"
#include "ncqo.h"

struct sigutils_specttuner_params {
  SUSCOUNT window_size;
};

enum sigutils_specttuner_state {
  SU_SPECTTUNER_STATE_EVEN,
  SU_SPECTTUNER_STATE_ODD,
};

struct sigutils_specttuner_channel;

struct sigutils_specttuner_channel_params {
  SUFLOAT f0; /* Central frequency (angular frequency) */
  SUFLOAT bw; /* Bandwidth (angular frequency) */

  void *private; /* Private data */
  SUBOOL (*on_data) (
      const struct sigutils_specttuner_channel *channel,
      void *private,
      const SUCOMPLEX *data, /* This pointer remains valid until the next call to feed */
      SUSCOUNT size);
};

struct sigutils_specttuner_channel {
  struct sigutils_specttuner_channel_params params;
  int index;           /* Back reference */

  SUFLOAT k;           /* Scaling factor */
  SUFLOAT decimation;  /* Equivalent decimation */
  unsigned int center; /* FFT center bin */
  unsigned int size;   /* FFT bins to allocate */
  unsigned int width;  /* FFT bins to copy (for guard bands, etc) */
  unsigned int halfw;  /* Half of channel width */
  unsigned int halfsz; /* Half of window size */
  unsigned int offset; /* Window offset for overlapping */

  SU_FFTW(_plan)     plan;
  SU_FFTW(_complex) *window; /* Window */
  SU_FFTW(_complex) *fft;    /* Filtered spectrum */
};

typedef struct sigutils_specttuner_channel su_specttuner_channel_t;

/*
 * The spectral tuner leverages its 3/2-sized window buffer by keeping
 * two FFT plans (even & odd) and conditionally saving the same sample
 * twice.
 *
 *  <---- Even ----->
 * |________|________|________|
 *           <----- Odd ----->
 *
 * The spectral tuner changes between two states. During the even state,
 * the tuner populates the even part. During the odd statem, it populates
 * the odd part.
 *
 * |11111111|22222___|________|
 *
 * When the EVEN part is full, the even FFT plan is performed and the freq
 * domain filtering takes place. The specttuner then switches to ODD:
 *
 * |33311111|22222222|333_____|
 *
 * During the ODD state, we fill the remaining half of the odd part, but
 * also the first half of the even part. When the ODD part is full, the
 * odd plan is performed as well as the usual frequency filtering.
 */

struct sigutils_specttuner {
  struct sigutils_specttuner_params params;

  SU_FFTW(_complex) *window; /* 3/2 the space, double allocation trick */
  SU_FFTW(_complex) *fft;

  enum sigutils_specttuner_state state;
  SU_FFTW(_plan) plans[2]; /* Even and odd plans */

  unsigned int half_size; /* 3/2 of window size */
  unsigned int full_size; /* 3/2 of window size */
  unsigned int p; /* From 0 to window_size - 1 */

  SUBOOL ready; /* FFT ready */

  /* Channel list */
  PTR_LIST(struct sigutils_specttuner_channel, channel);
};

typedef struct sigutils_specttuner su_specttuner_t;

void su_specttuner_destroy(su_specttuner_t *st);

su_specttuner_t *su_specttuner_new(
    const struct sigutils_specttuner_params *params);

SUBOOL su_specttuner_feed_bulk(
    su_specttuner_t *st,
    const SUCOMPLEX *buf,
    SUSCOUNT size);

su_specttuner_channel_t *su_specttuner_open_channel(
    su_specttuner_t *st,
    const struct sigutils_specttuner_channel_params *params);

SUBOOL su_specttuner_close_channel(
    su_specttuner_t *st,
    su_specttuner_channel_t *channel);

#endif /* _SIGUTILS_SPECTTUNER_H */
