#!/bin/bash

PATH_PCONFIG_SRC="/home/$USER/jetson_datv_repeater/source/config.txt"
PATH_PCONFIG_TX="/home/$USER/jetson_datv_repeater/dvbtx/scripts/config.txt"

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
CALL=$(get_config_var call $PATH_PCONFIG_TX)

case "$SOURCE" in
  "MIRE" )
gst-launch-1.0 -q videotestsrc ! video/x-raw,width=1280,height=720 ! nvvidconv ! 'video/x-raw(memory:NVMM), width=(int)1280, height=(int)720, framerate=(fraction)30/1' ! queue ! nvoverlaysink -e >/dev/null 2>/dev/null &
  ;;
  "MULTI" ) # pas encore fonctionnel
gst-launch-1.0 -q videomixer name=mix sink_00::xpos=0 sink_0::ypos=0 sink_0::width=640 sink_0::height=360 sink_1::xpos=640 sink_1::ypos=0 sink_1::width=640 \
sink_1::height=360 sink_2::xpos=0 sink_2::ypos=360 sink_2::width=640 sink_2::height=360 \
sink_3::xpos=640 sink_3::ypos=360 sink_3::width=640 sink_3::height=360 \
sink_00::alpha=1.0 sink_01::alpha=1.0 sink_02::alpha=1.0 sink_03::alpha=1.0 ! videoconvert ! nvoverlaysink sync=false \
videotestsrc ! textoverlay text="F5ZBC   http://f5zbc.fr" valignment=top halignment=left font-desc="Sans, 66" ! video/x-raw, width=640,  height=360, framerate=30/1 ! alpha alpha=1.0 ! mix. \
v4l2src device=/dev/video0 ! video/x-raw, width=640, height=360, framerate=30/1, 'format=(string)YUY2' ! alpha alpha=1.0 ! mix. \
multifilesrc location=/home/$USER/jetson_datv_repeater/film/film.mpg loop=true ! decodebin ! videoconvert ! videoscale ! video/x-raw, width=640, height=360 ! \
alpha alpha=1.0 ! mix. \
videotestsrc ! video/x-raw, width=640, height=360, framerate=30/1 ! alpha alpha=1.0 ! mix. -e >/dev/null 2>/dev/null & # video en format mpg, sinon pas de fonctionnement de la boucle.
  ;;
  "FILM" )
mpv --fs --no-cache --no-terminal --loop-playlist=inf /home/$USER/film.mp4 >/dev/null 2>/dev/null &
  ;;
  "HDMI" )
gst-launch-1.0 -q v4l2src device=/dev/video0 ! video/x-raw, width=720, height=480, format=(string)YUY2, framerate=30/1 ! nvvidconv ! 'video/x-raw(memory:NVMM), width=(int)720, height=(int)480, framerate=(fraction)30/1' ! queue ! nvoverlaysink >/dev/null 2>/dev/null &
  ;;
esac

exit
