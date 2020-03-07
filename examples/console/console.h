#ifndef _CONSOLE_H_
#define _CONSOLE_H_

struct bitmap;
struct font;

void ConsoleInit(struct bitmap *bm, struct font *font);
void ConsoleSetCursor(short x, short y);
void ConsoleDrawCursor(void);
void ConsolePutChar(char c);
void ConsolePrintf(const char *fmt, ...);

#endif
