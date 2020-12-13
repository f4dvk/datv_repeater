#!/bin/bash

PATH_PCONFIG_TX="/home/pi/datv_repeater/dvbtx/scripts/config_rpi.txt"
PATHBIN="/home/pi/datv_repeater/dvbtx/bin"

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

CHANNEL="Relais_de_SOISSONS(02)"
CALL=$(get_config_var call $PATH_PCONFIG_TX)
FREQ=$(get_config_var freq $PATH_PCONFIG_TX)
SYMBOLRATE=$(get_config_var symbolrate $PATH_PCONFIG_TX)
FEC=$(get_config_var fec $PATH_PCONFIG_TX)
MODE=$(get_config_var mode $PATH_PCONFIG_TX)
CONSTELLATION=$(get_config_var constellation $PATH_PCONFIG_TX)
FORMAT=$(get_config_var format $PATH_PCONFIG_TX)
GAIN=$(get_config_var gain $PATH_PCONFIG_TX)
UPSAMPLE=$(get_config_var upsample $PATH_PCONFIG_TX)
CODEC=$(get_config_var codec $PATH_PCONFIG_TX)

PILOT=$(get_config_var pilots $PATH_PCONFIG_TX)
if [ "$PILOT" == "on" ]; then
  PILOTS="-p"
else
  PILOTS=""
fi

FRAME=$(get_config_var frames $PATH_PCONFIG_TX)
if [ "$FRAME" == "short" ]; then
  FRAMES="-v"
else
  FRAMES=""
fi

AUDIO_BITRATE=24000
PCR_PTS=300 # 200 ?
IDR=100

LIME_GAINF=`echo - | awk '{print '$GAIN' / 100}'`

case "$FEC" in
  "1" | "2" | "3" | "5" | "7" )
    let FECNUM=FEC
    let FECDEN=FEC+1
  ;;
  "14" )
    FECNUM="1"
    FECDEN="4"
  ;;
  "13" )
    FECNUM="1"
    FECDEN="3"
  ;;
  "12" )
    FECNUM="1"
    FECDEN="2"
  ;;
  "35" )
    FECNUM="3"
    FECDEN="5"
  ;;
  "23" )
    FECNUM="2"
    FECDEN="3"
  ;;
  "34" )
    FECNUM="3"
    FECDEN="4"
  ;;
  "56" )
    FECNUM="5"
    FECDEN="6"
  ;;
  "89" )
    FECNUM="8"
    FECDEN="9"
  ;;
  "91" )
    FECNUM="9"
    FECDEN="10"
  ;;
esac

# Bitrate
BITRATE_TS=$($PATHBIN"/dvb2iq" -s $SYMBOLRATE -f $FECNUM"/"$FECDEN -d -m $MODE -c $CONSTELLATION $PILOTS $FRAMES )
let TS_AUDIO_BITRATE=AUDIO_BITRATE*15/10
let VIDEOBITRATE=(BITRATE_TS-24000-TS_AUDIO_BITRATE)*725/1000
let VIDEOPEAKBITRATE=VIDEOBITRATE*110/100

# Format

if [ "$VIDEOBITRATE" -gt 190000 ]; then
  VIDEO_RESX=704
  VIDEO_RESY=528
  VIDEO_FPS=25
else
  VIDEO_RESX=384
  VIDEO_RESY=288
  VIDEO_FPS=25
fi

if [ "$VIDEOBITRATE" -lt 100000 ]; then
  VIDEO_RESX=160
  VIDEO_RESY=120
  VIDEO_FPS=20
fi

if [ "$FORMAT" == "16:9" ]; then
  VIDEO_WIDTH=768
  VIDEO_HEIGHT=400
fi

sudo rm videots >/dev/null 2>/dev/null
sudo rm audioin.wav >/dev/null 2>/dev/null
mkfifo videots
mkfifo audioin.wav
v4l2-ctl --device=/dev/video0 --set-fmt-video=width=720,height=480,pixelformat=1 --set-parm=30
$PATHBIN"/limesdr_dvb" -i videots -s "$SYMBOLRATE"000 -f $FECNUM/$FECDEN -r $UPSAMPLE -m $MODE -c $CONSTELLATION $PILOTS $FRAMES \
-t "$FREQ"e6 -g $LIME_GAINF -q 1 &
arecord -f S16_LE -r 48000 -c 2 -B 55000 -D plughw:1 \
| sox -c 1 --buffer 1024 -t wav - audioin.wav rate 46500 &
sudo $PATHBIN"/avc2ts" -b $VIDEOBITRATE -m $BITRATE_TS -d $PCR_PTS -x $VIDEO_RESX -y $VIDEO_RESY \
-f $VIDEO_FPS -i $IDR -o videots -t 2 -e /dev/video0 -p 255 -s $CALL \
-a audioin.wav -z $AUDIO_BITRATE > /dev/null &

exit
