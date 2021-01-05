#!/bin/bash

delai=7 # en minutes, doit être supérieur au delai_TX (demod_dtmf.c)

top=$(date +"%s")

timestamp()
{
  date +"%s"
}


while true;do

  diff=$(($(timestamp) - $top))

  diff=$(($diff / 60))

  if [ $diff == $delai ] && [ $(ps -ef | grep "limesdr_dvb" | grep -v grep | wc -l) -gt 0 ]; then
    /home/pi/datv_repeater/dvbtx/scripts/TXstop_rpi.sh >/dev/null 2>/dev/null &
    exit
  elif [ $diff == $delai ] && [ $(ps -ef | grep "limesdr_dvb" | grep -v grep | wc -l) == 0 ]; then
    exit
  fi

done
