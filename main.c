#include <FreeRTOS.h>
#include <task.h>

#include <hardware.h>
#include <exception.h>
#include <interrupt.h>
#include <trap.h>
#include <cpu.h>
#include <stdio.h>

#define mainRED_TASK_PRIORITY 3
#define mainGREEN_TASK_PRIORITY 3

static void vRedTask(void *) {
  for (;;) {
    custom->color[0] = 0xf00;
  }
}

static void vGreenTask(void *) {
  for (;;) {
    custom->color[0] = 0x0f0;
  }
}

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
  [T_FMTERR] = "Stack Format Error"
};

void TrapHandler(trap_frame_t *frame) {
  int memflt = frame->trapnum == T_BUSERR || frame->trapnum == T_ADDRERR;

  /* We need to fix stack pointer, as processor pushes data on stack before it
   * enters the trap handler. */
  if (CpuModel > CF_68000) {
    frame->sp += memflt ? sizeof(frame->m68010_memacc) : sizeof(frame->m68010);
  } else {
    frame->sp += memflt ? sizeof(frame->m68000_memacc) : sizeof(frame->m68000);
  }

  dprintf("Exception: %s!\n"
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

  dprintf(" PC: %08x SR: %04x\n", pc, sr);

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

    dprintf("%s %s at $%08x!\n", (data ? "Instruction" : "Data"),
            (read ? "read" : "write"), addr);
  }
}

static ISR(VertBlankHandler) {
  /* Clear the interrupt. */
  custom->intreq = INTF_VERTB;

  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  BaseType_t xSwitchRequired = xTaskIncrementTick();
  portYIELD_FROM_ISR(xSwitchRequired);
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

static void SystemTimerInit(void) {
  SetIntVec(VERTB, VertBlankHandler);
  custom->intena = INTF_SETCLR | INTF_VERTB;
}

static xTaskHandle red_handle;
static xTaskHandle green_handle;

int main(void) {
  SystemTimerInit();

  xTaskCreate(vRedTask, "red", configMINIMAL_STACK_SIZE, NULL,
              mainRED_TASK_PRIORITY, &red_handle);

  xTaskCreate(vGreenTask, "green", configMINIMAL_STACK_SIZE, NULL,
              mainGREEN_TASK_PRIORITY, &green_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom->color[0] = 0x00f; }

void vApplicationMallocFailedHook(void) {
  dprintf("Memory exhausted!\n");
  portHALT();
}

void vApplicationStackOverflowHook(void) {
  dprintf("Stack overflow!\n");
  portHALT();
}
