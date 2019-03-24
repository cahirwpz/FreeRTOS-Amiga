#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <cia.h>

#define TMRF_ACQUIRED BIT(0)

typedef struct CIATimer {
  volatile TaskHandle_t waiter;
  uint8_t flags;
} CIATimer_t;

static CIATimer_t timer[4];

/* Interrupt handler for CIA-A timers. */
static ISR(CIAATimerHandler) {
  CIA_t cia = CIAA;

  /* Interrupt Control Register gets zeroed out during a read! */
  uint8_t icr = cia->ciaicr;

  ClearIRQ(PORTS);

  if (icr & CIAICRF_TA) {
    CIATimer_t *tmr = &timer[TIMER_CIAA_A];
    /* Wake up sleeping task and disable interrupt for Timer A. */
    vTaskNotifyGiveFromISR(tmr->waiter, NULL);
    tmr->waiter = NULL;
    cia->ciaicr = CIAICRF_TA;
  }

  if (icr & CIAICRF_TB) {
    CIATimer_t *tmr = &timer[TIMER_CIAA_B];
    /* Wake up sleeping task and disable interrupt for Timer B. */
    vTaskNotifyGiveFromISR(tmr->waiter, NULL);
    tmr->waiter = NULL;
    cia->ciaicr = CIAICRF_TB;
  }
}

/* Interrupt handler for CIA-B timers. */
static ISR(CIABTimerHandler) {
  CIA_t cia = CIAB;

  /* Interrupt Control Register gets zeroed out during a read! */
  uint8_t icr = cia->ciaicr;

  ClearIRQ(EXTER);

  if (icr & CIAICRF_TA) {
    CIATimer_t *tmr = &timer[TIMER_CIAB_A];
    /* Wake up sleeping task and disable interrupt for Timer A. */
    vTaskNotifyGiveFromISR(tmr->waiter, NULL);
    tmr->waiter = NULL;
    cia->ciaicr = CIAICRF_TA;
  }

  if (icr & CIAICRF_TB) {
    CIATimer_t *tmr = &timer[TIMER_CIAB_B];
    /* Wake up sleeping task and disable interrupt for Timer B. */
    vTaskNotifyGiveFromISR(tmr->waiter, NULL);
    tmr->waiter = NULL;
    cia->ciaicr = CIAICRF_TB;
  }
}

void TimerInit(void) {
  /* CIA-A & CIA-B: Stop timers! */
  CIAA->ciacra = 0;
  CIAA->ciacrb = 0;
  CIAB->ciacra = 0;
  CIAB->ciacrb = 0;

  /* CIA-A & CIA-B: Disable interrupts. */
  CIAA->ciaicr = CIAICRF_TA|CIAICRF_TB;
  CIAB->ciaicr = CIAICRF_TA|CIAICRF_TB;

  SetIntVec(PORTS, CIAATimerHandler);
  ClearIRQ(PORTS);
  EnableINT(PORTS);

  SetIntVec(EXTER, CIABTimerHandler);
  ClearIRQ(EXTER);
  EnableINT(EXTER);
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
    cia->ciaicr = CIAICRF_SETCLR | timer;
    tmr->waiter = xTaskGetCurrentTaskHandle();
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  } else {
    /* If delay is short, then just spin. */
    while (!(REGB(cia->ciaicr) & timer));
  }
}
