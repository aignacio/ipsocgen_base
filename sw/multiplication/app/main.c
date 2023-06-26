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

#define TOTAL_SIZE_KIB  8

TaskHandle_t xHandleRecvData;
static QueueHandle_t xCmdQ;
static QueueHandle_t xEthSentQ;
static QueueHandle_t xRecvArrayQ;

static SemaphoreHandle_t xDMAMutex;
MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;

uint16_t gSizeKiB = 0;
uint32_t gArray [256*TOTAL_SIZE_KIB];
uint32_t gTotal = 0;

void vSetEth (void);
void vSendAckEth (void);
void vSetEth (void);

/*static void vprvRecvCmd (void *pvParameters) {*/
  /*Cmd_t cmd;*/
  /*Oper_t op;*/

  /*for (;;) {*/
    /*dbg("\n\rTask vprvRecvCmd ready");*/
    /*xQueueReceive(xCmdQ, &cmd, portMAX_DELAY);*/
    /*switch (cmd.st.pkt_type) {*/
      /*case CMD_NONE:*/
        /*dbg("\n\r[CMD] None");*/
        /*break;*/
      /*case CMD_MULT_FACTOR:*/
        /*dbg("\n\r[CMD] CMD_MULT_FACTOR: factor: %d / times: %d", cmd.st.factor, cmd.st.times);*/
        /*op.factor = cmd.st.factor;*/
        /*op.times = cmd.st.times;*/

        /*xQueueSend(xRecvArrayQ, &op, portMAX_DELAY);*/
        /*break;*/
      /*default:*/
        /*dbg("\n\rDefault");*/
        /*break;*/
    /*}*/
    /*[>vSendAckEth();<]*/
  /*}*/
/*}*/

static void vprvProcVec (void *pvParameters) {
  Oper_t op;

  dbg("\n\rTask vprvProcVec ready");
  for (;;) {
    xQueueReceive(xRecvArrayQ, &op, portMAX_DELAY);
    /*dbg("\n\rFactor= %u / Times=%u", op.factor, op.times);*/
    gTotal = 0;

    for (size_t i=0; i<op.times; i++) {
      for (size_t j=0; j<TOTAL_SIZE_KIB*256; j++) {
        gTotal += gArray[j]*op.factor;
      }
    }

    /*dbg("\n\rgTotal: %x", gTotal);*/
    vEthClearInfifoPtr();
    vSendAckEth();
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

  // Enable IRQs
  vIRQClearMaskSingle(irqMASK_DMA_0_ERROR);
  vIRQClearMaskSingle(irqMASK_DMA_0_DONE);
  vIRQClearMaskSingle(irqMASK_ETH_RECV);
  vIRQClearMaskSingle(irqMASK_ETH_SENT);
  /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/

  // DMA
  vDMAInit();

  xDMAMutex = xSemaphoreCreateMutex();
  if (xDMAMutex == NULL) {
    masterCRASH_DBG_INFO("Cannot create the mutexes");
  }
  // ------------------------------------
  // Queues
  // ------------------------------------
  xCmdQ = xQueueCreate(1, sizeof(Oper_t));
  xEthSentQ = xQueueCreate(1, sizeof(uint8_t));
  xRecvArrayQ = xQueueCreate(1, sizeof(Oper_t));

  /*xReturned = xTaskCreate(*/
    /*vprvRecvCmd,*/
    /*"Receive Commands",*/
    /*configMINIMAL_STACK_SIZE*2U,*/
    /*NULL,*/
    /*tskIDLE_PRIORITY+1,*/
    /*&xHandleRecvData);*/
  /*masterCHECK_TASK(xReturned);*/

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
  masterCHECK_TASK(xReturned);

  vTaskStartScheduler();

  // Should never reach here...
  for(;;);
}

void vSystemIrqHandler(uint32_t ulMcause){
  IRQEncoding_t xIRQID = xIRQGetIDFifo();
  BaseType_t    xHigherPriorityTaskWoken;
  uint8_t       ucBuffer8 = 0x00;

  switch (xIRQID) {
    case(IRQ_ETH_RECV):
      if (xMasterCurStatus == MASTER_STATUS_IDLE) {
        /*Cmd_t cmd = {.word = ulEthGetRecvData()};*/
        Oper_t test;

        test.factor = ulEthGetRecvData();
        test.times = ulEthGetRecvData();

        /*xQueueSendFromISR(xCmdQ, &test, &xHigherPriorityTaskWoken);*/
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
    //.IPAddr.bytes     = {192, 168,   1, 223},
    .IPAddr.bytes     = {192, 168,   1, 141},
    //.MACAddr.bytes    = {0x00, 0x00, 0x04, 0x42, 0x1a, 0x09, 0xaf, 0xc7}
    .MACAddr.bytes    = {0x00, 0x00, 0x22, 0x20, 0x5c, 0x06, 0x13, 0xb9}
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
