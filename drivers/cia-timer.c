#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <cia.h>

/* Bitmask marking TIMER_* timers being in use. */
static uint8_t InUse;

/* Defines timer state after it has been acquired. */
struct CIATimer {
  IntServer_t server;
  volatile TaskHandle_t waiter;
  CIA_t cia;
  uint8_t icr;
  uint8_t num;
};

/* Interrupt handler for CIA timers. */
static void CIATimerHandler(CIATimer_t *timer) {
  CIA_t cia = timer->cia;
  uint8_t icr = timer->icr;

  if (SampleICR(cia, icr)) {
    /* Wake up sleeping task and disable interrupt for the timer. */
    vTaskNotifyGiveFromISR(timer->waiter, &xNeedRescheduleTask);
    timer->waiter = NULL;
    WriteICR(cia, icr);
  }
}

/* Statically allocated CIA timers. */
#define TIMER(CIA, TIMER)                                                      \
  [TIMER_##CIA##_##TIMER] = {                                                  \
    .num = TIMER_##CIA##_##TIMER, .cia = CIA, .icr = CIAICRF_T##TIMER,         \
    .server = INTSERVER(0, (ISR_t)CIATimerHandler,                             \
                        &Timers[TIMER_##CIA##_##TIMER])}

CIATimer_t Timers[4] = {
  TIMER(CIAA, A),
  TIMER(CIAA, B),
  TIMER(CIAB, A),
  TIMER(CIAB, B)
};

/* Implementation of procedures declared in header file. */

CIATimer_t *AcquireTimer(unsigned num) {
  CIATimer_t *timer = NULL;

  configASSERT(num <= TIMER_CIAB_B || num == TIMER_ANY);

  vTaskSuspendAll();
  {
    /* Allocate a timer that is not in use and its' number matches
     * (if specified instead of wildcard). */
    for (int i = TIMER_CIAA_A; i <= TIMER_CIAB_B; i++) {
      if (!BTST(InUse, i) && (num == TIMER_ANY || num == i)) {
        BSET(InUse, i);
        timer = &Timers[i];
        num = i;
        break;
      }
    }
  }
  xTaskResumeAll();

  if (timer) {
    IntChain_t *chain = (num & 2) ? ExterChain : PortsChain;
    timer->server.node.pvOwner = timer;
    AddIntServer(chain, &timer->server);
  }

  return timer;
}

void ReleaseTimer(CIATimer_t *timer) {
  vTaskSuspendAll();
  {
    RemIntServer(&timer->server);
    BCLR(InUse, timer->num);
  }
  xTaskResumeAll();
}

void WaitTimerGeneric(CIATimer_t *timer, uint16_t delay, bool spin) {
  CIA_t cia = timer->cia;
  uint8_t icr = timer->icr;

  taskENTER_CRITICAL();
  {
    /* Load counter and start timer in one-shot mode. */
    if (icr == CIAICRF_TB) {
      cia->ciacrb  = CIACRBF_LOAD;
      cia->ciatblo = delay;
      cia->ciatbhi = delay >> 8;
      cia->ciacrb  = CIACRBF_RUNMODE | CIACRBF_START;
    } else {
      cia->ciacra  = CIACRAF_LOAD;
      cia->ciatalo = delay;
      cia->ciatahi = delay >> 8;
      cia->ciacra  = CIACRAF_RUNMODE | CIACRAF_START;
    }
  }
  taskEXIT_CRITICAL();

  if (spin || (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)) {
    /* The scheduler is not active or we were requested to busy wait. */
    WriteICR(cia, icr);
    while (!SampleICR(cia, icr));
  } else {
    /* Turn on the interrupt and go to sleep. */
    timer->waiter = xTaskGetCurrentTaskHandle();
    WriteICR(cia, CIAICRF_SETCLR | icr);
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }
}
