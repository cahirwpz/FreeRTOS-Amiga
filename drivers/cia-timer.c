#include <cia.h>

void WaitTimerA(CIA_t cia, uint16_t delay) {
  cia->ciacra |= CIACRAF_RUNMODE;
  cia->ciaicr = CIAICRF_TA;
  cia->ciatalo = delay;
  cia->ciatahi = delay >> 8;
  while (!(REGB(cia->ciaicr) & CIAICRF_TA));
}

void WaitTimerB(CIA_t cia, uint16_t delay) {
  cia->ciacra |= CIACRBF_RUNMODE;
  cia->ciaicr = CIAICRF_TB;
  cia->ciatalo = delay;
  cia->ciatahi = delay >> 8;
  while (!(REGB(cia->ciaicr) & CIAICRF_TB));
}
