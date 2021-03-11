#include <sys/errno.h>
#include <filedesc.h>
#include <file.h>
#include <proc.h>

int FdGet(Proc_t *proc, int fd, File_t **filep) {
  if (fd < 0)
    return EINVAL;
  if (fd >= MAXFILES)
    return EBADF;

  *filep = proc->fdtab[fd];

  return (*filep) ? 0 : EBADF;
}

int FdInstall(Proc_t *proc, File_t *file, int *fdp) {
  for (int fd = 0; fd < MAXFILES; fd++) {
    if (!proc->fdtab[fd]) {
      proc->fdtab[fd] = file;
      *fdp = fd;
      return 0;
    }
  }

  return EMFILE;
}

int FdInstallAt(Proc_t *proc, File_t *file, int fd) {
  if (fd < 0 || fd >= MAXFILES)
    return EBADF;

  File_t **fp = &proc->fdtab[fd];
  if (*fp)
    FileClose(*fp);
  *fp = file;
  return 0;
}
