#pragma once

#include <sys/types.h>

typedef struct Driver Driver_t;

/* Returns value from <errno.h> on failure, 0 on success. */
typedef int (*DrvAttach_t)(Driver_t *drv);
typedef int (*DrvDetach_t)(Driver_t *drv);

/* There's an assumption that a driver can be associated with exactly one
 * hardware device. If changed then we need to introduce Device structure. */
struct Driver {
  const char *name;   /* short driver description */
  DrvAttach_t attach; /* attach device to system */
  DrvDetach_t detach; /* detach device from system */
  size_t size;        /* `Device::state` object size */
  void *state;        /* device state managed by the driver */
};

int DeviceAttach(Driver_t *drv);
int DeviceDetach(Driver_t *drv);

/* List all device driver here. */
extern Driver_t Display;
extern Driver_t Floppy;
extern Driver_t Keyboard;
extern Driver_t Mouse;
extern Driver_t Serial;
