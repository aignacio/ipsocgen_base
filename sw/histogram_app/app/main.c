/* Standard includes. */
#include <string.h>
#include <unistd.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>

#include "riscv_csr_encoding.h"
#include "soc.h"
#include "test.h"
#include "uart.h"
#include "irq_ctrl.h"
#include "dma.h"
#include "eth.h"
#include "soc_types.h"

TaskHandle_t xHandleProcCmd;
TaskHandle_t xHandleProcImg;

static SemaphoreHandle_t xDMAMutex;
static SemaphoreHandle_t xHistogramMutex;

static QueueHandle_t xStartFetchImgQ;
static QueueHandle_t xDataReqQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xDMADoneQ;

int main( void );

static void vprvProcCmd (void *pvParameters) {
  for (;;) {
  }
}

static void vprvProcImg (void *pvParameters) {
  for (;;) {
  }
}


static void vprvSetEth (void) {
  EthLocalCfg_t xLocalCfg = {
    .IPAddr.bytes     = {192, 168,   1, 130},
    .IPGateway.bytes  = {192, 168,   1,   1},
    .SubnetMask.bytes = {255, 255, 255,   0},
    .MACAddr.bytes    = {0x00, 0x00, 0x00, 0x0A, 0x35, 0xA2, 0x34, 0x56} // Xilinx Inc.
  };

  EthCfg_t xSendCfg = {
    .Src              = 1234,
    .Dst              = 1234,
    .Len              = 4,
    .IPAddr.bytes     = {192, 168,   1, 223},
    .MACAddr.bytes    = {0x00, 0x00, 0x04, 0x42, 0x1a, 0x09, 0xaf, 0xc7}
  };

  EthFilterCfg_t xFilterCfg = {
    .Enable  = 1,
    .UDPPort = 1234,
    .IPAddr  = xSendCfg.IPAddr
  };

  vEthSetLocalCfg(xLocalCfg);
  vEthSetSendCfg(xSendCfg);
  vEthSetFilter(xFilterCfg);

  dbg("Ethernet Configured");
}

int main(void){
  BaseType_t xReturned = pdFALSE;

  // Initialize the ETHERNET
  vprvSetEth();

  // IRQs
  // Enable IRQs
  vIRQClearMaskSingle(irqMASK_DMA_0_ERROR);
  vIRQClearMaskSingle(irqMASK_DMA_0_DONE);
  vIRQClearMaskSingle(irqMASK_ETH_RECV);
  vIRQClearMaskSingle(irqMASK_ETH_SENT);
  /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/

  // DMA
  vDMAInit();

  // Initialize the mutex
  xDMAMutex = xSemaphoreCreateMutex();
  xHistogramMutex = xSemaphoreCreateMutex();

  if ((xDMAMutex == NULL) || (xHistogramMutex == NULL)) {
    masterCRASH_DBG_INFO("Cannot create the mutexes");
  }

  xDataReqQ = xQueueCreate(2, sizeof(uint32_t));
  xDMADoneQ = xQueueCreate(1, sizeof(uint32_t));
  xEthSentQ = xQueueCreate(1, sizeof(uint8_t));
  xStartFetchImgQ = xQueueCreate(1, sizeof(CmdType_t));

  xReturned = xTaskCreate(
    vprvProcCmd,
    "Process Commands",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleProcCmd);
  masterCHECK_TASK(xReturned);

  xReturned = xTaskCreate(
    vprvProcImg,
    "Process Images",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleProcImg);
  masterCHECK_TASK(xReturned);

  vTaskStartScheduler();

  // Should never reach here...
  for(;;);

}

void vSystemIrqHandler(uint32_t ulMcause){
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  uint8_t       ucBuffer8 = 0x00;
  BaseType_t    xHigherPriorityTaskWoken;
}
