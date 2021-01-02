// sources: F5OEO, BATC, myorangedragon

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

char Freq[255];
char Sr[5];
char Gain[3];
char Scan[5];
char In[3];
char IP[10];
char Port[5];
char Player[2];
char Quiet[2];

unsigned long delai_TXT=2;          // delai entre deux écritures en secondes
unsigned long delai_reset=5;          // delai entre perte rx et reset en secondes
unsigned long delai_term=0.5;          // delai rafraichissement infos du terminal en secondes
time_t top;                        // variable de calcul temps reset mpv
time_t top2;                        // variable de calcul temps ecriture infos.txt
time_t top3;                        // variable de calcul temps affichage infos dans le terminal
time_t Time;                       // variable de calcul temps

void GetConfigParam(char *PathConfigFile, char *Param, char *Value)
{
  char * line = NULL;
  size_t len = 0;
  int read;
  char ParamWithEquals[255];
  strcpy(ParamWithEquals, Param);
  strcat(ParamWithEquals, "=");

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

void chopN(char *str, size_t n)
{
  size_t len = strlen(str);
  if ((n > len) || (n == 0))
  {
    return;
  }
  else
  {
    memmove(str, str+n, len - n + 1);
  }
}

int main() {
    const char* user = getenv("USER");
    char path[511];

    snprintf(path, 511, "/home/%s/datv_repeater/longmynd/config.txt", user);
    #define PATH_PCONFIG path

    int num;
    int ret;

    int fd_status_fifo;

    char status_message_char[14];
    char stat_string[63];
    char MERtext[63];
    char STATEtext[63];
    char FREQtext[63];
    char SRtext[63];
    char ServiceProvidertext[63] = " ";
    char Servicetext[63] = " ";
    char MODCODtext[63];
    char FECtext[63] = " ";
    char Modulationtext[63] = " ";
    char VidEncodingtext[63] = " ";
    char AudEncodingtext[63] = " ";
    char Encodingtext[63] = " ";
    int EncodingCode = 0;
    float MERThreshold = 0;
    int MODCOD;
    float FREQ;
    int STATE;
    int SR;
    float MER;
    int LCK=0;
    char Command[530];

    GetConfigParam(PATH_PCONFIG,"freq", Freq);
    GetConfigParam(PATH_PCONFIG,"symbolrate", Sr);
    GetConfigParam(PATH_PCONFIG,"input", In);
    GetConfigParam(PATH_PCONFIG,"gain", Gain);
    GetConfigParam(PATH_PCONFIG,"scan", Scan);
    GetConfigParam(PATH_PCONFIG,"ip", IP);
    GetConfigParam(PATH_PCONFIG,"port", Port);
    GetConfigParam(PATH_PCONFIG,"player", Player);
    GetConfigParam(PATH_PCONFIG,"quiet", Quiet);

    if(strcmp(In, "a") == 0)
    {
      strcpy(In, "");
    }
    else
    {
      strcpy(In, "-w");
    }

    /* Open status FIFO for read only  */
    system("sudo rm /home/$USER/datv_repeater/longmynd/longmynd_main_status >/dev/null 2>/dev/null");
    system("sudo rm /home/$USER/datv_repeater/longmynd/longmynd_main_ts >/dev/null 2>/dev/null");
    ret=mkfifo("longmynd_main_status", 0666);
    ret=mkfifo("longmynd_main_ts", 0666);

    snprintf(Command, 530, "sudo /home/$USER/datv_repeater/longmynd/longmynd -i %s %s %s -g %s -S %s -r -1 %s %s >/dev/null 2>/dev/null &", IP, Port, In, Gain, Scan, Freq, Sr);
    system(Command);

    fd_status_fifo = open("longmynd_main_status", O_RDONLY);
    if (fd_status_fifo<0) printf("Failed to open status fifo\n");

    printf("Listening\n");

    if (strcmp(Player, "1") == 0)
    {
      //snprintf(Command, 530, "mpv --fs --no-cache --no-terminal udp://%s:%s &", IP, Port);
      snprintf(Command, 530, "cvlc -f --codec ffmpeg --video-title-timeout=1 --sub-filter marq --marq-y 5 --marq-size 30 --marq-position={0,4} --marq-file /home/%s/infos.txt udp://@%s:%s >/dev/null 2>/dev/null &", user, IP, Port,);
      system(Command);
    }

    while (1) {
      Time=time(NULL);
      num=read(fd_status_fifo, status_message_char, 1);
      if (num >= 0 )
      {
        status_message_char[num]='\0';

        if (strcmp(status_message_char, "$") == 0)
        {
          if ((stat_string[0] == '1') && (stat_string[1] == ','))  // Decoder State
          {
            strcpy(STATEtext, stat_string);
            chopN(STATEtext, 2);
            STATE = atoi(STATEtext);
            switch(STATE)
            {
              case 0:
              strcpy(STATEtext, "Initialising");
              break;
              case 1:
              strcpy(STATEtext, "Searching");
              if (LCK == 1)
              {
                top=time(NULL);
                LCK=2;
              }
              if ((((unsigned long)difftime(Time, top)) > delai_reset) && (LCK == 2) && (strcmp(Player, "1") == 0))
              {
                //system("sudo killall mpv >/dev/null 2>/dev/null");
                system("sudo killall vlc >/dev/null 2>/dev/null");
                usleep(300);
                //snprintf(Command, 530, "mpv --fs --no-cache --no-terminal udp://%s:%s &", IP, Port);
                snprintf(Command, 530, "cvlc -f --codec ffmpeg --video-title-timeout=1 --sub-filter marq --marq-y 5 --marq-size 30 --marq-position={0,4} --marq-file /home/%s/infos.txt udp://@%s:%s >/dev/null 2>/dev/null &", user, IP, Port,);
                system(Command);
                LCK=0;
              }
              break;
              case 2:
              strcpy(STATEtext, "Found Headers");
              break;
              case 3:
              strcpy(STATEtext, "DVB-S");
              LCK=1;
              break;
              case 4:
              strcpy(STATEtext, "DVB-S2");
              LCK=1;
              break;
              default:
              snprintf(STATEtext, 10, "%d", STATE);
            }
          }

          if ((stat_string[0] == '6') && (stat_string[1] == ','))  // Frequency
          {
            strcpy(FREQtext, stat_string);
            chopN(FREQtext, 2);
            FREQ = atof(FREQtext);
            FREQ = FREQ / 1000;
            snprintf(FREQtext, 15, "%.3f MHz", FREQ);
          }

          if ((stat_string[0] == '9') && (stat_string[1] == ','))  // SR in S
          {
            strcpy(SRtext, stat_string);
            chopN(SRtext, 2);
            SR = atoi(SRtext) / 1000;
            snprintf(SRtext, 15, "%d kS", SR);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '3'))  // Service Provider
          {
            strcpy(ServiceProvidertext, stat_string);
            int length=strlen(ServiceProvidertext);
            if (ServiceProvidertext[length-1] == '\n') ServiceProvidertext[length-1] = '\0';
            chopN(ServiceProvidertext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '4'))  // Service
          {
            strcpy(Servicetext, stat_string);
            int length=strlen(Servicetext);
            if (Servicetext[length-1] == '\n') Servicetext[length-1] = '\0';
            chopN(Servicetext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '8'))  // MODCOD
          {
            strcpy(MODCODtext, stat_string);
            chopN(MODCODtext, 3);
            MODCOD = atoi(MODCODtext);
            //STATE = 4;
            if (STATE == 3)                                        // DVB-S
            {
              switch(MODCOD)
              {
                case 0:
                  strcpy(FECtext, "FEC 1/2");
                  MERThreshold = 01.7; //
                break;
                case 1:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 03.3; //
                break;
                case 2:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 04.2; //
                break;
                case 3:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 05.1; //
                break;
                case 4:
                  strcpy(FECtext, "FEC 6/7");
                  MERThreshold = 05.5; //
                break;
                case 5:
                  strcpy(FECtext, "FEC 7/8");
                  MERThreshold = 05.8; //
                break;
                default:
                  strcpy(FECtext, "FEC - ");
                  MERThreshold = 00.0; //
                  strcat(FECtext, MODCODtext);
                break;
              }
              strcpy(Modulationtext, "QPSK");
            }
            if (STATE == 4)                                        // DVB-S2
            {
              switch(MODCOD)
              {
                case 1:
                  strcpy(FECtext, "FEC 1/4");
                  MERThreshold = -2.3; //
                break;
                case 2:
                  strcpy(FECtext, "FEC 1/3");
                  MERThreshold = -1.2; //
                break;
                case 3:
                  strcpy(FECtext, "FEC 2/5");
                  MERThreshold = -0.3; //
                break;
                case 4:
                  strcpy(FECtext, "FEC 1/2");
                  MERThreshold = 01.0; //
                break;
                case 5:
                  strcpy(FECtext, "FEC 3/5");
                  MERThreshold = 02.3; //
                break;
                case 6:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 03.1; //
                break;
                case 7:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 04.1; //
                break;
                case 8:
                  strcpy(FECtext, "FEC 4/5");
                  MERThreshold = 04.7; //
                break;
                case 9:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 05.2; //
                break;
                case 10:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 06.2; //
                break;
                case 11:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 06.5; //
                break;
                case 12:
                  strcpy(FECtext, "FEC 3/5");
                  MERThreshold = 05.5; //
                break;
                case 13:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 06.6; //
                break;
                case 14:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 07.9; //
                break;
                case 15:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 09.4; //
                break;
                case 16:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 00.7; //
                break;
                case 17:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 11.0; //
                break;
                case 18:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 09.0; //
                break;
                case 19:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 10.2; //
                break;
                case 20:
                  strcpy(FECtext, "FEC 4/5");
                  MERThreshold = 11.0; //
                break;
                case 21:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 11.6; //
                break;
                case 22:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 12.9; //
                break;
                case 23:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 13.2; //
                break;
                case 24:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 12.8; //
                break;
                case 25:
                  strcpy(FECtext, "FEC 4/5");
                  MERThreshold = 13.7; //
                break;
                case 26:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 14.3; //
                break;
                case 27:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 15.7; //
                break;
                case 28:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 16.1; //
                break;
                default:
                  strcpy(FECtext, "FEC -");
                  MERThreshold = 00.0; //
                break;
              }
              if ((MODCOD >= 1) && (MODCOD <= 11 ))
              {
                strcpy(Modulationtext, "QPSK");
              }
              if ((MODCOD >= 12) && (MODCOD <= 17 ))
              {
                strcpy(Modulationtext, "8PSK");
              }
              if ((MODCOD >= 18) && (MODCOD <= 23 ))
              {
                strcpy(Modulationtext, "16APSK");
              }
              if ((MODCOD >= 24) && (MODCOD <= 28 ))
              {
                strcpy(Modulationtext, "32APSK");
              }
            }
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '7'))  // Video and audio encoding
          {
            strcpy(Encodingtext, stat_string);
            chopN(Encodingtext, 3);
            EncodingCode = atoi(Encodingtext);
            switch(EncodingCode)
            {
              case 2:
                strcpy(VidEncodingtext, "MPEG-2");
              break;
              case 3:
                strcpy(AudEncodingtext, " MPA");
              break;
              case 4:
                strcpy(AudEncodingtext, " MPA");
              break;
              case 15:
                strcpy(AudEncodingtext, " AAC");
              break;
              case 16:
                strcpy(VidEncodingtext, "H263");
              break;
              case 27:
                strcpy(VidEncodingtext, "H264");
              break;
              case 32:
                strcpy(AudEncodingtext, " MPA");
              break;
              case 36:
                strcpy(VidEncodingtext, "H265");
              break;
              default:
                printf("New Encoding Code = %d\n", EncodingCode);
              break;
            }
            strcpy(Encodingtext, VidEncodingtext);
            strcat(Encodingtext, AudEncodingtext);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '2'))  // MER
          {
            strcpy(MERtext, stat_string);
            chopN(MERtext, 3);
            MER = atof(MERtext)/10;
            if (MER > 51)  // Trap spurious MER readings
            {
              MER = 0;
            }
            snprintf(MERtext, 24, "MER %02.1f (%.1f needed)", MER, MERThreshold);

            if ((strcmp(Quiet, "0") == 0) && (((unsigned long)difftime(Time, top3)) > delai_term))
            {
              top3=time(NULL);
              system("clear && printf '\e[3J'");
              printf("##################\n\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n\n##################\n", STATEtext, FREQtext, SRtext, Modulationtext, FECtext, ServiceProvidertext, Servicetext, Encodingtext, MERtext);
            }

            if (((unsigned long)difftime(Time, top2)) > delai_TXT)
            {
              snprintf(Command, 640, "echo '%s  Freq: %s  SR: %s  %s  %s  Provider: %s  Call: %s  %s  %s' > /home/%s/infos.txt.tmp", STATEtext, FREQtext, SRtext, Modulationtext, FECtext, ServiceProvidertext, Servicetext, Encodingtext, MERtext, user);
              system(Command);
              snprintf(Command, 530, "mv /home/%s/infos.txt.tmp /home/%s/infos.txt", user, user);
              system(Command);
              top2=Time;
            }

          }
          stat_string[0] = '\0';
        }
        else
        {
          strcat(stat_string, status_message_char);
        }
      }
    }
    close(fd_status_fifo);
    return 0;
}
