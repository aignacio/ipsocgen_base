# IPSoC gen - SoC template project

In this repository you will find a template project of a SoC configuration using IPSoC gen.

## Assumptions

It is quite complex to have a generic project for a SoC because it requires you to support multiple configurations for different masters and peripherals across the set of options. Due to this characteristic, some assumptions are made here to have a basic but complete template project to allow any user to start playing with. Down below are list these assumptions considering a basic SoC:

1. The repository is based on an existing SoC configuration (see below for more info);
2. [NoX RISC-V CPU](https://github.com/aignacio/nox) is used here as the main core;
3. The system has 2x sets of RAMs, one for instruction and another one for data;
4. Some of the IOs at the top module ([rtl/test.sv](rtl/test.sv)) are used to drive clock and reset (arty_a7_100MHz/arty_a7_rst_n) and they have to match with the ones mentioned in the testbench ([tb/cpp/testbench.cpp](tb/cpp/testbench.cpp));
5. The designs contains a boot ROM ([sw/bootloader](sw/bootloader)) program which is compiled before starting of design compilation once it is required in order to build the final SoC;
6. User has access to docker and will compile the design using the provided images (requires internet);

## SoC configuration

```bash
Master ID  Description
0          NoX CPU - Instr. I/F
1          NoX CPU - LSU I/F
2          DMA Engine
3          Custom Master ACC.
                                                            
Slave ID  Base Addr  End Addr  Size (KiB)  Description
0         0x0        0xffff    64          Instruction RAM
1         0x10000    0x17fff   32          Data RAM
2         0x18000    0x1ffff   32          Boot ROM image
3         0x20000    0x21fff   8           UART Serial IP
4         0x22000    0x23fff   8           Machine Timer
5         0x24000    0x25fff   8           DMA Engine CSRs
6         0x26000    0x27fff   8           IRQ Controller
7         0x28000    0x29fff   8           Reset Controller
8         0x2a000    0x2bfff   8           My custom acc
```
