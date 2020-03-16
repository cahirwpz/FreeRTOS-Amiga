#ifndef _TRAP_H_
#define _TRAP_H_

#define T_UNKNOWN   0
#define T_BUSERR    1
#define T_ADDRERR   2
#define T_ILLINST   3
#define T_ZERODIV   4
#define T_CHKINST   5
#define T_TRAPVINST 6
#define T_PRIVINST  7
#define T_TRACE     8
#define T_FMTERR    9
#define T_TRAPINST  10
#define T_NTRAPS    11

#ifndef __ASSEMBLER__
#include <stdint.h>

/* Special Status Word: M68010 memory fault information */
#define SSW_RR 0x8000
#define SSW_IF 0x2000
#define SSW_DF 0x1000
#define SSW_RM 0x0800
#define SSW_HB 0x0400
#define SSW_BY 0x0200
#define SSW_RW 0x0100
#define SSW_FC 0x0007

#pragma pack(2)
typedef struct {
  uint16_t sr;
  uint32_t pc;
} trap_stk_000_t;

#pragma pack(2)
typedef struct {
  uint16_t status;
  uint32_t address;
  uint16_t instreg;
  uint16_t sr;
  uint32_t pc;
} trap_stk_000_memacc_t;

#pragma pack(2)
typedef struct {
  uint16_t sr;
  uint32_t pc;
  uint16_t format;
} trap_stk_010_t;

#pragma pack(2)
typedef struct {
  uint16_t sr;
  uint32_t pc;
  uint16_t format;
  uint16_t ssw;
  uint32_t address;
  uint16_t pad[22];
} trap_stk_010_memacc_t;

#pragma pack(2)
typedef struct TrapFrame {
  uint32_t d0, d1, d2, d3, d4, d5, d6, d7;
  uint32_t a0, a1, a2, a3, a4, a5, a6, sp;
  uint16_t trapnum;
  union {
    trap_stk_000_t m68000;
    trap_stk_000_memacc_t m68000_memacc;
    trap_stk_010_t m68010;
    trap_stk_010_memacc_t m68010_memacc;
  } u;
} TrapFrame_t;

#define m68000 u.m68000
#define m68000_memacc u.m68000_memacc
#define m68010 u.m68010
#define m68010_memacc u.m68010_memacc

void BadTrap(void);
void BusErrTrap(void);
void AddrErrTrap(void);
void IllegalTrap(void);
void ZeroDivTrap(void);
void ChkInstTrap(void);
void TrapvInstTrap(void);
void PrivInstTrap(void);
void TraceTrap(void);
void FmtErrTrap(void);
void TrapInstTrap(void);
#endif /* !__ASSEMBLER__ */

#endif /* !_TRAP_H_ */
