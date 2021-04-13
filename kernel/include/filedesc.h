#pragma once

typedef struct Proc Proc_t;
typedef struct File File_t;

/* Descriptor table management procedures.
 * All return 0 on success, otherwise an errno code. */

/* Fetch file object at index `fd` of `proc` descriptor table
 * and return it through `filep` pointer. */
int FdGet(Proc_t *proc, int fd, File_t **filep);

/* Choose the lowest available index in `proc` descriptor table
 * and associate `file` with it. Return chosen index through `fdp` pointer. */
int FdInstall(Proc_t *proc, File_t *file, int *fdp);

/* Associate `file` with `fd` index in `proc` descriptor table.
 * If there's opened file at `fd` position close it first. */
int FdInstallAt(Proc_t *proc, File_t *file, int fd);
