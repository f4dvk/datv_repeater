#!/bin/bash

sudo apt-get update
sudo apt-get -y dist-upgrade

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

cd /home/$USER

# Installation Longmynd pour la partie RX
wget https://raw.githubusercontent.com/f4dvk/longmynd/master/install.sh
chmod +x install.sh
./install.sh

cd /home/$USER
rm install.sh

#multimon-ng
