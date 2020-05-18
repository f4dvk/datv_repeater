/*
 *      multimon.h -- Monitor for many different modulation formats
 *
 *      Copyright (C) 1996
 *          Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 *
 *      Added eas parts - A. Maitland Bottoms 27 June 2000
 *
 *      Copyright (C) 2012-2014
 *          Elias Oenal    (multimon-ng@eliasoenal.com)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* ---------------------------------------------------------------------- */

#ifndef _MULTIMON_H
#define _MULTIMON_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _MSC_VER
#include "msvc_support.h"
#endif

/* ---------------------------------------------------------------------- */

extern const float costabf[0x400];
#define COS(x) costabf[(((x)>>6)&0x3ffu)]
#define SIN(x) COS((x)+0xc000)

/* ---------------------------------------------------------------------- */

enum EAS_L1_State
{
    EAS_L1_IDLE = 0,
    EAS_L1_SYNC = 1,
};

struct demod_state {
    const struct demod_param *dem_par;
    union{

        struct l1_state_dtmf {
            unsigned int ph[8];
            float energy[4];
            float tenergy[4][16];
            int blkcount;
            int lastch;
        } dtmf;
    } l1;
};

typedef struct buffer
{
    const short* sbuffer;
    const float* fbuffer;
} buffer_t;

struct demod_param {
    const char *name;
    bool float_samples; // if false samples are short instead
    unsigned int samplerate;
    unsigned int overlap;
    void (*init)(struct demod_state *s);
    void (*demod)(struct demod_state *s, buffer_t buffer, int length);
    void (*deinit)(struct demod_state *s);
};

/* ---------------------------------------------------------------------- */

extern const struct demod_param demod_dtmf;

#define ALL_DEMOD &demod_dtmf


/* ---------------------------------------------------------------------- */

void _verbprintf(int verb_level, const char *fmt, ...);
#if !defined(MAX_VERBOSE_LEVEL)
#   define MAX_VERBOSE_LEVEL 0
#endif
#define verbprintf(level, ...) \
    do { if (level <= MAX_VERBOSE_LEVEL) _verbprintf(level, __VA_ARGS__); } while (0)

/* ---------------------------------------------------------------------- */

void loop(void);
void Ptt(void);
void TX_LOW(void);
void RX_LOW(void);
void tempo_dtmf(void);
void tempo_TX(void);
void Commande(void);
void vocal(void);
void erreur(void);
void initGPIO(void);
void exportGPIO(void);

/* ---------------------------------------------------------------------- */
#endif /* _MULTIMON_H */
