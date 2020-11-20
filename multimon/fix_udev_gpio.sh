#!/bin/bash
#
# This script changes the group ("gpio") and permissions of most files in
# /sys/class/gpio directory, so that gpio could be accessed by non-root
# users in the "gpio" group.

sudo chown -R root.gpio /sys/class/gpio
sudo chmod 0220 /sys/class/gpio/export
sudo chmod 0220 /sys/class/gpio/unexport

sudo chown root.gpio /sys/class/gpio/*/direction
sudo chown root.gpio /sys/class/gpio/*/edge
sudo chown root.gpio /sys/class/gpio/*/value
sudo chown root.gpio /sys/class/gpio/*/active_low
sudo chmod 0664 /sys/class/gpio/*/direction
sudo chmod 0664 /sys/class/gpio/*/edge
sudo chmod 0664 /sys/class/gpio/*/value
sudo chmod 0664 /sys/class/gpio/*/active_low
