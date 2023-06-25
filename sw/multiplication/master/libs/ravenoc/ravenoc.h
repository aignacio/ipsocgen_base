#ifndef RAVENOC_H
#define RAVENOC_H

#include <stdio.h>
#include <stdint.h>
#include "master_tile.h"

#ifndef masterRAVENOC_BASE_ADDR
  #define masterRAVENOC_BASE_ADDR  0x00000000
#endif

#ifndef masterNOC_SIZE_X
  #define masterNOC_SIZE_X         1
  #define masterNOC_SIZE_Y         2
  #define masterNOC_TOTAL_TILES    2
#endif

#define ravenocWR_BUFFER           (masterRAVENOC_BASE_ADDR + 0x1000)
#define ravenocRD_BUFFER           (masterRAVENOC_BASE_ADDR + 0x2000)
#define ravenocCSR_BASE            (masterRAVENOC_BASE_ADDR + 0x3000)
#define ravenocCSR_VER             (ravenocCSR_BASE         + 0x0000)
#define ravenocCSR_ROW             (ravenocCSR_BASE         + 0x0004)
#define ravenocCSR_COL             (ravenocCSR_BASE         + 0x0008)
#define ravenocCSR_IRQ_RD_ST       (ravenocCSR_BASE         + 0x000c)
#define ravenocCSR_IRQ_RD_MUX      (ravenocCSR_BASE         + 0x0010)
#define ravenocCSR_IRQ_RD_MASK     (ravenocCSR_BASE         + 0x0014)
#define ravenocCSR_WR_BUFFER_FULL  (ravenocCSR_BASE         + 0x0018)
#define ravenocCSR_IRQ_ACK         (ravenocCSR_BASE         + 0x001c)
#define ravenocCSR_VC0_PKT_SIZE    (ravenocCSR_BASE         + 0x0020)

#define ravenocWIDTH_X             ucNoCMinWidth[masterNOC_SIZE_X]
#define ravenocWIDTH_Y             ucNoCMinWidth[masterNOC_SIZE_Y]
#define ravenocWIDTH_PKT           8
#define ravenocWIDTH_MSG           (32-ravenocWIDTH_X-ravenocWIDTH_Y-ravenocWIDTH_PKT)
#define ravenocOFFSET_X            (32-ravenocWIDTH_X)
#define ravenocOFFSET_Y            (ravenocOFFSET_X-ravenocWIDTH_Y)
#define ravenocOFFSET_PKT_SIZE     (ravenocOFFSET_Y-8)

#define ravenocASM_PKT_NOC(pkt)    (pkt.XDest<<ravenocOFFSET_X | \
                                    pkt.YDest<<ravenocOFFSET_Y | \
                                    pkt.PktWidth<<ravenocOFFSET_PKT_SIZE | pkt.Message)

typedef enum {
  RAVENOC_MUX_DEFAULT         = 0x0,
  RAVENOC_MUX_EMPTY_FLAGS     = 0x1,
  RAVENOC_MUX_FULL_FLAGS      = 0x2,
  RAVENOC_MUX_COMP_FLAGS      = 0x3,
  RAVENOC_MUX_PULSE_HEAD_FLIT = 0x4
} RaveNoCIRQMux_t;

typedef struct RaveNoCPkt_t {
  uint8_t           XDest;
  uint8_t           YDest;
  uint8_t           PktWidth;
  uint32_t          Message;
} RaveNoCPkt_t;

typedef union RaveNoCVersion_t {
  uint32_t          Code;
  uint8_t           CodeS[5];
} RaveNoCVersion_t;

typedef struct RaveNoCInfo_t {
  uint8_t           X;
  uint8_t           Y;
  RaveNoCVersion_t  Version;
  uint8_t           ID;
} RaveNoCInfo_t;

uint8_t ucRaveNoCGetTileID (void);
void vRaveNoCInitNoCLUT (void);
void vRaveNoCPrintNoCSettings (void);
void vRaveNoCSetIRQMux(RaveNoCIRQMux_t xIRQMux);
void vRaveNoCSendNoCMsg (uint8_t ucTileNumber, uint8_t ucPktSize, uint32_t ulData);
uint32_t ulRaveNoCGetHeader (uint8_t ucTileNumber, uint8_t ucPktSize, uint32_t ulData);
RaveNoCInfo_t xRaveNoCGetNoCID (void);
uint32_t ulRaveNoCGetNoCData (void);
uint8_t ucRaveNoCGetNoCPktSize (void);
uint32_t ucRaveNoCGetWrBufferFull (void);
void vRaveNoCWrBuffer (uint32_t ulData);

#endif
