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

TaskHandle_t xHandleRecvData;
static QueueHandle_t xSentReadyQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xRecvArrayQ;
static QueueHandle_t xNoCPktFromSlavesQ;

MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;
static SemaphoreHandle_t xSlaveTileResMutex;
static uint8_t  ucSlaveTileResVec [masterNOC_TOTAL_TILES];

void vSetEth (void);
void vSendAckEth (void);
void vSetEth (void);
static void ucprvSetFreeSlaveTile (uint8_t index);
static uint8_t ucprvGetFreeSlaveTile (void);

static void vprvProcVec (void *pvParameters) {
  Oper_t op;
  uint8_t ucBuffer;
  uint8_t empty;

  dbg("\n\rTask vprvProcVec ready");
  for (;;) {
    xQueueReceive(xRecvArrayQ, &op, portMAX_DELAY);
    /*dbg("\n\rLoop[%d] Factor[%d]", op.times, op.factor);*/
    
    for (size_t i=0; i<op.times; i++) {
      vRaveNoCSendNoCMsg(ucprvGetFreeSlaveTile(), 2, 0xaa);
      vRaveNoCWrBuffer(0x1);
      vRaveNoCWrBuffer(op.factor);
    }

    /*dbg("\n\rall sent!");*/
    empty = 0;

    while(!empty) {
      /*dbg(".");*/
      /*vTaskDelay(1000/portTICK_PERIOD_MS);*/
      empty = 1;
      for (size_t i=1; i<masterNOC_TOTAL_TILES; i++){
        if (ucSlaveTileResVec[i] == 0) {
          empty = 0;
          /*dbg(" %d/%d",i,ucSlaveTileResVec[i]);*/
          break;
        }
      }
    }
    vEthClearInfifoPtr();
    vSendAckEth();
    /*dbg("\n\rDone");*/
    
    /*xQueueSend(xSentReadyQ, &ucBuffer, portMAX_DELAY);*/
  }
}

/*static void vprvSendCmd (void *pvParameters) {*/
  /*Oper_t op;*/

  /*dbg("\n\rTask Send ready");*/
  /*for (;;) {*/
    /*op.times = 8;*/
    /*op.factor = 2;*/
    /*if( xQueueSend(xRecvArrayQ, &op, (TickType_t)10) != pdPASS ) {*/
      /*dbg("\n\rFailed to send");*/
    /*}*/
    /*vTaskDelay(1000/portTICK_PERIOD_MS);*/
  /*}*/
/*}*/

static void vprvFreeSlaves (void *pvParameters) {
  uint8_t ucBuffer;

  dbg("\n\rTask vprvFreeSlaves ready");
  for (;;) {
    xQueueReceive(xNoCPktFromSlavesQ, &ucBuffer, portMAX_DELAY);
    uint8_t xSlaveFree = (ulRaveNoCGetNoCData() & 0xFF);
    /*dbg("\n\rFree=%d", xSlaveFree);*/
    ucprvSetFreeSlaveTile(xSlaveFree);
    vRaveNoCIRQAck();
  }
}

