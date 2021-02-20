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
#include <curl/curl.h>

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

/////////////////// GPIO /////////////////// MAX 22
enum jetsonGPIONumber ptt_vocal = gpio78;
enum jetsonGPIONumber band_bit0 = gpio16;
enum jetsonGPIONumber band_bit1 = gpio17;
//enum jetsonGPIONumber ant_1255 = gpio18;
enum jetsonGPIONumber ptt_145 = gpio13;
enum jetsonGPIONumber ptt_437 = gpio19;
enum jetsonGPIONumber ptt_1255 = gpio20;
enum jetsonGPIONumber rx_tnt = gpio77;
int load=0;

/////////////////// DTMF ///////////////////
int Buffer[25];
int DTMF=0;
int OK=false;
int Cod=0;
int Cd;

///////////////// COMMANDE /////////////////
int emission,val_RX_DATV,freq1,freq2,freq3,RX_437,RX_145,RX_1255,TX_437,TX_145,TX_1255;
int TX=0;
int RX=0;
char path1[630];
char path2[630];
char path3[630];
char path4[630];

char Tx[10];
FILE *dvb;
char user[30];

/////////////////// VIDEO //////////////////
char Resolution[10];
int Fps=25;
int bitrate_v;
char new_bitrate[10];

///////////////// TEMPO TX /////////////////
unsigned long delai_TX=6;
time_t top;                        // variabe de calcul temps TX
time_t Time;                       // variabe de calcul temps TX

//////////////// TEMPO DTMF /////////////////
time_t topD;

//////////////// TEMPO PTT /////////////////
unsigned long delai_PTT=5;
time_t topPTT;
int TX_On=0;

static const char *dtmf_transl = "123A456B789C*0#D";

static const unsigned int dtmf_phinc[8] = {
	PHINC(1209), PHINC(1336), PHINC(1477), PHINC(1633),
	PHINC(697), PHINC(770), PHINC(852), PHINC(941)
};

void SetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char Command[511];
  char BackupConfigName[240];
  strcpy(BackupConfigName,PathConfigFile);
  strcat(BackupConfigName,".bak");
  FILE *fp=fopen(PathConfigFile,"r");
  FILE *fw=fopen(BackupConfigName,"w+");
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  if(fp!=0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        fprintf(fw, "%s=%s\n", Param, Value);
      }
      else
      {
        fprintf(fw, "%s", line);
      }
    }
    fclose(fp);
    fclose(fw);
    snprintf(Command, 511, "cp %s %s", BackupConfigName, PathConfigFile);
    system(Command);
  }
  else
  {
    printf("Config file not found \n");
    fclose(fp);
    fclose(fw);
  }
}

void GetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

  //printf("Get Config reads %s for %s ", PathConfigFile , Param);

  FILE *fp=fopen(PathConfigFile, "r");
  if(fp != 0)
  {
    while ((read = getline(&line, &len, fp)) != -1)
    {
      if(strncmp (line, ParamWithEquals, strlen(Param) + 1) == 0)
      {
        strcpy(Value, line+strlen(Param)+1);
        char *p;
        if((p=strchr(Value,'\n')) !=0 ) *p=0; //Remove \n
        break;
      }
    }
  }
  else
  {
    printf("Config file not found \n");
  }
  fclose(fp);
}

/* ---------------------------------------------------------------------- */

static void dtmf_init(struct demod_state *s)
{
	memset(&s->l1.dtmf, 0, sizeof(s->l1.dtmf));
}

void unexportGPIO(void)
{
  gpioUnexport(ptt_vocal);
  gpioUnexport(band_bit0);
  gpioUnexport(band_bit1);
  //gpioUnexport(ant_1255);
  gpioUnexport(ptt_145);
  gpioUnexport(ptt_437);
  gpioUnexport(ptt_1255);
  gpioUnexport(rx_tnt);
}

void exportGPIO(void)
{
  gpioExport(ptt_vocal);
  gpioExport(band_bit0);
  gpioExport(band_bit1);
  //gpioExport(ant_1255);
  gpioExport(ptt_145);
  gpioExport(ptt_437);
  gpioExport(ptt_1255);
  gpioExport(rx_tnt);
}

