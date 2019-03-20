#include <hardware.h>
#include <exception.h>
#include <interrupt.h>

#define DISPATCH(x)                                                            \
  if (custom->intreqr & INTF_##x)                                              \
    IntVec[INTB_##x]();                                                        \

ISR(AmigaLvl1Handler) {
  DISPATCH(TBE);
  DISPATCH(DSKBLK);
  DISPATCH(SOFTINT);
}

ISR(AmigaLvl2Handler) {
  IntVec[INTB_PORTS]();
}

ISR(AmigaLvl3Handler) {
  DISPATCH(VERTB);
  DISPATCH(BLIT);
  DISPATCH(COPER);
}

ISR(AmigaLvl4Handler) {
  DISPATCH(AUD0);
  DISPATCH(AUD1);
  DISPATCH(AUD2);
  DISPATCH(AUD3);
}

ISR(AmigaLvl5Handler) {
  DISPATCH(DSKSYNC);
  DISPATCH(RBF);
}

ISR(AmigaLvl6Handler) {
  IntVec[INTB_EXTER]();
}
