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

MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;

static SemaphoreHandle_t xDMAMutex;
static SemaphoreHandle_t xHistogramMutex;

static QueueHandle_t xCmdRecvQ;
static QueueHandle_t xDataReqQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xDMADoneQ;
static QueueHandle_t xProcHistQ;
static QueueHandle_t xProcDoneQ;

uint8_t ulImgSegment[masterETH_PKT_SIZE_BYTES];
static uint32_t ucGlobalHistogram [256]; // Store the global histogram
uint32_t ulNumPixels = 0;

int main( void );

static inline void vprvSendHeartbeat (TickType_t tick) {
  uint8_t* tickPtr = (uint8_t*)&tick;
  uint8_t payload;

  vEthClearOutfifoPtr();
  vEthWriteOutfifoData(tickPtr, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static inline void vprvSendAckEth (void) {
  uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  vEthClearOutfifoPtr();
  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvSendHistEth(void) {
  uint8_t ucBuffer8;
  // Copy the histogram to the Eth INFIFO
  vEthSetSendLenCfg(1024);
  vEthClearOutfifoPtr();
  for (uint16_t i=0; i<256; i++) {
    vEthWriteOutfifoWData(ucGlobalHistogram[i]);
  }
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &ucBuffer8, pdMS_TO_TICKS(500)),"Send UDP Pkt");
  vEthSetSendLenCfg(4);
}


static void vprvProcCmd (void *pvParameters) {
  CmdType_t cmd;
  uint32_t  ulBuffer32;
  uint32_t  ulTest;
  uint8_t   ucBuffer8 = 0;

  // Prepare the DMA descriptor 0 to receive data from Eth
  DMADesc_t xDMACopySeg = {
    .SrcAddr  = (uint32_t*)ethINFIFO_ADDR,
    .DstAddr  = (uint32_t*)ulImgSegment,
    .NumBytes = (masterETH_PKT_SIZE_BYTES),
    .Cfg = {
      .WrMode = DMA_MODE_INCR,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  // Programming the first descriptor
  vDMASetDescCfg(0, xDMACopySeg);

  for (;;) {
    //masterTIMEOUT_INFO(xQueueReceive(xCmdRecvQ, &cmd, pdMS_TO_TICKS(10000)), "Receive cmd");
    xQueueReceive(xCmdRecvQ, &cmd, portMAX_DELAY);
    //dbg("\n\r[%d]Num Pixels = %d", xTaskGetTickCount(), ulNumPixels);
    switch (cmd) {
      case CMD_NONE:
        dbg("\n\r[CMD] None");
        break;
      case CMD_TEST:
        dbg("\n\r[CMD] CMD_TEST");
        vprvSendHeartbeat(xTaskGetTickCount());
        break;
      case CMD_HISTOGRAM:
        xMasterCurStatus = MASTER_STATUS_RUNNING;
        /*dbg("\n\r[CMD] CMD_HISTOGRAM");*/
        /*dbg(".");*/
        // zero the main histogram vector
        for (size_t i=0; i<256; i++)
          ucGlobalHistogram[i] = 0;
        vprvSendAckEth();

        while (ulNumPixels > 0) {
            // Wait to receive image segment of 1KiB
            masterTIMEOUT_INFO(xQueueReceive(xDataReqQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO to recv 1KiB segment");
            /*xQueueReceive(xDataReqQ, &ulBuffer32, portMAX_DELAY);*/
            // Wait DMA to copy from ETH to local memory
            vDMASetDescGo(0);
            masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ulTest, pdMS_TO_TICKS(500)), "TO DMA to copy");
            // --> Ethernet has 1KiB image available
            // Send a msg to the other task to process the histogram
            // Clean the ptr to zero the INFIFO
            vEthClearInfifoPtr();

            masterTIMEOUT_INFO(xQueueSend(xProcHistQ, &ucBuffer8, pdMS_TO_TICKS(500)), "TO waiting to send proc");
            masterTIMEOUT_INFO(xQueueReceive(xProcDoneQ, &ulTest, pdMS_TO_TICKS(500)), "TO histogram to finish");

            // Get the next available slave tile, program the DMA
            // and send the data over the NoC
            /*dbg("\n\rProcessed %d pixels", ulBuffer32);*/
            ulNumPixels -= ulBuffer32;
            // Send request of another 1KiB
            if (ulNumPixels > 0) {
              vprvSendAckEth();
            }
        }

        vIRQSetMaskSingle(irqMASK_ETH_RECV_FULL);
        vIRQClearMaskSingle(irqMASK_ETH_RECV);
        xMasterCurStatus = MASTER_STATUS_IDLE;

        vprvSendAckEth();
        vprvSendHistEth();
        break;
      default:
        dbg("\n\r[CMD] Unknown!");
        break;
    }
  }
}

static void vprvProcImg (void *pvParameters) {
  uint8_t ucBuffer8 = 0;

  for (;;) {
    xQueueReceive(xProcHistQ, &ucBuffer8, portMAX_DELAY);

    xSemaphoreTake(xHistogramMutex, portMAX_DELAY);
    for (size_t i=0; i<masterETH_PKT_SIZE_BYTES; i++) {
      ucGlobalHistogram[ulImgSegment[i]]++;
    }
    xSemaphoreGive(xHistogramMutex);

    masterTIMEOUT_INFO(xQueueSend(xProcDoneQ, &ucBuffer8, pdMS_TO_TICKS(500)), "TO to send done hist!");
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

  xDataReqQ  = xQueueCreate(2, sizeof(uint32_t));
  xDMADoneQ  = xQueueCreate(1, sizeof(uint32_t));
  xEthSentQ  = xQueueCreate(1, sizeof(uint8_t));
  xCmdRecvQ  = xQueueCreate(1, sizeof(CmdType_t));
  xProcHistQ = xQueueCreate(1, sizeof(uint8_t));
  xProcDoneQ = xQueueCreate(1, sizeof(uint8_t));

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
  uint32_t      ucBuffer32 = 0x00;
  BaseType_t    xHigherPriorityTaskWoken;
  switch (xIRQID) {
    case(IRQ_DMA_0_DONE):
      // Clear the done IRQ
      vDMAClearGo();
      xQueueSendFromISR(xDMADoneQ, &ucBuffer32, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    case(IRQ_ETH_RECV):
      if (xMasterCurStatus == MASTER_STATUS_IDLE) {
        CmdType_t cmd_type;
        Cmd_t cmd = {
          .word = ulEthGetRecvData()
        };

        cmd_type = cmd.st.pkt_type;

        switch(cmd_type) {
          case CMD_TEST:
            break;
          case CMD_HISTOGRAM:
            cmd.st.dim_x += 1;
            cmd.st.dim_y += 1;
            ulNumPixels = cmd.st.dim_x*cmd.st.dim_y;
            vIRQSetMaskSingle(irqMASK_ETH_RECV);
            vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);
            break;
          default:
            dbg("CMD unknown");
            break;
        }
        vEthClearInfifoPtr();

        xQueueSendFromISR(xCmdRecvQ, &cmd_type, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
      }
      vEthClearIRQs();
      break;
    case(IRQ_ETH_RECV_FULL):
      uint32_t ulLen = ulEthGetRecvLen();
      xQueueSendFromISR(xDataReqQ, &ulLen, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      vEthClearIRQs();
      break;
    case(IRQ_ETH_SENT):
      xQueueSendFromISR(xEthSentQ, &ucBuffer8, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      vEthClearIRQs();
      break;
      break;
    default:
      dbg("\n\rMcause: %d", ulMcause);
      dbg("\n\rIRQ ID: %d", xIRQID);
      masterCRASH_DBG_INFO("Unexpected IRQ");
      break;
  }

}