void initGPIO(void)
{
  ////////////////////// config ///////////////////
  strcpy(user, getenv("USER"));

  snprintf(path1, 630, "/home/%s/datv_repeater/longmynd/config.txt", user);
  #define PATH_PCONFIG_RX path1

  snprintf(path2, 630, "/home/%s/datv_repeater/dvbtx/scripts/config.txt", user);
  #define PATH_PCONFIG_TX path2

  snprintf(path3, 630, "/home/%s/datv_repeater/source/config.txt", user);
  #define PATH_PCONFIG_SRC path3

  snprintf(path4, 630, "/home/%s/datv_repeater/config.txt", user);
  #define PATH_PCONFIG path4

  ///////////////////// GPIO ////////////////////
  gpioSetDirection(ptt_vocal, outputPin);
  gpioSetDirection(band_bit0, outputPin);
  gpioSetDirection(band_bit1, outputPin);
  //gpioSetDirection(ant_1255, outputPin);
  gpioSetDirection(ptt_145, outputPin);
  gpioSetDirection(ptt_437, outputPin);
  gpioSetDirection(ptt_1255, outputPin);
  gpioSetDirection(rx_tnt, outputPin);

  gpioSetValue(ptt_vocal, low);
  gpioSetValue(band_bit0, low);
  gpioSetValue(band_bit1, low);
  //gpioSetValue(ant_1255, high);
  gpioSetValue(ptt_145, low);
  gpioSetValue(ptt_437, low);
  gpioSetValue(ptt_1255, low);
  gpioSetValue(rx_tnt, low);

}

void initCOM(void)
{
  char USB[8];
  char commande[150];

  FILE *fp;

  fp = popen("dmesg | grep ttyUSB | grep ch341 | awk '{printf $NF}'", "r");
  if (fp == NULL) {
    printf("Erreur Commande recherche nom USB\n" );
    exit(1);
  }

  while (fgets(USB, 8, fp) != NULL)
  {
    //printf("%s", USB);
  }

  pclose(fp);

  system("sudo killall cat >/dev/null 2>/dev/null");

  snprintf(commande, 150, "sudo chmod o+rw /dev/%s", USB);
  system(commande);

  snprintf(commande, 150, "stty 9600 -F /dev/%s raw -echo", USB);
  system(commande);

  snprintf(commande, 150, "cat /dev/%s >/dev/null 2/dev/null &", USB);
  system(commande);

}

void initRX(void)
{
  usleep(500);
  SetConfigParam(PATH_PCONFIG_SRC, "source", "MULTI");
  system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
  verbprintf(0,"MULTI-VIDEOS\n");
  RX=8;
}

void GetIPAddr(char IPAddress[20])
{
  FILE *fp;

  fp = popen("ip -f inet -o addr show eth0 | cut -d\\  -f 7 | cut -d/ -f 1 | awk 1 ORS=", "r");
  if (fp == NULL) {
    printf("Erreur Commande IP\n" );
    exit(1);
  }

  while (fgets(IPAddress, 16, fp) != NULL)
  {
    //printf("%s", IPAddress);
  }

  pclose(fp);
}

