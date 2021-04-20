#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS_CLI.h>

#include <debug.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <file.h>
#include <devfile.h>
#include <driver.h>
#include <string.h>
#include <interrupt.h>
#include <tty.h>

#define SHELL_TASK_PRIO 0
#define BUFSIZE 256L

static File_t *File = NULL;

static BaseType_t cmdOpenFile(char *out, size_t len, const char *cmdline) {
  if (File) {
    FileClose(File);
    File = NULL;
  }

  BaseType_t sz;
  char *filename = (char *)FreeRTOS_CLIGetParameter(cmdline, 1, &sz);
  char *mode = (char *)FreeRTOS_CLIGetParameter(cmdline, 2, NULL);

  filename[sz] = '\0';

  short rflag = 0, wflag = 0, nflag = 0;
  char c;

  while ((c = *mode++)) {
    if (c == 'r')
      rflag = 1;
    if (c == 'w')
      wflag = 1;
    if (c == 'n')
      nflag = 1;
  }

  mode_t oflags = nflag ? O_NONBLOCK : 0;

  if (rflag && wflag)
    oflags |= O_RDWR;
  else if (rflag)
    oflags |= O_RDONLY;
  else if (wflag)
    oflags |= O_WRONLY;

  int error = FileOpen(filename, oflags, &File);
  if (error) {
    snprintf(out, len, "Could not open '%s' file (error: %d)\n", filename,
             error);
  } else {
    snprintf(out, len, "File '%s' opened.\n", filename);
  }

  return pdFALSE;
}

static BaseType_t cmdCloseFile(char *out, size_t len, const char *cmdline) {
  (void)cmdline;

  if (File == NULL) {
    snprintf(out, len, "No file opened!\n");
    return pdFALSE;
  }

  FileClose(File);
  File = NULL;

  return pdFALSE;
}

static void dumpLine(char *out, uint8_t *buf, int n) {
  static const char hex2char[16] = "0123456789abcdef";

  for (int i = 0; i < 16; i++) {
    *out++ = ' ';
    if (i < n) {
      uint8_t byte = buf[i];
      *out++ = hex2char[byte >> 4];
      *out++ = hex2char[byte & 15];
    } else {
      *out++ = ' ';
      *out++ = ' ';
    }
    if (i == 7)
      *out++ = ' ';
  }

  *out++ = ' ';
  *out++ = '|';

  for (int i = 0; i < n; i++) {
    uint8_t byte = buf[i];
    if (byte > 32 && byte < 127)
      *out++ = byte;
    else
      *out++ = '.';
  }

  *out++ = '|';
  *out++ = '\n';
  *out++ = '\0';
}

static BaseType_t cmdReadFile(char *out, size_t len, const char *cmdline) {
  if (File == NULL) {
    snprintf(out, len, "No file opened!\n");
    return pdFALSE;
  }

  static uint8_t data[BUFSIZE];
  static uint8_t *buf = NULL;
  static long left = 0;
  static long bufLeft = 0;
  static off_t pos = 0;

  if (left <= 0)
    left = strtoul(FreeRTOS_CLIGetParameter(cmdline, 1, NULL), NULL, 10);

  if (left > 0 && bufLeft == 0) {
    buf = data;
    FileSeek(File, 0, SEEK_CUR, &pos);
    long todo = min(BUFSIZE, left);
    int error = FileRead(File, buf, todo, &bufLeft);
    if (bufLeft < todo || error)
      left = 0;
  }

  if (bufLeft > 0) {
    size_t n = min(16L, bufLeft);

    out += snprintf(out, len, "%08lx:", pos);
    dumpLine(out, buf, n);

    buf += n;
    pos += n;
    bufLeft -= n;
    left -= n;
  }

  if (left < 0)
    left = 0;

  return left;
}

static BaseType_t cmdWriteFile(char *out, size_t len, const char *cmdline) {
  if (File == NULL) {
    snprintf(out, len, "No file opened!\n");
    return pdFALSE;
  }

  long sz;
  const char *data = FreeRTOS_CLIGetParameter(cmdline, 1, &sz);

  long done;
  int error = FileWrite(File, data, sz, &done);
  if (!error) {
    snprintf(out, len, "Wrote %ld bytes.\n", done);
  } else {
    snprintf(out, len, "Write failed with error %d!\n", error);
  }

  return pdFALSE;
}

