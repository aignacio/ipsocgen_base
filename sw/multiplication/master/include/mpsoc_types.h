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

typedef struct {
  uint32_t factor;
  uint32_t times;
} Oper_t;

#endif
