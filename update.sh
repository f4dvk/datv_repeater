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

if [ ! -d  /home/$USER/libiio ]; then
  echo "Installation libiio"
  echo
  sudo apt-get -y install libxml2 libxml2-dev bison flex libcdk5-dev
  sudo apt-get -y install libaio-dev libserialport-dev libavahi-client-dev
  cd /home/$USER
  git clone https://github.com/analogdevicesinc/libiio.git
  cd libiio
  cmake ./
  make all
  sudo make install
  cd /home/$USER
else
  echo "libiio déjà installé"
  echo
fi

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
echo "---------- Installation du décodeur SARSAT ----------"
echo "-----------------------------------------------------"

cd /home/$USER/datv_repeater/406
gcc ./dec406_V6.c -lm -o ./dec406_V6

echo
echo "-----------------------------------------------------"
echo "----------- Restauration fichiers config ------------"
echo "-----------------------------------------------------"

cd /home/$USER

cp -f -r "$BACKUP"/config.txt "$CONFIG"/config.txt

echo
echo "-----------------------------------------------------"
echo "--------------- Mise à jour Terminée ----------------"
echo "-----------------------------------------------------"

sleep 1

exit
