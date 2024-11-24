#!/usr/bin/env python3
# Program for building tables for computing colours from FFT bins.
# Copyright 2013, 2023 Boris Gjenero. Released under the MIT license.

import sys, math

def make_hz(last, add, div):
    hz = []
    for i in range(0,last+1):
        hz.append(float((i+add)*44100)/div)
    return hz

lastbin = int(sys.argv[1])
hz = make_hz(lastbin, int(sys.argv[2]), int(sys.argv[3]))
pitch = list(map(lambda x: 0 if x <= 0
                           else 69+12*math.log2(x/440), hz))
midpoint = (pitch[0] + pitch[lastbin])/2

green = list(map(lambda x: max(0.0, 1-abs(midpoint-x)/(midpoint-pitch[0])),
                 pitch))

print("#define RGBM_PIVOTBIN", green.index(max(green)))
print("#define RGBM_USEBINS", lastbin+1)
print("static const double green_tab[RGBM_USEBINS] = {")
print(",\n".join(map(str,green)))
print("};")
