#pragma once

#include <uae.h>
#include <cpu.h>

#define Log(...) UaeLog(__VA_ARGS__)
#define Panic(...)                                                             \
  {                                                                            \
    UaeLog(__VA_ARGS__);                                                       \
    PANIC();                                                                   \
  }

#ifdef NDEBUG
#define Assert(e) __nothing
#else
#define Assert(e)                                                              \
  {                                                                            \
    if (!(e))                                                                  \
      PANIC();                                                                 \
  }
#endif

#if DEBUG && !defined(NDEBUG)
#define DASSERT(e) Assert(e)
#define DLOG(...) Log(__VA_ARGS__)
#else
#define DASSERT(e) __nothing
#define DLOG(...) __nothing
#endif
