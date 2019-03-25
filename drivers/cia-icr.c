#include <cia.h>

static uint8_t _ICREnabled[2]; /* enabled interrupts mask */
static uint8_t _ICRPending[2]; /* pending interrupts mask */

/* If lower CIA address bit is set it's CIA-A, otherwise CIA-B. */
#define ICREnabled(cia) &_ICREnabled[(intptr_t)cia & 1]
#define ICRPending(cia) &_ICRPending[(intptr_t)cia & 1]

uint8_t WriteICR(CIA_t cia, uint8_t mask) {
  /* Write real ICR */
  cia->_ciaicr = mask;
  /* Read cached enabled interrupts bitmask. */
  uint8_t *enabled = ICREnabled(cia);
  /* Modify cached value accordingly to the mask. */
  if (mask & CIAICRF_SETCLR)
    *enabled |= mask & ~CIAICRF_SETCLR;
  else
    *enabled &= ~mask;
  /* Return enabled interrupts bitmask. */
  return *enabled;
}

uint8_t SampleICR(CIA_t cia, uint8_t mask) {
  /* Read cached pending interrupts bitmask. */
  uint8_t *pending = ICRPending(cia);
  /* Read real ICR and or its value into pending bitmask. */
  uint8_t icr = *pending | cia->_ciaicr;
  /* Clear bits masked by the user in cached bitmask. */
  *pending = icr & ~mask;
  /* Return only those bits that were requested by the user. */
  return icr & mask;
}
