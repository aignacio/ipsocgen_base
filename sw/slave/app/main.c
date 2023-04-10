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

QueueHandle_t xNoCMailboxQ, xPktProcQ, xDMAQ;

TaskHandle_t xProcHandle;

static uint16_t usgSizeSeg;

uint8_t  ucgImgBufferProc [slaveRESERVED_PROC_MEM];
uint16_t ucgHistbuffer [slaveHISTOGRAM_VEC_SIZE];
uint8_t  ucgRunProcessing = 0;

static void vprvProcessNoCPkts(void *pvParameters) {
  uint8_t ucSegSize;

  DMADesc_t xDMACopySegDesc = {
    .SrcAddr  = (uint32_t*)ravenocRD_BUFFER,
    .DstAddr  = (uint32_t*)&ucgImgBufferProc[0],
    .NumBytes = 1024,
    .Cfg = {
      .WrMode = DMA_MODE_INCR,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  // Program the descriptor 0 to copy from the NoC the image seg to the DRAM memory
  vDMASetDescCfg(0, xDMACopySegDesc);

  for(;;) {
    xQueueReceive(xNoCMailboxQ, &ucSegSize, portMAX_DELAY);
    ucgRunProcessing = 1;
    // Launch the DMA for descriptor 0 only
    // Copy from the NoC buffer into our local DRAM memory buffer
    vDMASetDescGo(0);
  }
}

static void vprvZeroHist (void) {
  for (size_t i=0; i<slaveHISTOGRAM_VEC_SIZE; i++) {
    ucgHistbuffer[i] = 0x00;
  }
}

static void vprvCalcHist (uint16_t usSize) {
  for (size_t i=4; i<usSize; i++) {
    ucgHistbuffer[ucgImgBufferProc[i]] += 1;
  }
}

static void vprvImgSeg(void *pvParameters) {
  uint16_t usBuffer16;
  DMADesc_t xDMASendHistDesc = {
    .SrcAddr  = (uint32_t*)&ucgHistbuffer,
    .DstAddr  = (uint32_t*)ravenocWR_BUFFER,
    .NumBytes = 512,
    .Cfg = {
      .WrMode = DMA_MODE_FIXED,
      .RdMode = DMA_MODE_INCR,
      .Enable = DMA_MODE_ENABLE
    }
  };

  // Program the descriptor 1 to copy histogram from DRAM memory to the NoC
  vDMASetDescCfg(1, xDMASendHistDesc);

  // Initialize histogram with zeros
  vprvZeroHist();

  for (;;) {
    xQueueReceive(xPktProcQ, &usBuffer16, portMAX_DELAY);
    //Start processing the image segment
    vprvCalcHist(usgSizeSeg);
    // Each local histogram pixel is 16 bits (because max value could be 1024)
    // thus we have the following calcs:
    // 2-bytes per histogram pixel * 256 possible values = 512 bytes
    vRaveNoCSendNoCMsg(0, 512>>2, ucRaveNoCGetTileID());
    vDMASetDescGo(1);
    // Wait DMA to finish...
    xQueueReceive(xDMAQ, &usBuffer16, portMAX_DELAY);
    // Zero the histogram again
    vprvZeroHist();
  }
}

int main(void) {
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

  // Queue to receive pkt from the NoC
  xNoCMailboxQ = xQueueCreate(1, sizeof(uint16_t));
  // Queue to indicate we have process cause DMA finished fetching the segment
  xPktProcQ = xQueueCreate(1, sizeof(uint16_t));
  // Queue to signal that we have sent the frame back to the master tile
  xDMAQ = xQueueCreate(1, sizeof(uint16_t));

  xReturned = xTaskCreate(
    vprvProcessNoCPkts,
    "Process NoC pkts",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    NULL);

  slaveCHECK_TASK(xReturned);

  xReturned = xTaskCreate(
    vprvImgSeg,
    "Process Image Segment",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xProcHandle);

  slaveCHECK_TASK(xReturned);

  vTaskStartScheduler();
  for(;;);
}

void vSystemIrqHandler(uint32_t ulMcause) {
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  BaseType_t xHigherPriorityTaskWoken;

  switch(xIRQID){
    case(IRQ_DMA_0_DONE):
      vDMAClearGo();

      if (ucgRunProcessing == 1) {
        ucgRunProcessing = 0;
        xQueueSendFromISR(xPktProcQ, &usgSizeSeg, &xHigherPriorityTaskWoken);
      }
      else {
        xQueueSendFromISR(xDMAQ, &usgSizeSeg, &xHigherPriorityTaskWoken);
      }
      if(xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    case(IRQ_RAVENOC_PKT):
      usgSizeSeg = (ucRaveNoCGetNoCPktSize()+1)*4;
      xQueueSendFromISR(xNoCMailboxQ, &usgSizeSeg, &xHigherPriorityTaskWoken);

      if(xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    default:
      dbg("\n\rMcause: %d", ulMcause);
      dbg("\n\rIRQ ID: %d", xIRQID);
      slaveCRASH_DBG_INFO("Unexpected IRQ");
      break;
  }
}