void strategy(int bitrate_ts) // Calcul firmware Pluto de F5OEO
{
  char calcul[150];

  int Audio_b=32000;
  //int AUDIOCHANNELS=2;
  int VIDEO_WIDTH=1920;
  int VIDEO_HEIGHT=1080;

  snprintf(calcul, 150, "echo '(%d/1000)*85/100-10-%d/1000' | bc", bitrate_ts, Audio_b);
  FILE *value=popen (calcul, "r");

  while (fgets(new_bitrate, 10, value) != NULL)
  {
    printf("Résultat Bitrate video: %s", new_bitrate);
  }
  pclose (value);

  int new_bitrate_v=atoi(new_bitrate);

  if (new_bitrate_v < 1200)
  {
    //AUDIOCHANNELS=2;
    Fps=25;
    VIDEO_WIDTH=1280;
    VIDEO_HEIGHT=720;
    snprintf(calcul, 150, "echo '(%d/1000)*80/100-10-%d/1000' | bc", bitrate_ts, Audio_b);
    FILE *value=popen (calcul, "r");

    while (fgets(new_bitrate, 10, value) != NULL)
    {
      //printf("Résultat Bitrate video: %s", new_bitrate);
    }
    pclose (value);
  }

  if (new_bitrate_v < 400)
  {
    //AUDIOCHANNELS=2;
    Fps=25;
    VIDEO_WIDTH=768;
    VIDEO_HEIGHT=432;
    snprintf(calcul, 150, "echo '(%d/1000)*75/100-10-%d/1000' | bc", bitrate_ts, Audio_b);
    FILE *value=popen (calcul, "r");

    while (fgets(new_bitrate, 10, value) != NULL)
    {
      //printf("Résultat Bitrate video: %s", new_bitrate);
    }
    pclose (value);
  }

  if (new_bitrate_v < 250)
  {
    //AUDIOCHANNELS=1;
    VIDEO_WIDTH=576;
    VIDEO_HEIGHT=324;
    Fps=15;
    snprintf(calcul, 150, "echo '(%d/1000)*75/100-10-%d/1000' | bc", bitrate_ts, Audio_b);
    FILE *value=popen (calcul, "r");

    while (fgets(new_bitrate, 10, value) != NULL)
    {
      //printf("Résultat Bitrate video: %s", new_bitrate);
    }
    pclose (value);
  }

  if (new_bitrate_v < 200)
  {
    //AUDIOCHANNELS=1;
    VIDEO_WIDTH=384;
    VIDEO_HEIGHT=216;
    Fps=15;
    snprintf(calcul, 150, "echo '(%d/1000)*65/100-10-%d/1000' | bc", bitrate_ts, Audio_b);
    FILE *value=popen (calcul, "r");

    while (fgets(new_bitrate, 10, value) != NULL)
    {
      //printf("Résultat Bitrate video: %s", new_bitrate);
    }
    pclose (value);
  }

  if (new_bitrate_v < 100)
  {
    //AUDIOCHANNELS=1;
    VIDEO_WIDTH=384;
    VIDEO_HEIGHT=216;
    Fps=10;
    strcpy(new_bitrate, "64");
  }

  snprintf(Resolution, 10, "%dx%d", VIDEO_WIDTH, VIDEO_HEIGHT);
  printf("Video Bitrate: %d Resolution: %s at %d\n", atoi(new_bitrate), Resolution, Fps);
}

int encoder_start_dvbt()
{
  char Ip[20];
  char PlutoIp[20];
  char ServeurIp[20];
  char ServeurPort[10];
  char Url[60];
  char Body[500];

  char result[20];

  GetIPAddr(result);

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG,"plutoip", PlutoIp);
  GetConfigParam(PATH_PCONFIG,"serveurip", ServeurIp);
  GetConfigParam(PATH_PCONFIG,"serveurport", ServeurPort);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=multicast", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 500, " <request><multicast>\
          <mcast>\
          <active>1</active>\
          <port>%s</port>\
          <addr>%s</addr>\
          </mcast>\
          <mcast>\
          <active>1</active>\
          <port>1314</port>\
          <addr>%s</addr>\
          </mcast>\
          <mcast>\
          <active>0</active>\
          <port>999</port>\
          <addr>192.168.10.20</addr>\
          </mcast>\
          </multicast></request>", ServeurPort, ServeurIp, result);

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  /* Clean up after yourself */
  curl_easy_cleanup(curl);
  return 0;
}

int encoder_stop_dvbt()
{
  char Ip[20];
  char PlutoIp[20];
  char ServeurIp[20];
  char ServeurPort[10];
  char Url[60];
  char Body[500];

  char result[20];

  GetIPAddr(result);

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG,"plutoip", PlutoIp);
  GetConfigParam(PATH_PCONFIG,"serveurip", ServeurIp);
  GetConfigParam(PATH_PCONFIG,"serveurport", ServeurPort);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=multicast", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 500, " <request><multicast>\
          <mcast>\
          <active>1</active>\
          <port>%s</port>\
          <addr>%s</addr>\
          </mcast>\
          <mcast>\
          <active>0</active>\
          <port>1314</port>\
          <addr>%s</addr>\
          </mcast>\
          <mcast>\
          <active>0</active>\
          <port>999</port>\
          <addr>192.168.10.20</addr>\
          </mcast>\
          </multicast></request>", ServeurPort, ServeurIp, result);

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  /* Clean up after yourself */
  curl_easy_cleanup(curl);
  return 0;
}

