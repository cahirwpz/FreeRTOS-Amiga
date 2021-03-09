#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    dprintf(STDERR_FILENO, "Usage: rm files...\n");
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    if (unlink(argv[i]) < 0) {
      dprintf(STDERR_FILENO, "rm: %s failed to delete\n", argv[i]);
      break;
    }
  }

  exit(0);
}
