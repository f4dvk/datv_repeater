#!/bin/bash

PATH_PCONFIG_SRC="/home/$USER/datv_repeater/source/config.txt"
PATH_PCONFIG_USR="/home/$USER/datv_repeater/config.txt"

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

SOURCE=$(get_config_var source $PATH_PCONFIG_SRC)
CALL=$(get_config_var call $PATH_PCONFIG_USR)
TYPE_CAM=$(get_config_var typecamip $PATH_PCONFIG_USR)
IP_CAM=$(get_config_var camip $PATH_PCONFIG_USR)
USR_CAM=$(get_config_var camusr $PATH_PCONFIG_USR)
PW_CAM=$(get_config_var campw $PATH_PCONFIG_USR)

case "$SOURCE" in
  "MIRE" )
gst-launch-1.0 -q videotestsrc ! video/x-raw,width=1280,height=720 ! nvvidconv ! 'video/x-raw(memory:NVMM), width=(int)1280, height=(int)720, framerate=(fraction)30/1' ! queue ! nvoverlaysink -e >/dev/null 2>/dev/null &
  ;;
  "MULTI" )
gst-launch-1.0 -vvv -q glvideomixer name=mix sink_0::xpos=0 sink_0::ypos=0 sink_0::width=640 sink_0::height=360 sink_1::xpos=640 sink_1::ypos=0 sink_1::width=640 \
sink_1::height=360 sink_2::xpos=0 sink_2::ypos=360 sink_2::width=640 sink_2::height=360 sink_3::xpos=640 sink_3::ypos=360 sink_3::width=640 sink_3::height=360 \
sink_0::alpha=1.0 sink_1::alpha=1.0 sink_2::alpha=1.0 sink_3::alpha=1.0 ! queue2 ! nvvidconv ! 'video/x-raw(memory:NVMM), width=1280, height=720, framerate=30/1' ! nvoverlaysink sync=false \
videotestsrc ! textoverlay text="F5ZBC   http://f5zbc.fr" valignment=top halignment=left font-desc="Sans, 66" ! video/x-raw, width=640, heigh=360, framerate=30/1 ! alpha alpha=1.0 ! queue2 ! mix.sink_0 \
rtspsrc location=rtsp://$USR_CAM:$PW_CAM@$IP_CAM:554//h264Preview_01_main ! queue ! application/x-rtp, media=video, framerate=30, encoding-name=H264 ! rtph264depay ! h264parse ! decodebin ! nvvidconv ! 'video/x-raw(memory:NVMM), width=640, height=360, format=(string)I420' ! nvvidconv ! queue2 ! mix.sink_1 \
multifilesrc location=/home/$USER/datv_repeater/media/film.mpg loop=true ! decodebin ! videoscale ! videorate ! nvvidconv ! \
'video/x-raw(memory:NVMM), width=640, height=360, framerate=30/1, format=(string)I420' ! nvvidconv ! alpha alpha=1.0 ! queue2 ! mix.sink_2 \
videotestsrc ! textoverlay text="JN19PG" valignment=center halignment=center font-desc="Sans, 70" ! video/x-raw, width=640, height=360, framerate=30/1 ! alpha alpha=1.0 ! queue2 ! mix.sink_3 >/dev/null 2>/dev/null & # video en format mpg, sinon pas de fonctionnement de la boucle.
  ;;
  "FILM" )
#mpv --fs --no-cache --no-terminal --loop-playlist=inf /home/$USER/datv_repeater/film.mp4 >/dev/null 2>/dev/null &
cvlc -f --repeat --codec ffmpeg --video-title-timeout=1 /home/$USER/datv_repeater/media/film.mpg >/dev/null 2>/dev/null &
  ;;
  "HDMI" ) # Audio ok voir pour détecter automatiquement l'entrée HDMI
#gst-launch-1.0 -q v4l2src device=/dev/video0 ! 'video/x-raw, width=720, height=480, format=(string)YUY2, framerate=30/1' ! nvvidconv ! 'video/x-raw(memory:NVMM), width=(int)720, height=(int)480, framerate=(fraction)30/1' ! nvoverlaysink -e >/dev/null 2>/dev/null &
cvlc -f --codec ffmpeg alsa://plughw:1,0 --input-slave=v4l2:///dev/video0 &
  ;;
  "CAMERA" ) # Audio ok.
if [ "$TYPE_CAM" == "1" ]; then
  cvlc -f --codec ffmpeg --video-title-timeout=1 rtsp://$USR_CAM:$PW_CAM@$IP_CAM:554//h264Preview_01_main >/dev/null 2>/dev/null &
else
  cvlc -f --codec ffmpeg --video-title-timeout=1 rtsp://$USR_CAM:$PW_CAM@$IP_CAM:554/live/ch0 >/dev/null 2>/dev/null &
fi
#mplayer -fs rtsp://$USR_CAM:$PW_CAM@$IP_CAM:554//h264Preview_01_main &
  ;;
esac

exit
