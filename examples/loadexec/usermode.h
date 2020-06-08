#ifndef _USERMODE_H_
#define _USERMODE_H_

typedef struct ExitMsg {
  int status;
} ExitMsg_t;

void RunProgram(File_t *exe);

#endif /* !_USERMODE_H_ */
