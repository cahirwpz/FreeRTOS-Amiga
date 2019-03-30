#include <cia.h>
#include <interrupt.h>

/* All TOD registers latch on a read of MSB event and remain latched
 * until after a read of LSB event. */

uint32_t ReadLineCounter(void) {
  uint32_t res = 0;
  DisableINT(INTF_INTEN);
  res |= ciab->ciatodhi;
  res <<= 8;
  res |= ciab->ciatodmid;
  res <<= 8;
  res |= ciab->ciatodlow;
  EnableINT(INTF_INTEN);
  return res;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */

void SetLineCounter(uint32_t frame) {
  DisableINT(INTF_INTEN);
  ciab->ciatodhi = frame >> 16;
  ciab->ciatodmid = frame >> 8;
  ciab->ciatodlow = frame;
  EnableINT(INTF_INTEN);
}
