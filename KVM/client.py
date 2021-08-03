#!/usr/bin/env python3

import socket
from pynput.mouse import Button, Controller as mouseController
from pynput.keyboard import Key, Controller as kbdController

HOST = '127.0.0.1' 
PORT = 12346 

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b'HELLO')
    mouse = mouseController()
    keyboard = kbdController()

    should_quit = False

    while not should_quit:
        data = s.recv(64).decode().split()
        # print('Received', data)
        if len(data) == 0:
            break

        if data[0] == 'mouse':
            mouse.position = (int(data[1]), int(data[2]))
            if data[3] == 'click' :
                exec("mouse.press(" + data[4] + ")")
                # mouse.click(Button.left, 1)
            elif data[3] == 'release':
                exec("mouse.release(" + data[4] + ")")
            elif data[3] == 'scroll':
                mouse.scroll(int(data[4]), int(data[5]))
            elif data[3] == 'move':
                pass # mouse.move(int(data[1]), int(data[2]))
        elif data[0] == 'keyboard':
            if data[1] == 'press':
                exec("keyboard.press(" + data[2] + ")")
            elif data[1] == 'release':
                exec("keyboard.release(" + data[2] + ")")
