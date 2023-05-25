#!/usr/bin/env python3

import socket
from pynput.mouse import Button, Controller as mouseController
from pynput.keyboard import Key, Controller as kbdController
import pynput
import os

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
        16777275: Key.f12,
        8: Key.backspace,
        9: Key.tab,
        13: Key.enter,
        27: Key.esc,
        127: Key.delete,
        1073742050: Key.alt_l,
        1073742051: Key.cmd_l,
        1073742054: Key.alt_r,
        1073742048: Key.ctrl_l,
        1073742052: Key.ctrl_r,
        1073742049: Key.shift_l,
        1073742053: Key.shift_r,
        1073741903: Key.right,
        1073741904: Key.left,
        1073741905: Key.down,
        1073741906: Key.up,
        1073741881: Key.caps_lock,
        1073741899: Key.page_up,
        1073741902: Key.page_down,
        1073741901: Key.end,
        1073741898: Key.home,
        1073741897: Key.insert,
        1073741882: Key.f1,
        1073741883: Key.f2,
        1073741884: Key.f3,
        1073741885: Key.f4,
        1073741886: Key.f5,
        1073741887: Key.f6,
        1073741888: Key.f7,
        1073741889: Key.f8,
        1073741890: Key.f9,
        1073741891: Key.f10,
        1073741892: Key.f11,
        1073741893: Key.f12,
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

        elif data[0] == 'video':
            if data[1] == 'stop':
                print("killing ffmpeg...")
                os.system("/usr/bin/pkill -SIGINT -P $(cat /tmp/ffmpeg.pid)")
                exit(0)
            elif data[1] == 'start':
                if "udp" in data[2]:
                    print("Streaming video to UDP/", data[3])
                    os.system("/usr/bin/kill -SIGUSR2 $(cat /tmp/ffmpeg.pid)")
                else:
                    print("Streaming video over TCP")
                    os.system("/usr/bin/kill -SIGUSR1 $(cat /tmp/ffmpeg.pid)")

