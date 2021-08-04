#!/usr/bin/env python3

import socket
from pynput.mouse import Button, Controller as mouseController
from pynput.keyboard import Key, Controller as kbdController
import pynput

HOST = '127.0.0.1' 
PORT = 12346 

special_codes = {
        32: 'Key.space',
        16777216: 'Key.esc',
        16777217: 'Key.tab',
        16777219: 'Key.backspace',
        16777220: 'Key.enter',
        16777234: 'Key.left',
        16777235: 'Key.up',
        16777236: 'Key.right',
        16777237: 'Key.down',
        16777248: 'Key.shift_l',
        16777249: 'Key.ctrl_l',
        16777299: 'Key.cmd_l',
        16777251: 'Key.alt_l',
        16781571: 'Key.alt_r'
        }

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

            if int(data[2]) in special_codes.keys():
                exec("keyboard." + data[1] + "(" + data[2] + ")")
            else:
                exec("keyboard." + data[1] + "(\'" + data[3] + "\')")

            # if data[1] == 'press':
            #     # print("press", data[2])
            #     try:
            #         if len(data[2]) < 3:
            #             keyboard.press(pynput.keyboard.KeyCode.from_vk(int(data[2])))
            #         else:
            #             exec("keyboard.press(" + data[2] + ")")
            #     except Exception as e:
            #         print(e)
            # elif data[1] == 'release':
            #     # print("release", data[2])
            #     try:
            #         if len(data[2]) < 3:
            #             keyboard.release(pynput.keyboard.KeyCode.from_vk(int(data[2])))
            #         else:
            #             exec("keyboard.release(" + data[2] + ")")
            #     except Exception as e:
            #         print(e)