int encoder_start()
{
  char Ip[20];
  char PlutoIp[20];
  char Call[10];
  char Freq[10];
  char Mode[10];
  char Constellation[10];
  char Sr[10];
  char Fec[10];
  char Gain[4];
  char command[150];
  char Gain_Pluto[10];
  int fec_num;
  int fec_den;

  char Url[60];
  char Body[500];

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG,"plutoip", PlutoIp);
  GetConfigParam(PATH_PCONFIG,"call", Call);
  GetConfigParam(PATH_PCONFIG_TX,"freq", Freq);
  GetConfigParam(PATH_PCONFIG_TX,"symbolrate", Sr);
  GetConfigParam(PATH_PCONFIG_TX,"fec", Fec);
  GetConfigParam(PATH_PCONFIG_TX,"mode", Mode);
  GetConfigParam(PATH_PCONFIG_TX,"constellation", Constellation);
  GetConfigParam(PATH_PCONFIG_TX,"gain", Gain);

  FILE *gain;
  snprintf(command, 150, "echo - | awk '{print ('%s' / 100)* 71+-71}'", Gain);
  gain=popen (command, "r");

  while (fgets(Gain_Pluto, 10, gain) != NULL)
  {
    //printf("Gain Pluto: %s", Gain_Pluto);
  }
  pclose (gain);

  snprintf(Gain_Pluto, 10, "%.0f", atof(Gain_Pluto));

  if ((strcmp (Mode, "DVBS") == 0) || (strcmp (Mode, "DVBT") == 0))
  {
    fec_num = atoi(Fec);
    fec_den = atoi(Fec)+1;
  }
  else
  {
    char fec_i[2];
    fec_num = log10(atoi(Fec));
    fec_num = atoi(Fec) / pow(10, fec_num);
    fec_den = atoi(Fec) % 10;
    sprintf(fec_i, "%d", fec_den);
    if (strcmp (fec_i, "1") == 0) fec_den = 10;
  }

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=rtmp", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 500, " <request><rtmp><port>1935</port>\
          <push>\
          <active>0</active>\
          <url></url>\
          </push>\
          <push>\
          <active>1</active>\
          <url>rtmp://%s:7272/,%s,%s,%s,%s,%d%d,%s,nocalib,800,32,/,%s,</url>\
          </push></rtmp></request>", PlutoIp, Freq, Mode, Constellation, Sr, fec_num, fec_den, Gain_Pluto, Call);

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  /* Clean up after yourself */
  curl_easy_cleanup(curl);
  return 0;
}

int encoder_stop()
{
  char Ip[20];
  char PlutoIp[20];
  char Call[10];
  char Freq[10];
  char Mode[10];
  char Constellation[10];
  char Sr[10];
  char Fec[10];
  int fec_num;
  int fec_den;

  char Url[60];
  char Body[500];

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG,"plutoip", PlutoIp);
  GetConfigParam(PATH_PCONFIG,"call", Call);
  GetConfigParam(PATH_PCONFIG_TX,"freq", Freq);
  GetConfigParam(PATH_PCONFIG_TX,"symbolrate", Sr);
  GetConfigParam(PATH_PCONFIG_TX,"fec", Fec);
  GetConfigParam(PATH_PCONFIG_TX,"mode", Mode);
  GetConfigParam(PATH_PCONFIG_TX,"constellation", Constellation);

  if (strcmp (Mode, "DVBS") == 0)
  {
    fec_num = atoi(Fec);
    fec_den = atoi(Fec)+1;
  }
  else
  {
    char fec_i[2];
    fec_num = log10(atoi(Fec));
    fec_num = atoi(Fec) / pow(10, fec_num);
    fec_den = atoi(Fec) % 10;
    sprintf(fec_i, "%d", fec_den);
    if (strcmp (fec_i, "1") == 0) fec_den = 10;
  }

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=rtmp", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 500, " <request><rtmp><port>1935</port>\
          <push>\
          <active>0</active>\
          <url></url>\
          </push>\
          <push>\
          <active>0</active>\
          <url>rtmp://%s:7272/,%s,%s,%s,%s,%d%d,-0,nocalib,800,32,/,%s,</url>\
          </push></rtmp></request>", PlutoIp, Freq, Mode, Constellation, Sr, fec_num, fec_den, Call);

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  /* Clean up after yourself */
  curl_easy_cleanup(curl);
  return 0;
}

