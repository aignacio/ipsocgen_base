#ifndef TEST_H
#define TEST_H

// AUTO-GENERATED header file through IPSoCGen
// 11/06/2023 15:14:51

// Master ID  Description
// 0          NoX CPU - Instr. IF
// 1          NoX CPU - LSU IF
// 2          DMA Engine

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0x7fff    32          Instruction RAM
// 1         0x8000     0xbfff    16          Data RAM
// 2         0x10000    0x17fff   32          Boot ROM image
// 3         0x18000    0x19fff   8           UART Serial IP
// 4         0x1a000    0x1bfff   8           Machine Timer
// 5         0x1c000    0x1dfff   8           DMA Engine CSRs
// 6         0x1e000    0x1ffff   8           IRQ Controller
// 7         0x20000    0x21fff   8           Reset Controller
// 8         0x22000    0x22fff   4           Ethernet CSR
// 9         0x23000    0x23fff   4           Ethernet InFIFO IF
// 10        0x24000    0x24fff   4           Ethernet OutFIFO IF

//       SLAVE                          BASE ADDRESS
#define  INSTRUCTION_RAM_BASE_ADDR      0x0
#define  DATA_RAM_BASE_ADDR             0x8000
#define  BOOT_ROM_IMAGE_BASE_ADDR       0x10000
#define  UART_SERIAL_IP_BASE_ADDR       0x18000
#define  MACHINE_TIMER_BASE_ADDR        0x1a000
#define  DMA_ENGINE_CSRS_BASE_ADDR      0x1c000
#define  IRQ_CONTROLLER_BASE_ADDR       0x1e000
#define  RESET_CONTROLLER_BASE_ADDR     0x20000
#define  ETHERNET_CSR_BASE_ADDR         0x22000
#define  ETHERNET_INFIFO_IF_BASE_ADDR   0x23000
#define  ETHERNET_OUTFIFO_IF_BASE_ADDR  0x24000

//       SLAVE                       MEM SIZE BYTES
#define  INSTRUCTION_RAM_SIZE                 32768
#define  DATA_RAM_SIZE                        16384
#define  BOOT_ROM_IMAGE_SIZE                  20480
#define  UART_SERIAL_IP_SIZE                   8192
#define  MACHINE_TIMER_SIZE                    8192
#define  DMA_ENGINE_CSRS_SIZE                  8192
#define  IRQ_CONTROLLER_SIZE                   8192
#define  RESET_CONTROLLER_SIZE                 8192
#define  ETHERNET_CSR_SIZE                     4096
#define  ETHERNET_INFIFO_IF_SIZE               4096
#define  ETHERNET_OUTFIFO_IF_SIZE              4096

#endif