#!/bin/bash

if grep -q "Jetson Nano" /proc/device-tree/model; then

  echo
  echo "-----------------------------------------------------"
  echo "----------- Installation sur Jetson Nano ------------"
  echo "-----------------------------------------------------"
  echo

  echo "$USER ALL=(ALL:ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/dont-prompt-$USER-for-password

  sudo apt-get update
  sudo apt-get -y dist-upgrade
  sudo apt-get -y install dkms
  sudo apt-get -y install apt-utils
  sudo apt-get -y install nano
  sudo apt-get -y install gst-1.0
  sudo apt-get -y install libpulse-dev # multimon
  sudo apt-get -y install mpv # longmynd
  sudo apt-get -y install libasound2-dev # longmynd
  sudo apt-get -y install lua5.3 # pour la fonction SH get_config_var
  sudo apt-get -y install sshpass
  # Installation lecteur image (image de fond).
  sudo apt-get -y install pqiv # longmynd
  sudo apt-get -y install mplayer
  sudo apt-get -y install vlc
  sudo apt-get -y install libusb-1.0-0-dev # longmynd
  sudo apt-get install libcurl4-openssl-dev # Command encodeur H.264 / H.265

  # Synthese vocale
  sudo apt-get -y install libttspico-utils

  sudo apt-get -y install libxml2 libxml2-dev bison flex libcdk5-dev                   # libiio
  sudo apt-get -y install libaio-dev libserialport-dev libavahi-client-dev             # libiio

  cd /home/$USER
  git clone https://github.com/analogdevicesinc/libiio.git
  cd libiio
  cmake ./
  make all
  sudo make install

  # gstreamer
  # sudo apt-get -y install python-gst-1.0 gstreamer1.0-plugins-good gstreamer1.0-plugins-ugly gstreamer1.0-plugins-bad gstreamer1.0-tools

  mkdir /home/$USER/tmp # pour sshpass

  echo "sub-auto=all" >> /home/$USER/.config/mpv/mpv.conf # sous-titre mpv

  cd /home/$USER

  #echo
  #echo "-----------------------------------------------------"
  #echo "---------------- Installation de VNC ----------------"
  #echo "-----------------------------------------------------"

  #sudo apt-get install tigervnc-scraping-server
  #sudo apt-get install git psmisc

  #mkdir -p ~/.vnc

  # Enregistrer un mot de passe
  # vncpasswd

  #git clone https://github.com/sebestyenistvan/runvncserver
  #cp ~/runvncserver/startvnc ~
  #chmod +x ~/startvnc

  # Démarrage suto:
  # echo "/home/$USER/startvnc start >/dev/null 2>&1" >> ~/.xsessionrc


  echo
  echo "-----------------------------------------------------"
  echo "------------ Téléchargement datv_repeater -----------"
  echo "-----------------------------------------------------"

  git clone https://github.com/f4dvk/datv_repeater

  sudo cp /opt/nvidia/jetson-gpio/etc/99-gpio.rules /etc/udev/rules.d/
  datv_repeater/multimon/setup_gpio.sh
  sudo udevadm control --reload-rules && sudo udevadm trigger

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

  cd /home/$USER/datv_repeater/multimon
  ./setup_gpio.sh
  ./fix_udev_gpio.sh

  sudo cp /home/$USER/datv_repeater/config/default.pa /etc/pluse/

  echo
  echo "-----------------------------------------------------"
  echo "----------- Installation de la partie RX ------------"
  echo "-----------------------------------------------------"

  # Installation Longmynd pour la partie RX
  cd /home/$USER/datv_repeater/longmynd
  make

  cd /home/$USER

  # désactivation transparence du terminal
  dconf write /org/gnome/terminal/legacy/profiles:/:b1dcc9dd-5262-4d8d-a863-c897e6d979b9/use-theme-transparency false

  # Suppression du curseur dans le terminal
  setterm -cursor off

  # Démarrage automatique
  #if [ "$1" == "-l" ]; then
  #  cp /home/$USER/datv_repeater/multimon/multimon.desktop /home/$USER/.config/autostart/
  #fi

  echo
  echo "-----------------------------------------------------"
  echo "-------------- Installation de dvb2iq ---------------"
  echo "-----------------------------------------------------"

  git clone https://github.com/f4dvk/limesdr_toolbox

  cd /home/$USER/limesdr_toolbox

  git clone https://github.com/F5OEO/libdvbmod
  cd libdvbmod/libdvbmod
  make
  cd ../DvbTsToIQ/
  make
  cp dvb2iq /home/$USER/

  cd /home/$USER

  echo
  echo "-----------------------------------------------------"
  echo "------------ Installation de dvb_t_stack ------------"
  echo "-----------------------------------------------------"

  cd /home/$USER/datv_repeater/dvb_t_stack/Release
  make clean
  make
  cp dvb_t_stack /home/$USER/datv_repeater/dvbtx/bin/dvb_t_stack

  #cd /home/$USER/datv_repeater/dvb_t_stack
  #sudo cp datvexpress16.ihx /lib/firmware/datvexpress/datvexpress16.ihx
  #sudo cp datvexpressraw16.rbf /lib/firmware/datvexpress/datvexpressraw16.rbf
  cd /home/$USER

  echo
  echo "-----------------------------------------------------"
  echo "---------- Installation du décodeur SARSAT ----------"
  echo "-----------------------------------------------------"

  /home/$USER/datv_repeater/406/install.sh

fi

