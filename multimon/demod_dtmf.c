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
#include <stdbool.h>
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
enum jetsonGPIONumber ptt_vocal = gpio20;  // Pin 26
enum jetsonGPIONumber band_bit0 = gpio16;  // Pin 19
enum jetsonGPIONumber band_bit1 = gpio17;  // Pin 21
enum jetsonGPIONumber Activation = gpio18; // Pin 23
enum jetsonGPIONumber sarsat = gpio13;     // Pin 22
enum jetsonGPIONumber ptt_437 = gpio19;    // Pin 24
enum jetsonGPIONumber ptt_1255 = gpio78;   // Pin 40
enum jetsonGPIONumber rx_tnt = gpio77;     // Pin 38
int load=0;

/////////////////// DTMF ///////////////////
int Buffer[25];
int DTMF=0;
int OK=false;
int Cod=0;
int Cd;

//////////////////// Commande PHP /////////////
char Cmd_php[10];

//////////////////// USB ///////////////////
char USB[8];

///////////////// COMMANDE /////////////////
int emission,val_RX_DATV,freq1,freq2,freq3,RX_437,RX_145,RX_1255,TX_437,TX_145,TX_1255;
int On=0;
int TX=0;
int RX=0;
char path1[630];
char path2[630];
char path3[630];
char path4[630];
char path5[630];
char path6[630];

char Tx[10];
FILE *dvb;
char user[30];

/////////////////// VIDEO //////////////////
char Resolution[10];
int Fps=25;
int bitrate_v;
char new_bitrate[10];

//////////////////// OSD ///////////////////
char Actif[2];

//////////////// VARIABLES /////////////////
char Mode[10];

///////////////// TEMPO TX /////////////////
unsigned long delai_TX=6;
time_t top;                        // variabe de calcul temps TX
time_t Time;                       // variabe de calcul temps TX

///////////// TEMPO Activation /////////////
unsigned long delai_Actif=10;
time_t top_on;

//////////////// TEMPO DTMF /////////////////
time_t topD;

//////////////// TIME LOG /////////////////
char date[20];

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

int Date(void)
{
  #define BUF_LEN 15
  char buf[BUF_LEN] = {0};
  time_t rawtime = time(NULL);

  if (rawtime == -1) {

    puts("The time() function failed");
    return 1;
  }

  struct tm *ptm = localtime(&rawtime);

  if (ptm == NULL) {

    puts("The localtime() function failed");
    return 1;
  }

  strftime(buf, BUF_LEN, "%d/%m/%Y", ptm);

  snprintf(date, 120, "%s %02d:%02d:%02d", buf, ptm->tm_hour,
         ptm->tm_min, ptm->tm_sec);

  return 0;
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
  gpioUnexport(Activation);
  gpioUnexport(sarsat);
  gpioUnexport(ptt_437);
  gpioUnexport(ptt_1255);
  gpioUnexport(rx_tnt);
}

void exportGPIO(void)
{
  gpioExport(ptt_vocal);
  gpioExport(band_bit0);
  gpioExport(band_bit1);
  gpioExport(Activation);
  gpioExport(sarsat);
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

  snprintf(path5, 630, "/home/%s/406/config.txt", user);
  #define PATH_PCONFIG_406 path5

  snprintf(path6, 630, "/home/%s/datv_repeater/cmd.txt", user);
  #define PATH_TCM_PHP path6

  ///////////////////// GPIO ////////////////////
  gpioSetDirection(ptt_vocal, outputPin);
  gpioSetDirection(band_bit0, outputPin);
  gpioSetDirection(band_bit1, outputPin);
  gpioSetDirection(Activation, outputPin);
  gpioSetDirection(sarsat, outputPin);
  gpioSetDirection(ptt_437, outputPin);
  gpioSetDirection(ptt_1255, outputPin);
  gpioSetDirection(rx_tnt, outputPin);

  gpioSetValue(ptt_vocal, low);
  gpioSetValue(band_bit0, low);
  gpioSetValue(band_bit1, low);
  gpioSetValue(Activation, low);
  gpioSetValue(sarsat, low);
  gpioSetValue(ptt_437, low);
  gpioSetValue(ptt_1255, low);
  gpioSetValue(rx_tnt, low);

}

void initCOM(void)
{
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

  ////////////////////// OSD //////////////////////

  SetConfigParam(PATH_PCONFIG, "texte", "OFF");
  encoder_osd();

}

