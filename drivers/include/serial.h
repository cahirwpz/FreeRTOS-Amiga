#pragma once

#include <file.h>

File_t *SerialOpen(unsigned baud);

void SerialInit(unsigned baud);
void SerialKill(void);
void SerialPutChar(char data);
int SerialGetChar(void);
