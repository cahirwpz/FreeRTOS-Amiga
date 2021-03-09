#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DIRSIZ MAXNAMLEN

static char *fmtname(char *path) {
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
  return buf;
}

static void ls(char *path) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    dprintf(STDERR_FILENO, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    dprintf(STDERR_FILENO, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.st_mode & S_IFMT) {
    case S_IFREG:
      dprintf(STDOUT_FILENO, "%s %d %d %d\n", fmtname(path), st.st_mode,
              st.st_ino, st.st_size);
      break;

    case S_IFDIR:
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
        dprintf(STDERR_FILENO, "ls: path too long\n");
        break;
      }
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.d_fileno == 0)
          continue;
        memmove(p, de.d_name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0) {
          dprintf(STDERR_FILENO, "ls: cannot stat %s\n", buf);
          continue;
        }
        dprintf(STDOUT_FILENO, "%s %d %d %d\n", fmtname(buf), st.st_mode,
                st.st_ino, st.st_size);
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    ls(".");
    exit(0);
  }
  for (i = 1; i < argc; i++)
    ls(argv[i]);
  exit(0);
}
