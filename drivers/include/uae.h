#pragma once

/*
 * Corresponding interface is implemented in fs-uae:
 * procedure uaelib_demux_common in src/uaelib.cpp file
 */

extern int UaeCallTrap(int i, ...);

enum {
  uaelib_HardReset = 3,
  uaelib_Reset = 4,
  uaelib_ExitEmu = 13,
  uaelib_Log = 40,
};

#define UaeHardReset() UaeCallTrap(uaelib_HardReset)
#define UaeReset() UaeCalTrap(uaelib_Reset)
#define UaeExit() UaeCallTrap(uaelib_ExitEmu)
#define UaeLog(...) UaeCallTrap(uaelib_Log, __VA_ARGS__)
