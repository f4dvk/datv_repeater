#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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
    int num;
    int ret;

    int fd_status_fifo;

    char status_message_char[14];
    char stat_string[63];
    char MERtext[63];
    char STATEtext[63];
    char FREQtext[63];
    char SRtext[63];
    char ServiceProvidertext[255] = " ";
    char Servicetext[255] = " ";
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

    /* Open status FIFO for read only  */
    ret=mkfifo("longmynd_main_status", 0666);
    fd_status_fifo = open("longmynd_main_status", O_RDONLY);
    if (fd_status_fifo<0) printf("Failed to open status fifo\n");

    printf("Listening\n");

    while (1) {
      num=read(fd_status_fifo, status_message_char, 1);
      if (num >= 0 )
      {
        status_message_char[num]='\0';
        //if (num>0) printf("%s",status_message_char);

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
              break;
              case 2:
              strcpy(STATEtext, "Found Headers");
              break;
              case 3:
              strcpy(STATEtext, "DVB-S Lock");
              break;
              case 4:
              strcpy(STATEtext, "DVB-S2 Lock");
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
            chopN(ServiceProvidertext, 3);
          }

          if ((stat_string[0] == '1') && (stat_string[1] == '4'))  // Service
          {
            strcpy(Servicetext, stat_string);
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
                  MERThreshold = 0; //
                break;
                case 1:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 0; //
                break;
                case 2:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 0; //
                break;
                case 3:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 0; //
                break;
                case 4:
                  strcpy(FECtext, "FEC 6/7");
                  MERThreshold = 0; //
                break;
                case 5:
                  strcpy(FECtext, "FEC 7/8");
                  MERThreshold = 0; //
                break;
                default:
                  strcpy(FECtext, "FEC - ");
                  MERThreshold = 0; //
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
                  MERThreshold = 1.0; //
                break;
                case 5:
                  strcpy(FECtext, "FEC 3/5");
                  MERThreshold = 2.3; //
                break;
                case 6:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 3.1; //
                break;
                case 7:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 4.1; //
                break;
                case 8:
                  strcpy(FECtext, "FEC 4/5");
                  MERThreshold = 4.7; //
                break;
                case 9:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 5.2; //
                break;
                case 10:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 6.2; //
                break;
                case 11:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 6.5; //
                break;
                case 12:
                  strcpy(FECtext, "FEC 3/5");
                  MERThreshold = 5.5; //
                break;
                case 13:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 6.6; //
                break;
                case 14:
                  strcpy(FECtext, "FEC 3/4");
                  MERThreshold = 7.9; //
                break;
                case 15:
                  strcpy(FECtext, "FEC 5/6");
                  MERThreshold = 9.4; //
                break;
                case 16:
                  strcpy(FECtext, "FEC 8/9");
                  MERThreshold = 10.7; //
                break;
                case 17:
                  strcpy(FECtext, "FEC 9/10");
                  MERThreshold = 11.0; //
                break;
                case 18:
                  strcpy(FECtext, "FEC 2/3");
                  MERThreshold = 9.0; //
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
                  MERThreshold = 0; //
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
            snprintf(MERtext, 24, "MER %.1f (%.1f needed)", MER, MERThreshold);

            printf("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n##################\n", STATEtext, FREQtext, SRtext, Modulationtext, FECtext, ServiceProvidertext, Servicetext, Encodingtext, MERtext);
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
