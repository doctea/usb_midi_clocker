#!/bin/python3

PPQN = 24.0

BPM_MINIMUM = 60
BPM_MAXIMUM = 180

for bpm_current in range(BPM_MINIMUM,BPM_MAXIMUM+1):
    micros_per_tick = 1000000.0 * (60.0 / bpm_current * PPQN)
    print("for bpm %s, micros_per_tick is %sus (%ums)" % (bpm_current, micros_per_tick, micros_per_tick/1000000.0))

