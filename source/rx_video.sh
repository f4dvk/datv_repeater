#!/bin/bash

PATH_PCONFIG_SRC="/home/$USER/jetson_datv_repeater/source/config.txt"

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

SOURCE=$(get_config_var source $PATH_PCONFIG_SRC)

case "$SOURCE" in
  "MIRE" )
gst-launch-1.0 videotestsrc ! video/x-raw,width=1280,height=720 ! vaapisink fullscreen=true >/dev/null 2>/dev/null &
  ;;
  "MULTI" ) # pas encore fonctionnel
gst-launch-1.0 -e videomixer name=mix ! autovideosink videotestsrc ! video/x-raw, framerate=15/1, width=1280, height=720 ! mix.sink_0 uridecodebin uri='file:///home/stephane/film.mp4' ! videoconvert ! videoscale ! video/x-raw,framrate=20/1,width=640,height=480 ! mix.sink_1 mpv --fs --no-cache --no-terminal --loop-playlist=inf /home/$USER/film.mp4 >/dev/null 2>/dev/null &
  ;;
  "FILM"
mpv --fs --no-cache --no-terminal --loop-playlist=inf /home/$USER/film.mp4 >/dev/null 2>/dev/null &
  ;;
esac

exit
