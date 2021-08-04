#!/usr/bin/env python3

import socket
from pynput.mouse import Button, Controller as mouseController
from pynput.keyboard import Key, Controller as kbdController
import pynput

HOST = '127.0.0.1' 
PORT = 12346 

special_codes = {
        32: Key.space,
        16777216: Key.esc,
        16777217: Key.tab,
        16777219: Key.backspace,
        16777220: Key.enter,
        16777234: Key.left,
        16777235: Key.up,
        16777236: Key.right,
        16777237: Key.down,
        16777248: Key.shift_l,
        16777249: Key.ctrl_l,
        16777299: Key.cmd_l,
        16777251: Key.alt_l,
        16781571: Key.alt_r,
        16777238: Key.page_up,
        16777239: Key.page_down,
        16777223: Key.delete,
        16777232: Key.home,
        16777233: Key.end,
        16777301: Key.menu,
        16777252: Key.caps_lock,
        16777264: Key.f1,
        16777265: Key.f2,
        16777266: Key.f3,
        16777267: Key.f4,
        16777268: Key.f5,
        16777269: Key.f6,
        16777270: Key.f7,
        16777271: Key.f8,
        16777272: Key.f9,
        16777273: Key.f10,
        16777274: Key.f11,
        16777275: Key.f12
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

            if data[1] == 'press':
                action = keyboard.press
            else:
                action = keyboard.release

            try:
                if int(data[2]) in special_codes:
                    action(special_codes[int(data[2])])
                else:
                    action(pynput.keyboard.KeyCode.from_vk(int(data[2])))
            except Exception as e:
                print("ERROR:", e)

