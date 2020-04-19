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
jetson_datv_repeater/multimon-ng/setup_gpio.sh
sudo udevadm control --reload-rules && sudo udevadm trigger

# Installation multimon-ng pour lecture des codes DTMF
cd jetson_datv_repeater/multimon-ng
mkdir build
cd build
cmake ..
make
sudo make install

cd /home/$USER

# Installation de dvbsdr pour la partie TX
git clone https://github.com/f4dvk/dvbsdr
cd dvbsdr
./install.sh

# Installation Longmynd pour la partie RX
cd /home/$USER/jetson_datv_repeater/longmynd
make

cd /home/$USER

#multimon-ng
