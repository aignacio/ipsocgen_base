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
  CMD_NONE       = 0,
  CMD_FILTER     = 1,
  CMD_HISTOGRAM  = 2,
  CMD_GET_RESULT = 3,
  CMD_TEST       = 4
} CmdType_t;

typedef uint32_t Dim_t;

typedef union {
  uint32_t word;
  struct {
    CmdType_t pkt_type:12;
    Dim_t     dim_y:10;
    Dim_t     dim_x:10;
  } st;
} Cmd_t;

#endif