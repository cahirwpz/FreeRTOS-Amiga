#include <driver.h>
#include <libkern.h>
#include <sys/errno.h>

int DeviceAttach(Driver_t *drv) {
  drv->state = kcalloc(1, drv->size);
  if (!drv->state)
    return ENOMEM;
  klog("Attaching '%s' driver.\n", drv->name);
  int error = drv->attach(drv);
  if (!error)
    return 0;
  kfree(drv->state);
  return error;
}

int DeviceDetach(Driver_t *drv) {
  klog("Detaching '%s' driver.\n", drv->name);
  int error = drv->detach(drv);
  if (error)
    return error;
  kfree(drv->state);
  return 0;
}
