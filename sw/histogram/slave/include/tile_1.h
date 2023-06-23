#ifndef TILE_1_H
#define TILE_1_H

// AUTO-GENERATED header file through IPSoCGen
// 30/04/2023 19:49:30

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
// 6         0x68000    0x69fff   8           Machine Timer
// 7         0x70000    0x71fff   8           DMA Engine Control CSRs
// 8         0x72000    0x73fff   8           IRQ Controller

//       SLAVE                              BASE ADDRESS
#define  BOOT_ROM_IMAGE_BASE_ADDR           0x0
#define  DATA_RAM_BASE_ADDR                 0x20000
#define  INSTRUCTION_RAM_BASE_ADDR          0x40000
#define  UART_SERIAL_IP_BASE_ADDR           0x60000
#define  RESET_CONTROLLER_BASE_ADDR         0x62000
#define  RAVENOC_SLAVE_IF_BASE_ADDR         0x64000
#define  MACHINE_TIMER_BASE_ADDR            0x68000
#define  DMA_ENGINE_CONTROL_CSRS_BASE_ADDR  0x70000
#define  IRQ_CONTROLLER_BASE_ADDR           0x72000

//       SLAVE                           MEM SIZE BYTES
#define  BOOT_ROM_IMAGE_SIZE                      20480
#define  DATA_RAM_SIZE                            16384
#define  INSTRUCTION_RAM_SIZE                     20480
#define  UART_SERIAL_IP_SIZE                       8192
#define  RESET_CONTROLLER_SIZE                     8192
#define  RAVENOC_SLAVE_IF_SIZE                    16384
#define  MACHINE_TIMER_SIZE                        8192
#define  DMA_ENGINE_CONTROL_CSRS_SIZE              8192
#define  IRQ_CONTROLLER_SIZE                       8192

#endif