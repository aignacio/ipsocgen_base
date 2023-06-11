#ifndef IRQ_CTRL_H
#define IRQ_CTRL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "soc.h"

// IRQ CSRs
#define irqCTRL_FIFO_READ     (masterIRQ_CTRL_BASE_ADDR+0x00)
#define irqCTRL_MASK          (masterIRQ_CTRL_BASE_ADDR+0x04)
#define irqCTRL_FIFO_CLEAR    (masterIRQ_CTRL_BASE_ADDR+0x08)

// IRQ Masks
#define irqMASK_DMA_0_ERROR   (uint32_t)(1 << 0)
#define irqMASK_DMA_0_DONE    (uint32_t)(1 << 1)
#define irqMASK_RESERVED_0    (uint32_t)(1 << 2)
#define irqMASK_RESERVED_1    (uint32_t)(1 << 3)
#define irqMASK_ETH_RECV      (uint32_t)(1 << 4)
#define irqMASK_ETH_SENT      (uint32_t)(1 << 5)
#define irqMASK_ETH_RECV_FULL (uint32_t)(1 << 6)

#define irqMASK_ALL           0xFFFFFFFF

typedef enum {
  IRQ_DMA_0_ERROR   = 0,
  IRQ_DMA_0_DONE    = 1,
  IRQ_RESERVED_0    = 2,
  IRQ_RESERVED_1    = 3,
  IRQ_ETH_RECV      = 4,
  IRQ_ETH_SENT      = 5,
  IRQ_ETH_RECV_FULL = 6,
  IRQ_ILLEGAL       = 7
} IRQEncoding_t;

IRQEncoding_t xIRQGetIDFifo(void);
void vIRQSetMask (uint32_t ulMask);
void vIRQSetMaskSingle (uint32_t ulSingle);
void vIRQClearMaskSingle (uint32_t ulSingle);
void vIRQSetClear (void);

#endif
