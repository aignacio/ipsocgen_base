#ifndef UART_H
#define UART_H

#include <stdarg.h>
#include <stdio.h>
#include "soc.h"

#ifndef masterUART_BASE_ADDR
  #define masterUART_BASE_ADDR  0x00000000
#endif

#ifdef DEBUG
  #define dbg(...)  vUartPrintf(__VA_ARGS__)
#else
  #define dbg(...)
#endif

#define uartTX_ADDR             (masterUART_BASE_ADDR       + 0x0C)
#define uartRX_ADDR             (masterUART_BASE_ADDR       + 0x08)
#define uartSTATS_ADDR          (masterUART_BASE_ADDR       + 0x04)
#define uartCFG_ADDR            (masterUART_BASE_ADDR       + 0x00)
#define uartTX_SIM_ADDR         (masterRESET_CTRL_BASE_ADDR + 0x10)
#define uartFREQ_SYSTEM         (masterHW_CLK_SPEED_HZ)
#define uartBR_UART             (masterUART_BAUD_RATE)

void vUartSetup (uint32_t baudrate);
void vUartPutc (char c);
char ucUartGetc (void);
uint32_t ulUartCharReceived (void);
char ucUartCharReceivedGet (void);
void vUartPrint (const char *pString);
void vUartPrintf (const char *pformat, ...);

#endif
