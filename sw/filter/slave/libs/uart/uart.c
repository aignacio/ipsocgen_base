#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"

#ifdef UART_SIM
static volatile uint32_t* const pulAddrPrint    = (uint32_t*) uartTX_SIM_ADDR;
#else
static volatile uint32_t* const pulUartPrintTx  = (uint32_t*) uartTX_ADDR;
#endif
static volatile uint32_t* const pulUartStats    = (uint32_t*) uartSTATS_ADDR;
static volatile uint32_t* const pulUartRxCsr    = (uint32_t*) uartRX_ADDR;
static volatile uint32_t* const pulUartCfg      = (uint32_t*) uartCFG_ADDR;

/// \cond
// Private functions
static void __uart_itoa(uint32_t x, char *res) __attribute__((unused)); // GCC: do not output a warning when this variable is unused
static void __uart_tohex(uint32_t x, char *res) __attribute__((unused)); // GCC: do not output a warning when this variable is unused
static void __uart_touppercase(uint32_t len, char *ptr) __attribute__((unused)); // GCC: do not output a warning when this variable is unused
/// \endcond

#ifdef UART_SIM
  #warning [UART] - Verilator mode
#else
  #warning [UART] - Real serial mode
#endif
// #################################################################################################
// Override default STDIO functions
// #################################################################################################

/**********************************************************************//**
 * Send char via UART0
 *
 * @param[in] Char to be send.
 * @return Char that has been sent.
 **************************************************************************/
int putchar(int ch) {
  vUartPutc((char)ch);
  return ch;
}


/**********************************************************************//**
 * Read char from UART0.
 *
 * @return Read char.
 **************************************************************************/
int getchar(void) {
  return (int)ucUartGetc();
}

/**********************************************************************//**
 * Enable and configure primary UART (UART0).
 *
 * @note The 'uart_SIM_MODE' compiler flag will configure UART0 for simulation mode: all UART0 TX data will be redirected to simulation output. Use this for simulations only!
 * @note To enable simulation mode add <USER_FLAGS+=-Duart_SIM_MODE> when compiling.
 *
 * @warning The baud rate is computed using INTEGER operations (truncation errors might occur).
 *
 * @param[in] baudrate Targeted BAUD rate (e.g. 9600).
 **************************************************************************/
void vUartSetup(uint32_t baudrate) {
  *pulUartCfg = uartFREQ_SYSTEM/baudrate;
}

/**********************************************************************//**
 * Send single char via UART0.
 *
 * @note This function is blocking.
 *
 * @param[in] c Char to be send.
 **************************************************************************/
void vUartPutc(char c) {
#ifdef UART_SIM
  *pulAddrPrint = c;
#else
  while((*pulUartStats & 0x10000) == 0);
  *pulUartPrintTx = c;
#endif
}

/**********************************************************************//**
 * Get char from UART0.
 *
 * @note This function is blocking and does not check for UART frame/parity errors.
 *
 * @return Received char.
 **************************************************************************/
char ucUartGetc(void) {
  uint32_t d = 0;
  d = *pulUartRxCsr;
  return (char)d;
}


/**********************************************************************//**
 * Check if UART0 has received a char.
 *
 * @note This function is non-blocking.
 * @note Use uart_char_received_get(void) to get the char.
 *
 * @return =!0 when a char has been received.
 **************************************************************************/
uint32_t ulUartCharReceived(void) {
  if ((*pulUartStats & 0x1) == 0) {
    return 0;
  }
  else {
    return 1;
  }
}


/**********************************************************************//**
 * Get a received char from UART0.
 *
 * @note This function is non-blocking.
 * @note Should only be used in combination with uart_char_received(void).
 *
 * @return Received char.
 **************************************************************************/
char ucUartCharReceivedGet(void) {
  return (char)*pulUartRxCsr;
}


/**********************************************************************//**
 * Print string (zero-terminated) via UART0. Print full line break "\r\n" for every '\n'.
 *
 * @note This function is blocking.
 *
 * @param[in] s Pointer to string.
 **************************************************************************/
void vUartPrint(const char *pString) {
  char c = 0;
  while ((c = *pString++)) {
    if (c == '\n') {
      vUartPutc('\r');
    }
    vUartPutc(c);
  }
}


