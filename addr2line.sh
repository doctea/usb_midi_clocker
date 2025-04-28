#!/bin/sh

FIRMWARE=.pio/build/$1/firmware_$1.elf
ADDR=$2

echo "Using firmware file '$FIRMWARE' and address $2.."

#~/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-addr2line.exe -a $2 -e .pio/build/$1/firmware_$1.elf

/mnt/c/Users/doctea/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-addr2line.exe -a $2 -e .pio/build/$1/firmware_$1.elf


