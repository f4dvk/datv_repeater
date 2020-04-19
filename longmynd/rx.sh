#!/bin/bash

FREQ=437000 # Frequence en KHz
SR=250 # SR en Ks
IN="" # pour entree A: ""   entee B: "-w"
SCAN=20 # scan +/- X KHz
GAIN=10 # gain: 0 ; 2 ; 4 ; 6 ; 8 ; 10 ; 12 ; 14 ; 16

# Image de fond
pqiv --fullscreen --hide-info-box /home/$USER/longmynd/mire.jpg >/dev/null 2>/dev/null &

sudo killall longmynd >/dev/null 2>/dev/null
sudo killall mpv >/dev/null 2>/dev/null
sudo rm /home/$USER/longmynd/longmynd_main_ts >/dev/null 2>/dev/null
sudo rm /home/$USER/longmynd/longmynd_main_status >/dev/null 2>/dev/null
mkfifo /home/$USER/longmynd/longmynd_main_ts
mkfifo /home/$USER/longmynd/longmynd_main_status

/home/$USER/longmynd/infos &

sudo /home/$USER/longmynd/longmynd -i 232.0.0.1 10005 $IN -g $GAIN -S $SCAN $FREQ $SR >/dev/null 2>/dev/null &

mpv --fs --no-cache --no-terminal udp://232.0.0.1:10005 &

exit
