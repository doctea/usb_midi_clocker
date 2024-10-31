#!/bin/sh
~/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-addr2line.exe -a $2 -e .pio/build/$1/firmware_$1.elf
