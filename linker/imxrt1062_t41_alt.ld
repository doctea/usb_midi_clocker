/* with thanks from Beermat on forum.pjrc.com -- alternate linker script that puts functions into FLASHMEM by default, to give us lots more RAM1 to play with (possibly at expense of speed?)! */

MEMORY
{
    ITCM (rwx):  ORIGIN = 0x00000000, LENGTH = 512K
    DTCM (rwx):  ORIGIN = 0x20000000, LENGTH = 512K
    RAM (rwx):   ORIGIN = 0x20200000, LENGTH = 512K
    FLASH (rwx): ORIGIN = 0x60000000, LENGTH = 7936K
    ERAM (rwx):  ORIGIN = 0x70000000, LENGTH = 16384K
}
ENTRY(ImageVectorTable)
SECTIONS
{
    .text.headers : {
        KEEP(*(.flashconfig))
        FILL(0xFF)
        . = ORIGIN(FLASH) + 0x1000;
        KEEP(*(.ivt))
        KEEP(*(.bootdata))
        . = ALIGN(1024);
    } > FLASH
    .text.code : {
        KEEP(*(.startup))
        *(.flashmem*)
        *(EXCLUDE_FILE (
            /* Have to add these to exclude in .text.code, put in text.itcm */
            eeprom.c.o
            EEPROM.c.o
            imxrt.c.o
            interrupt.c.o
            memcpy-armv7m.S.o
            memset.S.o
        ).text*)
        . = ALIGN(4);
        KEEP(*(.init))
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP (*(.init_array))
        __init_array_end = .;
        . = ALIGN(4);
    } > FLASH
    .text.progmem : {
        *(.progmem*)
        . = ALIGN(4);
    } > FLASH
    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .ARM.extab.text* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
        . = ALIGN(4);
    } > FLASH
    .text.itcm : {
        . = . + 32; /* MPU to trap NULL pointer deref */
        *(.fastrun)
        /* Have to add these to keep in text.itcm (excluded above from text.code) */
        *(eeprom.c.o .text*)
        *(EEPROM.c.o .text*)
        *(imxrt.c.o* .text*)
        *(interrupt.c.o .text*)
        *(memcpy-armv7m.S.o .text*)
        *(memset.S.o .text*)
        *(Display* .text*) /* this increases FPS from 9 to 13 */
        /* the rest of these made no noticeable difference */
        *(ILI9341_t3n* .text*)
        *(*arameter* .text*)
        *(*cv_input* .text*)
        *(*USBHost* .text*)
        *(*midi* .text*)
        *(*MIDI* .text*)
        *(*nvelope* .text*)
        *(uClock* .text*)
        *(*menu* .text*)
        *(*handlers* .text*)
        *(*Spi* .text*)
        *(*Voltage* .text*)
        *(MCP23S17* .text*)
        *(*Wire* .text*)
        . = ALIGN(16);
    } > ITCM  AT> FLASH
    .data : {
        *(.endpoint_queue)
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*)))
        KEEP(*(.vectorsram))
    } > DTCM  AT> FLASH
    .bss ALIGN(4) : {
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))
        . = ALIGN(32);
        . = . + 32; /* MPU to trap stack overflow */
    } > DTCM
    .bss.dma (NOLOAD) : {
        *(.hab_log)
        *(.dmabuffers)
        . = ALIGN(32);
    } > RAM
    .bss.extram (NOLOAD) : {
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.externalram)))
        . = ALIGN(32);
    } > ERAM
    .text.csf : {
        FILL(0xFF)
        . = ALIGN(1024);
        KEEP(*(.csf))
        __text_csf_end = .;
    } > FLASH
    _stext = ADDR(.text.itcm);
    _etext = ADDR(.text.itcm) + SIZEOF(.text.itcm);
    _stextload = LOADADDR(.text.itcm);
    _sdata = ADDR(.data);
    _edata = ADDR(.data) + SIZEOF(.data);
    _sdataload = LOADADDR(.data);
    _sbss = ADDR(.bss);
    _ebss = ADDR(.bss) + SIZEOF(.bss);
    _dma_start = ADDR(.bss.dma);
    _dma_end = ADDR(.bss.dma) + SIZEOF(.bss.dma);
    _heap_start = ADDR(.bss.dma) + SIZEOF(.bss.dma);
    _heap_end = ORIGIN(RAM) + LENGTH(RAM);
    _extram_start = ADDR(.bss.extram);
    _extram_end = ADDR(.bss.extram) + SIZEOF(.bss.extram);
    _itcm_block_count = (SIZEOF(.text.itcm) + 0x7FFF) >> 15;
    _flexram_bank_config = 0xAAAAAAAA | ((1 << (_itcm_block_count * 2)) - 1);
    _estack = ORIGIN(DTCM) + ((16 - _itcm_block_count) << 15);
    _flashimagelen = __text_csf_end - ORIGIN(FLASH);
    _teensy_model_identifier = 0x25;
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
}