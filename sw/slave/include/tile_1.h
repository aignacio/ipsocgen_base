#ifndef TILE_1_H
#define TILE_1_H

// AUTO-GENERATED header file through IPSoCGen
// 10/04/2023 21:14:06

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
// 5         0x22000    0x23fff   8           Machine Timer
// 6         0x24000    0x25fff   8           DMA Engine Control CSRs
// 7         0x26000    0x27fff   8           IRQ Controller
// 8         0x28000    0x29fff   8           Reset Controller

//       SLAVE                              BASE ADDRESS
#define  RAVENOC_SLAVE_IF_BASE_ADDR         0x0
#define  INSTRUCTION_RAM_BASE_ADDR          0x8000
#define  DATA_RAM_BASE_ADDR                 0x10000
#define  BOOT_ROM_IMAGE_BASE_ADDR           0x18000
#define  UART_SERIAL_IP_BASE_ADDR           0x20000
#define  MACHINE_TIMER_BASE_ADDR            0x22000
#define  DMA_ENGINE_CONTROL_CSRS_BASE_ADDR  0x24000
#define  IRQ_CONTROLLER_BASE_ADDR           0x26000
#define  RESET_CONTROLLER_BASE_ADDR         0x28000

#endif