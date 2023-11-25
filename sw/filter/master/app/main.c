/* Standard includes. */
#include <string.h>
#include <unistd.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>

#include "riscv_csr_encoding.h"
#include "master_tile.h"
#include "uart.h"
#include "irq_ctrl.h"
#include "ravenoc.h"
#include "dma.h"
#include "eth.h"
#include "mpsoc_types.h"

uint32_t ulImageWidth = 0;
uint32_t ulImageHeight = 0;

static SemaphoreHandle_t xDMAMutex;
static SemaphoreHandle_t xHistogramMutex;
static SemaphoreHandle_t xSlaveTileResMutex;

static QueueHandle_t xStartFetchImgQ;
static QueueHandle_t xDataReqQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xDMADoneQ;
static QueueHandle_t xNoCPktFromSlavesQ;
static QueueHandle_t xRowReadyQ;

static uint8_t  ucSlaveTileResVec [masterNOC_TOTAL_TILES];
uint8_t  ucDataBuffer[masterNOC_TOTAL_TILES][IMAGE_WIDTH+4]; // Store the row ready

TaskHandle_t xHandleCopyImg;
TaskHandle_t xHandleRecvData;

MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;
uint32_t     ulNumPixels = 0;
uint32_t     ulTmp;

static uint8_t ucprvGetFreeSlaveTile (void) {
  uint8_t ucAllSlavesBusy = 1;
  uint8_t ucFreeSlaveIndex = 0;
  uint32_t ulTimeoutSlaveCnt = 0;

  while (ucAllSlavesBusy) {
    for (size_t i=1; i<masterNOC_TOTAL_TILES; i++) {
      if (ucSlaveTileResVec[i] == 1) {
        ucFreeSlaveIndex = i;
        ucAllSlavesBusy = 0;
        break;
      }
    }
    ulTimeoutSlaveCnt++;
    if (ulTimeoutSlaveCnt > 100000) {
      masterCRASH_DBG_INFO("[TIMEOUT] No slaves available");
    }
  }

  xSemaphoreTake(xSlaveTileResMutex, portMAX_DELAY);
  ucSlaveTileResVec[ucFreeSlaveIndex] = 0;
  xSemaphoreGive(xSlaveTileResMutex);

  if (ucFreeSlaveIndex == 0) {
    masterCRASH_DBG_INFO("Master cannot be a valid free resource!");
  }

  return ucFreeSlaveIndex;
}

static void ucprvSetFreeSlaveTile (uint8_t index) {
  xSemaphoreTake(xSlaveTileResMutex, portMAX_DELAY);
  ucSlaveTileResVec[index] = 1;
  xSemaphoreGive(xSlaveTileResMutex);
}