/**********************************************************************//**
 * Custom version of 'printf' function using UART0.
 *
 * @note This function is blocking.
 *
 * @param[in] format Pointer to format string.
 *
 * <TABLE>
 * <TR><TD>%s</TD><TD>String (array of chars, zero-terminated)</TD></TR>
 * <TR><TD>%c</TD><TD>Single char</TD></TR>
 * <TR><TD>%d/%i</TD><TD>32-bit signed number, printed as decimal</TD></TR>
 * <TR><TD>%u</TD><TD>32-bit unsigned number, printed as decimal</TD></TR>
 * <TR><TD>%x</TD><TD>32-bit number, printed as 8-char hexadecimal - lower-case</TD></TR>
 * <TR><TD>%X</TD><TD>32-bit number, printed as 8-char hexadecimal - upper-case</TD></TR>
 * <TR><TD>%p</TD><TD>32-bit pointer, printed as 8-char hexadecimal - lower-case</TD></TR>
 * </TABLE>
 **************************************************************************/
void vUartPrintf(const char *pformat, ...) {
  char c, string_buf[11];
  int32_t n;

  va_list a;
  va_start(a, pformat);

  while ((c = *pformat++)) {
    if (c == '%') {
      c = *pformat++;
      switch (c) {
        case 's': // string
          vUartPrint(va_arg(a, char*));
          break;
        case 'c': // char
          vUartPutc((char)va_arg(a, int));
          break;
        case 'i': // 32-bit signed
        case 'd':
          n = (int32_t)va_arg(a, int32_t);
          if (n < 0) {
            n = -n;
            vUartPutc('-');
          }
          __uart_itoa((uint32_t)n, string_buf);
          vUartPrint(string_buf);
          break;
        case 'u': // 32-bit unsigned
          __uart_itoa(va_arg(a, uint32_t), string_buf);
          vUartPrint(string_buf);
          break;
        case 'x': // 32-bit hexadecimal
        case 'p':
        case 'X':
          __uart_tohex(va_arg(a, uint32_t), string_buf);
          if (c == 'X') {
            __uart_touppercase(11, string_buf);
          }
          vUartPrint(string_buf);
          break;
        default: // unsupported format
          vUartPutc('%');
          vUartPutc(c);
          break;
      }
    }
    else {
      if (c == '\n') {
        vUartPutc('\r');
      }
      vUartPutc(c);
    }
  }
  va_end(a);
}

// #################################################################################################
// Shared functions
// #################################################################################################

/**********************************************************************//**
 * Private function for 'printf' to convert into decimal.
 *
 * @param[in] x Unsigned input number.
 * @param[in,out] res Pointer for storing the reuslting number string (11 chars).
 **************************************************************************/
static void __uart_itoa(uint32_t x, char *res) {

  static const char numbers[] = "0123456789";
  char buffer1[11];
  uint16_t i, j;

  buffer1[10] = '\0';
  res[10] = '\0';

  // convert
  for (i=0; i<10; i++) {
    buffer1[i] = numbers[x%10];
    x /= 10;
  }

  // delete 'leading' zeros
  for (i=9; i!=0; i--) {
    if (buffer1[i] == '0')
      buffer1[i] = '\0';
    else
      break;
  }

  // reverse
  j = 0;
  do {
    if (buffer1[i] != '\0')
      res[j++] = buffer1[i];
  } while (i--);

  res[j] = '\0'; // terminate result string
}


/**********************************************************************//**
 * Private function for 'printf' to convert into hexadecimal.
 *
 * @param[in] x Unsigned input number.
 * @param[in,out] res Pointer for storing the resulting number string (9 chars).
 **************************************************************************/
static void __uart_tohex(uint32_t x, char *res) {

  static const char symbols[] = "0123456789abcdef";

  int i;
  for (i=0; i<8; i++) { // nibble by nibble
    uint32_t num_tmp = x >> (4*i);
    res[7-i] = (char)symbols[num_tmp & 0x0f];
  }

  res[8] = '\0'; // terminate result string
}


/**********************************************************************//**
 * Private function to cast a string to UPPERCASE.
 *
 * @param[in] len Total length of input string.
 * @param[in,out] ptr Pointer for input/output string.
 **************************************************************************/
static void __uart_touppercase(uint32_t len, char *ptr) {

  char tmp;

  while (len > 0) {
    tmp = *ptr;
    if ((tmp >= 'a') && (tmp <= 'z')) {
      *ptr = tmp - 32;
    }
    ptr++;
    len--;
  }
}
