#!/usr/bin/env python3

import socket
import time
from pynput import mouse, keyboard
import sys

HOST = '127.0.0.1' 
PORT = 12346 
XOFFSET = 1280
YOFFSET = 0
XRES = 1920
YRES = 1080

mouse_down = False
last_mouse_pos = (0,0)
conn = None


def send_msg(msg):
    try:
        msg += " "
        msg = msg.encode().ljust(64, b'\0')
        conn.sendall(msg)
    except Exception as e:
        print(e)


def valid_coords(coord):
    global last_mouse_pos
    x = coord[0]
    y = coord[1]
    last_mouse_pos = (x, y)
    return (x >= XOFFSET \
            and x <= XRES + XOFFSET \
            and y >= YOFFSET \
            and y <= YRES + YOFFSET)


###### MOUSE HOOKS ######

def on_move(x, y):
    if not valid_coords((x, y)):
        return
    if not mouse_down:
        return

    x -= XOFFSET
    y -= YOFFSET

    msg = '{} {} {} {}'.format('mouse', x, y, 'move')
    send_msg(msg)


def on_click(x, y, button, pressed):
    if not valid_coords((x, y)):
        return

    global mouse_down
    x -= XOFFSET
    y -= YOFFSET

    action = ('{}'.format('click' if pressed else 'release'))
    mouse_down = (True if action == 'click' else False)
    msg = '{} {} {} {} {}'.format('mouse', x, y, action, button)
    send_msg(msg)


def on_scroll(x, y, dx, dy):
    if not valid_coords((x, y)):
        return

    x -= XOFFSET
    y -= YOFFSET
    
    msg = 'mouse {} {} scroll {} {}'.format(x, y, dx, dy)
    send_msg(msg)


###### KEYBOARD HOOKS ######

def on_press(key):
    if not valid_coords(last_mouse_pos):
        return

    msg = 'keyboard press {}'.format(key)
    send_msg(msg)

def on_release(key):
    if not valid_coords(last_mouse_pos):
        return

    msg = 'keyboard release {}'.format(key)
    send_msg(msg)

    if key == keyboard.Key.f12:
        # Stop listener
        return False

# Keyboard listener
klistener = keyboard.Listener(
    on_press=on_press,
    on_release=on_release)
klistener.start()

# Mouse listener
mlistener = mouse.Listener(
    on_move=on_move,
    on_click=on_click,
    on_scroll=on_scroll)
mlistener.start()


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()

    while True:
        conn.sendall(b'ping...')
        time.sleep(5)

