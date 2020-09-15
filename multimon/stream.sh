#!/bin/bash

gst-launch-1.0 ximagesrc use-damage=0 ! video/x-raw,framerate=30/1 ! videoscale ! videoconvert ! x264enc tune=zerolatency bitrate=1024 speed-preset=superfast ! queue ! mux. alsasrc device=hw:2 ! 'audio/x-raw, format=S16LE, layout=interleaved, rate=48000, channels=1' ! voaacenc bitrate=48000 ! queue  ! mux. mpegtsmux alignment=7 name=mux ! fdsink | ffmpeg -i - -ss 8 -c:v copy udp://localhost:10001?pkt_size=1316
