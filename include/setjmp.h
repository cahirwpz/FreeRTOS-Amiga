#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <cdefs.h>

#define _JB_D2 0
#define _JB_D3 1
#define _JB_D4 2
#define _JB_D5 3
#define _JB_D6 4
#define _JB_D7 5
#define _JB_A2 6
#define _JB_A3 7
#define _JB_A4 8
#define _JB_A5 9
#define _JB_A6 10
#define _JB_A6 11
#define _JB_PC 12

#define _JBLEN 13

typedef int32_t jmp_buf[_JBLEN];

__returns_twice int setjmp(jmp_buf);
__noreturn void longjmp(jmp_buf, int);

#endif /* !_SETJMP_H_ */
