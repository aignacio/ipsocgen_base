#ifndef TILE_0_H
#define TILE_0_H

// AUTO-GENERATED header file through IPSoCGen
// 10/04/2023 15:13:24

// Master ID  Description
// 0          NoX CPU - Instr. IF
// 1          NoX CPU - LSU IF
// 2          DMA Engine

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0x7fff    32          RaveNoC Slave IF
// 1         0x8000     0xffff    32          Instruction RAM
// 2         0x10000    0x13fff   16          Data RAM
// 3         0x18000    0x1ffff   32          Boot ROM image
// 4         0x20000    0x21fff   8           UART Serial IP
// 5         0x22000    0x23fff   8           Machine Timer (mtimer)
// 6         0x24000    0x25fff   8           DMA Engine Control CSRs
// 7         0x26000    0x27fff   8           IRQ Controller
// 8         0x28000    0x29fff   8           Reset Controller
// 9         0x2a000    0x2afff   4           Ethernet Slave RGMII
// 10        0x2b000    0x2bfff   4           Ethernet InFIFO IF
// 11        0x2c000    0x2cfff   4           Ethernet OutFIFO IF

//       SLAVE                              BASE ADDRESS
#define  RAVENOC_SLAVE_IF_BASE_ADDR         0x0
#define  INSTRUCTION_RAM_BASE_ADDR          0x8000
#define  DATA_RAM_BASE_ADDR                 0x10000
#define  BOOT_ROM_IMAGE_BASE_ADDR           0x18000
#define  UART_SERIAL_IP_BASE_ADDR           0x20000
#define  MACHINE_TIMER_(MTIMER)_BASE_ADDR   0x22000
#define  DMA_ENGINE_CONTROL_CSRS_BASE_ADDR  0x24000
#define  IRQ_CONTROLLER_BASE_ADDR           0x26000
#define  RESET_CONTROLLER_BASE_ADDR         0x28000
#define  ETHERNET_SLAVE_RGMII_BASE_ADDR     0x2a000
#define  ETHERNET_INFIFO_IF_BASE_ADDR       0x2b000
#define  ETHERNET_OUTFIFO_IF_BASE_ADDR      0x2c000

#endif