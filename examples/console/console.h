#ifndef _CONSOLE_H_
#define _CONSOLE_H_

void ConsoleInit(void);
void ConsoleSetCursor(short x, short y);
void ConsoleDrawCursor(void);
void ConsolePutChar(char c);
void ConsoleWrite(const char *buf, size_t nbyte);

void ConsoleMovePointer(short x, short y);

#endif
