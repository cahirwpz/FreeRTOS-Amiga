#ifndef _CONSOLE_H_
#define _CONSOLE_H_

typedef struct File File_t;
struct bitmap;
struct font;

void ConsoleInit(struct bitmap *bm, struct font *font);
File_t *ConsoleOpen(void);

void ConsoleSetCursor(short x, short y);
void ConsoleDrawCursor(void);
void ConsolePutChar(char c);

#endif
