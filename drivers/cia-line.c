#include <cia.h>

/* All TOD registers latch on a read of MSB event and remain latched
 * until after a read of LSB event. */

uint32_t ReadLineCounter(void) {
  uint32_t res = 0;
  res |= ciab->ciatodhi;
  res <<= 8;
  res |= ciab->ciatodmid;
  res <<= 8;
  res |= ciab->ciatodlow;
  return res;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */

void SetLineCounter(uint32_t frame) {
  ciab->ciatodhi = frame >> 16;
  ciab->ciatodmid = frame >> 8;
  ciab->ciatodlow = frame;
}
