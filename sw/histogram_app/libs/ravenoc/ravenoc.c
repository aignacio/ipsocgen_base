#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ravenoc.h"
#include "uart.h"
                                 //  0  1  2  3  4  5  6  7  8  9 10
const  uint8_t  ucNoCMinWidth[11] = {1, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4};
static volatile uint32_t* const pulRaveNoCWrBuffer    = (uint32_t*)ravenocWR_BUFFER;
static volatile uint32_t* const pulRaveNoCRdBuffer    = (uint32_t*)ravenocRD_BUFFER;
static volatile uint32_t* const pulRaveNoCVersion     = (uint32_t*)ravenocCSR_VER;
static volatile uint32_t* const pulRaveNoCRow         = (uint32_t*)ravenocCSR_ROW;
static volatile uint32_t* const pulRaveNoCCol         = (uint32_t*)ravenocCSR_COL;
static volatile uint32_t* const pulRaveNoCWrBuffFull  = (uint32_t*)ravenocCSR_WR_BUFFER_FULL;
static volatile uint32_t* const pulRaveNoCVc0PktSz    = (uint32_t*)ravenocCSR_VC0_PKT_SIZE;
static volatile uint32_t* const pulRaveNoCIRQMux      = (uint32_t*)ravenocCSR_IRQ_RD_MUX;

RaveNoCInfo_t xgTile;
uint8_t       ucgTileLUTx[masterNOC_TOTAL_TILES];
uint8_t       ucgTileLUTy[masterNOC_TOTAL_TILES];

static uint32_t ulprvMaskMsg (uint32_t ulData) {
  for (size_t i=ravenocWIDTH_MSG; i<32; i++) {
    ulData &= ~(1 << i);
  }
  return ulData;
}

static void vprvRaveNoCGetNoCInfo (void) {
  xgTile.X            = *pulRaveNoCRow;
  xgTile.Y            = *pulRaveNoCCol;
  xgTile.Version.Code = *pulRaveNoCVersion;

  // Revert string
  uint8_t ucTemp[5];
  for (size_t i=0; i<4; i++)
    ucTemp[i] = xgTile.Version.CodeS[3-i];
  ucTemp[4] = '\0';
  strcpy((char*)xgTile.Version.CodeS,(char*)ucTemp);
}

uint8_t ucRaveNoCGetTileID (void) {
  return xgTile.ID;
}

void vRaveNoCInitNoCLUT (void) {
  vprvRaveNoCGetNoCInfo();
  for (size_t i=0,x=0,y=0; i<masterNOC_TOTAL_TILES; i++) {
    ucgTileLUTx[i] = x;
    ucgTileLUTy[i] = y;

    if ((xgTile.X == x) && (xgTile.Y == y)) {
      xgTile.ID = i;
    }

    y++;
    if (y == masterNOC_SIZE_Y) {
      y = 0;
      x++;
    }
  }
}

void vRaveNoCPrintNoCSettings (void) {
  vprvRaveNoCGetNoCInfo();

  dbg("\n\rRaveNoC Settings:");
  dbg("\n\rNoC ID: (%d,%d) / PE=%d", xgTile.X, xgTile.Y, xgTile.ID);
  dbg("\n\rNoC Size: %dx%d", masterNOC_SIZE_X, masterNOC_SIZE_Y);
  dbg("\n\rNoC Version: %x / %s", xgTile.Version.Code, xgTile.Version.CodeS);
  dbg("\n\rNoC max pkt: 2^%d flits", ravenocWIDTH_PKT);
  dbg("\n\rNoC max single flit: %d bits", ravenocWIDTH_MSG);
}

void vRaveNoCSetIRQMux (RaveNoCIRQMux_t xIRQMux) {
  *pulRaveNoCIRQMux = xIRQMux;
}

void vRaveNoCSendNoCMsg (uint8_t ucTileNumber, uint8_t ucPktSize, uint32_t ulData) {
  if (ucTileNumber > (masterNOC_TOTAL_TILES)-1) {
    dbg("\n\r[RaveNoC-Error] Tile number out of range, please choose between 0 to %d!", masterNOC_TOTAL_TILES-1);
  }
  else if (ucTileNumber == xgTile.ID) {
    dbg("\n\r[RaveNoC-Error] Tile number cannot be the same ID of the this tile %d!", xgTile.ID);
  }
  else {
    RaveNoCPkt_t xNoCPkt = {
      .XDest    = ucgTileLUTx[ucTileNumber],
      .YDest    = ucgTileLUTy[ucTileNumber],
      .PktWidth = ucPktSize,
      .Message  = ulprvMaskMsg(ulData)
    };
    *pulRaveNoCWrBuffer = ravenocASM_PKT_NOC(xNoCPkt);
  }
}

uint32_t ulRaveNoCGetHeader (uint8_t ucTileNumber, uint8_t ucPktSize, uint32_t ulData) {
  if (ucTileNumber > (masterNOC_TOTAL_TILES)-1) {
    dbg("\n\r[RaveNoC-Error] Tile number out of range, please choose between 0 to %d!", masterNOC_TOTAL_TILES-1);
    return 0;
  }
  else if (ucTileNumber == xgTile.ID) {
    dbg("\n\r[RaveNoC-Error] Tile number cannot be the same ID of the this tile %d!", xgTile.ID);
    return 0;
  }
  else {
    RaveNoCPkt_t xNoCPkt = {
      .XDest    = ucgTileLUTx[ucTileNumber],
      .YDest    = ucgTileLUTy[ucTileNumber],
      .PktWidth = ucPktSize,
      .Message  = ulprvMaskMsg(ulData)
    };
    return ravenocASM_PKT_NOC(xNoCPkt);
  }
}

RaveNoCInfo_t xRaveNoCGetNoCID (void) {
  return xgTile;
}

uint32_t ulRaveNoCGetNoCData (void) {
  uint32_t ulTmp = *pulRaveNoCRdBuffer;
  return ulTmp;
}

uint8_t ucRaveNoCGetNoCPktSize (void) {
  uint32_t ulTmp = *pulRaveNoCVc0PktSz;
  return  (uint8_t)ulTmp;
}

uint32_t ucRaveNoCGetWrBufferFull (void) {
  uint32_t ulTmp = *pulRaveNoCWrBuffFull;
  return  ulTmp;
}
