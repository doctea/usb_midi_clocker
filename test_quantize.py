#!/bin/python3

PPQN = 24
BARS_PER_PHRASE = 4
BEATS_PER_BAR = 4
LOOP_LENGTH = PPQN * BARS_PER_PHRASE * BEATS_PER_BAR

def is_bpm_on_multiplier(ticks, multiplier, offset = 0):
    p = PPQN*multiplier
    v = ticks==offset or ticks%p==offset
    return v

def find_nearest_quantized_time(time, quantization):
    print("time to quantize is %s at level %s " % (time, quantization), end='\t')

    ticks_per_quant_level = PPQN/quantization
    print("ticks_per_quant_level=%s" % ticks_per_quant_level, end='\t')
    #quantization = ticks_per_quant_level

    step_num = int(time / ticks_per_quant_level)
    step_start_at_tick = step_num * ticks_per_quant_level
    diff = time - step_start_at_tick

    print("step_num:\t%s\tstep_start_at_tick:\t%s\tdiff:\t%s" % (step_num, step_start_at_tick, diff), end='\t')

    if (diff < ticks_per_quant_level/2):
        print ("diff is less than quantization/2 (%s)" % (ticks_per_quant_level/2), end='\t')
        step = step_start_at_tick
    else:
        print ("diff is more than or equal to quantization/2 (%s)" % (ticks_per_quant_level/2), end='\t')
        step = step_start_at_tick+1 * ticks_per_quant_level
    
    quantized_time = step

    print ("got result %s" % quantized_time)

    return quantized_time % LOOP_LENGTH

# 1 = quantize to beat
# 2 = quantize to half-beat
# 3 = quantize to third-beat
# 4 = quantize to quarter-beat

for x in range(0,LOOP_LENGTH):
    if (is_bpm_on_multiplier(x, 1)):
        print ("BEAT!")
    find_nearest_quantized_time(x, 2)
