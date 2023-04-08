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

The SoC configuration used in this template follows the below memory map. This configuration is available [here](https://github.com/aignacio/ipsocgen/blob/main/ipsocgen/examples/template_soc.yaml) and its output can be generated using the following command:
```bash
ipsocgen -c template_soc.yaml
```

### Memory Map

```bash
Master ID  Description
0          NoX CPU - Instr. I/F
1          NoX CPU - LSU I/F
2          DMA Engine
3          Custom Master ACC.

Slave ID  Base Addr  End Addr  Size (KiB)  Description
0         0x0        0x3fff    16          Instruction RAM
1         0x4000     0x5fff    8           Data RAM
2         0x8000     0xffff    32          Boot ROM image
3         0x10000    0x11fff   8           UART Serial IP
4         0x12000    0x13fff   8           Machine Timer
5         0x14000    0x15fff   8           DMA Engine CSRs
6         0x16000    0x17fff   8           IRQ Controller
7         0x18000    0x19fff   8           Reset Controller
8         0x1a000    0x1bfff   8           My custom acc
```

## Running the simulation

To run the compilation and generation of the executable SoC through **Verilator**, please run the following commands:
```bash
make all
make run # Once the build passes with no errors
```
After the end of the simulation, you should be able to read the file `soc_log.txt` that contains the output of the program execution.
```bash
  _   _       __  __
 | \ | |  ___ \ \/ /
 |  \| | / _ \ \  /
 | |\  || (_) |/  \
 |_| \_| \___//_/\_\
 NoX RISC-V Core RV32I

 CSRs:
 mstatus 	0x1880
 misa    	0x40000100
 mhartid 	0x0
 mie     	0x0
 mip     	0x0
 mtvec   	0x101
 mepc    	0x0
 mscratch	0x0
 mtval   	0x0
 mcause  	0x0
 cycle   	358
```

## Changing the design files

If you want to test a different design, some steps might be required to be performed depending on the specific use case, please check the following instructions if you intend to perform any changes in this design.

* The name of your SoC needs to be updated in different places (if different from the example **test**)
  * `makefile` -> **PROJECT_NAME** variable;
  * The *testbench* ([tb/cpp/testbench.cpp](tb/cpp/testbench.cpp)) needs to be updated in different locations, try to search and replace *test* keyword by your project name, paying attention to not swap words like testbench;
* If your design contains RAMs (Instr/Data) used by the CPU and you want to be able to load different programs (.elf), please update the following variables within the `makefile`
```bash
IRAM_KB_SIZE	?=	16
DRAM_KB_SIZE	?=	8
IRAM_ADDR		?=	0x00000000
DRAM_ADDR		?=	0x00004000
```
* Once you generate your design with IP SoC Gen, copy across to this project the files inside **rtl** and the **sw** folders to the local `rtl/` and `sw/common/`, this will require an update as well in the 2x software programs (bootloader & hello_world) to indicate the correct header file. By default these 2x programs will require a header file with reset controller (for printf, see current design) and 2x memories IRAM/DRAM;
