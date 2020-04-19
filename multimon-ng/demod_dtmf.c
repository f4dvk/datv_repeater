/*
 *      demod_dtmf.c -- DTMF signalling demodulator/decoder
 *
 *      Copyright (C) 1996
 *          Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
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

#include "multimon.h"
#include "filter.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include "jetsonGPIO.h"
/* ---------------------------------------------------------------------- */

/*
 *
 * DTMF frequencies
 *
 *      1209 1336 1477 1633
 *  697   1    2    3    A
 *  770   4    5    6    B
 *  852   7    8    9    C
 *  941   *    0    #    D
 *
 */

#define SAMPLE_RATE 22050
#define BLOCKLEN (SAMPLE_RATE/100)  /* 10ms blocks */
#define BLOCKNUM 4    /* must match numbers in multimon.h */

#define PHINC(x) ((x)*0x10000/SAMPLE_RATE)

/////////////////// GPIO ///////////////////
enum jetsonGPIONumber PTT = gpio78;
int load=0;

/////////////////// DTMF ///////////////////
int Buffer[25];
int DTMF=0;
int OK=false;
int Cod=0;
int Cd;
int New=0;

///////////////// COMMANDE /////////////////
int emission,val_RX_DATV,freq1,freq2,freq3,RX_437,RX_145,RX_1255,TX_437,TX_145,TX_1255;
int TX=0;
int RX=0;

///////////////// TEMPO TX /////////////////
unsigned long delai_TX=6;
time_t top;                        // variabe de calcul temps TX
time_t Time;                       // variabe de calcul temps TX

//////////////// TEMPO DTMF /////////////////
time_t topD;

static const char *dtmf_transl = "123A456B789C*0#D";

static const unsigned int dtmf_phinc[8] = {
	PHINC(1209), PHINC(1336), PHINC(1477), PHINC(1633),
	PHINC(697), PHINC(770), PHINC(852), PHINC(941)
};

/* ---------------------------------------------------------------------- */

static void dtmf_init(struct demod_state *s)
{
	memset(&s->l1.dtmf, 0, sizeof(s->l1.dtmf));
}

void exportGPIO(void)
{
  gpioExport(PTT);
}
void initGPIO(void)
{
  gpioSetDirection(PTT,outputPin);
  gpioSetValue(PTT, low);
}
/* ---------------------------------------------------------------------- */

static int find_max_idx(const float *f)
{
	float en = 0;
	int idx = -1, i;

	for (i = 0; i < 4; i++)
		if (f[i] > en) {
			en = f[i];
			idx = i;
		}
	if (idx < 0)
		return -1;
	en *= 0.1;
	for (i = 0; i < 4; i++)
		if (idx != i && f[i] > en)
			return -1;
	return idx;
}

/* ---------------------------------------------------------------------- */

static inline int process_block(struct demod_state *s)
{
	float tote;
	float totte[16];
	int i, j;

	tote = 0;
	for (i = 0; i < BLOCKNUM; i++)
		tote += s->l1.dtmf.energy[i];
	for (i = 0; i < 16; i++) {
		totte[i] = 0;
		for (j = 0; j < BLOCKNUM; j++)
			totte[i] += s->l1.dtmf.tenergy[j][i];
	}
	for (i = 0; i < 8; i++)
		totte[i] = fsqr(totte[i]) + fsqr(totte[i+8]);
	memmove(s->l1.dtmf.energy+1, s->l1.dtmf.energy,
		sizeof(s->l1.dtmf.energy) - sizeof(s->l1.dtmf.energy[0]));
	s->l1.dtmf.energy[0] = 0;
	memmove(s->l1.dtmf.tenergy+1, s->l1.dtmf.tenergy,
		sizeof(s->l1.dtmf.tenergy) - sizeof(s->l1.dtmf.tenergy[0]));
	memset(s->l1.dtmf.tenergy, 0, sizeof(s->l1.dtmf.tenergy[0]));
	tote *= (BLOCKNUM*BLOCKLEN*0.5);  /* adjust for block lengths */
	verbprintf(10, "DTMF: Energies: %8.5f  %8.5f %8.5f %8.5f %8.5f  %8.5f %8.5f %8.5f %8.5f\n",
		   tote, totte[0], totte[1], totte[2], totte[3], totte[4], totte[5], totte[6], totte[7]);
	if ((i = find_max_idx(totte)) < 0)
		return -1;
	if ((j = find_max_idx(totte+4)) < 0)
		return -1;
	if ((tote * 0.4) > (totte[i] + totte[j+4]))
		return -1;
	if (totte[i]*3.0<totte[j+4])
		return -1;
	if (totte[j+4]*3.0<totte[i])
		return -1;
	return (i & 3) | ((j << 2) & 0xc);
}

/* ---------------------------------------------------------------------- */

