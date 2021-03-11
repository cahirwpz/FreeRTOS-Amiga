#pragma once

typedef struct Proc Proc_t;
typedef struct File File_t;

/* Descriptor table management procedures. */

int FdGet(Proc_t *proc, int fd, File_t **filep);
int FdInstall(Proc_t *proc, File_t *file, int *fdp);
int FdInstallAt(Proc_t *proc, File_t *file, int fd);
