[![Build](https://github.com/aignacio/ipsocgen_template/actions/workflows/build.yaml/badge.svg?branch=soc_example)](https://github.com/aignacio/ipsocgen_template/actions/workflows/build.yaml)

# Histogram SoC template app

```bash
docker run -it --rm --name ship_hist_app -v $(pwd):/hist_app -w /hist_app aignacio/mpsoc_sw bash
```

Once the container is up and running, proceed executing **all the steps** within the container terminal. In order to run the makefile, first generate with cmake through these cmds:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug|Release 
make all -j24
```
The **Debug** mode enables the `dbg/printf` through the UART while the **Release** option suppress all print statements.

## Programming the hardware with the elf file

If you have the MPSoC HW flashed in a FPGA device and the tile is running the bootloader app, you can easily transfer
the `elf file` with the bootloader script below. Please note that the [bootloader script](scripts/bootloader_elf.py) is
considering the **memory map** address below and the UART speed equal to 8-N-1 / 115200 bits/s.
```bash
python3 scripts/bootloader_elf.py --elf build/histogram_app.elf --device /dev/ttyUSB0
```

Memory map SoC:
```bash
Master ID  Description
0          NoX CPU - Instr. IF
1          NoX CPU - LSU IF
2          DMA Engine

Slave ID  Base Addr  End Addr  Size (KiB)  Description
0         0x0        0x7fff    32          Instruction RAM
1         0x8000     0xbfff    16          Data RAM
2         0x10000    0x17fff   32          Boot ROM image
3         0x18000    0x19fff   8           UART Serial IP
4         0x1a000    0x1bfff   8           Machine Timer
5         0x1c000    0x1dfff   8           DMA Engine CSRs
6         0x1e000    0x1ffff   8           IRQ Controller
7         0x20000    0x21fff   8           Reset Controller
8         0x22000    0x22fff   4           Ethernet CSR
9         0x23000    0x23fff   4           Ethernet InFIFO IF
10        0x24000    0x24fff   4           Ethernet OutFIFO IF
```

## Code Style

In order to support future features, some basic coding std must be followed instead of "freestyle mood". There is not strict coding style but some rules are followed (or at least tried to), and they are leaverage from [FreeRTOS coding style](http://www.openrtos.net/FreeRTOS-Coding-Standard-and-Style-Guide.html). In summary, here is the list of coding guidelines:

### Naming conventions
1. uint32_t --> **ul** prefixed (usigned long) 
2. uint16_t --> **us** prefixed (usigned short) 
3. uint8_t  --> **uc** prefixed (usigned char) 
4. Non-stdint types are prefixed with **x**
  * Unsigned var of non-std type --> **ux** 
  * size_t --> prefixed with **x**
  * Custom type --> **_t** suffixed
5. Enumerated vars --> **e** prefixed 
6. Pointers have additional prefixes --> example: **pu** (pointer to unsigned short)

## Data types

1. Only stdint and defined types are allowed to be used, except when:
  * Char - Used only to hold ASCII char
  * Char * - Used  only when pointing to ASCII strings

### Functions

1. File scope static (private) functions are prefixed with **prv**
2. **API functions** are prefixed with their **return type**
  * void vDMASetGo(void); / uint32_t ulGetSizeString(void);
3. **API functions** start with the name which they are defined
  * void vDMAClearGo(void); / uint8_t ucUARTGetChar(void);

### Macros

1. **MACROS** are **lowercase** prefixed with the file in which they are defined (rest is **uppercase**).
  * masterBOOT_ROM_BASE_ADDR --> master_tile.h
2. **MACRO** words are separeted by **underscores**

### Others

1. [**Do not**](https://www.youtube.com/watch?v=SsoOG6ZeyUI&t) use **tabs**, use **spaces**
  * Tabs are less consistent across different platforms
  * The editorconfig will fmt to use soft tabs following the num of spaces defined there
2. Use the `.editorconfig` file available here
3. Use [include guards](https://en.wikipedia.org/wiki/Include_guard) in the header files to avoid double inclusion
4. Use [CamelCase Style](https://en.wikipedia.org/wiki/Camel_case) when possible
