#!/bin/bash

# *********************************************************************
# ************** ENCODE AND MODULATE FOR JETSON NANO ******************
# ************** (c)F5OEO April 2019                 ******************
# *********************************************************************
PROVIDER_NAME=Digital_TV
CALL=$7
SOURCE=$8
# ------- MODULATION PARAMETERS --------
# 1/4,1/3,2/5,1/2,3/5,2/3,3/4,4/5,5/6,8/9,9/10 for DVB-S2 QPSK.
# 3/5,2/3,3/4,5/6,8/9,9/10 for DVB-S2 8PSK
source /home/$USER/dvbsdr/detect_platform.sh
source /home/$USER/dvbsdr/scripts/include/modulateparam.sh
FREQ=$2
SYMBOLRATE=$3
FEC=$4
#DVBS,DVBS2
MODE=$5
#MODE=DVBS2
#QPSK,8PSK,16APSK,32APSK
CONSTELLATION=$6
#CONSTELLATION=QPSK
GAIN=$1
#Digital Gain (FPGA Mode Only 1..31)
DGAIN=31
# $LONG_FRAME,$SHORT_FRAME
var=$(echo $@ | grep '\-v' | wc -l)
if [ "$var" == 0 ]; then
  TYPE_FRAME=""
else
  TYPE_FRAME="-v"
fi
# $WITH_PILOTS,WITHOUT_PILOTS
var=$(echo $@ | grep '\-p' | wc -l)
if [ "$var" == 0 ]; then
  PILOTS=""
else
  PILOTS="-p"
fi
#$WITH_FPGA,$WITHOUT_FPGA : Be Sure to update special firmware if WITH_FPGA
FPGA_MODE=$WITHOUT_FPGA
# Upsample 1,2 or 4 : 4 delivers the best quality but should not be up to 500KS
UPSAMPLE=4
if [ "$SYMBOLRATE" -gt 500 ]; then
  UPSAMPLE=2
fi

#CALIBRATION PROCESS : 0 Normal Tx, 1 perform a calibration and save the result (should be done only once) : Carefull, big spike when calibration, do not plug a PA.
CALIBRATE_BEFORE_TX=1

# ------- ENCODER PARAMETERS --------

#VIDEO_RESX=352
# 16:9 or 4:3
#RATIO=4:3
#case "$RATIO" in
#"16:9")
#let VIDEO_RESY=VIDEO_RESX*9/16 ;;
#"4:3")
#let VIDEO_RESY=VIDEO_RESX*3/4 ;;
#esac

#Uncomment if don't want to use ratio calculation
#VIDEO_RESY=1080


#Only 25 is working well with audio
#VIDEO_FPS=25
#Gop Size 1..400 (in frame)
VIDEO_GOP=100
PCR_PTS=200000
#VIDEO INPUT

#Could be  VIDEOSOURCE_PICAMERA, VIDEOSOURCE_HDMI , VIDEOSOURCE_IP , VIDEOSOURCE_SCREEN , MIRE
if [ "$SOURCE" == "picam" ]; then
  VIDEOSOURCE=VIDEOSOURCE_PICAMERA
elif [ "$SOURCE" == "HDMI" ]; then
  VIDEOSOURCE=VIDEOSOURCE_HDMI
elif [ "$SOURCE" == "IP" ]; then
  VIDEOSOURCE=VIDEOSOURCE_IP
elif [ "$SOURCE" == "screen" ]; then
  VIDEOSOURCE=VIDEOSOURCE_SCREEN
elif [ "$SOURCE" == "mire" ]; then
  VIDEOSOURCE=MIRE
else
  VIDEOSOURCE=VIDEOSOURCE_SCREEN
fi

VIDEOSOURCE_IP_ADRESS=${10}
VIDEOSOURCE_IP_PORT=${11}

# H264 or H265
CODEC=$9

#AUDIO INPUT

# NO_AUDIO,USB_AUDIO,FILE_WAV,BEEP
AUDIOSOURCE=USB_AUDIO
AUDIO_BITRATE=20000


# Bitrate
source /home/$USER/dvbsdr/scripts/include/getbitrate.sh
#let TS_AUDIO_BITRATE=AUDIO_BITRATE*14/10
#let VIDEOBITRATE=(BITRATE_TS-12000-TS_AUDIO_BITRATE)*650/1000
let TS_AUDIO_BITRATE=AUDIO_BITRATE*15/10
let VIDEOBITRATE=(BITRATE_TS-24000-TS_AUDIO_BITRATE)*725/1000
let VIDEOPEAKBITRATE=VIDEOBITRATE*110/100

# Format

if [ "$VIDEOBITRATE" -gt 190000 ]; then
  VIDEO_RESX=704
  VIDEO_RESY=576
  VIDEO_FPS=25
else
  VIDEO_RESX=352
  VIDEO_RESY=288
  VIDEO_FPS=25
fi

if [ "$VIDEOBITRATE" -lt 100000 ]; then
  VIDEO_RESX=160
  VIDEO_RESY=120
fi

#OUTPUT TYPE LIME or IP
OUTPUT=LIME
OUTPUT_NETWORK="230.0.0.10:10000"
# Launch processes
echo
echo VideoBitrate = $VIDEOBITRATE
echo VideoPeakBitrate = $VIDEOPEAKBITRATE

case "$OUTPUT" in
"LIME")
source /home/$USER/dvbsdr/scripts/include/nanoencode.sh | source /home/$USER/dvbsdr/scripts/include/limerf.sh
;;
"IP")
source /home/$USER/dvbsdr/scripts/include/nanoencode.sh
;;
esac
