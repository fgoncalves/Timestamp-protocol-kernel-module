#!/bin/bash
ifconfig $1 up
iwconfig $1 mode ad-hoc essid teste channel 1 ap 02:0C:F1:B5:CC:5D
ifconfig $1 192.168.0.$2
