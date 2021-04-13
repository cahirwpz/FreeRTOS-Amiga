#pragma once

typedef struct File File_t;

/* Creates a pipe object. Returns read end file object through `rfilep` and
 * write end through `wfilep`. Returns 0 on success, otherwise errno code. */
int PipeAlloc(File_t **rfilep, File_t **wfilep);
