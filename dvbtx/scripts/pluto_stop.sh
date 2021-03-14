#!/bin/bash

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

PLUTOIP=$(get_config_var plutoip $PATH_PCONFIG_USR)

/bin/cat <<EOM >$CMDFILE
(sshpass -p analog ssh -o StrictHostKeyChecking=no root@$PLUTOIP 'sh -s' <<'ENDSSH'
  killall pluto_dvb >/dev/null 2>/dev/null
ENDSSH
) &
EOM

source "$CMDFILE"

exit
