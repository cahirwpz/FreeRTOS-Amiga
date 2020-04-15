#include <FreeRTOS/FreeRTOS.h>

#include <trap.h>
#include <cpu.h>
#include <stdio.h>

static const char *const trapname[T_NTRAPS] = {
  [T_UNKNOWN] = "Bad Trap",
  [T_BUSERR] = "Bus Error",
  [T_ADDRERR] = "Address Error",
  [T_ILLINST] = "Illegal Instruction",
  [T_ZERODIV] = "Division by Zero",
  [T_CHKINST] = "CHK Instruction",
  [T_TRAPVINST] = "TRAPV Instruction",
  [T_PRIVINST] = "Privileged Instruction",
  [T_TRACE] = "Trace",
  [T_FMTERR] = "Stack Format Error",
  [T_TRAPINST] = "Trap Instruction"
};

void vPortDefaultTrapHandler(struct TrapFrame *frame) {
  int memflt = frame->trapnum == T_BUSERR || frame->trapnum == T_ADDRERR;

  /* We need to fix stack pointer, as processor pushes data on stack before it
   * enters the trap handler. */
  if (CpuModel > CF_68000) {
    frame->sp += memflt ? sizeof(frame->m68010_memacc) : sizeof(frame->m68010);
  } else {
    frame->sp += memflt ? sizeof(frame->m68000_memacc) : sizeof(frame->m68000);
  }

  printf("Exception: %s!\n"
         " D0: %08x D1: %08x D2: %08x D3: %08x\n"
         " D4: %08x D5: %08x D6: %08x D7: %08x\n"
         " A0: %08x A1: %08x A2: %08x A3: %08x\n"
         " A4: %08x A5: %08x A6: %08x SP: %08x\n",
         trapname[frame->trapnum],
         frame->d0, frame->d1, frame->d2, frame->d3,
         frame->d4, frame->d5, frame->d6, frame->d7,
         frame->a0, frame->a1, frame->a2, frame->a3,
         frame->a4, frame->a5, frame->a6, frame->sp);

  uint32_t pc;
  uint16_t sr;

  if (CpuModel > CF_68000) {
    pc = frame->m68010.pc;
    sr = frame->m68010.sr;
  } else {
    if (memflt) {
      pc = frame->m68000_memacc.pc;
      sr = frame->m68000_memacc.sr;
    } else {
      pc = frame->m68000.pc;
      sr = frame->m68000.sr;
    }
  }

  printf(" PC: %08x SR: %04x\n", pc, sr);

  if (memflt) {
    uint32_t addr;
    int data, read;

    if (CpuModel > CF_68000) {
      addr = frame->m68010_memacc.address;
      data = frame->m68010_memacc.ssw & SSW_DF;
      read = frame->m68010_memacc.ssw & SSW_RW;
    } else {
      addr = frame->m68000_memacc.address;
      data = frame->m68000_memacc.status & 8;
      read = frame->m68000_memacc.status & 16;
    }

    printf("%s %s at $%08x!\n", (data ? "Instruction" : "Data"),
           (read ? "read" : "write"), addr);
  }

  portHALT();
}

__weak_alias(vPortTrapHandler, vPortDefaultTrapHandler);
