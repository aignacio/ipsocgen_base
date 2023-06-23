#include <stdio.h>
#include <stdint.h>
#include "dma.h"

static volatile uint32_t* const pulDMACtrlCsr       = (uint32_t*)dmaCTRL_CSR;
static volatile uint32_t* const pulDMAStat          = (uint32_t*)dmaSTATUS;
static volatile uint32_t* const pulDMAErrorAddr     = (uint32_t*)dmaERROR_ADDR;
static volatile uint32_t* const pulDMAErrorStat     = (uint32_t*)dmaERROR_STAT;

static volatile uint32_t* const pulDMADesc0SrcAddr  = (uint32_t*)dmaDESC_0_SRC_ADDR;
static volatile uint32_t* const pulDMADesc0DstAddr  = (uint32_t*)dmaDESC_0_DST_ADDR;
static volatile uint32_t* const pulDMADesc0NumBytes = (uint32_t*)dmaDESC_0_NUM_BYTES;
static volatile uint32_t* const pulDMADesc0Cfg      = (uint32_t*)dmaDESC_0_DESC_CFG;

static volatile uint32_t* const pulDMADesc1SrcAddr  = (uint32_t*)dmaDESC_1_SRC_ADDR;
static volatile uint32_t* const pulDMADesc1DstAddr  = (uint32_t*)dmaDESC_1_DST_ADDR;
static volatile uint32_t* const pulDMADesc1NumBytes = (uint32_t*)dmaDESC_1_NUM_BYTES;
static volatile uint32_t* const pulDMADesc1Cfg      = (uint32_t*)dmaDESC_1_DESC_CFG;

void vDMAInit (void) {
  DMAStatus_t xDMAStatus = xDMAGetStatus();

  dbg("\n\r");
  dbg("\n\rDMA init:");
  dbg("\n\rVersion: %x", xDMAStatus.Version);
  dbg("\n\r");
}

void vDMASetDescCfg (uint8_t ucDescID, DMADesc_t xDesc) {
  switch (ucDescID) {
    case 0:
      *pulDMADesc0SrcAddr  = (uint32_t)xDesc.SrcAddr;
      *pulDMADesc0DstAddr  = (uint32_t)xDesc.DstAddr;
      *pulDMADesc0NumBytes = xDesc.NumBytes;
      *pulDMADesc0Cfg      = (((uint8_t)xDesc.Cfg.WrMode & 0x1) << dmaCFG_WRITE_MODE) |
                             (((uint8_t)xDesc.Cfg.RdMode & 0x1) << dmaCFG_READ_MODE)  |
                             (((uint8_t)xDesc.Cfg.Enable & 0x1) << dmaCFG_ENABLE);
      break;
    case 1:
      *pulDMADesc1SrcAddr  = (uint32_t)xDesc.SrcAddr;
      *pulDMADesc1DstAddr  = (uint32_t)xDesc.DstAddr;
      *pulDMADesc1NumBytes = xDesc.NumBytes;
      *pulDMADesc1Cfg      = (((uint8_t)xDesc.Cfg.WrMode & 0x1) << dmaCFG_WRITE_MODE) |
                             (((uint8_t)xDesc.Cfg.RdMode & 0x1) << dmaCFG_READ_MODE)  |
                             (((uint8_t)xDesc.Cfg.Enable & 0x1) << dmaCFG_ENABLE);
      break;
    default:
      slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID");
      break;
  }
}

void vDMASetDescBytes (uint8_t ucDescID, uint32_t ulNumBytes) {
  switch (ucDescID) {
    case 0:
      *pulDMADesc0NumBytes = ulNumBytes;
      break;
    case 1:
      *pulDMADesc1NumBytes = ulNumBytes;
      break;
    default:
      slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID");
      break;
  }
}

void vDMASetDescCfgSrcAddr (uint8_t ucDescID, uint32_t* pulAddr) {
  switch (ucDescID) {
    case 0: *pulDMADesc0SrcAddr  = (uint32_t)pulAddr; break;
    case 1: *pulDMADesc1SrcAddr  = (uint32_t)pulAddr; break;
    default: slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID"); break;
  }
}

