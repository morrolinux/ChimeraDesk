#!/bin/bash
env
$APPDIR/usr/bin/python -m client &
sleep 2
ffmpeg -f x11grab -draw_mouse 0 -s 1920x1080 -framerate 30 -i $DISPLAY  -c:v libx264 -preset medium -profile high -pix_fmt yuv420p -tune zerolatency -b:v 2000K -minrate 2000K -maxrate 2000K -bufsize 512k -f mpegts tcp://127.0.0.1:12345