void initRX(void)
{
  TX_LOW();
  RX_LOW();
  usleep(500);
  SetConfigParam(PATH_PCONFIG_SRC, "source", "MULTI");
  system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
  verbprintf(0,"MULTI-VIDEOS\n");
  RX=4;
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
  int VIDEO_WIDTH=1280;
  int VIDEO_HEIGHT=720;

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
    VIDEO_WIDTH=720;
    VIDEO_HEIGHT=480;
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
    VIDEO_WIDTH=720;
    VIDEO_HEIGHT=480;
    Fps=20;
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
    VIDEO_WIDTH=720;
    VIDEO_HEIGHT=480;
    Fps=20;
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
    Fps=15;
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
          <audioen>1</audioen>\
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
    VIDEO_WIDTH=720;
    VIDEO_HEIGHT=480;
    Fps=25;
  }
  else
  {
    VIDEO_WIDTH=720;
    VIDEO_HEIGHT=480;
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
          <audioen>1</audioen>\
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

int encoder_osd()
{
  char Ip[20];
  char X_pos[6];
  char Y_pos[6];
  char Taille[4];
  char Couleur[10];
  char Texte[20];

  char Url[60];
  char Body[800];

  GetConfigParam(PATH_PCONFIG, "encoderip", Ip);
  GetConfigParam(PATH_PCONFIG, "osd", Actif);
  GetConfigParam(PATH_PCONFIG, "xpos", X_pos);
  GetConfigParam(PATH_PCONFIG, "ypos", Y_pos);
  GetConfigParam(PATH_PCONFIG, "taille", Taille);
  GetConfigParam(PATH_PCONFIG, "couleur", Couleur);
  GetConfigParam(PATH_PCONFIG, "texte", Texte);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  snprintf(Url, 60, "http://%s/action/set?subject=osd&stream=0", Ip);
  curl_easy_setopt(curl, CURLOPT_URL, Url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "authorization: Basic YWRtaW46MTIzNDU=");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  snprintf(Body, 800, " <request><osd>\
        <datetime>\
          <active>0</active>\
          <xpos>10</xpos>\
          <ypos>60</ypos>\
          <font>64</font>\
          <transparent>128</transparent>\
          <color>#FF0000</color>\
          <datefmt>0</datefmt>\
          <hourfmt>0</hourfmt>\
        </datetime>\
        <text>\
          <active>%s</active>\
          <xpos>%s</xpos>\
          <ypos>%s</ypos>\
          <font>%s</font>\
          <transparent>128</transparent>\
          <color>%s</color>\
          <move>0</move>\
          <content>%s</content>\
        </text>\
      </osd></request>", Actif, X_pos, Y_pos, Taille, Couleur, Texte);

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
				if (On != 1)
				{
					gpioSetValue(Activation, high); // Commutation ON
					GetConfigParam(PATH_PCONFIG, "osd", Actif);
					if (strcmp (Actif, "1") == 0)
					{
						SetConfigParam(PATH_PCONFIG, "texte", "ON");
						encoder_osd();
					}
					Date();
					verbprintf(0,"%s Relais Actif\n", date);
					On=1;
				}
				top_on=time(NULL); // Reset tempo Activation
				Date();
				verbprintf(0, "%s DTMF: %c\n", date, dtmf_transl[i]);
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
						DTMF=0;
					}
					else if ((Buffer[1] == 12) && (Buffer[2] == 4) && (Buffer[3] == 13)){  // *40
						Cd = 5;
					}

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

  if ((Cod < 3) && (OK == true) && (((unsigned long)difftime(Time, topD)) >= 4)) {
    verbprintf(0,"Hors temporisation\n");
    Cod=0;
    OK=false;
  }
}

void Efface_Fichier()
{
  FILE* fichier = NULL;

  fichier = fopen(PATH_TCM_PHP, "w+");
  if(fichier != 0)
  {
     fputs("x\n", fichier);
     fclose(fichier);
  }
}