void vDMASetDescCfgDstAddr (uint8_t ucDescID, uint32_t* pulAddr) {
  switch (ucDescID) {
    case 0: *pulDMADesc0DstAddr  = (uint32_t)pulAddr; break;
    case 1: *pulDMADesc1DstAddr  = (uint32_t)pulAddr; break;
    default: slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID"); break;
  }
}

void vDMASetDescCfgMode (uint8_t ucDescID, DMACfg_t xDMACfg) {
  switch (ucDescID) {
    case 0:
      *pulDMADesc0Cfg = (((uint8_t)xDMACfg.WrMode & 0x1) << dmaCFG_WRITE_MODE) |
                        (((uint8_t)xDMACfg.RdMode & 0x1) << dmaCFG_READ_MODE)  |
                        (((uint8_t)xDMACfg.Enable & 0x1) << dmaCFG_ENABLE);
      break;
    case 1:
      *pulDMADesc1Cfg = (((uint8_t)xDMACfg.WrMode & 0x1) << dmaCFG_WRITE_MODE) |
                        (((uint8_t)xDMACfg.RdMode & 0x1) << dmaCFG_READ_MODE)  |
                        (((uint8_t)xDMACfg.Enable & 0x1) << dmaCFG_ENABLE);
      break;
    default: slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID"); break;
  }
}

DMAErrorStat_t xDMAGetErrorStat (void) {
  DMAErrorStat_t xDMAErrorStat;
  uint32_t ulVal = *pulDMAErrorStat;
  xDMAErrorStat.Type = (DMAErrorType_t)((ulVal >> 0) & 0x1);
  xDMAErrorStat.Src  = (DMAErrorSrc_t)((ulVal >> 1) & 0x1);
  xDMAErrorStat.Trig = ((ulVal >> 2) & 0x1);

  return xDMAErrorStat;
}

uint32_t ulDMAGetErrorAddr (void) {
  return *pulDMAErrorAddr;
}

DMAStatus_t xDMAGetStatus (void) {
  DMAStatus_t xDMAStatus;
  uint32_t ulVal = *pulDMAStat;
  xDMAStatus.Version = (ulVal & 0xFFFF);
  xDMAStatus.Done    = ((ulVal >> 16) & 0x1);
  xDMAStatus.Error   = ((ulVal >> 17) & 0x1);

  return xDMAStatus;
}

DMACtrl_t xDMAGetCfg (void) {
  DMACtrl_t xDMACtrl;
  uint32_t ulVal = *pulDMACtrlCsr;

  xDMACtrl.MaxBurst = ((ulVal >> 2) & 0xFF);
  xDMACtrl.Abort    = ((ulVal >> 1) & 0x1);
  xDMACtrl.Go       = ((ulVal >> 0) & 0x1);
  return xDMACtrl;
}

void vDMASetCfg (DMACtrl_t xDMACtrl) {
  *pulDMACtrlCsr = (((uint8_t)xDMACtrl.MaxBurst & 0xFF) << dmaCTRL_MAX_BURST)  |
                   (((uint8_t)xDMACtrl.Abort    & 0x1)  << dmaCTRL_ABORT)      |
                   (((uint8_t)xDMACtrl.Go       & 0x1)  << dmaCTRL_GO);
}

void vDMASetMaxBurst (uint8_t ucMaxBurst) {
  *pulDMACtrlCsr |= ((ucMaxBurst & 0xFF) << dmaCTRL_MAX_BURST);
}

void vDMASetAbort (void) {
  *pulDMACtrlCsr |= 0x2;
}

void vDMAClearAbort (void) {
  *pulDMACtrlCsr &= ~0x2;
}

void vDMASetDescGo (uint8_t ucDescID) {
  switch (ucDescID) {
    case 0:
      *pulDMADesc0Cfg |= 0x4;
      *pulDMADesc1Cfg &= ~0x4;
      break;
    case 1:
      *pulDMADesc0Cfg &= ~0x4;
      *pulDMADesc1Cfg |= 0x4;
      break;
    default: slaveCRASH_DBG_INFO("Unexpected DMA Descriptor ID"); break;
  }

  // Set the DMA go
  *pulDMACtrlCsr |= 0x1;
}

void vDMASetGo (void) {
  // Set the DMA go
  *pulDMACtrlCsr |= 0x1;
}

void vDMAClearGo (void) {
  *pulDMACtrlCsr &= ~0x1;
}
