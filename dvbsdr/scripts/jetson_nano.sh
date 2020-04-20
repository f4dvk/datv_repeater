#!/bin/bash

# *********************************************************************
# ************** ENCODE AND MODULATE FOR JETSON NANO ******************
# ************** (c)F5OEO April 2019                 ******************
# *********************************************************************
SERVICE_PROVIDER=Digital_TV
CALL=F5OEO
# ------- MODULATION PARAMETERS --------
# 1/4,1/3,2/5,1/2,3/5,2/3,3/4,4/5,5/6,8/9,9/10 for DVB-S2 QPSK.
# 3/5,2/3,3/4,5/6,8/9,9/10 for DVB-S2 8PSK
source ../detect_platform.sh
source ./include/modulateparam.sh
FREQ=2100
SYMBOLRATE=2000
FECNUM=2
FECDEN=3
#DVBS,DVBS2
MODE=DVBS2
#QPSK,8PSK,16APSK,32APSK
CONSTELLATION=8PSK
GAIN=0.8
#Digital Gain (FPGA Mode Only 1..31)
DGAIN=31
# $LONG_FRAME,$SHORT_FRAME
TYPE_FRAME=$SHORT_FRAME
# $WITH_PILOTS,WITHOUT_PILOTS
PILOTS=$WITHOUT_PILOTS
#$WITH_FPGA,$WITHOUT_FPGA : Be Sure to update special firmware if WITH_FPGA
FPGA_MODE=$WITHOUT_FPGA
# Upsample 1,2 or 4 : 4 delivers the best quality but should not be up to 500KS
UPSAMPLE=1

#CALIBRATION PROCESS : 0 Normal Tx, 1 perform a calibration and save the result (should be done only once) : Carefull, big spike when calibration, do not plug a PA.
CALIBRATE_BEFORE_TX=1

# ------- ENCODER PARAMETERS --------

VIDEO_RESX=640
# 16:9 or 4:3
RATIO=16:9
case "$RATIO" in
"16:9")
let VIDEO_RESY=VIDEO_RESX*9/16 ;;
"4:3")
let VIDEO_RESY=VIDEO_RESX*3/4 ;;
esac

#Uncomment if don't want to use ratio calculation
#VIDEO_RESY=1080


#Only 25 is working well with audio
VIDEO_FPS=25
#Gop Size 1..400 (in frame)
VIDEO_GOP=100
PCR_PTS=200000
#VIDEO INPUT

#Could be  VIDEOSOURCE_PICAMERA, VIDEOSOURCE_HDMI , VIDEOSOURCE_IP , VIDEOSOURCE_SCREEN , MIRE
VIDEOSOURCE=VIDEOSOURCE_IP
VIDEOSOURCE_IP_ADRESS=230.0.0.1
VIDEOSOURCE_IP_PORT=5004

# H264 or H265
CODEC=H264

#AUDIO INPUT

# NO_AUDIO,USB_AUDIO,FILE_WAV,BEEP
AUDIOSOURCE=USB_AUDIO
AUDIO_BITRATE=20000


# Bitrate
source ./include/getbitrate.sh
let TS_AUDIO_BITRATE=AUDIO_BITRATE*14/10
let VIDEOBITRATE=(BITRATE_TS-12000-TS_AUDIO_BITRATE)*650/1000
let VIDEOPEAKBITRATE=VIDEOBITRATE*110/100
#OUTPUT TYPE LIME or IP
OUTPUT=LIME
OUTPUT_NETWORK="230.0.0.10:10000"
# Launch processes
echo
echo VideoBitrate = $VIDEOBITRATE
echo VideoPeakBitrate = $VIDEOPEAKBITRATE

case "$OUTPUT" in
"LIME")
source ./include/nanoencode.sh | source ./include/limerf.sh
;;
"IP")
source ./include/nanoencode.sh
;;
esac
