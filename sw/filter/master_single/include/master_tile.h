#ifndef MASTER_TILE_H
#define MASTER_TILE_H

#include <stdint.h>
#include "tile_0.h"

// [Hardware Memory map]
#define masterETH_OUTFIFO_BASE_ADDR   ETHERNET_OUTFIFO_IF_BASE_ADDR
#define masterETH_INFIFO_BASE_ADDR    ETHERNET_INFIFO_IF_BASE_ADDR
#define masterETH_CSR_BASE_ADDR       ETHERNET_SLAVE_RGMII_BASE_ADDR
#define masterIRQ_CTRL_BASE_ADDR      IRQ_CONTROLLER_BASE_ADDR
#define masterDMA_0_BASE_ADDR         DMA_ENGINE_CONTROL_CSRS_BASE_ADDR
#define masterMTIMER_BASE_ADDR        MACHINE_TIMER_MTIMER_BASE_ADDR
#define masterMTIMER_CMP_ADDR         (masterMTIMER_BASE_ADDR+0x08)
#define masterRAVENOC_BASE_ADDR       RAVENOC_SLAVE_IF_BASE_ADDR
#define masterRESET_CTRL_BASE_ADDR    RESET_CONTROLLER_BASE_ADDR
#define masterUART_BASE_ADDR          UART_SERIAL_IP_BASE_ADDR
#define masterIRAM_BASE_ADDR          INSTRUCTION_RAM_BASE_ADDR
#define masterDRAM_BASE_ADDR          DATA_RAM_BASE_ADDR
#define masterBOOT_ROM_BASE_ADDR      BOOT_ROM_IMAGE_BASE_ADDR

// [Hardware settings]
#define masterNOC_CMD_PKT_SIZE        0
#define masterNOC_SIZE_X              3
#define masterNOC_SIZE_Y              3
#define masterNOC_TOTAL_TILES         9
#define masterHW_CLK_SPEED_HZ         50000000
#define masterUART_BAUD_RATE          115200
#define masterETH_PKT_SIZE_BYTES      1024
#define IMAGE_WIDTH                   320
#define IMAGE_HEIGHT                  240
#define KERNEL_SIZE                   3
#define SEGMENT_SIZE                  ((3*(IMAGE_WIDTH+2))+4+2)
#define N_SEG                         2

// [Macro functions]
#define masterCHECK_TASK(X)           if(X == pdFALSE) {                            \
                                        dbg("\n\r[FATAL] Cannot create the task!"); \
                                        vCpuSleep();                                \
                                        return 1;                                   \
                                      }

#define masterCRASH_DBG_INFO(MSG)     dbg("\n\r[FATAL] Illegal behaviour - %s:\n\r File: %s \
                                      \n\r Function: %s \n\r Line:%d", MSG, __FILE__, __FUNCTION__, __LINE__);\
                                      while(1)

#define masterTICK_TO_MS(tick_delta)  (uint32_t)((tick_delta) << ((1000/configTICK_RATE_HZ)>>1))

inline void __attribute__ ((always_inline)) vCpuSleep(void) {
  asm volatile ("wfi");
}

#define masterTIMEOUT_INFO(X,MSG)     if (X == pdFALSE){ \
                                        dbg("\n\r[TIMEOUT] - %s:\n\r File: %s \n\r Function: %s \n\r Line:%d", MSG, __FILE__, __FUNCTION__, __LINE__);\
                                        reset_soc();}

inline void __attribute__ ((always_inline)) reset_soc(void) {
  static volatile uint32_t* const reset_act = (uint32_t*) (masterRESET_CTRL_BASE_ADDR+0x20);
  *reset_act = 1;
}

#endif
