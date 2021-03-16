#pragma once

#include <sys/cdefs.h>

typedef struct Device Device_t;

Device_t *SerialInit(unsigned baud);
void SerialKill(void);
