#include <cpu.h>
#include <trap.h>
#include <string.h>
#include <proc.h>
#include <pipe.h>
#include <file.h>
#include <filedesc.h>

#include <sys/errno.h>
#include <sys/syscall.h>

static int SysExit(Proc_t *p, long *arg, long *res __unused) {
  ProcExit(p, arg[0]);
}

static int SysOpen(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysClose(Proc_t *p, long *arg, long *res __unused) {
  return FdInstallAt(p, NULL, arg[0]);
}

static int SysRead(Proc_t *p, long *arg, long *res) {
  File_t *f;
  int error;

  if ((error = FdGet(p, arg[0], &f)))
    return error;

  return FileRead(f, (void *)arg[1], (size_t)arg[2], res);
}

static int SysWrite(Proc_t *p, long *arg, long *res) {
  File_t *f;
  int error;

  if ((error = FdGet(p, arg[0], &f)))
    return error;

  return FileWrite(f, (const void *)arg[1], (size_t)arg[2], res);
}

static int SysIoctl(Proc_t *p, long *arg, long *res __unused) {
  File_t *f;
  int error;

  if ((error = FdGet(p, arg[0], &f)))
    return error;

  return FileIoctl(f, arg[1], (void *)arg[2]);
}

static int SysExecv(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysVfork(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysChdir(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysDup(Proc_t *p, long *arg, long *res) {
  File_t *f;
  int error, fd;

  if ((error = FdGet(p, arg[0], &f)))
    return error;

  if ((error = FdInstall(p, f, &fd)))
    return error;

  *res = fd;
  return 0;
}

static int SysFstat(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysKill(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysMkdir(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysPipe(Proc_t *p, long *arg, long *res __unused) {
  int error, rfd, wfd;

  File_t *rfile, *wfile;
  if ((error = PipeAlloc(&rfile, &wfile)))
    return error;

  if ((error = FdInstall(p, rfile, &rfd)))
    goto bad_rfd;

  if ((error = FdInstall(p, wfile, &wfd)))
    goto bad_wfd;

  int *fd = (int *)arg[0];
  fd[0] = rfd;
  fd[1] = wfd;
  return 0;

bad_wfd:
  FdInstallAt(p, NULL, rfd);

bad_rfd:
  FileClose(rfile);
  FileClose(wfile);
  return error;
}

static int SysStat(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysUnlink(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

static int SysWait(Proc_t *p, long *arg, long *res) {
  /* TODO */
  (void)p, (void)arg, (void)res;
  return ENOSYS;
}

typedef int (*SysCall_t)(Proc_t *, long *, long *);

static SysCall_t SysEnt[] = {
  /* clang-format off */
  [SYS_exit] = SysExit,
  [SYS_open] = SysOpen,
  [SYS_close] = SysClose,
  [SYS_read] = SysRead,
  [SYS_write] = SysWrite,
  [SYS_execv] = SysExecv,
  [SYS_vfork] = SysVfork,
  [SYS_chdir] = SysChdir,
  [SYS_dup] = SysDup,
  [SYS_fstat] = SysFstat,
  [SYS_kill] = SysKill,
  [SYS_mkdir] = SysMkdir,
  [SYS_pipe] = SysPipe,
  [SYS_stat] = SysStat,
  [SYS_unlink] = SysUnlink,
  [SYS_wait] = SysWait,
  [SYS_ioctl] = SysIoctl,
  /* clang-format on */
};

extern void vPortDefaultTrapHandler(TrapFrame_t *);

void vPortTrapHandler(TrapFrame_t *frame) {
  uint16_t sr = (CpuModel > CF_68000) ? frame->m68010.sr : frame->m68000.sr;

  /* Trap instruction from user-space ? */
  if (frame->trapnum == T_TRAPINST && (sr & SR_S) == 0) {
    Proc_t *p = TaskGetProc();
    int num = frame->d0;

    if (num && num < SYS_MAXSYSCALL) {
      frame->d1 = SysEnt[num](p, (long *)&frame->d1, (long *)&frame->d0);
      return;
    }
  }

  /* Everything else goes to default trap handler. */
  vPortDefaultTrapHandler(frame);
}
