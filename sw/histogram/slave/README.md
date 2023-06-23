[![Build](https://github.com/aignacio/mpsoc_sw/actions/workflows/build.yaml/badge.svg?branch=slave_tile)](https://github.com/aignacio/mpsoc_sw/actions/workflows/build.yaml)

# MPSoC - Slave Tile SW

This branch contains the code for the **MPSoC** Slave tile **only**.

Run with a pre-built container with all the tools, including toolchain and python:
```bash
docker run -it --rm --name ship_slave_tile -v $(pwd):/slave_tile -w /slave_tile aignacio/mpsoc bash
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
python3 scripts/bootloader_elf.py --elf build/slave_tile.elf --device /dev/ttyUSB1
```

Memory map SoC:
```bash
 ____ _____________ _____________ _____________ ___________ _________
|_ID_|_Description_|___End Addr__|__Start Addr__|_Reserved_|__Used___|
| 08 |  IRQ CTRL   | 0x0007_4FFF | 0x0007_2000  |  008 KiB | 008 KiB |
| 07 |  DMA CSRs   | 0x0007_1FFF | 0x0007_0000  |  008 KiB | 008 KiB |
| 06 |   MTIMER    | 0x0006_9FFF | 0x0006_8000  |  008 KiB | 008 KiB |
| 05 |    NOC      | 0x0006_7FFF | 0x0006_4000  |  016 KiB | 016 KiB |
| 04 |  RST CTRL   | 0x0006_3FFF | 0x0006_2000  |  008 KiB | 008 KiB |
| 03 |    UART     | 0x0006_1FFF | 0x0006_0000  |  008 KiB | 008 KiB |
| 02 |    IRAM     | 0x0005_FFFF | 0x0004_0000  |  128 KiB | 016 KiB |
| 01 |    DRAM     | 0x0003_FFFF | 0x0002_0000  |  128 KiB | 008 KiB |
| 00 |   BOOTROM   | 0x0000_7FFF | 0x0000_0000  |  032 KiB | 017 KiB |
|____|_____________|_____________|______________|__________|_________|
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
