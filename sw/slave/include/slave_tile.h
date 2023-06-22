#ifndef SLAVE_TILE_H
#define SLAVE_TILE_H

#include <stdint.h>
#include "tile_1.h"

// [Hardware Memory map]
#define slaveBOOT_ROM_BASE_ADDR       BOOT_ROM_IMAGE_BASE_ADDR
#define slaveDRAM_BASE_ADDR           DATA_RAM_BASE_ADDR
#define slaveIRAM_BASE_ADDR           INSTRUCTION_RAM_BASE_ADDR
#define slaveUART_BASE_ADDR           UART_SERIAL_IP_BASE_ADDR
#define slaveRESET_CTRL_BASE_ADDR     RESET_CONTROLLER_BASE_ADDR
#define slaveRAVENOC_BASE_ADDR        RAVENOC_SLAVE_IF_BASE_ADDR
#define slaveMTIMER_BASE_ADDR         MACHINE_TIMER_BASE_ADDR
#define slaveMTIMER_CMP_ADDR          (slaveMTIMER_BASE_ADDR+0x08)
#define slaveDMA_0_BASE_ADDR          DMA_ENGINE_CONTROL_CSRS_BASE_ADDR
#define slaveIRQ_CTRL_BASE_ADDR       IRQ_CONTROLLER_BASE_ADDR

// [Hardware settings]
#define slaveNOC_CMD_PKT_SIZE         0
#define slaveNOC_SIZE_X               3
#define slaveNOC_SIZE_Y               3
#define slaveNOC_TOTAL_TILES          9
#define slaveHW_CLK_SPEED_HZ          50000000
#define slaveUART_BAUD_RATE           115200
#define slaveRESERVED_PROC_MEM        1024
#define slaveHISTOGRAM_VEC_SIZE       256

// [Macro functions]
#define slaveCHECK_TASK(X)            if(X == pdFALSE) {                            \
                                        dbg("\n\r[FATAL] Cannot create the task!"); \
                                        vCpuSleep();                                \
                                        return 1;                                   \
                                      }

#define slaveCRASH_DBG_INFO(MSG)      dbg("\n\r[FATAL] Illegal behaviour - %s:\n\r File: %s \
                                      \n\r Function: %s \n\r Line:%d", MSG, __FILE__, __FUNCTION__, __LINE__);\
                                      while(1)

#define slaveTICK_TO_MS(tick_delta)   (uint32_t)((tick_delta) >> ((1000/configTICK_RATE_HZ) >> 1))

inline void __attribute__ ((always_inline)) vCpuSleep(void) {
  asm volatile ("wfi");
}

#define slaveTIMEOUT_INFO(X,MSG)     if (X == pdFALSE){ \
                                        dbg("\n\r[TIMEOUT] - %s:\n\r File: %s \
                                            \n\r Function: %s \n\r Line:%d", MSG, __FILE__, __FUNCTION__, __LINE__);\
                                        reset_soc();}

inline void __attribute__ ((always_inline)) reset_soc(void) {
  static volatile uint32_t* const reset_act = (uint32_t*) (masterRESET_CTRL_BASE_ADDR+0x20);
  *reset_act = 1;
}
#endif
