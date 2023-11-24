/* Standard includes. */
#include <string.h>
#include <unistd.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <semphr.h>

#include "riscv_csr_encoding.h"
#include "tile_0.h"
#include "master_tile.h"
#include "uart.h"
#include "irq_ctrl.h"
#include "dma.h"
#include "eth.h"
#include "mpsoc_types.h"

TaskHandle_t xHandleProcCmd;
TaskHandle_t xHandleProcImg;

MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;

static SemaphoreHandle_t xDMAMutex;
static SemaphoreHandle_t xHistogramMutex;

static QueueHandle_t xCmdRecvQ;
static QueueHandle_t xDataReqQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xDMADoneQ;
static QueueHandle_t xProcFilterQ;
static QueueHandle_t xProcDoneQ;

uint32_t ulImageWidth = 0;
uint32_t ulImageHeight = 0;


uint8_t  ucImgSegment[SEGMENT_SIZE];
uint8_t  ucIndexSeg = 0;

uint8_t  ucImgFiltered[4+IMAGE_WIDTH];
uint32_t ulRowCount = 0;

// Prepare the DMA descriptor 0 to receive data from Eth
DMADesc_t xDMACopyFilt = {
    .SrcAddr  = (uint32_t*)ucImgFiltered,
    .DstAddr  = (uint32_t*)ethOUTFIFO_ADDR,
    .NumBytes = (SEGMENT_SIZE),
    .Cfg = {
      .WrMode = DMA_MODE_FIXED,
      .RdMode = DMA_MODE_INCR,
      .Enable = DMA_MODE_ENABLE
  }
};

int main( void );

