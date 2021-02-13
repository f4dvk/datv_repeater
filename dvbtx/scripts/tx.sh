#!/bin/bash

# Le pilotage du TX se fait en SSH

PATH_PCONFIG_TX="/home/$USER/datv_repeater/dvbtx/scripts/config.txt"
PATH_PCONFIG_USR="/home/$USER/datv_repeater/config.txt"
CMDFILE="/home/$USER/tmp/jetson_command.txt"

PATHCONFIGRPI="/home/pi/datv_repeater/dvbtx/scripts/config_rpi.txt"

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

CHANNEL="Relais_de_SOISSONS(02)"
CALL=$(get_config_var call $PATH_PCONFIG_USR)
FREQ=$(get_config_var freq $PATH_PCONFIG_TX)
SYMBOLRATE=$(get_config_var symbolrate $PATH_PCONFIG_TX)
FEC=$(get_config_var fec $PATH_PCONFIG_TX)
MODE=$(get_config_var mode $PATH_PCONFIG_TX)
CONSTELLATION=$(get_config_var constellation $PATH_PCONFIG_TX)
FORMAT=$(get_config_var format $PATH_PCONFIG_USR)
GAIN=$(get_config_var gain $PATH_PCONFIG_TX)
UPSAMPLE=$(get_config_var upsample $PATH_PCONFIG_TX)
CODEC=$(get_config_var codec $PATH_PCONFIG_TX)
PILOTS=$(get_config_var pilots $PATH_PCONFIG_TX)
FRAMES=$(get_config_var frames $PATH_PCONFIG_TX)
RPIIP=$(get_config_var rpiip $PATH_PCONFIG_USR)
RPIUSER=$(get_config_var rpiuser $PATH_PCONFIG_USR)
RPIPW=$(get_config_var rpipw $PATH_PCONFIG_USR)
Tx=$(get_config_var tx $PATH_PCONFIG_USR)
PLUTOIP=$(get_config_var plutoip $PATH_PCONFIG_USR)
QAM=$(get_config_var qam $PATH_PCONFIG_TX)
GUARD=$(get_config_var guard $PATH_PCONFIG_TX)

if [ "$Tx" != "pluto" ]; then
  /bin/cat <<EOM >$CMDFILE
    (sshpass -p $RPIPW ssh -o StrictHostKeyChecking=no $RPIUSER@$RPIIP 'bash -s' <<'ENDSSH'
    sed -i '/\(^call=\).*/s//\1$CALL/' $PATHCONFIGRPI
    sed -i '/\(^freq=\).*/s//\1$FREQ/' $PATHCONFIGRPI
    sed -i '/\(^symbolrate=\).*/s//\1$SYMBOLRATE/' $PATHCONFIGRPI
    sed -i '/\(^fec=\).*/s//\1$FEC/' $PATHCONFIGRPI
    sed -i '/\(^mode=\).*/s//\1$MODE/' $PATHCONFIGRPI
    sed -i '/\(^constellation=\).*/s//\1$CONSTELLATION/' $PATHCONFIGRPI
    sed -i '/\(^format=\).*/s//\1$FORMAT/' $PATHCONFIGRPI
    sed -i '/\(^gain=\).*/s//\1$GAIN/' $PATHCONFIGRPI
    sed -i '/\(^upsample=\).*/s//\1$UPSAMPLE/' $PATHCONFIGRPI
    sed -i '/\(^codec=\).*/s//\1$CODEC/' $PATHCONFIGRPI
    sed -i '/\(^pilots=\).*/s//\1$PILOTS/' $PATHCONFIGRPI
    sed -i '/\(^frames=\).*/s//\1$FRAMES/' $PATHCONFIGRPI
    sleep 1
    /home/pi/datv_repeater/dvbtx/scripts/tx_rpi.sh >/dev/null 2>/dev/null &
    /home/pi/datv_repeater/dvbtx/scripts/watchdog.sh >/dev/null 2>/dev/null &
ENDSSH
    ) &
EOM

  source "$CMDFILE"

elif [ "$Tx" == "pluto" ] && [ "$MODE" == "DVB-T" ]; then

  let FECNUM=FEC
  let FECDEN=FEC+1

  # awk affiche en exposant si la fréquence est supérieure à 2147 MHz
  FREQ_OUTPUTHZ=`echo - | awk '{print '$FREQ' * 100000}'`
  FREQ_OUTPUTHZ="$FREQ_OUTPUTHZ"0

  /home/$USER/datv_repeater/dvbtx/bin/dvb_t_stack -m $QAM -f $FREQ_OUTPUTHZ -a $GAIN -r pluto \
    -g 1/"$GUARD" -b $SYMBOLRATE -p 1314 -e "$FECNUM"/"$FECDEN" -n $PLUTOIP -i /dev/null &

fi

exit
