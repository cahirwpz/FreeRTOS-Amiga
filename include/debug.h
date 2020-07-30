#ifndef _DEBUG_H_
#define _DEBUG_H_

#if DEBUG
#include <FreeRTOSConfig.h>
#include <stdio.h>

#define DASSERT(x) configASSERT(x)
#define DPRINTF(...) printf(__VA_ARGS__)
#else
#define DASSERT(x)
#define DPRINTF(...)
#endif

#endif /* !__DEBUG_H__ */
