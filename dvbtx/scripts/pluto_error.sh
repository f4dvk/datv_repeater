#!/bin/bash

# Source BATC Scripts, thanks!

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

PLUTOIP=$(get_config_var plutoip $PATH_PCONFIG_USR)

# Make sure that we will be able to log in
ssh-keygen -f "/home/$USER/.ssh/known_hosts" -R "$PLUTOIP" >/dev/null 2>/dev/null

sshpass -p analog ssh -o StrictHostKeyChecking=no root@"$PLUTOIP" 'cat /infortmp | grep error' >/dev/null 2>/dev/null

if [ $? == 0 ]; then
  /home/$USER/datv_repeater/dvbtx/scripts/pluto_reboot.sh
fi

exit