static void dtmf_demod(struct demod_state *s, buffer_t buffer, int length)
{

	float s_in;
	int i;

	for (; length > 0; length--, buffer.fbuffer++) {
		s_in = *buffer.fbuffer;
		s->l1.dtmf.energy[0] += fsqr(s_in);
		for (i = 0; i < 8; i++) {
			s->l1.dtmf.tenergy[0][i] += COS(s->l1.dtmf.ph[i]) * s_in;
			s->l1.dtmf.tenergy[0][i+8] += SIN(s->l1.dtmf.ph[i]) * s_in;
			s->l1.dtmf.ph[i] += dtmf_phinc[i];
		}
		if ((s->l1.dtmf.blkcount--) <= 0) {
			s->l1.dtmf.blkcount = BLOCKLEN;
			i = process_block(s);
			if (i != s->l1.dtmf.lastch && i >= 0){
				DTMF=i;
				verbprintf(0, "DTMF: %c\n", dtmf_transl[i]);
				//verbprintf(0,"Valeur: %d\n", DTMF);
				New=1;
			}
			s->l1.dtmf.lastch = i;
		}
	}

  if ((DTMF == 12) || (DTMF == 3) || (DTMF == 11)) (OK = true);

  if ((OK == true) && (New == 1)){
    Cod++;                                           // Incrementation du compteur curseur (seconde ligne)
    topD=time(NULL);

    Buffer[Cod] = DTMF;
    New=0;

    if ((DTMF == 3) || (DTMF == 11)){
      Cd = 1;
      DTMF=0;
    }
    else if (DTMF == 12){
      Cd = 3;
      DTMF=0;}

    if (Cod == Cd) {
      Commande();
      //verbprintf(0,"Execution d'une commande\n");
      Cd=0;
      Cod=0;
      OK=false;
      DTMF=0;
    }
  }
}

void tempo_dtmf(void)
{

  if ((Cod < 3) && (OK == true) && (((unsigned long)difftime(Time, topD)) > 4)) {
    verbprintf(0,"Hors temporisation\n");
    Cod=0;
    OK=false;
  }
}

void loop(void)
{
  if (load == 0)
  {
    initGPIO();
    load=1;
  }

  Time=time(NULL);
  tempo_dtmf();
  tempo_TX();
}

void Commande(void)
{
usleep(100);

//////////////////////////////////////// TX ////////////////////////////////////////
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 0) && (Cod>0)) // Code *01
    {
      if (RX_437 == 0){
        if (TX != 1){
          TX_LOW();
          usleep(500);
          verbprintf(0,"TX 437MHz SR250\n");
          TX_437=1;
          TX=1;
          emission=1;
          top=time(NULL);
          gpioSetValue(PTT, high);
          vocal();
        }
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 1) && (Cod>0)) // Code *02
    {
      if (RX_145 == 0){
        if (TX != 2){
          TX_LOW();
          usleep(500);
          verbprintf(0,"TX 145.9MHz SR125\n");
          TX_145=1;
          TX=2;
          emission=1;
          top=time(NULL);
          vocal();
        }
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 2) && (Cod>0)) // Code *03
    {
      if (RX_145 == 0){
        if (TX != 3){
          TX_LOW();
          usleep(500);
          verbprintf(0,"TX 145.9MHz SR250\n");
          TX_145=1;
          TX=3;
          emission=1;
          top=time(NULL);
          vocal();
        }
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 4) && (Cod>0)) // Code *04
    {
      if (RX_437 == 0){
        if (TX != 4){
          TX_LOW();
          usleep(500);
          verbprintf(0,"TX 437MHz SR125\n");
          TX_437=1;
          TX=4;
          emission=1;
          top=time(NULL);
          vocal();
        }
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 5) && (Cod>0)) // Code *05
    {
      if (RX_1255 == 0){
        if (TX != 4){
          TX_LOW();
          usleep(500);
          verbprintf(0,"TX 1255MHz SR250\n");
          TX_1255=1;
          TX=4;
          emission=1;
          top=time(NULL);
          vocal();
        }
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 6) && (Cod>0)) // Code *06
    {
      //TX_LOW();
      //usleep(500);
      verbprintf(0,"RESERVE\n");
      //TX=5;
      //top=time(NULL);
      //vocal();
    }

