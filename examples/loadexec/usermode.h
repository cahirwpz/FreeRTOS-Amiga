#ifndef _USERMODE_H_
#define _USERMODE_H_

#include <cdefs.h>

#define USERMODE_STACK_SIZE (configMINIMAL_STACK_SIZE * 4)
#define KERNMODE_STACK_SIZE (configMINIMAL_STACK_SIZE * 2)

int EnterUserMode(void *pc, void *sp);
__noreturn void ExitUserMode(uintptr_t ctx, int status);
int RunProgram(File_t *exe, size_t ustksz);

#endif /* !_USERMODE_H_ */
