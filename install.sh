#!/bin/bash

sudo apt-get update
sudo apt-get -y dist-upgrade

cd /home/$USER

# Installation multimon-ng pour lecture des codes DTMF
git clone https://github.com/AliasOenal/multimon-ng
cd multimon-ng
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
