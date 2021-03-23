#pragma once

#if DEBUG
#include <FreeRTOSConfig.h>
#include <portmacro.h>
#include <libkern.h>

#define DASSERT(x) configASSERT(x)
#define DPRINTF(...) kprintf(__VA_ARGS__)
#else
#define DASSERT(x) __nothing
#define DPRINTF(...) __nothing
#endif
