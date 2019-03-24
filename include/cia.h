#ifndef _CIA_H_
#define _CIA_H_

#include <cia_regdef.h>

typedef struct CIA *const CIA_t;

extern volatile CIA_t ciaa;
extern volatile CIA_t ciab;

#define E_CLOCK 709379 /* ticks per second */
#define TIMER_MS(ms) ((ms) * E_CLOCK / 1000)
#define TIMER_US(us) ((us) * E_CLOCK / (1000 * 1000))

/* Maximum delay is around 92.38ms */
void WaitTimerA(CIA_t cia, uint16_t delay);
void WaitTimerB(CIA_t cia, uint16_t delay);

/* 24-bit frame counter offered by CIA A */
uint32_t ReadFrameCounter(void);
void SetFrameCounter(uint32_t frame);

/* 24-bit frame counter offered by CIA B */
uint32_t ReadLineCounter(void);
void SetLineCounter(uint32_t line);

#endif
