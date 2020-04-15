#ifndef _CONSOLE_H_
#define _CONSOLE_H_

struct bitmap;
struct font;
typedef struct File File_t;

File_t *ConsoleOpen(struct bitmap *bm, struct font *font);

#endif
