#!/bin/python3


#mults = [ 4, 2, 1, 0.5 ]
clock_multiplier = [ 4, 4, 4, 4, 4, 4, 4, 4 ]
clock_delay = [ 0, 1, 2, 3, 4, 5, 6, 7 ]
width = 16 

PPQN = 24

def is_bpm_on_multiplier(ticks, multiplier, offset = 0):
    p = PPQN*multiplier
    v = ticks==offset or ticks%p==offset
    return v

for clock,multiplier in enumerate(clock_multiplier):
    print("multiplier %s no delay:\t\t" % multiplier, end="")
    for x in range(0,width):
        #if (x % multiplier == 0):
        if is_bpm_on_multiplier(x*PPQN, multiplier):
            print("x ", end="")
        else:
            print("_ ", end="")
    print()
    print ("multiplier %s with delay %s:\t" % (multiplier, clock_delay[clock]), end="")
    """offset = (delay[c]-i) % width
    print("modded offset for %s-%s is %s" % (delay[c], i, offset))"""
    for x in range(0,width):
        #offset = (delay[c]-x) % width
        #if ((clock_delay[clock]-x) % width == 0):
        if is_bpm_on_multiplier(PPQN*(clock_delay[clock]-x), multiplier):
            print("x ", end="")
        else:
            print("_ ", end="")
    print("\n--")

print()
