#ifndef SOC_TYPES_H
#define SOC_TYPES_H

#include <stdint.h>
#include "portmacro.h"

typedef enum {
  MASTER_STATUS_IDLE      = 0,
  MASTER_STATUS_RUNNING   = 1,
  MASTER_STATUS_COMPLETE  = 2,
  MASTER_STATUS_ERROR     = 3
} HostType_t;

typedef enum {
  CMD_NONE       = 0,
  CMD_HISTOGRAM  = 1,
  CMD_TEST       = 2
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
