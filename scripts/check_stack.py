"""
check_stack.py - post-build script for PlatformIO / Teensy 4.x
After each link, reads _estack and _ebss from the ELF and warns if the
available stack (= _estack - _ebss) is below the configured thresholds.

Thresholds (bytes):
    STACK_WARN_THRESHOLD  - prints a yellow warning
    STACK_ERROR_THRESHOLD - prints a red error and FAILS the build
                            so low-stack builds can never be silently shipped

Background: on this project the stack sits at the top of RAM1 (DTCM) and
grows downward toward _ebss.  Static globals and the saveloadlib label pools
eat RAM1 from below; when the gap shrinks below ~8 KB, setup()'s deep
constructor chains overflow the stack and cause hard-to-diagnose boot crashes.
"""

import os, sys

Import("env")

STACK_WARN_THRESHOLD  = 16 * 1024   # 16 KB - getting tight
STACK_ERROR_THRESHOLD =  8 * 1024   #  8 KB - will crash during setup()


def _read_elf_symbols(elf_path):
    """
    Pure-Python ELF32 symbol table reader.
    Returns a dict of {symbol_name: address} for all defined symbols.
    No external tools required.
    """
    import struct
    symbols = {}
    with open(elf_path, "rb") as f:
        data = f.read()

    # ELF32 header
    if data[:4] != b'\x7fELF':
        return symbols
    ei_class = data[4]
    ei_data  = data[5]   # 1=LE, 2=BE
    if ei_class != 1:    # only ELF32
        return symbols
    endian = "<" if ei_data == 1 else ">"

    # ELF32 header layout:
    #  28: e_phoff(4), 32: e_shoff(4), 36: e_flags(4),
    #  40: e_ehsize(2), 42: e_phentsize(2), 44: e_phnum(2),
    #  46: e_shentsize(2), 48: e_shnum(2), 50: e_shstrndx(2)
    (e_shoff,) = struct.unpack_from(endian + "I", data, 32)
    (e_shentsize, e_shnum, e_shstrndx) = struct.unpack_from(endian + "HHH", data, 46)

    def sh(idx):
        off = e_shoff + idx * e_shentsize
        return struct.unpack_from(endian + "IIIIIIIIII", data, off)

    # Find .symtab and its associated .strtab
    symtab_sh = strtab_sh = None
    for i in range(e_shnum):
        s = sh(i)
        sh_type = s[1]
        if sh_type == 2:    # SHT_SYMTAB
            symtab_sh = s
        # We'll resolve the string table via sh_link after finding symtab

    if symtab_sh is None:
        return symbols

    strtab_sh = sh(symtab_sh[6])  # sh_link = associated string table section index
    sym_offset  = symtab_sh[4]    # sh_offset
    sym_size    = symtab_sh[5]    # sh_size
    sym_entsize = symtab_sh[9]    # sh_entsize (16 for ELF32)
    str_offset  = strtab_sh[4]
    str_size    = strtab_sh[5]

    str_data = data[str_offset: str_offset + str_size]

    if sym_entsize == 0:
        sym_entsize = 16

    for off in range(sym_offset, sym_offset + sym_size, sym_entsize):
        (st_name, st_value, st_size, st_info, st_other, st_shndx) = \
            struct.unpack_from(endian + "IIIBBH", data, off)
        if st_shndx == 0:   # SHN_UNDEF — skip undefined symbols
            continue
        end = str_data.index(b'\x00', st_name)
        name = str_data[st_name:end].decode("ascii", errors="replace")
        symbols[name] = st_value

    return symbols


def check_stack_free(source, target, env):
    elf = str(target[0])
    if not os.path.isfile(elf):
        print("check_stack: ELF not found, skipping check")
        return

    try:
        syms = _read_elf_symbols(elf)
    except Exception as e:
        print(f"check_stack: ELF parse failed ({e}), skipping check")
        return

    estack = syms.get("_estack")
    ebss   = syms.get("_ebss")

    if estack is None or ebss is None:
        print("check_stack: could not find _estack/_ebss symbols, skipping check")
        return

    free = estack - ebss
    kb   = free / 1024.0

    RED    = "\033[1;31m"
    YELLOW = "\033[1;33m"
    GREEN  = "\033[1;32m"
    RESET  = "\033[0m"

    if free < STACK_ERROR_THRESHOLD:
        print(
            f"\n{RED}*** STACK ERROR: only {free} bytes ({kb:.1f} KB) free for stack "
            f"(_estack=0x{estack:08X} _ebss=0x{ebss:08X}). "
            f"Firmware WILL crash at runtime! "
            f"Move more globals to EXTMEM or reduce static data. ***{RESET}\n"
        )
        sys.exit(1)
    elif free < STACK_WARN_THRESHOLD:
        print(
            f"\n{YELLOW}*** STACK WARNING: only {free} bytes ({kb:.1f} KB) free for stack "
            f"(_estack=0x{estack:08X} _ebss=0x{ebss:08X}). "
            f"Consider moving globals to EXTMEM before adding more code. ***{RESET}\n"
        )
    else:
        print(
            f"\n{GREEN}Stack OK: {free} bytes ({kb:.1f} KB) free for stack "
            f"(_estack=0x{estack:08X} _ebss=0x{ebss:08X}){RESET}\n"
        )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", check_stack_free)