static inline void vprvSendHeartbeat (TickType_t tick) {
  uint8_t* tickPtr = (uint8_t*)&tick;
  uint8_t payload;

  vEthClearOutfifoPtr();
  vEthWriteOutfifoData(tickPtr, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static inline void vprvSendRowFilter (void) {
  uint8_t dummy;

  vEthClearOutfifoPtr();
  vEthSetSendLenCfg(IMAGE_WIDTH);
  /*dbg("\n\rSending %d bytes", SEGMENT_SIZE); */
  /*xDMACopyFilt.SrcAddr = (uint32_t*)ucImgFiltered[ucIndexFilter];*/
  /*vDMASetDescCfg(1, xDMACopyFilt);*/
  vDMASetDescGo(1);
  masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &dummy, pdMS_TO_TICKS(500)), "TO DMA to copy");
 
  /*if (ucIndexFilter == (N_SEG-1)){*/
    /*ucIndexFilter = 0;*/
  /*}*/
  /*else {*/
    /*ucIndexFilter += 1;*/
  /*}*/
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &dummy, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static inline void vprvSendAckEth (void) {
  uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  vEthSetSendLenCfg(4);
  vEthClearOutfifoPtr();
  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

static void vprvProcCmd (void *pvParameters) {
  CmdType_t cmd;
  uint32_t  ulBuffer32;
  uint32_t  ulTest;

  // Prepare the DMA descriptor 0 to receive data from Eth
  DMADesc_t xDMACopySeg = {
    .SrcAddr  = (uint32_t*)ethINFIFO_ADDR,
    .DstAddr  = (uint32_t*)ucImgSegment,
    .NumBytes = (SEGMENT_SIZE),
    .Cfg = {
      .WrMode = DMA_MODE_INCR,
      .RdMode = DMA_MODE_FIXED,
      .Enable = DMA_MODE_ENABLE
    }
  };

  // Programming the first descriptor
  vDMASetDescCfg(0, xDMACopySeg);

  for (;;) {
    xQueueReceive(xCmdRecvQ, &cmd, portMAX_DELAY);
    switch (cmd) {
      case CMD_FILTER:
        xMasterCurStatus = MASTER_STATUS_RUNNING;
        /*dbg("\n\r[CMD] Filter request received - %d x %d ", ulImageWidth, ulImageHeight);*/
        vprvSendAckEth();
        xDMACopySeg.DstAddr = (uint32_t*)ucImgSegment;
        vDMASetDescCfg(0, xDMACopySeg);

        for (size_t i=0; i< (IMAGE_HEIGHT); i++) {
          masterTIMEOUT_INFO(xQueueReceive(xDataReqQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO to recv rows");
          vDMASetDescGo(0);
          masterTIMEOUT_INFO(xQueueReceive(xDMADoneQ, &ulTest, pdMS_TO_TICKS(500)), "TO DMA to copy");
          vEthClearInfifoPtr();
          masterTIMEOUT_INFO(xQueueSend(xProcFilterQ, &ulRowCount, pdMS_TO_TICKS(500)), "TO to send next to proc");
          /*if (ucIndexSeg == (N_SEG-1)) {*/
            /*ucIndexSeg = 0;*/
          /*}*/
          /*else {*/
            /*ucIndexSeg += 1;*/
          /*}*/
          masterTIMEOUT_INFO(xQueueReceive(xProcDoneQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO to recv proc done");
          vprvSendRowFilter();
        }
        /*dbg(" -> Done"); */
        ucIndexSeg = 0;
        xMasterCurStatus = MASTER_STATUS_IDLE;
        break;
      case CMD_NONE:
        dbg("\n\r[CMD] None");
        break;
      case CMD_TEST:
        dbg("\n\r[CMD] CMD_TEST");
        vprvSendHeartbeat(xTaskGetTickCount());
        break;
      case CMD_HISTOGRAM:
        xMasterCurStatus = MASTER_STATUS_RUNNING;
        vprvSendAckEth();
        xMasterCurStatus = MASTER_STATUS_IDLE;
        vprvSendAckEth();
        break;
      default:
        dbg("\n\r[CMD] Unknown!");
        break;
    }
  }
}

static void vprvProcImg (void *pvParameters) {
  uint32_t ulBuffer32 = 0;
  uint32_t sum = 0;


  // Programming the first descriptor
  vDMASetDescCfg(1, xDMACopyFilt);

  for (;;) {
    xQueueReceive(xProcFilterQ, &ulBuffer32, portMAX_DELAY);
    
    for (size_t i=0; i<4; i++) {
      ucImgFiltered[i] = ucImgSegment[i];
    }
    
    for (size_t pixel=0; pixel<IMAGE_WIDTH; pixel++){
      sum = 0;
      /*dbg("Pixel %d\n\r", pixel);*/
      for (size_t row=0; row<3; row++) { // 3x rows per packet
        for (size_t j=0; j<3; j++){ // cols
          uint16_t index = 4+pixel+(row*(IMAGE_WIDTH+2))+j;
          sum += ucImgSegment[index]; 
          /*dbg("[%d]",index);*/
        }
        /*dbg("\n\r");*/
      }
      sum = sum/9;
      ucImgFiltered[4+pixel] = (uint8_t *)sum;
    }
    
    // Image bypass
    /*for (size_t i=0; i<SEGMENT_SIZE; i++) {*/
      /*ucImgFiltered[i] = ucImgSegment[ucIndexSeg][i];*/
    /*}*/

    // Send the current row filtered
    masterTIMEOUT_INFO(xQueueSend(xProcDoneQ, &ulBuffer32, pdMS_TO_TICKS(500)), "TO to send done hist!");
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
  xProcFilterQ = xQueueCreate(1, sizeof(uint32_t));
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
        uint32_t buffer[3];

        for (size_t i=0; i<3; i++) {
          buffer[i] = ulEthGetRecvData();
        }

        CmdType_t cmd_type = buffer[0];

        ulImageWidth = buffer[1];
        ulImageHeight = buffer[2];

        switch(cmd_type) {
          case CMD_TEST:
            break;
          case CMD_FILTER:
            /*vIRQSetMaskSingle(irqMASK_ETH_RECV);*/
            /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/
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
