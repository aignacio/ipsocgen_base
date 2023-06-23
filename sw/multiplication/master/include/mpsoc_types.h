#ifndef MPSOC_TYPES_H
#define MPSOC_TYPES_H

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
    arg_t     arg1:15; // Array size in KiB
    arg_t     arg2:15; // Factor
  } st;
} Cmd_t;

#endif
