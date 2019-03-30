#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <cia.h>

#define TMRF_ACQUIRED BIT(0)

typedef struct CIATimer {
  volatile TaskHandle_t waiter;
  CIA_t cia;
  uint8_t icr;
  uint8_t flags;
} CIATimer_t;

static CIATimer_t timer[4] = {
  [TIMER_CIAA_A] = {.cia = CIAA, .icr = CIAICRF_TA},
  [TIMER_CIAA_B] = {.cia = CIAA, .icr = CIAICRF_TB},
  [TIMER_CIAB_A] = {.cia = CIAB, .icr = CIAICRF_TA},
  [TIMER_CIAB_B] = {.cia = CIAB, .icr = CIAICRF_TB}
};

/* Interrupt handler for CIA-A timers. */
static void CIATimerHandler(CIATimer_t *tmr) {
  if (SampleICR(tmr->cia, tmr->icr)) {
    /* Wake up sleeping task and disable interrupt the timer. */
    vTaskNotifyGiveFromISR(tmr->waiter, NULL);
    tmr->waiter = NULL;
  }
}

static INTSERVER(CIAATimerA, 0, (ISR_t)CIATimerHandler, &timer[TIMER_CIAA_A]);
static INTSERVER(CIAATimerB, 0, (ISR_t)CIATimerHandler, &timer[TIMER_CIAA_B]);
static INTSERVER(CIABTimerA, 0, (ISR_t)CIATimerHandler, &timer[TIMER_CIAB_A]);
static INTSERVER(CIABTimerB, 0, (ISR_t)CIATimerHandler, &timer[TIMER_CIAB_B]);

void TimerInit(void) {
  /* CIA-A & CIA-B: Stop timers! */
  CIAA->ciacra = 0;
  CIAA->ciacrb = 0;
  CIAB->ciacra = 0;
  CIAB->ciacrb = 0;

  /* CIA-A & CIA-B: Disable timer interrupts. */
  WriteICR(CIAA, CIAICRF_TA|CIAICRF_TB);
  WriteICR(CIAB, CIAICRF_TA|CIAICRF_TB);

  AddIntServer(PortsChain, CIAATimerA);
  AddIntServer(PortsChain, CIAATimerB);
  AddIntServer(ExterChain, CIABTimerA);
  AddIntServer(ExterChain, CIABTimerB);

  ClearIRQ(INTF_PORTS|INTF_EXTER);
  EnableINT(INTF_PORTS|INTF_EXTER);
}

int AcquireTimer(unsigned num) {
  int result = -1;

  configASSERT(num <= TIMER_CIAB_B || num == TIMER_ANY);

  vTaskSuspendAll();
  {
    /* Determine timer number. */
    if (num == TIMER_ANY) {
      for (num = TIMER_CIAA_A; num <= TIMER_CIAB_B; num++)
        if (!(timer[num].flags & TMRF_ACQUIRED))
          break;
    }

    /* Check if selected timer is not busy. */
    if (num <= TIMER_CIAB_B) {
      CIATimer_t *tmr = &timer[num];
      if (!(tmr->flags & TMRF_ACQUIRED)) {
        tmr->flags |= TMRF_ACQUIRED;
        result = num;
      }
    }
  }
  xTaskResumeAll();

  return result;
}

void ReleaseTimer(unsigned num) {
  vTaskSuspendAll();
  {
    configASSERT(num <= TIMER_CIAB_B);
    CIATimer_t *tmr = &timer[num];
    configASSERT(tmr->flags & TMRF_ACQUIRED);
    tmr->flags &= ~TMRF_ACQUIRED;
  }
  xTaskResumeAll();
}

/* Timer uses interrupts when waiting for events that happen in more than
 * 4 raster lines (each takes 64us). Otherwise it spins. */
#define DELAY_SPIN TIMER_US(64*4)

void WaitTimer(unsigned num, uint16_t delay) {
  configASSERT(num <= TIMER_CIAB_B);
  CIATimer_t *tmr = &timer[num];
  configASSERT(tmr->flags & TMRF_ACQUIRED);
  CIA_t cia = (num & 2) ? CIAB : CIAA;
  uint8_t timer = (num & 1) ? CIAICRF_TB : CIAICRF_TA;

  taskENTER_CRITICAL();
  {
    /* Load counter and start timer in one-shot mode. */
    if (num & 1) {
      cia->ciacrb  = CIACRBF_LOAD;
      cia->ciatblo = delay;
      cia->ciatbhi = delay >> 8;
      cia->ciacrb  = CIACRBF_RUNMODE | CIACRBF_START;
    } else {
      cia->ciacra  = CIACRBF_LOAD;
      cia->ciatalo = delay;
      cia->ciatahi = delay >> 8;
      cia->ciacra  = CIACRAF_RUNMODE | CIACRAF_START;
    }
  }
  taskEXIT_CRITICAL();

  if ((delay >= DELAY_SPIN) && 
      (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
    /* If delay is long enough, then turn on the interrupt and go to sleep. */
    WriteICR(cia, CIAICRF_SETCLR | timer);
    tmr->waiter = xTaskGetCurrentTaskHandle();
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  } else {
    /* If delay is short, then just spin. */
    while (!SampleICR(cia, timer));
  }
}