int encoder_video()
{
  char command[150];
  char Ip[20];
  char Mode[10];
  char Constellation[10];
  char Sr[10];
  char Fec[10];
  char Codec[10];
  int fec_num;
  int fec_den;
  char bitrate[10];

  char Url[60];
  char Body[350];

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG_TX,"symbolrate", Sr);
  GetConfigParam(PATH_PCONFIG_TX,"fec", Fec);
  GetConfigParam(PATH_PCONFIG_TX,"mode", Mode);
  GetConfigParam(PATH_PCONFIG_TX,"constellation", Constellation);
  GetConfigParam(PATH_PCONFIG_TX,"codec", Codec);

  if (strcmp (Codec, "H265") == 0)
  {
    strcpy(Codec, "1");  // H265
  }
  else
  {
    strcpy(Codec, "0");  // H264
  }

  if ((strcmp (Mode, "DVBS") == 0) || (strcmp (Mode, "DVBT") == 0))
  {
    fec_num = atoi(Fec);
    fec_den = atoi(Fec)+1;
  }
  else
  {
    char fec_i[2];
    fec_num = log10(atoi(Fec));
    fec_num = atoi(Fec) / pow(10, fec_num);
    fec_den = atoi(Fec) % 10;
    sprintf(fec_i, "%d", fec_den);
    if (strcmp (fec_i, "1") == 0) fec_den = 10;
  }

  snprintf(command, 150, "/home/%s/dvb2iq -s %s -f %d/%d -d -r 1 -m %s -c %s 2>/dev/null", user, Sr, fec_num, fec_den, Mode, Constellation);

  dvb=popen (command, "r");

  while (fgets(bitrate, 10, dvb) != NULL)
  {
    //printf("Résultat Bitrate TS: %s", bitrate);
  }
  pclose (dvb);

  strategy(atoi(bitrate));

/*  // Bitrate Audio: 32K
  int new_bitrate = ((atoi(bitrate)-12000-(32000*15/10))*650/1000);
  new_bitrate = new_bitrate/1000;
  //printf("Bitrate video: %d kbit/s\n", new_bitrate);


  if (new_bitrate < 180) // DVBS 250Ks 3/4 > 185 kbits/s
  {
    strcpy(Resolution, "384x216");
    Fps=20;
  }
  else
  {
    strcpy(Resolution, "1024x576");
  }
*/
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=videoenc&stream=1", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 350, " <request><videoenc>\
          <codec>%s</codec>\
          <resolution>%s</resolution>\
          <framerate>%d</framerate>\
          <audioenc>1</audioenc>\
          <rc>1</rc>\
          <keygop>100</keygop>\
          <bitrate>%d</bitrate>\
          <quality>5</quality>\
          <profile>0</profile>\
          </videoenc></request>", Codec, Resolution, Fps, atoi(new_bitrate));

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  curl_easy_cleanup(curl);
  return 0;
}

