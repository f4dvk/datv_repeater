#!/bin/bash

sudo apt-get update
sudo apt-get -y dist-upgrade
sudo apt-get -y install apt-utils
sudo apt-get -y install nano
sudo apt-get -y install mpv # longmynd
sudo apt-get -y install libasound2-dev # longmynd

# Installation lecteur image (image de fond).
sudo apt-get -y install pqiv # longmynd

cd /home/$USER

git clone https://github.com/f4dvk/jetson_datv_repeater

# GPIO

sudo groupadd -f -r gpio
sudo usermod -a -G gpio $USER

sudo cp /opt/nvidia/jetson-gpio/etc/99-gpio.rules /etc/udev/rules.d/
jetson_datv_repeater/multimon/setup_gpio.sh
sudo udevadm control --reload-rules && sudo udevadm trigger

# Installation multimon pour lecture des codes DTMF
cd jetson_datv_repeater/multimon
mkdir build
cd build
cmake ..
make
sudo make install

# Installation de dvbsdr pour la partie TX
cd /home/$USER/jetson_datv_repeater/dvbsdr
./install.sh

# Installation Longmynd pour la partie RX
cd /home/$USER/jetson_datv_repeater/longmynd
make

cd /home/$USER

# désactivation transparence du terminal
dconf write /org/gnome/terminal/legacy/profiles:/:b1dcc9dd-5262-4d8d-a863-c897e6d979b9/use-theme-transparency false

# Démarrage automatique
#cp /home/$USER/longmynd/rx.desktop /home/$USER/.config/autostart/

#multimon
