#!/bin/bash

mkdir build
mkdir bin
cd build

sudo apt-get update
sudo apt-get install -y git g++ cmake libsqlite3-dev libi2c-dev libusb-1.0-0-dev netcat
# Pluto SDR
sudo apt-get install -y libiio-dev

#Firmware DVB LimeSDR
wget -q https://github.com/natsfr/LimeSDR_DVBSGateware/releases/download/v0.3/LimeSDR-Mini_lms7_trx_HW_1.2_auto.rpd -O LimeSDR-Mini_lms7_trx_HW_1.2_auto.rpd

#Installation de LimeSuite
git clone --depth=1 https://github.com/myriadrf/LimeSuite
cd LimeSuite
mkdir dirbuild
cd dirbuild
cmake ../
make
sudo make install
sudo ldconfig

cd ../udev-rules/
chmod +x install.sh
sudo ./install.sh
cd ../../

#sudo LimeUtil --update

# limesdr_toolbox

git clone https://github.com/f4dvk/limesdr_toolbox
cd limesdr_toolbox

# libdvbmod
sudo apt-get install -y libfftw3-dev
git clone https://github.com/F5OEO/libdvbmod
cd libdvbmod/libdvbmod
make
cd ../DvbTsToIQ/
make
cp dvb2iq ../../../../bin/
cd ../../

make
cp limesdr_send ../../bin/
cp limesdr_stopchannel ../../bin/
make dvb
cp limesdr_dvb ../../bin/
cd ../

sudo apt-get install -y buffer ffmpeg

cd ../scripts

exit