//////////////////////////////////////// RX ////////////////////////////////////////
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 13) && (Cod>0)) // Code *10
    {
      if (TX_437 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 437MHz SR250\n");
        RX_437=1;
        RX=1;
        vocal();
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 0) && (Cod>0)) // Code *11
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 145.9MHz SR125\n");
        RX_145=1;
        RX=2;
        vocal();
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 1) && (Cod>0)) // Code *12
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 145.9MHz SR250\n");
        RX_145=1;
        RX=3;
        vocal();
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 2) && (Cod>0)) // Code *13
    {
      if (TX_437 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 437MHz SR125\n");
        RX_437=1;
        RX=4;
        vocal();
      }
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 4) && (Cod>0)) // Code *14
    {
      if (TX_1255 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 1255MHz SR250\n");
        RX_1255=1;
        RX=5;
        vocal();
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 5) && (Cod>0)) // Code *15
    {
      if (TX_437 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 437MHz TNT\n");
        RX_437=1;
        RX=6;
        vocal();
      }
    else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 6) && (Cod>0)) // Code *16
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"MIRE\n");
      RX=7;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 8) && (Cod>0)) // Code *17
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"MULTI-VIDEOS\n");
      RX=8;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 9) && (Cod>0)) // Code *18
    {
      //RX_LOW();
      //usleep(500);
      verbprintf(0,"RESERVE\n");
      //RX=9;
      //vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 10) && (Cod>0)) // Code *19
    {
      //RX_LOW();
      //usleep(500);
      verbprintf(0,"RESERVE\n");
      //RX=10;
      //vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 13) && (Cod>0)) // Code *20
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"CAMERA\n");
      RX=11;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 0) && (Cod>0)) // Code *21
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"FILM\n");
      RX=12;
      vocal();
    }

/////////////////////////////////////// ARRET ///////////////////////////////////////
    if ((Buffer[1] == 3) && (Cod>0)) // Code A
    {
      verbprintf(0,"ARRET TX\n");
      TX_LOW();
    }
    if ((Buffer[1] == 11) && (Cod>0)) // Code C
    {
      verbprintf(0,"CONTROLE GENERAL\n");
      vocal();
    }
}

void tempo_TX(void)
{
    if ((emission == 1) && (((unsigned long)difftime(Time, top)) > delai_TX*60)){
      verbprintf(0,"Tempo de fin TX");
      TX_LOW();
      RX_LOW();
      usleep(500);
      RX=8;
      //digitalWrite (all_videos, LOW);
      vocal();
    }
}

void TX_LOW(void)
{
  system("sudo killall limesdr_dvb >/dev/null 2>/dev/null");
  system("sudo killall gst-launch-1.0 >/dev/null 2>/dev/null");
  system("sudo killall ffmpeg >/dev/null 2>/dev/null");
  system("sudo killall -9 limesdr_dvb >/dev/null 2>/dev/null");
  system("/home/$USER/dvbsdr/build/limesdr_toolbox/limesdr_stopchannel >/dev/null 2>/dev/null");
  //digitalWrite (PTT_DATV, HIGH);
  //digitalWrite (PTT_TNT, HIGH);
  //digitalWrite (PTT_UHF, HIGH);
  //digitalWrite (PTT_VHF, HIGH);
  //digitalWrite (PTT_1255, HIGH);
  gpioSetValue(PTT, low);
  emission=0;
  freq1=0;
  freq2=0;
  freq3=0;
  TX_437=0;
  TX_145=0;
  TX_1255=0;
  TX=0;
}

void RX_LOW(void)
{
  system("sudo killall full_rx >/dev/null 2>/dev/null");
  system("sudo killall longmynd >/dev/null 2>/dev/null");
  system("sudo killall mpv >/dev/null 2>/dev/null");
  //digitalWrite (RX_DATV, HIGH);
  //digitalWrite (RX_TNT, HIGH);
  //digitalWrite (MIRE, HIGH);
  //digitalWrite (all_videos, HIGH);
  //digitalWrite (VIDEO, HIGH);
  //digitalWrite (VIDEO_DATV, HIGH);
  //digitalWrite (RX_1255_FM, HIGH);
  //digitalWrite (RX_438_AM, HIGH);
  //digitalWrite (CAMERA, HIGH);
  //digitalWrite (MIRE_ANIMEE, HIGH);
  //digitalWrite (ANT_145, HIGH);
  //digitalWrite (ANT_437, HIGH);
  //digitalWrite (ANT_1255, HIGH);
  RX_437=0;
  RX_145=0;
  RX_1255=0;
  RX=0;
}

void vocal(void) {

  //digitalWrite (PTT_Vocal, LOW);
  usleep(400);

  if (TX == 1)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (TX == 2)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (TX == 3)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (TX == 4)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (TX == 5)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (TX == 6)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 1)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 2)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 3)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 4)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 5)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 6)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 7)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 8)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 9)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 10)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 11)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 12)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  if (RX == 13)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  }

  usleep(200);

  //digitalWrite (PTT_Vocal, HIGH);
}

void erreur(void) {
  usleep(500);
  //digitalWrite (PTT_Vocal, LOW);
  usleep(300);
  system("aplay -D plughw:2,0 --quiet /home/$USER/Musique/");
  usleep(200);
  //digitalWrite (PTT_Vocal, HIGH);
}

/* ---------------------------------------------------------------------- */

const struct demod_param demod_dtmf = {
    "DTMF", true, SAMPLE_RATE, 0, dtmf_init, dtmf_demod, NULL
};

/* ---------------------------------------------------------------------- */