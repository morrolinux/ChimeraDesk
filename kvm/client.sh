#!/bin/bash

env

# chech weather we are running from terminal or by double click
if [[ $TERM != "dumb" ]]; then export UI=0; else export UI=1; fi

# try setting an arbitrary (often correct) DISPLAY variable if not set (ssh?)
export DISPLAY=$(ps -u $(id -u) -o pid=|xargs -I{} cat /proc/{}/environ 2>/dev/null|tr '\0' '\n'|grep -m1 '^DISPLAY='|cut -d= -f2)
# see https://unix.stackexchange.com/questions/429092/what-is-the-best-way-to-find-the-current-display-and-xauthority-in-non-interacti

# try to guess the screen resolution for ffmpeg
Xres=$(xrandr --current | grep '*' | uniq | awk '{print $1}' | cut -d 'x' -f1)
Yres=$(xrandr --current | grep '*' | uniq | awk '{print $1}' | cut -d 'x' -f2)

# Give the user some feedback when running on UI as shell prompt doesn't pop up
if [[ $UI -eq 1 ]]; then
    if [[ $(which zenity) != "" ]]; then 
        zenity --question --text="Allow remote control?" --no-wrap
        export ABORT=$? 
    else
	export ABORT=0
    fi
fi

if [[ $ABORT -eq 1 ]]; then exit; fi

# Launch KVM client process and store its PID for later termination
if [[ -z $APPDIR ]]; then
    SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
    python $SCRIPTPATH/client/src/client.py &
else
    $APPDIR/usr/bin/python $APPDIR/usr/bin/client.py &
fi
kvmpid=$!

# Check weather the KVM process is running, terminate if not.
sleep 1 && kill -0 $kvmpid
kvm_error=$?
if [[ $kvm_error -eq 0 ]]; then
    notify-send "Screen sharing is starting..."
else
    notify-send "Screen sharing could not start"
    exit 1
fi

# If no ffmpeg config file is found, generate a default one
if [[ ! -f ~/.config/hprdpvideo.sh ]]; then 
    echo "echo \$BASHPID > /tmp/ffmpeg.pid; ffmpeg -f x11grab -draw_mouse 0 -s ${Xres}x${Yres} -framerate 30 -i \$DISPLAY  -c:v libx264 -preset medium -profile high -pix_fmt yuv420p -tune zerolatency -b:v 2000K -minrate 2000K -maxrate 2000K -bufsize 512k -f mpegts tcp://127.0.0.1:12345" > ~/.config/hprdpvideo.sh
fi

# Give the user the option to terminate screen sharing
if [[ $UI -eq 1 ]]; then
    if [[ $(which zenity) != "" ]]; then 
	    (zenity --info --text="Close this window to stop sharing" --no-wrap; pkill -TERM -P $(cat /tmp/ffmpeg.pid))&
    fi
fi

# Launch ffmpeg screen sharing
bash ~/.config/hprdpvideo.sh

# Terminate KVM process when ffmpeg stops and feedback the user
kill $kvmpid && notify-send "Screen sharing terminated"
