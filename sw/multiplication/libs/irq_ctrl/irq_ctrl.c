#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "irq_ctrl.h"

static volatile uint32_t* const ulIRQFifoAddr  = (uint32_t*) irqCTRL_FIFO_READ;
static volatile uint32_t* const ulIRQMask      = (uint32_t*) irqCTRL_MASK;
static volatile uint32_t* const ulIRQFifoClear = (uint32_t*) irqCTRL_FIFO_CLEAR;

IRQEncoding_t xIRQGetIDFifo (void) {
  uint32_t ulID = *ulIRQFifoAddr;

  switch(ulID) {
    case 0:   return IRQ_DMA_0_ERROR;
    case 1:   return IRQ_DMA_0_DONE;
    case 2:   return IRQ_RESERVED_0;
    case 3:   return IRQ_RESERVED_1;
    case 4:   return IRQ_ETH_RECV;
    case 5:   return IRQ_ETH_SENT;
    case 6:   return IRQ_ETH_RECV_FULL;
    default:  return IRQ_ILLEGAL;
  }
}

void vIRQSetMask (uint32_t ulMask) {
  *ulIRQMask = ~ulMask;
}

void vIRQSetMaskSingle (uint32_t ulSingle) {
  *ulIRQMask &= ~ulSingle;
}

void vIRQClearMaskSingle (uint32_t ulSingle) {
  *ulIRQMask |= ulSingle;
}

void vIRQSetClear (void){
  *ulIRQFifoClear = 0x1;
}
