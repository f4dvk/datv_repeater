#!/bin/bash

PATH_406CONFIG="/home/$USER/datv_repeater/406/config.txt"

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

FREQLOW=$(get_config_var low $PATH_406CONFIG)
FREQHIGH=$(get_config_var high $PATH_406CONFIG)

/home/$USER/datv_repeater/406/scan406.pl $FREQLOW $FREQHIGH

exit
