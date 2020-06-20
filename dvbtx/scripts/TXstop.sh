#! /bin/bash

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

JETSONIP=$(get_config_var jetsonip $PATH_PCONFIG_TX)
JETSONUSER=$(get_config_var jetsonuser $PATH_PCONFIG_TX)
JETSONPW=$(get_config_var jetsonpw $PATH_PCONFIG_TX)

sshpass -p $JETSONPW ssh -o StrictHostKeyChecking=no $JETSONUSER@$JETSONIP 'bash -s' <<'ENDSSH'
  killall gst-launch-1.0
  killall ffmpeg
  killall limesdr_dvb
  /home/$JETSONUSER/jetson_datv_repeater/dvbtx/bin/limesdr_stopchannel >/dev/null 2>/dev/null
ENDSSH

exit
