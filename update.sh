#!/bin/bash

echo
echo "-----------------------------------------------------"
echo "------------- Mise à jour datv_repeater -------------"
echo "-----------------------------------------------------"

sudo killall ffmpeg >/dev/null 2>/dev/null
sudo killall longmynd >/dev/null 2>/dev/null
sudo killall multimon >/dev/null 2>/dev/null
sudo killall gst-launch-1.0 >/dev/null 2>/dev/null
sudo killall limesdr_dvb >/dev/null 2>/dev/null
/home/$USER/datv_repeater/dvbtx/scripts/TXstop.sh >/dev/null 2>/dev/null

sudo apt-get update
sudo apt-get -y dist-upgrade

echo
echo "-----------------------------------------------------"
echo "---------- Sauvegarde des fichiers config -----------"
echo "-----------------------------------------------------"

cd /home/$USER

BACKUP="/home/$USER/backup"
CONFIG="/home/$USER/datv_repeater"

mkdir "$BACKUP" >/dev/null 2>/dev/null

cp -f -r "$CONFIG"/config.txt "$BACKUP"/config.txt

echo
echo "-----------------------------------------------------"
echo "------------ Téléchargement datv_repeater -----------"
echo "-----------------------------------------------------"

cd /home/$USER
rm -rf datv_repeater >/dev/null 2>/dev/null

git clone https://github.com/f4dvk/datv_repeater

echo
echo "-----------------------------------------------------"
echo "------------- Installation Logique DTMF -------------"
echo "-----------------------------------------------------"

# Installation multimon pour lecture des codes DTMF
cd datv_repeater/multimon
mkdir build
cd build
cmake ..
make
sudo make install

#echo
#echo "-----------------------------------------------------"
#echo "----------- Installation de la partie TX ------------"
#echo "-----------------------------------------------------"

# Installation de dvbsdr pour la partie TX
#cd /home/$USER/datv_repeater/dvbtx
#./install.sh

echo
echo "-----------------------------------------------------"
echo "----------- Installation de la partie RX ------------"
echo "-----------------------------------------------------"

# Installation Longmynd pour la partie RX
cd /home/$USER/datv_repeater/longmynd
make

echo
echo "-----------------------------------------------------"
echo "----------- Restauration fichiers config ------------"
echo "-----------------------------------------------------"

cd /home/$USER

cp -f -r "$BAKUP"/config.txt "$CONFIG"/config.txt

echo
echo "-----------------------------------------------------"
echo "--------------- Mise à jour Terminée ----------------"
echo "-----------------------------------------------------"

sleep 1

exit
