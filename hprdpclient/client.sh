#!/bin/bash

if [[ -z $DISPLAY ]]
then 
	export DISPLAY=:0.0
else 
	zenity --question --title="Remote Control" --text="Allow remote control?" --no-wrap
	CONTINUE=$? 
fi

if [[ $CONTINUE -eq 1 ]]; then exit; fi

$APPDIR/usr/bin/python -m client &

sleep 1

if [[ ! -f ~/.config/hprdpvideo.sh ]]; then 

cat > ~/.config/hprdpvideo.sh <<\EOF
ffmpeg -f x11grab -draw_mouse 0 -s 1920x1080 -framerate 30 -i $DISPLAY  -c:v libx264 -preset medium -profile high -pix_fmt yuv420p -tune zerolatency -b:v 2000K -minrate 2000K -maxrate 2000K -bufsize 512k -f mpegts tcp://127.0.0.1:12345
EOF

fi

bash ~/.config/hprdpvideo.sh

zenity --info --title="Terminating" --text="Screen sharing ended" --no-wrap
