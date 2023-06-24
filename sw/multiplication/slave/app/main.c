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

#define TOTAL_SIZE_KIB  8

TaskHandle_t xHandleRecvData;

static QueueHandle_t xNoCMailboxQ;

uint16_t gSizeKiB = 0;
uint32_t gArray [256*TOTAL_SIZE_KIB];
uint32_t gTotal = 0;

static void vprvProcVec (void *pvParameters);

int main (void) {
  BaseType_t xReturned = pdFALSE;

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

  // DMA
  vDMAInit();

  // ------------------------------------
  // Queues
  // ------------------------------------
  xNoCMailboxQ = xQueueCreate(1, sizeof(uint8_t));

  for (size_t i=0; i<TOTAL_SIZE_KIB*256; i++) {
    gArray[i] = 1;
  }

  xReturned = xTaskCreate(
    vprvProcVec,
    "Process the vector",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleRecvData);
  slaveCHECK_TASK(xReturned);

  vTaskStartScheduler();

  // Should never reach here...
  for(;;);
}

static void vprvProcVec (void *pvParameters) {
  uint8_t ucBuffer8;
  uint32_t header, times, factor;

  for (;;) {
    dbg("\n\rTask vprvProcVec ready");

    xQueueReceive(xNoCMailboxQ, &ucBuffer8, portMAX_DELAY);

    header = ulRaveNoCGetNoCData();
    times  = ulRaveNoCGetNoCData();
    factor = ulRaveNoCGetNoCData();

    dbg("\n\rHeader = %x / Factor= %u / Times=%u", header, factor, times);

    for (size_t i=0; i<times; i++) {
      for (size_t j=0; j<TOTAL_SIZE_KIB*256; j++) {
        gTotal += gArray[j]*factor;
      }
    }
    vRaveNoCSendNoCMsg(0, 0, ucRaveNoCGetTileID());
    dbg("\n\rgTotal: %x", gTotal);
  }
}

void vSystemIrqHandler(uint32_t ulMcause){
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  BaseType_t    xHigherPriorityTaskWoken;
  uint8_t       ucBuffer8 = 0x00;

  switch (xIRQID) {
    case(IRQ_RAVENOC_PKT):
      xQueueSendFromISR(xNoCMailboxQ, &ucBuffer8, &xHigherPriorityTaskWoken);

      if(xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    default:
      dbg("\n\rDefault!");
      break;
  }
}
// ----------------------------- Complementary fn --------------------------------
