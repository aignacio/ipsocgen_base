#ifndef TEST_H
#define TEST_H

// AUTO-GENERATED header file through IPSoCGen
// 01/04/2023 19:42:37

// Master ID  Description
// 0          NoX CPU - Instr. I/F
// 1          NoX CPU - LSU I/F
// 2          DMA Engine
// 3          Custom Master ACC.

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0xffff    64          Instruction RAM
// 1         0x10000    0x17fff   32          Data RAM
// 2         0x18000    0x1ffff   32          Boot ROM image
// 3         0x20000    0x21fff   8           UART Serial IP
// 4         0x22000    0x23fff   8           Machine Timer
// 5         0x24000    0x25fff   8           DMA Engine CSRs
// 6         0x26000    0x27fff   8           IRQ Controller
// 7         0x28000    0x29fff   8           Reset Controller
// 8         0x2a000    0x2bfff   8           My custom acc

//       SLAVE                       BASE ADDRESS
#define  INSTRUCTION_RAM_BASE_ADDR   0x0
#define  DATA_RAM_BASE_ADDR          0x10000
#define  BOOT_ROM_IMAGE_BASE_ADDR    0x18000
#define  UART_SERIAL_IP_BASE_ADDR    0x20000
#define  MACHINE_TIMER_BASE_ADDR     0x22000
#define  DMA_ENGINE_CSRS_BASE_ADDR   0x24000
#define  IRQ_CONTROLLER_BASE_ADDR    0x26000
#define  RESET_CONTROLLER_BASE_ADDR  0x28000
#define  MY_CUSTOM_ACC_BASE_ADDR     0x2a000

#endif