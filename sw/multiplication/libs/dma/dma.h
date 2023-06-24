#ifndef DMA_H
#define DMA_H

#include <stdio.h>
#include <stdint.h>
#include "soc.h"
#include "csr_dma.h"
#include "uart.h" // Required for dbg crash macro

// DMA CTRL/CFG/STATUS
#define dmaCTRL_CSR          (masterDMA_0_BASE_ADDR)
#define dmaCTRL_GO           (CSR_DMA_DMA_CONTROL_GO_BIT_OFFSET)
#define dmaCTRL_ABORT        (CSR_DMA_DMA_CONTROL_ABORT_BIT_OFFSET)
#define dmaCTRL_MAX_BURST    (CSR_DMA_DMA_CONTROL_MAX_BURST_BIT_OFFSET)

#define dmaCFG_WRITE_MODE    (CSR_DMA_DMA_DESC_CFG_WRITE_MODE_BIT_OFFSET)
#define dmaCFG_READ_MODE     (CSR_DMA_DMA_DESC_CFG_READ_MODE_BIT_OFFSET)
#define dmaCFG_ENABLE        (CSR_DMA_DMA_DESC_CFG_ENABLE_BIT_OFFSET)

#define dmaSTATUS            (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_STATUS_BYTE_OFFSET)
#define dmaERROR_ADDR        (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_ERROR_ADDR_BYTE_OFFSET)
#define dmaERROR_STAT        (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_ERROR_STATS_BYTE_OFFSET)

// DMA Descriptors
#define dmaDESC_0_SRC_ADDR   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_SRC_ADDR_BYTE_OFFSET_0)
#define dmaDESC_0_DST_ADDR   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_DST_ADDR_BYTE_OFFSET_0)
#define dmaDESC_0_NUM_BYTES  (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_NUM_BYTES_BYTE_OFFSET_0)
#define dmaDESC_0_DESC_CFG   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_CFG_BYTE_OFFSET_0)

#define dmaDESC_1_SRC_ADDR   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_SRC_ADDR_BYTE_OFFSET_1)
#define dmaDESC_1_DST_ADDR   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_DST_ADDR_BYTE_OFFSET_1)
#define dmaDESC_1_NUM_BYTES  (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_NUM_BYTES_BYTE_OFFSET_1)
#define dmaDESC_1_DESC_CFG   (masterDMA_0_BASE_ADDR+CSR_DMA_DMA_DESC_CFG_BYTE_OFFSET_1)

typedef enum {
  DMA_MODE_DISABLE = 0x0,
  DMA_MODE_ENABLE  = 0x1
} DMACfgEn_t;

typedef enum {
  DMA_MODE_INCR  = 0x0,
  DMA_MODE_FIXED = 0x1
} DMACfgMode_t;

typedef enum {
  DMA_ERROR_OPERAT = 0x0,
  DMA_ERROR_CONFIG = 0x1
} DMAErrorType_t;

typedef enum {
  DMA_ERROR_RD = 0x0,
  DMA_ERROR_WR = 0x1
} DMAErrorSrc_t;

typedef struct DMAErrorStat_t {
  DMAErrorType_t  Type;
  DMAErrorSrc_t   Src;
  uint8_t         Trig;
} DMAErrorStat_t;

typedef struct DMAStatus_t {
  uint16_t        Version;
  uint8_t         Done;
  uint8_t         Error;
} DMAStatus_t;

typedef struct DMACtrl_t {
  uint8_t         MaxBurst;
  uint8_t         Abort;
  uint8_t         Go;
} DMACtrl_t;

typedef struct DMACfg_t {
  DMACfgMode_t    WrMode;
  DMACfgMode_t    RdMode;
  DMACfgEn_t      Enable;
} DMACfg_t;

typedef struct DMADesc_t {
  uint32_t*       SrcAddr;
  uint32_t*       DstAddr;
  uint32_t        NumBytes;
  DMACfg_t        Cfg;
} DMADesc_t;

void vDMAInit (void);
void vDMASetDescCfgSrcAddr (uint8_t ucDescID, uint32_t* pulAddr);
void vDMASetDescCfgDstAddr (uint8_t ucDescID, uint32_t* pulAddr);
void vDMASetDescCfg (uint8_t ucDescID, DMADesc_t xDesc);
void vDMASetDescBytes (uint8_t ucDescID, uint32_t ulNumBytes);
void vDMASetDescCfgMode (uint8_t ucDescID, DMACfg_t xDMACfg);
DMAErrorStat_t xDMAGetErrorStat (void);
uint32_t ulDMAGetErrorAddr (void);
DMAStatus_t xDMAGetStatus (void);
DMACtrl_t xDMAGetCfg (void);
void vDMASetCfg (DMACtrl_t xDMACtrl);
void vDMASetMaxBurst (uint8_t ucMaxBurst);
void vDMASetAbort (void);
void vDMAClearAbort (void);
void vDMASetDescGo (uint8_t ucDescID);
void vDMASetGo (void);
void vDMAClearGo (void);

#endif
