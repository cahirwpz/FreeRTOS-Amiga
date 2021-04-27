#include <driver.h>
#include <debug.h>
#include <memory.h>
#include <sys/errno.h>

int DeviceAttach(Driver_t *drv) {
  drv->state = MemAlloc(drv->size, MF_ZERO);
  if (!drv->state)
    return ENOMEM;
  Log("Attaching '%s' driver.\n", drv->name);
  int error = drv->attach(drv);
  if (!error)
    return 0;
  MemFree(drv->state);
  return error;
}

int DeviceDetach(Driver_t *drv) {
  Log("Detaching '%s' driver.\n", drv->name);
  int error = drv->detach(drv);
  if (error)
    return error;
  MemFree(drv->state);
  return 0;
}