void Lecture_Fichier(char *code)
{
  FILE* fichier = NULL;
  int caractereActuel = 0;
  char caractere[2];
  strcpy(code,"");
  int b=4;

  fichier = fopen(PATH_TCM_PHP, "r");
  if(fichier != 0)
  {
     // Boucle de lecture des caractères un à un
     do
     {
       caractereActuel = fgetc(fichier); // On lit le caractère
       //printf("%c", caractereActuel); // On l'affiche
       snprintf(caractere, 2, "%c", caractereActuel);
       if (strcmp (code, "*40") != 0)
       {
       strcat(code,caractere);
       }
       else if ((caractereActuel != EOF) && ((strchr(caractere,'\n')) ==0) && (b < 6))
       {
         Buffer[b]=atoi(caractere);
         //printf("%s\n", caractere);
         b++;
       }
     } while (caractereActuel != EOF); // On continue tant que fgetc n'a pas retourné EOF (fin de fichier)

     fclose(fichier);
  }
  char *p;
  if((p=strchr(code,'\n')) !=0 ) *p=0; //Remove \n
  //printf("%s",code);
}

void loop(void)
{
  if (load == 0)
  {
    system("killall pqiv >/dev/null 2>/dev/null");
    system("pqiv --fullscreen --hide-info-box /home/$USER/datv_repeater/media/image.gif &");
    initGPIO();
    initCOM();
    initRX();
    encoder_stop_dvbt();
    load=1;
  }

  Time=time(NULL);
  tempo_dtmf();
  tempo_TX();
  tempo_activation();
  Ptt();

  ////// Commande PHP //////
  Lecture_Fichier(Cmd_php);
  if (strcmp (Cmd_php, "x") != 0)
  {
    if ((On != 1) && (strcmp (Cmd_php, "*99") != 0))
    {
      gpioSetValue(Activation, high); // Commutation ON
      GetConfigParam(PATH_PCONFIG, "osd", Actif);
      if (strcmp (Actif, "1") == 0)
      {
        SetConfigParam(PATH_PCONFIG, "texte", "ON");
        encoder_osd();
      }
      Date();
      verbprintf(0,"%s Relais Actif\n", date);
      On=1;
    }
    Commande();
    Efface_Fichier();
    top_on=time(NULL); // Reset tempo Activation
  }

}

