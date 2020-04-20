#!/bin/bash

if [ $1 == "-w" ]; then
  IN=$1
  GAIN=$2
  SCAN=$3
  FREQ=$4
  SR=$5
else
  IN=""
  GAIN=$1
  SCAN=$2
  FREQ=$3
  SR=$4
fi

sudo killall longmynd >/dev/null 2>/dev/null
sudo killall mpv >/dev/null 2>/dev/null
sudo rm /home/$USER/longmynd_main_ts >/dev/null 2>/dev/null
sudo rm /home/$USER/longmynd_main_status >/dev/null 2>/dev/null
mkfifo /home/$USER/longmynd_main_ts
mkfifo /home/$USER/longmynd_main_status

/home/$USER/longmynd/infos &

sudo /home/$USER/longmynd/longmynd -i 232.0.0.1 10005 $IN -g $GAIN -S $SCAN $FREQ $SR >/dev/null 2>/dev/null &

mpv --no-cache --no-terminal udp://232.0.0.1:10005 &

exit
