/* Standard includes. */
#include <string.h>
#include <unistd.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>

#include "riscv_csr_encoding.h"
#include "slave_tile.h"
#include "uart.h"
#include "irq_ctrl.h"
#include "ravenoc.h"
#include "dma.h"
#include "mpsoc_types.h"

int main (void) {
  BaseType_t xReturned = pdFALSE;

  // Initialize the ETHERNET
  vprvSetEth();

  // RaveNoC
  // Initialise NoC & set IRQ to PULSE HEAD Flit type
  vRaveNoCInitNoCLUT();
  vRaveNoCSetIRQMux(RAVENOC_MUX_PULSE_HEAD_FLIT);
  vRaveNoCPrintNoCSettings();

  // IRQs
  // Enable IRQs
  vIRQClearMaskSingle(irqMASK_RAVENOC_PKT);
  vIRQClearMaskSingle(irqMASK_DMA_0_ERROR);
  vIRQClearMaskSingle(irqMASK_DMA_0_DONE);
  vIRQClearMaskSingle(irqMASK_ETH_RECV);
  vIRQClearMaskSingle(irqMASK_ETH_SENT);
  /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/

  // DMA
  vDMAInit();
}
