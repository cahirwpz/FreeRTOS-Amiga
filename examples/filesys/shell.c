#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS_CLI.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <devfile.h>
#include <driver.h>
#include <string.h>
#include <tty.h>

#include "filesys.h"

#define SHELL_TASK_PRIO 2

#define BUFSIZE 256UL
#define MAXFILES 10

static File_t *Files[MAXFILES];
static short Active = -1;

static BaseType_t cmdOpenFile(char *out, size_t len, const char *cmdline) {
  File_t **fp = NULL;
  int i = 0;
  for (i = 0; i < MAXFILES; i++) {
    if (Files[i] == NULL) {
      fp = &Files[i];
      break;
    }
  }

  const char *str = FreeRTOS_CLIGetParameter(cmdline, 1, NULL);

  if (fp) {
    *fp = FsOpen(str);
    if (*fp) {
      snprintf(out, len, "File '%s' opened at slot %d.\n", str, i);
      if (Active < 0)
        Active = i;
    } else {
      snprintf(out, len, "No such file: '%s'!\n", str);
    }
  } else {
    snprintf(out, len, "No free slots!\n");
  }

  return pdFALSE;
}

static BaseType_t cmdCloseFile(char *out, size_t len, const char *cmdline) {
  const char *numstr = FreeRTOS_CLIGetParameter(cmdline, 1, NULL);
  int num = strtoul(numstr, NULL, 10);

  if (num < MAXFILES && Files[num] != NULL) {
    FileClose(Files[num]);
    Files[num] = NULL;
    if (num == Active)
      Active = -1;
  } else {
    snprintf(out, len, "No file at slot %d!\n", num);
  }

  return pdFALSE;
}

static BaseType_t cmdSelectFile(char *buf, size_t len, const char *cmdline) {
  const char *numstr = FreeRTOS_CLIGetParameter(cmdline, 1, NULL);
  int num = strtoul(numstr, NULL, 10);

  if (num < MAXFILES && Files[num] != NULL) {
    Active = num;
  } else {
    snprintf(buf, len, "No file at slot %d!\n", num);
  }
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
    if (!iscntrl(byte))
      *out++ = byte;
    else
      *out++ = '.';
  }

  *out++ = '|';
  *out++ = '\n';
  *out++ = '\0';
}

static BaseType_t cmdReadFile(char *out, size_t len, const char *cmdline) {
  static uint8_t *buf = NULL;
  static size_t bufLeft = 0;
  static size_t left = 0;
  static off_t pos = 0;

  if (Active < 0)
    return pdFALSE;

  File_t *f = Files[Active];

  if (left == 0)
    left = strtoul(FreeRTOS_CLIGetParameter(cmdline, 1, NULL), NULL, 10);

  if (left > 0 && bufLeft == 0) {
    static uint8_t data[BUFSIZE];
    long bufLeft;
    buf = data;
    FileSeek(f, 0, SEEK_CUR, &pos);
    FileRead(f, buf, min(BUFSIZE, left), &bufLeft);
    if (bufLeft == 0)
      left = 0;
  }

  if (bufLeft > 0) {
    size_t n = min(16UL, bufLeft);

    out += snprintf(out, len, "%08lx:", pos);
    dumpLine(out, buf, n);

    buf += n;
    pos += n;
    bufLeft -= n;
    left -= n;
  }

  return left;
}

static BaseType_t cmdDummy(char *buf, size_t len, const char *cmdline) {
  (void)buf;
  (void)len;
  (void)cmdline;
  return pdFALSE;
}

static const CLI_Command_Definition_t xOpenFileCmd = {
  "open",
  "open <filename>:\n"
  " Open <filename> from the floppy disk and assign it a number\n\n",
  cmdOpenFile, 1};

static const CLI_Command_Definition_t xCloseFileCmd = {
  "close",
  "close <number>:\n"
  " Close <number> file previously opened with 'open' command\n\n",
  cmdCloseFile, 1};

static const CLI_Command_Definition_t xSelectFileCmd = {
  "select",
  "select <number>:\n"
  " Set currently active file to <number>\n\n",
  cmdSelectFile, 1};

static const CLI_Command_Definition_t xReadFileCmd = {
  "read",
  "read <number>:\n"
  " Read <number> bytes from currently active file\n\n",
  cmdReadFile, 1};

static const CLI_Command_Definition_t xSeekFileCmd = {
  "seek",
  "seek <offset> <whence>:\n"
  " Move currently active file cursor at position <offset>\n"
  " from <whence> position (i.e. set, cur, end)\n\n",
  cmdDummy, 2};

static const CLI_Command_Definition_t xListDirCmd = {
  "ls",
  "ls:\n"
  " List directory entries\n\n",
  cmdDummy, 0};

#define MAX_INPUT_LENGTH 80
#define MAX_OUTPUT_LENGTH 160

static void vShellTask(__unused void *data) {
  static char pcOutputString[MAX_OUTPUT_LENGTH];
  static char pcInputString[MAX_INPUT_LENGTH];

  File_t *ser;
  FileOpen("tty", O_RDWR, &ser);

  FsMount();

  for (;;) {
    long nbytes;
    BaseType_t xMoreData;

    if (Active < 0)
      FilePrintf(ser, "? > ");
    else
      FilePrintf(ser, "%d > ", Active);
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

static TaskHandle_t shellHandle;

void StartShellTask(void) {
  DeviceAttach(&Serial);
  AddTtyDevFile("tty", DevFileLookup("serial"));

  FreeRTOS_CLIRegisterCommand(&xOpenFileCmd);
  FreeRTOS_CLIRegisterCommand(&xCloseFileCmd);
  FreeRTOS_CLIRegisterCommand(&xSelectFileCmd);
  FreeRTOS_CLIRegisterCommand(&xReadFileCmd);
  FreeRTOS_CLIRegisterCommand(&xSeekFileCmd);
  FreeRTOS_CLIRegisterCommand(&xListDirCmd);

  xTaskCreate(vShellTask, "shell", configMINIMAL_STACK_SIZE, NULL,
              SHELL_TASK_PRIO, &shellHandle);
}
