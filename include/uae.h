#ifndef _UAE_H_
#define _UAE_H_

/*
 * Corresponding interface is implemented in fs-uae:
 * procedure uaelib_demux_common in src/uaelib.cpp file
 */

#define CALLTRAP ((void *)0xF0FF60)

enum {
  emulib_HardReset = 3,
  emulib_Reset = 4,
  emulib_ExitEmu = 13,
};

static inline void UaeExit(void) {
  int (*calltrap)(int i) = CALLTRAP;
  calltrap(emulib_ExitEmu);
}

#endif
