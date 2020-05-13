#!/bin/bash

PATH_PCONFIG_TX="/home/$USER/jetson_datv_repeater/dvbsdr/scripts/config.txt"

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

PROVIDER_NAME=Digital_TV
CALL=$(get_config_var call $PATH_PCONFIG_TX)
FREQ=$(get_config_var freq $PATH_PCONFIG_TX)
SYMBOLRATE=$(get_config_var symbolrate $PATH_PCONFIG_TX)
MODE=$(get_config_var mode $PATH_PCONFIG_TX)
CONSTELLATION=$(get_config_var constellation $PATH_PCONFIG_TX)
GAIN=$(get_config_var gain $PATH_PCONFIG_TX)
UPSAMPLE=$(get_config_var upsample $PATH_PCONFIG_TX)
VIDEOSOURCE=$(get_config_var videosource $PATH_PCONFIG_TX)
CODEC=$(get_config_var codec $PATH_PCONFIG_TX)
AUDIOSOURCE=$(get_config_var audiosource $PATH_PCONFIG_TX)
VIDEOSOURCE_IP_ADRESS=$(get_config_var videosourceip $PATH_PCONFIG_TX)
VIDEOSOURCE_IP_PORT=$(get_config_var videosourceport $PATH_PCONFIG_TX)

OUTPUT_NETWORK="239.252.252.2:10004"
CALIBRATE_BEFORE_TX=1
AUDIO_BITRATE=20000
VIDEO_GOP=100
PCR_PTS=200000

# Bitrate
source /home/$USER/jetson_datv_repeater/dvbsdr/scripts/include/getbitrate.sh
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

source /home/$USER/jetson_datv_repeater/dvbsdr/scripts/include/nanoencode.sh | source /home/$USER/jetson_datv_repeater/dvbsdr/scripts/include/limerf.sh
