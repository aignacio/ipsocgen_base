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