static inline void vprvSendAckEth (void) {
  uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  vEthSetSendLenCfg(4);
  vEthClearOutfifoPtr();
  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvSendSegSlave (uint8_t slave_index) {
  uint8_t ucBuffer8;

  // Prepare the DMA descriptor 0 to send the pkt
  DMADesc_t xDMACopyDesc = {
    .SrcAddr  = (uint32_t*)ethINFIFO_ADDR,
    .DstAddr  = (uint32_t*)ravenocWR_BUFFER,
    .NumBytes = SEGMENT_SIZE,
    .Cfg = {
      .WrMode = DMA_MODE_FIXED,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  vRaveNoCSendNoCMsg(slave_index, SEGMENT_SIZE >> 2, CMD_FILTER);
  /*dbg("\n\r%d - %x",slave_index, ulRaveNoCGetHeader(slave_index, ((masterETH_PKT_SIZE_BYTES>>2)-1), CMD_HISTOGRAM));*/
  // Wait to obtain access to the DMA peripheral
  xSemaphoreTake(xDMAMutex, portMAX_DELAY);

  // Programming the first descriptor
  vDMASetDescCfg(0, xDMACopyDesc);

  // Launch the DMA for descriptor 0 only
  vDMASetDescGo(0);
  //Wait DMA completion
  masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucBuffer8, pdMS_TO_TICKS(500)), "Timeout DMA");
  // Release the mutex
  xSemaphoreGive(xDMAMutex);
  /*dbg("sent");*/
  // Clean the ptr to zero the INFIFO
  vEthClearInfifoPtr();
}

static void vprvSendNothing (void) {
  uint8_t payload[4] = {0xFF, 0xFF, 0xFF, 0xFF};

  vEthSetSendLenCfg(4);
  vEthClearOutfifoPtr();

  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvSendRow (uint8_t row_ready) {
  uint8_t ucBuffer8;

  DMADesc_t xDMARow = {
    .SrcAddr  = (uint32_t*)ucDataBuffer[row_ready],
    .DstAddr  = (uint32_t*)ethOUTFIFO_ADDR,
    .NumBytes = (IMAGE_WIDTH+4),
    .Cfg = {
      .WrMode = DMA_MODE_FIXED,
      .RdMode = DMA_MODE_INCR,
      .Enable = DMA_MODE_ENABLE
    }
  };

  vEthSetSendLenCfg(4+IMAGE_WIDTH);
  vEthClearOutfifoPtr();

  xSemaphoreTake(xDMAMutex, portMAX_DELAY);
  vDMASetDescCfg(0, xDMARow);
  vDMASetDescGo(0);
  //Wait DMA completion
  masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucBuffer8, pdMS_TO_TICKS(500)), "Timeout DMA");
  // Release the mutex
  xSemaphoreGive(xDMAMutex);
  vEthSendPkt();
  
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &ucBuffer8, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvCopyImg (void *pvParameters) {
  CmdType_t cmd;
  uint32_t  ulBuffer32;
  uint8_t   ucRowReady;

  for (;;) {
    //masterTIMEOUT_INFO(xQueueReceive(xStartFetchImgQ, &cmd, pdMS_TO_TICKS(10000)), "Receive cmd");
    xQueueReceive(xStartFetchImgQ, &cmd, portMAX_DELAY);
    switch (cmd) {
      case CMD_FILTER:
        xMasterCurStatus = MASTER_STATUS_RUNNING;
        dbg("\n\r[CMD] Filter request received - %d x %d ", ulImageWidth, ulImageHeight);
        vprvSendAckEth();

        for (size_t i=0; i<(IMAGE_HEIGHT); i++) {
          /*dbg("\n\rWaiting for eth data...");*/
          masterTIMEOUT_INFO(xQueueReceive(xDataReqQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO to recv first 240 rows");

          // Get a free slave and send the row - BLOCKING
          /*dbg("\n\rGetting a slave to send");*/
          vprvSendSegSlave(ucprvGetFreeSlaveTile());
          
          // Check the image queue to see if we have rows ready - NON BLOCKING
          // if rows are ready, send back to the host
          /*dbg("\n\rChecking if there rows ready");*/
          if (xQueueReceive(xRowReadyQ, &ucRowReady, pdMS_TO_TICKS(0)) == pdPASS) {
            /*dbg("\n\rSendrow");*/
            vprvSendRow(ucRowReady); 
          }
          else {
            /*dbg("\n\rSendnothing");*/
            // send 0xFF
            vprvSendNothing(); 
          }
        }
                
        uint8_t temp = 0;

        while (temp<3) {
          masterTIMEOUT_INFO(xQueueReceive(xDataReqQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO after 240");
          vEthClearInfifoPtr();
          masterTIMEOUT_INFO(xQueueReceive(xRowReadyQ, &ucRowReady, pdMS_TO_TICKS(500)), "TO to recv rows");
          if (temp == 2)
            xMasterCurStatus = MASTER_STATUS_IDLE;
          vprvSendRow(ucRowReady); 
          temp++;
        }
        dbg("\n\r---------DONE-----------");
        break;
      default:
        break;
    }
  }
}

static void vprvRecvData (void *pvParameters) {
  uint8_t ucSizeSeg;

  DMADesc_t xDMARecvData = {
    .SrcAddr  = (uint32_t*)ravenocRD_BUFFER,
    .DstAddr  = (uint32_t*)ucDataBuffer[0],
    .NumBytes = 0,
    .Cfg = {
      .WrMode = DMA_MODE_INCR,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  // Program the descriptor 1 to copy data from the NoC to the DRAM
  vDMASetDescCfg(1, xDMARecvData);

  for (;;) {
    xQueueReceive(xNoCPktFromSlavesQ, &ucSizeSeg, portMAX_DELAY);
    uint8_t xSlaveFree = (ulRaveNoCGetNoCData() & 0xFF);
    /*dbg("\n\r[vprvRecvData] %d / %d / %d", xSlaveFree, ucRaveNoCGetNoCPktSize(), ucSizeSeg);*/

    xDMARecvData.NumBytes = 4*(ucSizeSeg);
    xDMARecvData.DstAddr = (uint32_t *)ucDataBuffer[xSlaveFree];

    xSemaphoreTake(xDMAMutex, portMAX_DELAY);
    /*dbg("\n\rhere");*/
    vDMASetDescCfg(1, xDMARecvData);
    vDMASetDescGo(1);
    /*dbg("\n\rThere");*/
    masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucSizeSeg, pdMS_TO_TICKS(500)), "Timeout DMA");
    /*dbg("\n\r[vprvRecvData] DONE!");*/
    vRaveNoCIRQAck();
    xSemaphoreGive(xDMAMutex);

    masterTIMEOUT_INFO(xQueueSend(xRowReadyQ, &xSlaveFree, pdMS_TO_TICKS(500)), "Timeout DMA");
    // Free-up the slave availability
    /*dbg("\n\rxSlaveFree %d", xSlaveFree);*/
    ucprvSetFreeSlaveTile(xSlaveFree);
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
    .IPAddr.bytes     = {192, 168,   1, 176},
    .MACAddr.bytes    = {0x00, 0x00, 0xbc, 0x09, 0x1b, 0x98, 0x12, 0x28}
  };

  EthFilterCfg_t xFilterCfg = {
    .Enable  = 1,
    .UDPPort = 1234,
    .IPAddr  = xSendCfg.IPAddr
  };

  vEthSetLocalCfg(xLocalCfg);
  vEthSetSendCfg(xSendCfg);
  vEthSetFilter(xFilterCfg);
}

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

  // Initialize the mutex
  xDMAMutex = xSemaphoreCreateMutex();
  xHistogramMutex = xSemaphoreCreateMutex();
  xSlaveTileResMutex = xSemaphoreCreateMutex();

  if ((xDMAMutex == NULL) || (xHistogramMutex == NULL) || (xSlaveTileResMutex == NULL)) {
    masterCRASH_DBG_INFO("Cannot create the mutexes");
  }

  // Initialize array of slaves availability
  for (size_t i=0; i<masterNOC_TOTAL_TILES; i++)
    ucSlaveTileResVec[i] = 1;

  xDataReqQ = xQueueCreate(2, sizeof(uint32_t));
  xDMADoneQ = xQueueCreate(1, sizeof(uint32_t));
  xEthSentQ = xQueueCreate(1, sizeof(uint8_t));
  xStartFetchImgQ = xQueueCreate(1, sizeof(CmdType_t));
  xNoCPktFromSlavesQ = xQueueCreate(1, sizeof(uint8_t));
  xRowReadyQ = xQueueCreate(masterNOC_TOTAL_TILES, sizeof(uint8_t));

  xReturned = xTaskCreate(
    vprvRecvData,
    "Recv data",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleRecvData);
  masterCHECK_TASK(xReturned);

  xReturned = xTaskCreate(
    vprvCopyImg,
    "Copy image",
    configMINIMAL_STACK_SIZE*4U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleCopyImg);
  masterCHECK_TASK(xReturned);

  vTaskStartScheduler();

  // Should never reach here...
  for(;;);
}

void vSystemIrqHandler(uint32_t ulMcause){
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  uint8_t       ucBuffer8 = 0x00;
  BaseType_t    xHigherPriorityTaskWoken;

  switch (xIRQID) {
    case(IRQ_RAVENOC_PKT):
      ucBuffer8 = ucRaveNoCGetNoCPktSize();
      xQueueSendFromISR(xNoCPktFromSlavesQ, &ucBuffer8, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }

      break;
    case(IRQ_DMA_0_DONE):
      // Clear the done IRQ
      vDMAClearGo();
      xQueueSendFromISR(xDMADoneQ, &ucBuffer8, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    case(IRQ_ETH_RECV):
      if (xMasterCurStatus == MASTER_STATUS_IDLE) {
        uint32_t buffer[3];

        for (size_t i=0; i<3; i++) {
          buffer[i] = ulEthGetRecvData();
        }

        CmdType_t cmd_type = buffer[0];

        ulImageWidth = buffer[1];
        ulImageHeight = buffer[2];

        switch(cmd_type) {
          case CMD_FILTER:
            /*vIRQSetMaskSingle(irqMASK_ETH_RECV);*/
            /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/
            break;
          default:
            dbg("CMD unknown");
            break;
        }
        vEthClearInfifoPtr();

        xQueueSendFromISR(xStartFetchImgQ, &cmd_type, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
      }
      else { 
        uint32_t ulLen = ulEthGetRecvLen();
        xQueueSendFromISR(xDataReqQ, &ulLen, &xHigherPriorityTaskWoken);
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
    default:
      dbg("\n\rMcause: %d", ulMcause);
      dbg("\n\rIRQ ID: %d", xIRQID);
      masterCRASH_DBG_INFO("Unexpected IRQ");
      break;
  }
}