int main (void) {
  BaseType_t xReturned = pdFALSE;

  // -------------------------------------
  // Initialization
  // -------------------------------------
  //
  // Initialize the ETHERNET
  vSetEth();

  // RaveNoC
  // Initialise NoC & set IRQ to PULSE HEAD Flit type
  vRaveNoCInitNoCLUT();
  vRaveNoCSetIRQMux(RAVENOC_MUX_PULSE_HEAD_FLIT);
  vRaveNoCPrintNoCSettings();

  // IRQs
  // Enable IRQs
  vIRQClearMaskSingle(irqMASK_RAVENOC_PKT);
  vIRQClearMaskSingle(irqMASK_ETH_RECV);
  vIRQClearMaskSingle(irqMASK_ETH_SENT);

  // ------------------------------------
  // Queues
  // ------------------------------------
  xEthSentQ = xQueueCreate(1, sizeof(uint8_t));
  xRecvArrayQ = xQueueCreate(1, sizeof(Oper_t));
  xSentReadyQ = xQueueCreate(1, sizeof(uint8_t));
  xNoCPktFromSlavesQ = xQueueCreate(1, sizeof(uint8_t));

  xSlaveTileResMutex = xSemaphoreCreateMutex();
  if (xSlaveTileResMutex == NULL) {
    masterCRASH_DBG_INFO("Cannot create the mutexes");
  }
  // Initialize array of slaves availability
  for (size_t i=0; i<masterNOC_TOTAL_TILES; i++)
    ucSlaveTileResVec[i] = 1;

  /*xReturned = xTaskCreate(*/
    /*vprvSendCmd,*/
    /*"Send Commands",*/
    /*configMINIMAL_STACK_SIZE*2U,*/
    /*NULL,*/
    /*tskIDLE_PRIORITY+1,*/
    /*&xHandleRecvData);*/
  /*masterCHECK_TASK(xReturned);*/

  xReturned = xTaskCreate(
    vprvProcVec,
    "Process the vector",
    configMINIMAL_STACK_SIZE*2U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleRecvData);
  masterCHECK_TASK(xReturned);

  xReturned = xTaskCreate(
    vprvFreeSlaves,
    "Process the slave answer",
    configMINIMAL_STACK_SIZE*3U,
    NULL,
    tskIDLE_PRIORITY+1,
    &xHandleRecvData);
  masterCHECK_TASK(xReturned);

  //vRaveNoCIRQAck();
  vTaskStartScheduler();

  // Should never reach here...
  for(;;);
}

void vSystemIrqHandler(uint32_t ulMcause) {
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  BaseType_t    xHigherPriorityTaskWoken;
  uint8_t       ucBuffer8 = 0x00;

  switch (xIRQID) {
    case(IRQ_RAVENOC_PKT):
      ucBuffer8 = ucRaveNoCGetNoCPktSize();
      xQueueSendFromISR(xNoCPktFromSlavesQ, &ucBuffer8, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      break;
    case(IRQ_DMA_0_DONE):
      break;
    case(IRQ_ETH_RECV):
      if (xMasterCurStatus == MASTER_STATUS_IDLE) {
        Oper_t test;

        test.factor = ulEthGetRecvData();
        test.times = ulEthGetRecvData();
        xQueueSendFromISR(xRecvArrayQ, &test, &xHigherPriorityTaskWoken);
        if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
      }
      vEthClearIRQs();
      break;
    case(IRQ_ETH_RECV_FULL):
      break;
    case(IRQ_ETH_SENT):
      xQueueSendFromISR(xEthSentQ, &ucBuffer8, &xHigherPriorityTaskWoken);
      if(xHigherPriorityTaskWoken == pdTRUE){
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }
      vEthClearIRQs();
      break;
    default:
      break;
  }
}

// ----------------------------- Complementary fn --------------------------------
static uint8_t ucprvGetFreeSlaveTile (void) {
  uint8_t ucAllSlavesBusy = 1;
  uint8_t ucFreeSlaveIndex = 0;
  uint8_t temp;
  uint32_t ulTimeoutSlaveCnt = 0;

  while (ucAllSlavesBusy) {
    for (size_t i=1; i<masterNOC_TOTAL_TILES; i++) {

      xSemaphoreTake(xSlaveTileResMutex, portMAX_DELAY);
      temp = ucSlaveTileResVec[i]; 
      xSemaphoreGive(xSlaveTileResMutex);

      if (temp == 1) {
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

void vSendAckEth (void) {
  uint8_t payload[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  vEthClearOutfifoPtr();
  vEthWriteOutfifoData((uint8_t*)&payload, 4);
  vEthSendPkt();
  masterTIMEOUT_INFO(xQueueReceive(xEthSentQ, &payload, pdMS_TO_TICKS(500)),"Send UDP Pkt");
}

void vSetEth (void) {
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
}
