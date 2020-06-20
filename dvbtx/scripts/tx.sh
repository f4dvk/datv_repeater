#!/bin/bash

# Le pilotage du TX se fait en SSH

PATH_PCONFIG_TX="/home/$USER/jetson_datv_repeater/dvbtx/scripts/config.txt"
CMDFILE="/home/$USER/tmp/jetson_command.txt"

get_config_var() {
lua5.3 - "$1" "$2" <<EOF
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

CHANNEL=Digital_TV
CALL=$(get_config_var call $PATH_PCONFIG_TX)
FREQ=$(get_config_var freq $PATH_PCONFIG_TX)
SYMBOLRATE=$(get_config_var symbolrate $PATH_PCONFIG_TX)
FEC=$(get_config_var fec $PATH_PCONFIG_TX)
MODE=$(get_config_var mode $PATH_PCONFIG_TX)
CONSTELLATION=$(get_config_var constellation $PATH_PCONFIG_TX)
GAIN=$(get_config_var gain $PATH_PCONFIG_TX)
UPSAMPLE=$(get_config_var upsample $PATH_PCONFIG_TX)
CODEC=$(get_config_var codec $PATH_PCONFIG_TX)
JETSONIP=$(get_config_var jetsonip $PATH_PCONFIG_TX)
JETSONUSER=$(get_config_var jetsonuser $PATH_PCONFIG_TX)
JETSONPW=$(get_config_var jetsonpw $PATH_PCONFIG_TX)

AUDIO_BITRATE=20000
VIDEO_GOP=100
PCR_PTS=200000

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
BITRATE_TS=$(/home/$USER/jetson_datv_repeater/dvbtx/bin/dvb2iq -s $SYMBOLERATE -f $FECNUM"/"$FECDEN -m $MODE -c $CONSTELLATION $FRAME $PILOTS $TYPE_FRAME -d)
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
fi

case "$CODEC" in
  "H264" )
    # Write the assembled Jetson command to a temp file
    /bin/cat <<EOM >$CMDFILE
    (sshpass -p $JETSONPW ssh -o StrictHostKeyChecking=no $JETSONUSER@$JETSONIP 'bash -s' <<'ENDSSH'
    gst-launch-1.0 -q v4l2src device=/dev/video1 \
      '!' 'video/x-raw,width=640, height=480, format=(string)YUY2' \
      '!' nvvidconv flip-method=0 \
      '!' 'video/x-raw(memory:NVMM), format=(string)I420, framerate=(fraction)30/1' \
      '!' nvvidconv \
      '!' 'video/x-raw(memory:NVMM), width=(int)$VIDEO_RESX, height=(int)$VIDEO_RESY, format=(string)I420' \
      '!' omxh264enc vbv-size=15 control-rate=2 bitrate=$VIDEOBITRATE peak-bitrate=$VIDEOPEAKBITRATE \
      insert-sps-pps=1 insert-vui=1 cabac-entropy-coding=1 preset-level=3 profile=8 iframeinterval=$VIDEO_GOP \
      '!' 'video/x-h264, level=(string)4.1, stream-format=(string)byte-stream' '!' queue '!' mux. alsasrc device=plughw:2 \
      '!' 'audio/x-raw, format=S16LE, layout=interleaved, rate=48000, channels=1' '!' voaacenc bitrate=$AUDIO_BITRATE \
      '!' queue '!' mux. mpegtsmux alignment=7 name=mux '!' fdsink \
    | ffmpeg -i - -ss 8 \
      -c:v copy -max_delay $PCR_PTS -muxrate $BITRATE_TS \
      -c:a copy -f mpegts \
      -metadata service_provider="$CHANNEL" -metadata service_name="$CALL" \
      -streamid 0:256 - \
    | /home/$JETSONUSER/jetson_datv_repeater/dvbtx/bin/limesdr_dvb -s "$SYMBOLRATE"000 -f $FECNUM/$FECDEN -r $UPSAMPLE -m $MODE -c $CONSTELLATION $PILOTS $FRAMES \
      -t "$FREQ"e6 -g $LIME_GAINF -q 1
ENDSSH
      ) &
EOM
  ;;
  "H265" )
    # Write the assembled Jetson command to a temp file
    /bin/cat <<EOM >$CMDFILE
    (sshpass -p $JETSONPW ssh -o StrictHostKeyChecking=no $JETSONUSER@$JETSONIP 'bash -s' <<'ENDSSH'
    gst-launch-1.0 -q v4l2src device=/dev/video1 \
      '!' 'video/x-raw,width=640, height=480, format=(string)YUY2' \
      '!' nvvidconv flip-method=0 \
      '!' 'video/x-raw(memory:NVMM), format=(string)I420, framerate=(fraction)30/1' \
      '!' nvvidconv \
      '!' 'video/x-raw(memory:NVMM), width=(int)$VIDEO_RESX, height=(int)$VIDEO_RESY, format=(string)I420' \
      '!' omxh265enc control-rate=2 bitrate=$VIDEOBITRATE peak-bitrate=$VIDEOPEAKBITRATE preset-level=3 iframeinterval=$VIDEO_GOP \
      '!' 'video/x-h265,stream-format=(string)byte-stream' '!' queue '!' mux. alsasrc device=plughw:2 \
      '!' 'audio/x-raw, format=S16LE, layout=interleaved, rate=48000, channels=1' '!' voaacenc bitrate=$AUDIO_BITRATE \
      '!' queue '!' mux. mpegtsmux alignment=7 name=mux '!' fdsink \
    | ffmpeg -i - -ss 8 \
      -c:v copy -max_delay $PCR_PTS -muxrate $BITRATE_TS \
      -c:a copy -f mpegts \
      -metadata service_provider="$CHANNEL" -metadata service_name="$CALL" \
      -streamid 0:256 - \
    | /home/$JETSONUSER/jetson_datv_repeater/dvbtx/bin/limesdr_dvb -s "$SYMBOLRATE"000 -f $FECNUM/$FECDEN -r $UPSAMPLE -m $MODE -c $CONSTELLATION $PILOTS $FRAMES \
      -t "$FREQ"e6 -g $LIME_GAINF -q 1
ENDSSH
      ) &
EOM
  ;;
esac

source "$CMDFILE"

exit
