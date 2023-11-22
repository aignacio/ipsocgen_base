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

static SemaphoreHandle_t xDMAMutex;
static SemaphoreHandle_t xHistogramMutex;
static SemaphoreHandle_t xSlaveTileResMutex;

static QueueHandle_t xStartFetchImgQ;
static QueueHandle_t xDataReqQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xDMADoneQ;
static QueueHandle_t xNoCPktFromSlavesQ;

static uint8_t  ucSlaveTileResVec [masterNOC_TOTAL_TILES];
static uint32_t ucGlobalHistogram [256]; // Store the global histogram
static uint16_t ucDataBuffer [256]; // Store the partial histogram from the slave tiles
static uint8_t  ucDataEthBuffer [1024]; // Used only when local processing without slave tiles

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
    // REMOVE THIS CODE WHEN FINAL - BELOW
    /*if (ucSlaveTileResVec[1] == 1) {*/
      /*ucFreeSlaveIndex = 1;*/
      /*ucAllSlavesBusy = 0;*/
      /*break;*/
    /*}*/
    /*if (ucSlaveTileResVec[2] == 1) {*/
      /*ucFreeSlaveIndex = 2;*/
      /*ucAllSlavesBusy = 0;*/
      /*break;*/
    /*}*/
    // REMOVE THIS CODE WHEN FINAL - ABOVE
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

static void vprvSendHistEth(void) {
  // Wait till all slaves are available
  uint8_t ucAllAvail = 0;

  while (ucAllAvail == 0) {
    ucAllAvail = 1;
    for (size_t i=1; i<masterNOC_TOTAL_TILES; i++) {
      if (ucSlaveTileResVec[i] == 0) {
        ucAllAvail = 0;
      }
    }
  }

  // Copy the histogram to the Eth INFIFO
  vEthSetSendLenCfg(1024);

  vEthClearOutfifoPtr();
  for (uint16_t i=0; i<256; i++) {
    vEthWriteOutfifoWData(ucGlobalHistogram[i]);
  }
  vEthSendPkt();

  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &ucAllAvail, pdMS_TO_TICKS(500)),"Send UDP Pkt");
  vEthSetSendLenCfg(4);
}

static void ucprvSetFreeSlaveTile (uint8_t index) {
  xSemaphoreTake(xSlaveTileResMutex, portMAX_DELAY);
  ucSlaveTileResVec[index] = 1;
  xSemaphoreGive(xSlaveTileResMutex);
}

static inline void vprvSendAckEth (void) {
  uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  vEthClearOutfifoPtr();
  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvCalcHist (uint32_t pixels_4) {
  uint8_t pixel = 0;

  xSemaphoreTake(xHistogramMutex, portMAX_DELAY);
  for (size_t i=0; i<4; i++) {
    pixel = ((pixels_4 >> 8*i) & 0xFF);
    ucGlobalHistogram[pixel]++;
  }
  xSemaphoreGive(xHistogramMutex);
}

static void vprvCalcHistSlave (void) {
  xSemaphoreTake(xHistogramMutex, portMAX_DELAY);

  for (size_t i=0; i<256; i++) {
    ucGlobalHistogram[i] += ucDataBuffer[i];
  }

  xSemaphoreGive(xHistogramMutex);
}

static void vprvSendSegSlave (uint8_t slave_index) {
  uint8_t ucBuffer8;

  // All packets are 1KiB
  vRaveNoCSendNoCMsg(slave_index, ((masterETH_PKT_SIZE_BYTES>>2)-1), CMD_HISTOGRAM);
  /*dbg("\n\r%d - %x",slave_index, ulRaveNoCGetHeader(slave_index, ((masterETH_PKT_SIZE_BYTES>>2)-1), CMD_HISTOGRAM));*/
  // Wait to obtain access to the DMA peripheral
  xSemaphoreTake(xDMAMutex, portMAX_DELAY);
  // Launch the DMA for descriptor 0 only
  vDMASetDescGo(0);
  //Wait DMA completion
  masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucBuffer8, pdMS_TO_TICKS(500)), "Timeout DMA");
  // Release the mutex
  xSemaphoreGive(xDMAMutex);

  // Clean the ptr to zero the INFIFO
  vEthClearInfifoPtr();
}

