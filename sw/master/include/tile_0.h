#ifndef TILE_0_H
#define TILE_0_H

// AUTO-GENERATED header file through IPSoCGen
// 26/04/2023 20:36:52

// Master ID  Description
// 0          NoX CPU - Instr. IF
// 1          NoX CPU - LSU IF
// 2          DMA Engine

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0x7fff    32          Boot ROM image
// 1         0x20000    0x3ffff   128         Data RAM
// 2         0x40000    0x5ffff   128         Instruction RAM
// 3         0x60000    0x61fff   8           UART Serial IP
// 4         0x62000    0x63fff   8           Reset Controller
// 5         0x64000    0x67fff   16          RaveNoC Slave IF
// 6         0x68000    0x69fff   8           Machine Timer mtimer
// 7         0x70000    0x71fff   8           DMA Engine Control CSRs
// 8         0x72000    0x73fff   8           IRQ Controller
// 9         0x74000    0x75fff   8           Ethernet Slave RGMII
// 10        0x76000    0x77fff   8           Ethernet InFIFO IF
// 11        0x78000    0x79fff   8           Ethernet OutFIFO IF

//       SLAVE                              BASE ADDRESS
#define  BOOT_ROM_IMAGE_BASE_ADDR           0x0
#define  DATA_RAM_BASE_ADDR                 0x20000
#define  INSTRUCTION_RAM_BASE_ADDR          0x40000
#define  UART_SERIAL_IP_BASE_ADDR           0x60000
#define  RESET_CONTROLLER_BASE_ADDR         0x62000
#define  RAVENOC_SLAVE_IF_BASE_ADDR         0x64000
#define  MACHINE_TIMER_MTIMER_BASE_ADDR     0x68000
#define  DMA_ENGINE_CONTROL_CSRS_BASE_ADDR  0x70000
#define  IRQ_CONTROLLER_BASE_ADDR           0x72000
#define  ETHERNET_SLAVE_RGMII_BASE_ADDR     0x74000
#define  ETHERNET_INFIFO_IF_BASE_ADDR       0x76000
#define  ETHERNET_OUTFIFO_IF_BASE_ADDR      0x78000

#endif