if grep -q "Raspberry" /proc/device-tree/model; then

  echo
  echo "-----------------------------------------------------"
  echo "----------- Installation sur Raspberry PI -----------"
  echo "-----------------------------------------------------"

  sudo dpkg --configure -a
  sudo apt-get update
  sudo apt-get -y remove apt-listchanges
  sudo apt-get -y dist-upgrade
  sudo apt-get -y install git cmake libusb-1.0-0-dev wiringpi libfftw3-dev libxcb-shape0
  sudo apt-get -y install libx11-dev buffer libjpeg-dev indent
  sudo apt-get -y install ttf-dejavu-core bc usbmount libvncserver-dev
  sudo apt-get -y install fbi netcat imagemagick omxplayer
  sudo apt-get -y install libvdpau-dev libva-dev  # ffmpeg
  sudo apt-get -y install g++ libsqlite3-dev libi2c-dev # Lime
  sudo apt-get -y install sshpass
  sudo apt-get -y install libbsd-dev
  sudo apt-get -y install libasound2-dev sox # avc2ts
  sudo apt-get -y install libavcodec-dev libavformat-dev libswscale-dev libavdevice-dev # ffmpegsrc.cpp

  #sudo apt-get -y install xscreensaver # utilitaire pour désactiver la mise en veille

  sudo apt-get -y install libxml2 libxml2-dev bison flex libcdk5-dev                   # libiio
  sudo apt-get -y install libaio-dev libserialport-dev libavahi-client-dev             # libiio

  cd /home/pi
  git clone https://github.com/analogdevicesinc/libiio.git
  cd libiio
  cmake ./
  make all
  sudo make install

  cd /home/pi

  # Login auto
  sudo raspi-config nonint do_boot_behaviour B2

  echo
  echo "----------------------------------------------"
  echo "---- Setting Framebuffer to 32 bit depth -----"
  echo "----------------------------------------------"

  sudo sed -i "/^dtoverlay=vc4-fkms-v3d/c\#dtoverlay=vc4-fkms-v3d" /boot/config.txt

  echo
  echo "-------------------------------------------"
  echo "---- Reducing the dhcp client timeout -----"
  echo "-------------------------------------------"
  sudo bash -c 'echo -e "\n# Shorten dhcpcd timeout from 30 to 5 secs" >> /etc/dhcpcd.conf'
  sudo bash -c 'echo -e "timeout 5\n" >> /etc/dhcpcd.conf'
  cd /home/pi/

  echo
  echo "-------------------------------------------"
  echo "------- Installation de LimeSuite ---------"
  echo "-------------------------------------------"

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
  cd /home/pi

  echo
  echo "-------------------------------------------"
  echo "-------- Installation datv_repeater -------"
  echo "-------------------------------------------"

  git clone https://github.com/f4dvk/datv_repeater

  echo
  echo "-----------------------------------------------------"
  echo "-------------- Installation de avc2ts ---------------"
  echo "-----------------------------------------------------"

  wget https://github.com/f4dvk/avc2ts/archive/master.zip

  unzip -o master.zip
  mv avc2ts-master avc2ts
  rm master.zip
  sed -i 's/Portsdown/Relais de Soissons(02)/g' /home/pi/avc2ts/avc2ts.cpp

  cd /home/pi/avc2ts
  git clone git://github.com/F5OEO/libmpegts
  cd libmpegts
  ./configure
  make
  cd ../

  git clone https://github.com/mstorsjo/fdk-aac
  cd fdk-aac
  ./autogen.sh
  ./configure
  make && sudo make install
  sudo ldconfig
  cd ../

  git clone https://chromium.googlesource.com/libyuv/libyuv
  cd libyuv
  make V=1 -f linux.mk
  cd ../

  cd /home/pi/avc2ts
  make
  cp avc2ts ../datv_repeater/dvbtx/bin/
  cd /home/pi

  echo
  echo "-----------------------------------------------------"
  echo "---------- Installation de LimeSDR Toolbox ----------"
  echo "-----------------------------------------------------"

  git clone https://github.com/f4dvk/limesdr_toolbox

  cd /home/pi/limesdr_toolbox

  git clone https://github.com/F5OEO/libdvbmod
  cd libdvbmod/libdvbmod
  make
  cd ../DvbTsToIQ/
  make
  cp dvb2iq /home/pi/datv_repeater/dvbtx/bin/
  cd /home/pi/rpidatv/src/limesdr_toolbox/

  make
  cp limesdr_send /home/pi/datv_repeater/dvbtx/bin/
  cp limesdr_dump /home/pi/datv_repeater/dvbtx/bin/
  cp limesdr_stopchannel /home/pi/datv_repeater/dvbtx/bin/
  cp limesdr_forward /home/pi/datv_repeater/dvbtx/bin/
  make dvb
  cp limesdr_dvb /home/pi/datv_repeater/dvbtx/bin/
  cd /home/pi

  echo
  echo "-----------------------------------------------------"
  echo "------------ Installation de dvb_t_stack ------------"
  echo "-----------------------------------------------------"

  cd /home/pi/datv_repeater/dvb_t_stack/Release
  make clean
  make
  cp dvb_t_stack /home/pi/datv_repeater/dvbtx/bin/dvb_t_stack

  #cd /home/pi/datv_repeater/dvb_t_stack
  #sudo cp datvexpress16.ihx /lib/firmware/datvexpress/datvexpress16.ihx
  #sudo cp datvexpressraw16.rbf /lib/firmware/datvexpress/datvexpressraw16.rbf
  cd /home/pi

fi

echo
echo "-----------------------------------------------------"
echo "--------------- Installation Terminée ---------------"
echo "-----------------------------------------------------"

sleep 1

exit