void Commande(void)
{
  char COM_USB[40];
  GetConfigParam(PATH_PCONFIG,"tx", Tx);
  usleep(100);

  //////////////////////////////////////// TX ////////////////////////////////////////
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 0) && (Cod>0)) || (strcmp (Cmd_php, "*01") == 0)) // Code *01
    {
      if ((RX_437 == 0) && (RX != 40) && (RX != 43)){
        if (TX != 1){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz SR250 DVB-S\n", date);
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "QPSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "3");
          usleep(100);
          TX_437=1;
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
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 1) && (Cod>0)) || (strcmp (Cmd_php, "*02") == 0)) // Code *02
    {
      if ((RX_437 == 0) && (RX != 40) && (RX != 43)){
        if (TX != 2){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz SR250 DVB-S2 8PSK\n", date);
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBS2");
          SetConfigParam(PATH_PCONFIG_TX, "constellation", "8PSK");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "34");
          usleep(100);
          TX_437=1;
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
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 2) && (Cod>0)) || (strcmp (Cmd_php, "*03") == 0)) // Code *03
    {
      if ((RX_437 == 0) && (RX != 40) && (RX != 43)){
        if (TX != 3){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz DVB-T 250\n", date);
          SetConfigParam(PATH_PCONFIG_TX, "freq", "437");
          SetConfigParam(PATH_PCONFIG_TX, "mode", "DVBT");
          SetConfigParam(PATH_PCONFIG_TX, "qam", "qpsk");
          SetConfigParam(PATH_PCONFIG_TX, "symbolrate", "250");
          SetConfigParam(PATH_PCONFIG_TX, "fec", "3");
          SetConfigParam(PATH_PCONFIG_TX, "guard", "32");
          usleep(100);
          TX_437=1;
          band_select();
          TX=7;
          emission=1;
          encoder_video_dvbt();
          encoder_start_dvbt();
          system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    }
    /*if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 4) && (Cod>0)) || (strcmp (Cmd_php, "*04") == 0)) // Code *04
    {
      if (RX_437 == 0){
        if (TX != 4){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz SR250\n", date);
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
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 5) && (Cod>0)) || (strcmp (Cmd_php, "*05") == 0)) // Code *05
    {
      if (RX_437 == 0){
        if (TX != 5){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz SR500\n", date);
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
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 6) && (Cod>0)) || (strcmp (Cmd_php, "*06") == 0)) // Code *06
    {
      if (RX_1255 == 0){
        if (TX != 6){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 1255MHz SR250\n", date);
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
    if (((Buffer[1] == 12) && (Buffer[2] == 13) && (Buffer[3] == 8) && (Cod>0)) || (strcmp (Cmd_php, "*07") == 0)) // Code *07
    {
      if (RX_437 == 0){
        if (TX != 7){
          TX_LOW();
          usleep(800);
          Date();
          verbprintf(0,"%s TX 437MHz DVB-T 250\n", date);
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
          encoder_video_dvbt();
          encoder_start_dvbt();
          system("/home/$USER/datv_repeater/dvbtx/scripts/tx.sh >/dev/null 2>/dev/null &");
          top=time(NULL);
          topPTT=time(NULL);
          vocal();
        }
      }
      else erreur();
    } */

//////////////////////////////////////// RX ////////////////////////////////////////
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 13) && (Cod>0)) || (strcmp (Cmd_php, "*10") == 0)) // Code *10
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        Date();
        verbprintf(0,"%s RX 145.9MHz SR92\n", date);
        SetConfigParam(PATH_PCONFIG_RX, "freq", "145900");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "92");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_145=1;
        band_select();
        RX=1;
        vocal();
      }
      else erreur();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 0) && (Cod>0)) || (strcmp (Cmd_php, "*11") == 0)) // Code *11
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        Date();
        verbprintf(0,"%s RX 145.9MHz SR125\n", date);
        SetConfigParam(PATH_PCONFIG_RX, "freq", "145900");
        SetConfigParam(PATH_PCONFIG_RX, "symbolrate", "125");
        usleep(100);
        system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/datv_repeater/longmynd/full_rx &'");
        RX_145=1;
        band_select();
        RX=2;
        vocal();
      }
      else erreur();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 1) && (Cod>0)) || (strcmp (Cmd_php, "*12") == 0)) // Code *12
    {
      if (TX_145 == 0){
        RX_LOW();
        usleep(500);
        snprintf(COM_USB, 40, "echo 'OUTA_1'>/dev/%s", USB);
        system(COM_USB);
        snprintf(COM_USB, 40, "echo 'OUTB_1'>/dev/%s", USB);
        system(COM_USB);
        Date();
        verbprintf(0,"%s RX 145.9MHz DVB-T\n", date);
        RX_145=1;
        SetConfigParam(PATH_PCONFIG_SRC, "source", "HDMI");
        system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
        //band_select();
        RX=3;
        //gpioSetValue(ant_437, low);
        gpioSetValue(rx_tnt, high);
        vocal();
      }
      else erreur();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 2) && (Cod>0)) || (strcmp (Cmd_php, "*13") == 0)) // Code *13
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s MULTI-VIDEOS\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "MULTI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=4;
      vocal();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 4) && (Cod>0)) || (strcmp (Cmd_php, "*14") == 0)) // Code *14
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s CAMERA\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "CAMERA");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=5;
      vocal();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 5) && (Cod>0)) || (strcmp (Cmd_php, "*15") == 0)) // Code *15
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s FILM\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "FILM");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=6;
      vocal();
    }
   /* if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 6) && (Cod>0)) || (strcmp (Cmd_php, "*16") == 0)) // Code *16
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s MIRE\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "MIRE");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=7;
      vocal();
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 8) && (Cod>0)) || (strcmp (Cmd_php, "*17") == 0)) // Code *17
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s MULTI-VIDEOS\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "MULTI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=8;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 9) && (Cod>0)) // Code *18
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s RESERVE\n", date);
      snprintf(COM_USB, 40, "echo 'OUTA_2'>/dev/%s", USB);
      system(COM_USB);
      snprintf(COM_USB, 40, "echo 'OUTB_2'>/dev/%s", USB);
      system(COM_USB);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "HDMI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=9;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 0) && (Buffer[3] == 10) && (Cod>0)) // Code *19
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s RESERVE\n", date);
      snprintf(COM_USB, 40, "echo 'OUTA_3'>/dev/%s", USB);
      system(COM_USB);
      snprintf(COM_USB, 40, "echo 'OUTB_3'>/dev/%s", USB);
      system(COM_USB);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "HDMI");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=10;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 13) && (Cod>0)) // Code *20
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s CAMERA\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "CAMERA");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=11;
      vocal();
    }
    if ((Buffer[1] == 12) && (Buffer[2] == 1) && (Buffer[3] == 0) && (Cod>0)) // Code *21
    {
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s FILM\n", date);
      SetConfigParam(PATH_PCONFIG_SRC, "source", "FILM");
      system("/home/$USER/datv_repeater/source/rx_video.sh >/dev/null 2>/dev/null");
      RX=12;
      vocal();
    } */

