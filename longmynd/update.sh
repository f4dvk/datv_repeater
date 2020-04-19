#!/bin/bash

# Mise à jour
sudo apt-get update
sudo apt-get -y dist-upgrade

PATHUBACKUP="/home/$USER/user_backups"
PATHSCRIPT="/home/$USER/longmynd"
mkdir "$PATHUBACKUP" >/dev/null 2>/dev/null

# Sauvegarde fichier config
cp -f -r "$PATHSCRIPT"/config.txt "$PATHUBACKUP"/config.txt

cd /home/$USER/

sudo rm -r longmynd

# Installation Longmynd (RX Minitiouner)
wget https://github.com/f4dvk/longmynd/archive/master.zip
unzip -o master.zip
mv longmynd-master longmynd
rm master.zip
cd longmynd
make

# Restauration du fichier config
cp -f -r "$PATHUBACKUP"/config.txt "$PATHSCRIPT"/config.txt

exit
