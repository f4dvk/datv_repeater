#!/bin/bash

echo
echo "-----------------------------------------------------"
echo "--------- Installation jetson_datv_repeater ---------"
echo "-----------------------------------------------------"

sudo apt-get update
sudo apt-get -y dist-upgrade
sudo apt-get -y install dkms
sudo apt-get -y dist-upgrade
sudo apt-get -y install apt-utils
sudo apt-get -y install nano
sudo apt-get -y install gst-1.0
sudo apt-get -y install libasound2-dev sox # avc2ts Audio
sudo apt-get -y install libpulse-dev # multimon
sudo apt-get -y install mpv # longmynd
sudo apt-get -y install libasound2-dev # longmynd
sudo apt-get -y install lua5.3 # pour la fonction SH get_config_var
sudo apt-get -y install sshpass
# Installation lecteur image (image de fond).
sudo apt-get -y install pqiv # longmynd
#sudo apt-get -y install vlc-bin # cvlc
sudo apt-get -y install mplayer

mkdir /home/$USER/tmp # pour sshpass

cd /home/$USER

echo
echo "-----------------------------------------------------"
echo "-------- Téléchargement jetson_datv_repeater --------"
echo "-----------------------------------------------------"

git clone https://github.com/f4dvk/jetson_datv_repeater

sudo cp /opt/nvidia/jetson-gpio/etc/99-gpio.rules /etc/udev/rules.d/
jetson_datv_repeater/multimon/setup_gpio.sh
sudo udevadm control --reload-rules && sudo udevadm trigger

echo
echo "-----------------------------------------------------"
echo "------------- Installation Logique DTMF -------------"
echo "-----------------------------------------------------"

# Installation multimon pour lecture des codes DTMF
cd jetson_datv_repeater/multimon
mkdir build
cd build
cmake ..
make
sudo make install

cd /home/$USER/jetson_datv_repeater/multimon
./setup_gpio.sh
./fix_udev_gpio.sh

echo
echo "-----------------------------------------------------"
echo "----------- Installation de la partie TX ------------"
echo "-----------------------------------------------------"

# Installation de dvbsdr pour la partie TX
cd /home/$USER/jetson_datv_repeater/dvbtx
./install.sh

echo
echo "-----------------------------------------------------"
echo "----------- Installation de la partie RX ------------"
echo "-----------------------------------------------------"

# Installation Longmynd pour la partie RX
cd /home/$USER/jetson_datv_repeater/longmynd
make

cd /home/$USER

# désactivation transparence du terminal
dconf write /org/gnome/terminal/legacy/profiles:/:b1dcc9dd-5262-4d8d-a863-c897e6d979b9/use-theme-transparency false

# Suppression du curseur dans le terminal
setterm -cursor off

# Démarrage automatique
#if [ "$1" == "-l" ]; then
#  cp /home/$USER/jetson_datv_repeater/multimon/multimon.desktop /home/$USER/.config/autostart/
#fi

echo
echo "-----------------------------------------------------"
echo "--------------- Installation Terminée ---------------"
echo "-----------------------------------------------------"

sleep 1

exit