int encoder_video_dvbt()
{
  char Ip[20];
  char Sr[10];
  char Fec[10];
  char Codec[10];
  int fec_num;
  int fec_den;
  char Qam[10];
  char Guard[10];
  int BitsPerSymbol;
  long long Bitrate_TS;
  int Bitrate_Video;
  int Fps=20;
  int VIDEO_WIDTH;
  int VIDEO_HEIGHT;

  char Url[60];
  char Body[350];

  GetConfigParam(PATH_PCONFIG,"encoderip", Ip);
  GetConfigParam(PATH_PCONFIG_TX,"symbolrate", Sr);
  GetConfigParam(PATH_PCONFIG_TX,"fec", Fec);
  GetConfigParam(PATH_PCONFIG_TX,"codec", Codec);
  GetConfigParam(PATH_PCONFIG_TX,"qam", Qam);
  GetConfigParam(PATH_PCONFIG_TX,"guard", Guard);

  if (strcmp (Qam, "qpsk") == 0)
  {
    BitsPerSymbol=2;
  }
  else if (strcmp (Qam, "16qam") == 0)
  {
    BitsPerSymbol=4;
  }
  else if (strcmp (Qam, "64qam") == 0)
  {
    BitsPerSymbol=6;
  }
  else
  {
    BitsPerSymbol=2;
  }

  if (strcmp (Codec, "H265") == 0)
  {
    strcpy(Codec, "1");  // H265
  }
  else
  {
    strcpy(Codec, "0");  // H264
  }

  fec_num = atoi(Fec);
  fec_den = atoi(Fec)+1;

  /// Calcul BATC ///
  Bitrate_TS=423*(atoi(Sr)*1000);
  Bitrate_TS=Bitrate_TS*fec_num;
  Bitrate_TS=Bitrate_TS*BitsPerSymbol;
  Bitrate_TS=Bitrate_TS*atoi(Guard);

  Bitrate_TS=Bitrate_TS/fec_den;
  Bitrate_TS=Bitrate_TS/(atoi(Guard)+1);
  Bitrate_TS=Bitrate_TS/544;

  //int Margin=100;
  //Bitrate_TS=Bitrate_TS*Margin;
  //Bitrate_TS=Bitrate_TS/100;

  Bitrate_Video=(Bitrate_TS-24000-((32000)*15/10))*725/1000;

  Bitrate_Video=Bitrate_Video/1000;

  if (Bitrate_Video > 190)
  {
    VIDEO_WIDTH=704;
    VIDEO_HEIGHT=576;
    Fps=25;
  }
  else
  {
    VIDEO_WIDTH=352;
    VIDEO_HEIGHT=288;
    //Fps=25;
  }

  if (Bitrate_Video < 100)
  {
    VIDEO_WIDTH=160;
    VIDEO_HEIGHT=120;
  }

  snprintf(Resolution, 10, "%dx%d", VIDEO_WIDTH, VIDEO_HEIGHT);
  printf("DVB-T Video Bitrate: %d Resolution: %s at %d\n", Bitrate_Video, Resolution, Fps);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=videoenc&stream=1", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 350, " <request><videoenc>\
          <codec>%s</codec>\
          <resolution>%s</resolution>\
          <framerate>%d</framerate>\
          <audioenc>1</audioenc>\
          <rc>1</rc>\
          <keygop>100</keygop>\
          <bitrate>%d</bitrate>\
          <quality>5</quality>\
          <profile>0</profile>\
          </videoenc></request>", Codec, Resolution, Fps, Bitrate_Video);

  char *body = Body;

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    return 1;
  }
  curl_easy_cleanup(curl);
  return 0;
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
				if ((DTMF == 12) || (DTMF == 3) || (DTMF == 11)) (OK = true);

				if (OK == true){
					Cod++;                                           // Incrementation du compteur curseur (seconde ligne)
					topD=time(NULL);

					Buffer[Cod] = DTMF;

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
				//verbprintf(0,"Valeur: %d\n", DTMF);
			}
			s->l1.dtmf.lastch = i;
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
    system("pqiv --fullscreen --hide-info-box /home/$USER/datv_repeater/media/image.gif &");
    initGPIO();
    initCOM();
    TX_LOW();
    RX_LOW();
    initRX();
    load=1;
  }

  Time=time(NULL);
  tempo_dtmf();
  tempo_TX();
  Ptt();
}

