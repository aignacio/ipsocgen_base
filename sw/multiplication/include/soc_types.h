#ifndef SOC_TYPES_H
#define SOC_TYPES_H

#include <stdint.h>
#include "portmacro.h"

typedef enum {
  MASTER_STATUS_IDLE      = 0,
  MASTER_STATUS_RUNNING   = 1,
  MASTER_STATUS_COMPLETE  = 2,
  MASTER_STATUS_ERROR     = 3
} MasterType_t;

typedef enum {
  CMD_NONE        = 0,
  CMD_RECV_ARRAY  = 1,
  CMD_MULT_FACTOR = 2,
  CMD_GET_RESULT  = 3
} CmdType_t;

typedef uint32_t arg_t;

typedef union {
  uint32_t word;
  struct {
    CmdType_t pkt_type:2;
    arg_t     factor:15; // Array size in KiB
    arg_t     times:15; // Factor
  } st;
} Cmd_t;

typedef struct {
  uint32_t factor;
  uint32_t times;
} Oper_t;

typedef uint32_t Dim_t;
#endif
