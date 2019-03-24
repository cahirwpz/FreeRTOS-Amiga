#include <cia.h>

/* All TOD registers latch on a read of MSB event and remain latched
 * until after a read of LSB event. */

uint32_t ReadFrameCounter(void) {
  uint32_t res = 0;
  res |= ciaa->ciatodhi;
  res <<= 8;
  res |= ciaa->ciatodmid;
  res <<= 8;
  res |= ciaa->ciatodlow;
  return res;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */

void SetFrameCounter(uint32_t frame) {
  ciaa->ciatodhi = frame >> 16;
  ciaa->ciatodmid = frame >> 8;
  ciaa->ciatodlow = frame;
}