void Commande(void)
{
  GetConfigParam(PATH_PCONFIG,"tx", Tx);
  usleep(100);

  //////////////////////////////////////// TX ////////////////////////////////////////
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 0) && (Cod>0)) // Code *01
    {
      if (RX_145 == 0){
        if (TX != 1){
          TX_LOW();
          usleep(800);
          verbprintf(0,"TX 145.9MHz SR125\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "145.9");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "125");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "7");
          usleep(100);
          TX_145=1;
          band_select();
          TX=1;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
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
          usleep(800);
          verbprintf(0,"TX 145.9MHz SR92\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "145.9");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS2");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "8PSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "92");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "3");
          usleep(100);
          TX_145=1;
          band_select();
          TX=2;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 2) && (Cod>0)) // Code *03
    {
      if (RX_437 == 0){
        if (TX != 3){
          TX_LOW();
          usleep(800);
          verbprintf(0,"TX 437MHz SR125\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "125");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "7");
          usleep(100);
          TX_437=1;
          band_select();
          TX=3;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
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
          usleep(800);
          verbprintf(0,"TX 437MHz SR250\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "3");
          usleep(100);
          TX_437=1;
          band_select();
          TX=4;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 5) && (Cod>0)) // Code *05
    {
      if (RX_437 == 0){
        if (TX != 5){
          TX_LOW();
          usleep(800);
          verbprintf(0,"TX 437MHz SR500\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "500");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "1");
          usleep(100);
          TX_437=1;
          band_select();
          TX=5;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 6) && (Cod>0)) // Code *06
    {
      if (RX_1255 == 0){
        if (TX != 6){
          TX_LOW();
          usleep(800);
          verbprintf(0,"TX 1255MHz SR250\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "1255");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "3");
          usleep(100);
          TX_1255=1;
          band_select();
          TX=6;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video();
            encoder_start();
          }
          else // Limesdr via raspberry
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 8) && (Cod>0)) // Code *07
    {
      if (RX_437 == 0){
        if (TX != 7){
          TX_LOW();
          usleep(800);
          verbprintf(0,"TX 437MHz DVB-T 250\n");
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBT");
          SetConfigParam(PATH_PCONFIG_TX, "qam", "qpsk");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "2");
          SetConfigParam(PATH_PCONFIG_TX, "guard", "32");
          usleep(100);
          TX_437=1;
          band_select();
          TX=7;
          emission=1;
          if (strcmp (Tx, "pluto") == 0)
          {
            encoder_video_dvbt();
            encoder_start();
          }
          else
          {
            system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          }
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }

//////////////////////////////////////// RX ////////////////////////////////////////
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 13) && (Cod>0)) // Code *10
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 145.9MHz SR125\n");
        SetConfigParam(PATH_PCONFIG_RX, "freq", "145900");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "125");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_145=1;
        band_select();
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
        verbprintf(0,"RX 145.9MHz SR250\n");
        SetConfigParam(PATH_PCONFIG_RX, "freq", "145900");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "250");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_145=1;
        band_select();
        RX=2;
        vocal();
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 1) && (Cod>0)) // Code *12
    {
      if (TX_437 == 0){
        RX_LOW();
        usleep(500);
        verbprintf(0,"RX 437MHz SR125\n");
        SetConfigParam(PATH_PCONFIG_RX, "freq", "437000");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "125");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_437=1;
        band_select();
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
        verbprintf(0,"RX 437MHz SR250\n");
        SetConfigParam(PATH_PCONFIG_RX, "freq", "437000");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "250");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_437=1;
        band_select();
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
        SetConfigParam(PATH_PCONFIG_RX, "freq", "1255000");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "250");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_1255=1;
        band_select();
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
        system("echo 'OUTA_1'>/dev/ttyUSB0");
        system("echo 'OUTB_1'>/dev/ttyUSB0");
        verbprintf(0,"RX 437MHz TNT\n");
        RX_437=1;
        //band_select();
        RX=6;
        //gpioSetValue(ant_437, low);
        gpioSetValue(rx_tnt, high);
        vocal();
      }
      else erreur();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 6) && (Cod>0)) // Code *16
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"MIRE\n");
      SetConfigParam(PATH_PCONFIG_SRC, "source", "MIRE");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=7;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 8) && (Cod>0)) // Code *17
    {
      RX_LOW();
      usleep(500);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "MULTI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      verbprintf(0,"MULTI-VIDEOS\n");
      RX=8;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 9) && (Cod>0)) // Code *18
    {
      RX_LOW();
      usleep(500);
      system("echo 'OUTA_2'>/dev/ttyUSB0");
      system("echo 'OUTB_2'>/dev/ttyUSB0");
      SetConfigParam(PATH_PCONFIG_SRC, "source", "HDMI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      verbprintf(0,"RESERVE\n");
      RX=9;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 10) && (Cod>0)) // Code *19
    {
      RX_LOW();
      usleep(500);
      system("echo 'OUTA_3'>/dev/ttyUSB0");
      system("echo 'OUTB_3'>/dev/ttyUSB0");
      SetConfigParam(PATH_PCONFIG_SRC, "source", "HDMI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      verbprintf(0,"RESERVE\n");
      RX=10;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 13) && (Cod>0)) // Code *20
    {
      RX_LOW();
      usleep(500);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "CAMERA");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      verbprintf(0,"CAMERA\n");
      RX=11;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 0) && (Cod>0)) // Code *21
    {
      RX_LOW();
      usleep(500);
      verbprintf(0,"FILM\n");
      SetConfigParam(PATH_PCONFIG_SRC, "source", "FILM");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
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
  if ((emission == 1) && (((unsigned long)difftime(Time, top)) > delai_TX*60))
  {
    verbprintf(0,"Tempo de fin TX\n");
    TX_LOW();
    RX_LOW();
    //usleep(500);
    initRX();
    vocal();
  }
}

