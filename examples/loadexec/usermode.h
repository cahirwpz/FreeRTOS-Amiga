#ifndef _USERMODE_H_
#define _USERMODE_H_

int EnterUserMode(void *pc, void *sp);
int RunProgram(File_t *exe, size_t ustksz);

#endif /* !_USERMODE_H_ */
