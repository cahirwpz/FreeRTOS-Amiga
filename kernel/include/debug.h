#pragma once

#if DEBUG
#include <FreeRTOSConfig.h>
#include <stdio.h>

#define DASSERT(x) configASSERT(x)
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DASSERT(x)
#define DPRINTF(...)
#endif
