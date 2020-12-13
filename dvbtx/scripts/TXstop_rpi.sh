#! /bin/bash

# Kill the key TX processes as nicely as possible
sudo killall ffmpeg >/dev/null 2>/dev/null
sudo killall avc2ts >/dev/null 2>/dev/null
sudo killall netcat >/dev/null 2>/dev/null
sudo killall dvb2iq >/dev/null 2>/dev/null
sudo killall limesdr_send >/dev/null 2>/dev/null
sudo killall limesdr_dvb >/dev/null 2>/dev/null
sudo killall sox >/dev/null 2>/dev/null

# Turn the mpeg-2 camera overlay off
v4l2-ctl -d /dev/video0 --overlay 0 >/dev/null 2>/dev/null
v4l2-ctl -d /dev/video1 --overlay 0 >/dev/null 2>/dev/null

# Then pause and make sure that avc2ts has really been stopped (needed at high SRs)
sleep 0.1
sudo killall -9 avc2ts >/dev/null 2>/dev/null

# And make sure limesdr_send has been stopped
sudo killall -9 limesdr_send >/dev/null 2>/dev/null
sudo killall -9 limesdr_dvb >/dev/null 2>/dev/null
sudo killall -9 sox >/dev/null 2>/dev/null

# Reset the LimeSDR
/home/pi/datv_repeater/dvbtx/bin/limesdr_stopchannel >/dev/null 2>/dev/null
# Stop the audio for CompVid mode
sudo killall arecord >/dev/null 2>/dev/null


exit
