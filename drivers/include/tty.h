#pragma once

typedef struct DevFile DevFile_t;

int AddTtyDevFile(const char *name, DevFile_t *cons);
