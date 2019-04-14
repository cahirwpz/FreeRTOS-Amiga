#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdarg.h>

void SerialInit(unsigned baud);
void SerialKill(void);
void SerialPrint(const char *format, ...);
void SerialPutChar(char data);
int SerialGetChar(void);

#endif /* !_SERIAL_H_ */