/////////////////////////////////////// 406 ///////////////////////////////////////
    if (((Buffer[1] == 12) && (Buffer[2] == 4) && (Buffer[3] == 13) && (Cod>0)) || (strcmp (Cmd_php, "*40") == 0)) // Code *40
    {
      char Freq_sarsat[12];
      int D_Khz;
      int Khz;
      TX_LOW();
      RX_LOW();
      usleep(500);

      if (strcmp (Cmd_php, "*40") == 0)
      {
        D_Khz = Buffer[4];
        Khz = Buffer[5];
      }
      else
      {
        if (Buffer[4] < 3)
        {
          D_Khz = Buffer[4] + 1;
        }
        else if ((Buffer[4] > 7) && (Buffer[4] < 11))
        {
          D_Khz = Buffer[4] - 1;
        }
        else if (Buffer[4] == 13)
        {
          D_Khz = 0;
        }
        else
        {
          D_Khz = Buffer[4];
        }

        if (Buffer[5] < 3)
        {
          Khz = Buffer[5] + 1;
        }
        else if ((Buffer[5] > 7) && (Buffer[5] < 11))
        {
          Khz = Buffer[5] - 1;
        }
        else if (Buffer[5] == 13)
        {
          Khz = 0;
        }
        else
        {
          Khz = Buffer[5];
        }
      }

      snprintf(Freq_sarsat, 12, "406.0%d%dM", D_Khz, Khz);
      Date();
      verbprintf(0,"%s Décodeur SARSAT %s\n", date, Freq_sarsat);
      SetConfigParam(PATH_PCONFIG_406, "low", Freq_sarsat);
      SetConfigParam(PATH_PCONFIG_406, "high", Freq_sarsat);
      gpioSetValue(sarsat, high);
      system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/406/scan.sh &'");
      RX=40;
    }
    if (((Buffer[1] == 12) && (Buffer[2] == 4) && (Buffer[3] == 2) && (Cod>0)) || (strcmp (Cmd_php, "*43") == 0)) // Code *43
    {
      TX_LOW();
      RX_LOW();
      usleep(500);
      Date();
      verbprintf(0,"%s Décodeur SARSAT 433.95M\n", date);
      SetConfigParam(PATH_PCONFIG_406, "low", "433.95M");
      SetConfigParam(PATH_PCONFIG_406, "high", "433.95M");
      gpioSetValue(sarsat, high);
      system("sh -c 'gnome-terminal --window --full-screen -- /home/$USER/406/scan.sh &'");
      RX=43;
    }

/////////////////////////////////////// KILL ///////////////////////////////////////

    if (((Buffer[1] == 12) && (Buffer[2] == 10) && (Buffer[3] == 10) && (Cod>0)) || (strcmp (Cmd_php, "*99") == 0)) // Code *99
    {
      Date();
      TX_LOW();
      verbprintf(0,"%s Relais Inactif\n", date);
      kill_ATV();
    }

/////////////////////////////////////// ARRET ///////////////////////////////////////
    if (((Buffer[1] == 3) && (Cod>0)) || (strcmp (Cmd_php, "A") == 0)) // Code A
    {
      Date();
      verbprintf(0,"%s ARRET TX\n", date);
      TX_LOW();
    }
    if (((Buffer[1] == 11) && (Cod>0)) || (strcmp (Cmd_php, "C") == 0)) // Code C
    {
      Date();
      verbprintf(0,"%s CONTROLE GENERAL\n", date);
      vocal();
    }
}

void kill_ATV(void)
{
  gpioSetValue(Activation, low);
  On=0;
  GetConfigParam(PATH_PCONFIG, "osd", Actif);
  if (strcmp (Actif, "1") == 0)
  {
    SetConfigParam(PATH_PCONFIG, "texte", "OFF");
    encoder_osd();
  }
}

void tempo_activation(void)
{
  if ((On == 1) && (((unsigned long)difftime(Time, top_on)) >= delai_Actif*60))
  {
    kill_ATV();
    Date();
    verbprintf(0,"%s Tempo de fin d'activation\n", date);
    if ((RX == 40) || (RX == 43)) initRX();
  }
}

