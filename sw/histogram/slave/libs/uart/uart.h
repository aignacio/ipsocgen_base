#ifndef UART_H
#define UART_H

#include <stdarg.h>
#include <stdio.h>
#include "slave_tile.h"

#ifndef slaveUART_BASE_ADDR
  #define slaveUART_BASE_ADDR  0x00000000
#endif

#ifdef DEBUG
  #define dbg(...)  vUartPrintf(__VA_ARGS__)
#else
  #define dbg(...)
#endif

#define uartTX_ADDR             (slaveUART_BASE_ADDR       + 0x0C)
#define uartRX_ADDR             (slaveUART_BASE_ADDR       + 0x08)
#define uartSTATS_ADDR          (slaveUART_BASE_ADDR       + 0x04)
#define uartCFG_ADDR            (slaveUART_BASE_ADDR       + 0x00)
#define uartTX_SIM_ADDR         (slaveRESET_CTRL_BASE_ADDR + 0x10)
#define uartFREQ_SYSTEM         (slaveHW_CLK_SPEED_HZ)
#define uartBR_UART             (slaveUART_BAUD_RATE)

void vUartSetup (uint32_t baudrate);
void vUartPutc (char c);
char ucUartGetc (void);
uint32_t ulUartCharReceived (void);
char ucUartCharReceivedGet (void);
void vUartPrint (const char *pString);
void vUartPrintf (const char *pformat, ...);

#endif
