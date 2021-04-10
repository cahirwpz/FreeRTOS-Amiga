#pragma once

#include <sys/cdefs.h>

#define LMB BIT(0)
#define RMB BIT(1)

typedef struct Device Device_t;

Device_t *MouseInit(void);
