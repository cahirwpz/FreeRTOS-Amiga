#include <custom.h>
#include <string.h>
#include "sysapi.h"

#define MAXLINE 80
#define MAXARGS 16

#define MSG(x) x, (sizeof(x) - 1)

static int spawn(char *argv[]) {
  int pid = vfork();
  if (pid == 0) {
    /* Child runs user job */
    if (execv(argv[0], argv) < 0) {
      write(STDOUT_FILENO, MSG("Command not found!\n"));
      exit(0);
    }
  }
  return pid;
}

/* If first arg is a builtin command, run it and return true */
static int builtin_command(char **argv) {
  if (!strcmp(argv[0], "quit")) /* quit command */
    exit(0);
  if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
    return 1;
  return 0;                     /* Not a builtin command */
}

/* parseline - Parse the command line and build the argv array */
static int parseline(char *buf, char **argv) {
  char *delim; /* Points to first space delimiter */
  int argc;    /* Number of args */
  int bg;      /* Background job? */

  buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* Ignore leading spaces */
    buf++;

  /* Build the argv list */
  argc = 0;
  while ((delim = strchr(buf, ' '))) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* Ignore spaces */
      buf++;
  }
  argv[argc] = NULL;

  if (argc == 0)  /* Ignore blank line */
    return 1;

  /* Should the job run in the background? */
  if ((bg = (*argv[argc-1] == '&')) != 0)
    argv[--argc] = NULL;

  return bg;
}

/* eval - Evaluate a command line */
static void eval(char *cmdline) {
  char *argv[MAXARGS]; /* Argument list execv() */
  char buf[MAXLINE];   /* Holds modified command line */

  strcpy(buf, cmdline);
  int bg = parseline(buf, argv);
  if (argv[0] == NULL) /* Ignore empty lines */
    return;

  if (!builtin_command(argv)) {
    spawn(argv);

    /* Parent waits for foreground job to terminate */
    if (!bg) {
      int status;
      wait(&status);
    }
  }
}

int main(void) {
  char cmdline[MAXLINE];
  int res;

  while (1) {
    write(STDOUT_FILENO, "> ", 2);
    res = read(STDIN_FILENO, cmdline, MAXLINE);
    if (res == 0)
      break;
    eval(cmdline);
  }

  return 0;
}
