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
static QueueHandle_t xCmdQ;
static QueueHandle_t xEthSentQ;
MasterType_t xMasterCurStatus = MASTER_STATUS_IDLE;

void vSetEth (void);
void vSendAckEth (void);
void vSetEth (void);

static void vprvRecvCmd (void *pvParameters) {
  Cmd_t cmd;

  for (;;) {
    xQueueReceive(xCmdQ, &cmd, portMAX_DELAY);
    switch (cmd.st.pkt_type) {
      case CMD_NONE:
        dbg("\n\r[CMD] None");
        break;
      case CMD_RECV_ARRAY:
        dbg("\n\r[CMD] CMD_RECV_ARRAY: arg1: %d / arg2: %d", cmd.st.arg1, cmd.st.arg2);
        break;
      case CMD_MULT_FACTOR:
        dbg("\n\r[CMD] CMD_MULT_FACTOR: arg1: %d / arg2: %d", cmd.st.arg1, cmd.st.arg2);
        break;
      case CMD_GET_RESULT:
        dbg("\n\r[CMD] CMD_GET_RESULT");
        break;
      default:
        dbg("\n\rDefault");
        break;
    }
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
  
  // ------------------------------------
  // Queues
  // ------------------------------------
  xCmdQ = xQueueCreate(2, sizeof(Cmd_t));
  xEthSentQ = xQueueCreate(1, sizeof(uint8_t));

  xReturned = xTaskCreate(
    vprvRecvCmd,
    "Receive Commands",
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
    case(IRQ_RAVENOC_PKT):
      break;
    case(IRQ_DMA_0_DONE):
      break;
    case(IRQ_ETH_RECV):
      if (xMasterCurStatus == MASTER_STATUS_IDLE) {
        Cmd_t cmd = {.word = ulEthGetRecvData()};

        // Disable local eth recv and only when eth has received 1 KiB
        // also, clears the pointers
        /*vIRQSetMaskSingle(irqMASK_ETH_RECV);*/
        /*vIRQClearMaskSingle(irqMASK_ETH_RECV_FULL);*/
        vEthClearInfifoPtr();
        xQueueSendFromISR(xCmdQ, &cmd, &xHigherPriorityTaskWoken);
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