static BaseType_t cmdSeekFile(char *out, size_t len, const char *cmdline) {
  if (File == NULL) {
    snprintf(out, len, "No file opened!\n");
    return pdFALSE;
  }

  BaseType_t sz;
  char *offset = (char *)FreeRTOS_CLIGetParameter(cmdline, 1, &sz);
  char *whence = (char *)FreeRTOS_CLIGetParameter(cmdline, 2, NULL);

  long off = strtoul(offset, NULL, 10);
  long wh = SEEK_SET;
  if (strcmp(whence, "set") == 0)
    wh = SEEK_SET;
  else if (strcmp(whence, "cur") == 0)
    wh = SEEK_CUR;
  else if (strcmp(whence, "end") == 0)
    wh = SEEK_END;

  int error = FileSeek(File, off, wh, &off);
  if (error) {
    snprintf(out, len, "Seek failed (error: %d)\n", error);
  } else {
    snprintf(out, len, "New absolute position is %ld\n", off);
  }

  return pdFALSE;
}

static const CLI_Command_Definition_t xOpenFileCmd = {
  "open",
  "open <filename> <mode>:\n"
  " Open <filename> file device with <mode> flags\n\n",
  cmdOpenFile, 2};

static const CLI_Command_Definition_t xCloseFileCmd = {
  "close",
  "close:\n"
  " Close currently opened file file\n\n",
  cmdCloseFile, 0};

static const CLI_Command_Definition_t xReadFileCmd = {
  "read",
  "read <nbytes>:\n"
  " Read <nbytes> bytes from currently opened file\n\n",
  cmdReadFile, 1};

static const CLI_Command_Definition_t xWriteFileCmd = {
  "write",
  "write <string>:\n"
  " Write characters in <string> to currently opened file\n\n",
  cmdWriteFile, 1};

static const CLI_Command_Definition_t xSeekFileCmd = {
  "seek",
  "seek <offset> <whence>:\n"
  " Move currently opened file cursor at position <offset>\n"
  " from <whence> position (i.e. set, cur, end)\n\n",
  cmdSeekFile, 2};

#define MAX_INPUT_LENGTH 80
#define MAX_OUTPUT_LENGTH 160

static void vShellTask(__unused void *data) {
  static char pcOutputString[MAX_OUTPUT_LENGTH];
  static char pcInputString[MAX_INPUT_LENGTH];

  File_t *ser;
  FileOpen("tty", O_RDWR, &ser);

  for (;;) {
    long nbytes;
    BaseType_t xMoreData;

    FileWrite(ser, "> ", 2, NULL);
    FileRead(ser, pcInputString, MAX_INPUT_LENGTH, &nbytes);
    if (pcInputString[nbytes - 1] == '\n')
      nbytes--;
    pcInputString[nbytes] = '\0';
    pcOutputString[0] = '\0';

    do {
      xMoreData = FreeRTOS_CLIProcessCommand(pcInputString, pcOutputString,
                                             MAX_OUTPUT_LENGTH);
      FileWrite(ser, pcOutputString, strlen(pcOutputString), NULL);
    } while (xMoreData);
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

int main(void) {
  NOP(); /* Breakpoint for simulator. */

  /* Configure system clock. */
  AddIntServer(VertBlankChain, SystemClockTick);

  DeviceAttach(&Serial);
  DeviceAttach(&Display);
  DeviceAttach(&Mouse);
  DeviceAttach(&Keyboard);
  DeviceAttach(&Floppy);

  AddTtyDevFile("tty", DevFileLookup("serial"));

  FreeRTOS_CLIRegisterCommand(&xOpenFileCmd);
  FreeRTOS_CLIRegisterCommand(&xCloseFileCmd);
  FreeRTOS_CLIRegisterCommand(&xReadFileCmd);
  FreeRTOS_CLIRegisterCommand(&xWriteFileCmd);
  FreeRTOS_CLIRegisterCommand(&xSeekFileCmd);

  TaskHandle_t shellHandle;
  xTaskCreate(vShellTask, "shell", configMINIMAL_STACK_SIZE, NULL,
              SHELL_TASK_PRIO, &shellHandle);

#if 0
  extern void vConsoleTask(void *);

  TaskHandle_t consoleHandle;
  xTaskCreate(vConsoleTask, "console", configMINIMAL_STACK_SIZE, NULL,
              SHELL_TASK_PRIO, &consoleHandle);
#endif

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
