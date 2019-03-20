#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <hardware/custom.h>
#include <hardware/cia.h>
#include <hardware/adkbits.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>

typedef struct Custom *const Custom_t;
typedef struct CIA *const CIA_t;

extern volatile Custom_t custom;
extern volatile CIA_t ciaa;
extern volatile CIA_t ciab;

#endif /* !_HARDWARE_H_ */
