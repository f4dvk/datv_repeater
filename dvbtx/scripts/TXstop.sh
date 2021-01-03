#! /bin/bash

PATH_PCONFIG_USR="/home/$USER/datv_repeater/config.txt"
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

RPIIP=$(get_config_var jetsonip $PATH_PCONFIG_USR)
RPIUSER=$(get_config_var jetsonuser $PATH_PCONFIG_USR)
RPIPW=$(get_config_var jetsonpw $PATH_PCONFIG_USR)

/bin/cat <<EOM >$CMDFILE
(sshpass -p $RPIPW ssh -o StrictHostKeyChecking=no $RPIUSER@$RPIIP 'bash -s' <<'ENDSSH'
  /home/pi/datv_repeater/dvbtx/scripts/TXstop_rpi.sh >/dev/null 2>/dev/null &
ENDSSH
) &
EOM

source "$CMDFILE"
sleep 0.7

exit