static void vprvProcLocal (void) {
  uint8_t ucBuffer8;
  xSemaphoreTake(xDMAMutex, portMAX_DELAY);
  // Launch the DMA for descriptor 0 only
  vDMASetDescGo(0);
  //Wait DMA completion
  masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucBuffer8, pdMS_TO_TICKS(500)), "Timeout DMA");
  // Release the mutex
  xSemaphoreGive(xDMAMutex);

  vEthClearInfifoPtr();

  for (size_t i=0; i<1024; i++) {
    ucGlobalHistogram[ucDataEthBuffer[i]]++;
  }
}

static void vprvCopyImg (void *pvParameters) {
  CmdType_t cmd;
  uint32_t  ulBuffer32;
  TickType_t start = xTaskGetTickCount(), stop;
  TickType_t start_recv = xTaskGetTickCount(), stop_recv;
  TickType_t start_proc_slave = xTaskGetTickCount(), stop_proc_slave;

  // Prepare the DMA descriptor 0 to send the pkt
  DMADesc_t xDMACopyDesc = {
    .SrcAddr  = (uint32_t*)ethINFIFO_ADDR,
    .DstAddr  = (uint32_t*)ravenocWR_BUFFER,
    .NumBytes = (masterETH_PKT_SIZE_BYTES-4),
    .Cfg = {
      .WrMode = DMA_MODE_FIXED,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  /*DMADesc_t xDMACopyDesc = {*/
    /*.SrcAddr  = (uint32_t*)ethINFIFO_ADDR,*/
    /*.DstAddr  = (uint32_t*)&ucDataEthBuffer,*/
    /*.NumBytes = (masterETH_PKT_SIZE_BYTES),*/
    /*.Cfg = {*/
      /*.WrMode = DMA_MODE_INCR,*/
      /*.RdMode = DMA_MODE_FIXED,*/
      /*.Enable = DMA_MODE_ENABLE*/
    /*}*/
  /*};*/

  // Programming the first descriptor
  vDMASetDescCfg(0, xDMACopyDesc);

  for (;;) {
    //masterTIMEOUT_INFO(xQueueReceive(xStartFetchImgQ, &cmd, pdMS_TO_TICKS(10000)), "Receive cmd");
    xQueueReceive(xStartFetchImgQ, &cmd, portMAX_DELAY);
    switch (cmd) {
      case CMD_FILTER:
        vprvSendAckEth();
        break;
      case CMD_NONE:
        dbg("\n\r[CMD] None");
        break;
      case CMD_GET_RESULT:
        vprvSendHistEth();
        vIRQSetMaskSingle(irqMASK_ETH_RECV_FULL);
        vIRQClearMaskSingle(irqMASK_ETH_RECV);
        break;
      case CMD_HISTOGRAM:
        /*dbg("\n\r[CMD] Histogram - Pixels - %d", ulNumPixels);*/
        xMasterCurStatus = MASTER_STATUS_RUNNING;

        // zero the main histogram vector
        for (size_t i=0; i<256; i++)
          ucGlobalHistogram[i] = 0;

        vprvSendAckEth();

        start = xTaskGetTickCount();

        while (ulNumPixels > 0) {
          /*dbg("\n\r%d", ulNumPixels);*/

          // Wait to receive image segment of 1KiB
          start_recv = xTaskGetTickCount();
          masterTIMEOUT_INFO(xQueueReceive(xDataReqQ, &ulBuffer32, pdMS_TO_TICKS(500)), "Receive frame");

          // --> Ethernet has 1KiB image available
          // Lets compute the histogram of the first 4x pixels (bytes) because
          // the maximum NoC packet is 1KiB and we have to subtract 4b for the pkt header
          vprvCalcHist(ulEthGetRecvData());

          // Get the next available slave tile, program the DMA
          // and send the data over the NoC
          vprvSendSegSlave(ucprvGetFreeSlaveTile());
          stop_recv = xTaskGetTickCount();
          /*vprvProcLocal();*/

          ulNumPixels -= ulBuffer32;
          // Send request of another 1KiB
          if (ulNumPixels > 0) {
            vprvSendAckEth();
          }
        }

        /*stop = xTaskGetTickCount();*/
        /*dbg("\n\r%d",stop-start);*/

        vIRQSetMaskSingle(irqMASK_ETH_RECV_FULL);
        vIRQClearMaskSingle(irqMASK_ETH_RECV);
        xMasterCurStatus = MASTER_STATUS_IDLE;
        vprvSendAckEth();
        vprvSendHistEth();

        stop = xTaskGetTickCount();
        dbg("Frame %d ms \n\r",((stop-start) << 1));
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
    .DstAddr  = (uint32_t*)&ucDataBuffer,
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

    xDMARecvData.NumBytes = 4*(ucSizeSeg);

    xSemaphoreTake(xDMAMutex, portMAX_DELAY);
    vDMASetDescCfg(1, xDMARecvData);
    vDMASetDescGo(1);
    masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ucSizeSeg, pdMS_TO_TICKS(500)), "Timeout DMA");
    vRaveNoCIRQAck();
    // Sum the histogram
    vprvCalcHistSlave();
    xSemaphoreGive(xDMAMutex);
    // Free-up the slave availability
    /*dbg("\n\rxSlaveFree %d", xSlaveFree);*/
    ucprvSetFreeSlaveTile(xSlaveFree);
  }
}

static void vprvWatchDog (void *pvParameters) {
  for (;;){

    /*dbg("\n\rR/W -> IN=%d/%d - OUT=%d/%d", xEthInfifoPtr().Read, xEthInfifoPtr().Write,*/
                                           /*xEthOutfifoPtr().Read, xEthOutfifoPtr().Write);*/
    dbg("\n\r%d-",xTaskGetTickCount());
    for (size_t i=0; i<masterNOC_TOTAL_TILES; i++)
      dbg("%d", ucSlaveTileResVec[i]);

    /*TaskStatus_t xTaskDetailsRecvData, xTaskDetailsCopyImg;*/
    /*vTaskGetInfo(xHandleRecvData, &xTaskDetailsRecvData, pdTRUE, eInvalid);*/
    /*vTaskGetInfo(xHandleCopyImg, &xTaskDetailsCopyImg, pdTRUE, eInvalid);*/

    /*dbg("\n\r---Tasks---");*/
    /*dbg("\n\r%s - %d - %d",xTaskDetailsRecvData.pcTaskName,*/
                           /*xTaskDetailsRecvData.eCurrentState,*/
                           /*xTaskDetailsRecvData.usStackHighWaterMark);*/
    /*dbg("\n\r%s - %d - %d",xTaskDetailsCopyImg.pcTaskName,*/
                           /*xTaskDetailsCopyImg.eCurrentState,*/
                           /*xTaskDetailsCopyImg.usStackHighWaterMark);*/

    vTaskDelay(pdMS_TO_TICKS(1000));
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
    /*.IPAddr.bytes     = {192, 168,   1, 141},*/
    //.IPAddr.bytes     = {192, 168,   1, 223},
    .IPAddr.bytes     = {192, 168,   1, 176},
    /*.MACAddr.bytes    = {0x00, 0x00, 0x22, 0x20, 0x5c, 0x06, 0x13, 0xb9}*/
    /*.MACAddr.bytes    = {0x00, 0x00, 0x04, 0x42, 0x1a, 0x09, 0xaf, 0xc7}*/
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

  /*xReturned = xTaskCreate(*/
    /*vprvWatchDog,*/
    /*"Watchdog",*/
    /*configMINIMAL_STACK_SIZE*2U,*/
    /*NULL,*/
    /*tskIDLE_PRIORITY+1,*/
    /*NULL);*/
  /*masterCHECK_TASK(xReturned);*/

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
    configMINIMAL_STACK_SIZE*2U,
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
        CmdType_t cmd_type;

        cmd_type = CMD_FILTER;
        uint32_t data[5];

        dbg("\n\rData=");
        for (size_t val=0; val<5; val++) {
          data[val] = ulEthGetRecvData();
          dbg("_%d_", data[val]);
        }
        /*vIRQSetMaskSingle(irqMASK_ETH_RECV);*/
        /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/
        vEthClearInfifoPtr();

        xQueueSendFromISR(xStartFetchImgQ, &cmd_type, &xHigherPriorityTaskWoken);
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