void tempo_TX(void)
{
  if ((emission == 1) && (((unsigned long)difftime(Time, top)) >= delai_TX*60))
  {
    Date();
    verbprintf(0,"%s Tempo de fin TX\n", date);
    if (RX != 6)
    {
      //TX_LOW();
      //RX_LOW();
      //usleep(500);
      initRX();
    }
    else
    {
      TX_LOW();
    }
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
  if ((TX != 0) && (TX_On == 0) && (((unsigned long)difftime(Time, topPTT)) >= delai_PTT))
  {
    if (TX_437 == 1)
      gpioSetValue(ptt_437, high);
    else if (TX_1255 == 1)
      gpioSetValue(ptt_1255, high);
    TX_On=1;
  }
}

void TX_LOW(void)
{
  GetConfigParam(PATH_PCONFIG,"tx", Tx);
  GetConfigParam(PATH_PCONFIG_TX,"mode", Mode);

  if (strcmp (Tx, "pluto") == 0)
  {
    if (strcmp (Mode, "DVBT") == 0)
    {
      encoder_stop_dvbt();
      system("sudo killall dvb_t_stack >/dev/null 2>/dev/null");
    }
    else
    {
      encoder_stop();
      system("/home/$USER/datv_repeater/dvbtx/scripts/pluto_stop.sh >/dev/null 2>/dev/null");
    }
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
  //gpioSetValue(ptt_145, low);
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
  system("sudo killall perl >/dev/null 2>/dev/null");
  system("sudo killall rtl_sdr >/dev/null 2>/dev/null");
  system("sudo killall sox >/dev/null 2>/dev/null");
  system("sudo killall dec406_V6 >/dev/null 2>/dev/null");
  gpioSetValue(sarsat, low);
  gpioSetValue(rx_tnt, low);
  RX_145=0;
  RX_437=0;
  RX_1255=0;
  RX=0;
}

void vocal(void) {

  usleep(600000);

  gpioSetValue(ptt_vocal, high);

  usleep(600000);

  if (TX == 1)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B S, SR 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/01.wav");
  }

  if (TX == 2)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B S2, SR 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/02.wav");
  }

  if (TX == 3)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B T, 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/03.wav");
  }

  /* if (TX == 4)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B S, SR 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/04.wav");
  }

  if (TX == 5)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B S, SR 500'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/05.wav");
  }

  if (TX == 6)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 1255 méga-hertz, D V B S, SR 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/06.wav");
  }

  if (TX == 7)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'TX 437 méga-hertz, D V B T 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/07.wav");
  } */

  if (RX == 1)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'RX 145.9 méga-hertz, D V B S, SR 92'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/10.wav");
  }

  if (RX == 2)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'RX 145.9 méga-hertz, D V B S, SR 125'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/11.wav");
  }

  if (RX == 3)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'RX 145.9 méga-hertz, D V B T, 250'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/12.wav");
  }

  if (RX == 4)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Multi-vidéos'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/13.wav");
  }

  if (RX == 5)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Caméra'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/14.wav");
  }

  if (RX == 6)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Film'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/15.wav");
  }

  /*if (RX == 7)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Mire'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/16.wav");
  }

  if (RX == 8)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Multi-vidéos'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/17.wav");
  }

  if (RX == 9)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Réserve'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/18.wav");
  }

  if (RX == 10)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Réserve'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/19.wav");
  }

  if (RX == 11)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Caméra'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/20.wav");
  }

  if (RX == 12)
  {
    system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Film'");
    system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
    //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/21.wav");
  } */

  usleep(200000);

  gpioSetValue(ptt_vocal, low);
}

void erreur(void) {
  usleep(600000);
  gpioSetValue(ptt_vocal, high);
  usleep(600000);
  system("pico2wave -l fr-FR -w /home/$USER/datv_repeater/son/vocal.wav 'Erreur, fréquences RX et TX identiques'");
  system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/vocal.wav");
  //system("aplay -D plughw:2,0 --quiet /home/$USER/datv_repeater/son/erreur.wav");
  usleep(200000);
  gpioSetValue(ptt_vocal, low);
}

/* ---------------------------------------------------------------------- */

const struct demod_param demod_dtmf = {
    "DTMF", true, SAMPLE_RATE, 0, dtmf_init, dtmf_demod, NULL
};

/* ---------------------------------------------------------------------- */
