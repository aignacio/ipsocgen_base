#ifndef TEST_H
#define TEST_H

// AUTO-GENERATED header file through IPSoCGen
// 08/04/2023 13:22:11

// Master ID  Description
// 0          NoX CPU - Instr. I/F
// 1          NoX CPU - LSU I/F
// 2          DMA Engine
// 3          Custom Master ACC.

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0x3fff    16          Instruction RAM
// 1         0x4000     0x5fff    8           Data RAM
// 2         0x8000     0xffff    32          Boot ROM image
// 3         0x10000    0x11fff   8           UART Serial IP
// 4         0x12000    0x13fff   8           Machine Timer
// 5         0x14000    0x15fff   8           DMA Engine CSRs
// 6         0x16000    0x17fff   8           IRQ Controller
// 7         0x18000    0x19fff   8           Reset Controller
// 8         0x1a000    0x1bfff   8           My custom acc

//       SLAVE                       BASE ADDRESS
#define  INSTRUCTION_RAM_BASE_ADDR   0x0
#define  DATA_RAM_BASE_ADDR          0x4000
#define  BOOT_ROM_IMAGE_BASE_ADDR    0x8000
#define  UART_SERIAL_IP_BASE_ADDR    0x10000
#define  MACHINE_TIMER_BASE_ADDR     0x12000
#define  DMA_ENGINE_CSRS_BASE_ADDR   0x14000
#define  IRQ_CONTROLLER_BASE_ADDR    0x16000
#define  RESET_CONTROLLER_BASE_ADDR  0x18000
#define  MY_CUSTOM_ACC_BASE_ADDR     0x1a000

#endif