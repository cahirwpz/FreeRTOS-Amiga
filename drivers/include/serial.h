#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <file.h>

File_t *SerialOpen(unsigned baud);

void SerialInit(unsigned baud);
void SerialKill(void);
void SerialPutChar(char data);
int SerialGetChar(void);

#endif /* !_SERIAL_H_ */