void band_select(void)
{
  if ((TX_145 == 1) || (RX_145 == 1))
  {
    gpioSetValue(band_bit0, high);
    gpioSetValue(band_bit1, low);
  }
  else if ((TX_437 == 1) || (RX_437 == 1))
  {
    gpioSetValue(band_bit0, low);
    gpioSetValue(band_bit1, high);
  }
  else if ((TX_1255 == 1) || (RX_1255 == 1))
  {
    gpioSetValue(band_bit0, high);
    gpioSetValue(band_bit1, high);
  }
  else
  {
    gpioSetValue(band_bit0, low);
    gpioSetValue(band_bit1, low);
  }
}

void Ptt(void)
{
  if ((TX != 0) && (TX_On == 0) && (((unsigned long)difftime(Time, topPTT)) > delai_PTT))
  {
    if (TX_145 == 1)
      gpioSetValue(ptt_145, high);
    else if (TX_437 == 1)
      gpioSetValue(ptt_437, high);
    else if (TX_1255 == 1)
      gpioSetValue(ptt_1255, high);
    TX_On=1;
  }
}

void TX_LOW(void)
{
  GetConfigParam(PATH_PCONFIG,"tx", Tx);

  if (strcmp (Tx, "pluto") == 0)
  {
    encoder_stop();
    encoder_stop_dvbt();
    system("sudo killall dvb_t_stack >/dev/null 2>/dev/null");
  }
  else // Limesdr via raspberry
  {
    system("/home/$USER/datv_repeater/dvbtx/scripts/TXstop.sh >/dev/null 2>/dev/null &");
  }
  //digitalWrite (PTT_DATV, HIGH);
  //digitalWrite (PTT_TNT, HIGH);
  //digitalWrite (PTT_UHF, HIGH);
  //digitalWrite (PTT_VHF, HIGH);
  //digitalWrite (PTT_1255, HIGH);
  gpioSetValue(ptt_145, low);
  gpioSetValue(ptt_437, low);
  gpioSetValue(ptt_1255, low);
  emission=0;
  freq1=0;
  freq2=0;
  freq3=0;
  TX_145=0;
  TX_437=0;
  TX_1255=0;
  TX=0;
  TX_On=0;
}

void RX_LOW(void)
{
  system("sudo killall full_rx >/dev/null 2>/dev/null");
  system("sudo killall longmynd >/dev/null 2>/dev/null");
  system("killall mpv >/dev/null 2>/dev/null");
  system("killall gst-launch-1.0 >/dev/null 2>/dev/null");
  system("sudo killall vlc >/dev/null 2>/dev/null");
  //digitalWrite (MIRE, HIGH);
  //digitalWrite (all_videos, HIGH);
  //digitalWrite (VIDEO, HIGH);
  //digitalWrite (VIDEO_DATV, HIGH);
  //digitalWrite (RX_1255_FM, HIGH);
  //digitalWrite (RX_438_AM, HIGH);
  //digitalWrite (CAMERA, HIGH);
  gpioSetValue(rx_tnt, low);
  RX_145=0;
  RX_437=0;
  RX_1255=0;
  RX=0;
}

void vocal(void) {

  gpioSetValue(ptt_vocal, high);

  usleep(400);

  if (TX == 1)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/01.wav");
  }

  if (TX == 2)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/02.wav");
  }

  if (TX == 3)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/03.wav");
  }

  if (TX == 4)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/04.wav");
  }

  if (TX == 5)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/05.wav");
  }

  if (TX == 6)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/06.wav");
  }

  if (RX == 1)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/10.wav");
  }

  if (RX == 2)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/11.wav");
  }

  if (RX == 3)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/12.wav");
  }

  if (RX == 4)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/13.wav");
  }

  if (RX == 5)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/14.wav");
  }

  if (RX == 6)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/15.wav");
  }

  if (RX == 7)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/16.wav");
  }

  if (RX == 8)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/17.wav");
  }

  if (RX == 9)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/18.wav");
  }

  if (RX == 10)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/19.wav");
  }

  if (RX == 11)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/20.wav");
  }

  if (RX == 12)
  {
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/21.wav");
  }

  usleep(200);

  gpioSetValue(ptt_vocal, low);
}

void erreur(void) {
  usleep(500);
  gpioSetValue(ptt_vocal, high);
  usleep(300);
  system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/erreur.wav");
  usleep(200);
  gpioSetValue(ptt_vocal, low);
}

/* ---------------------------------------------------------------------- */

const struct demod_param demod_dtmf = {
    "DTMF", true, SAMPLE_RATE, 0, dtmf_init, dtmf_demod, NULL
};

/* ---------------------------------------------------------------------- */
