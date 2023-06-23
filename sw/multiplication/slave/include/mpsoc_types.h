#ifndef MPSOC_TYPES_H
#define MPSOC_TYPES_H

#include <stdint.h>
#include "portmacro.h"

typedef enum {
  NOC_NONE,
  NOC_CMD,
  NOC_DATA,
  NOC_PROFILING
} CmdType_t;

typedef uint32_t SizePkt_t;

typedef struct {
  TickType_t  Timestamp;
  CmdType_t   Cmd;
  SizePkt_t   Size;
} Cmd_t;

#endif
