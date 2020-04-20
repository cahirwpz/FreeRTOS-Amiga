#include <custom.h>
#include "syscalls.h"

#define MSG(x) x, (sizeof(x) - 1)

/* WARNING! Since executable loader is very primitive _start procedure
 * MUST BE first one in .text section of the program! */
void _start(void) {
  File_t *ser = open("SER:");

  for(;;) {
    static char firstName[80];
    static char lastName[80];

    write(ser, MSG("First name: "));
    long firstLen = read(ser, firstName, sizeof(firstName));
    write(ser, MSG("Last name: "));
    long lastLen = read(ser, lastName, sizeof(lastName));

    write(ser, MSG("Hello, "));
    write(ser, firstName, firstLen - 1);
    write(ser, MSG(" "));
    write(ser, lastName, lastLen - 1);
    write(ser, MSG("!\n"));
  }
}
