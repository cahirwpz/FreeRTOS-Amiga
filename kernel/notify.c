#include <notify.h>

uint32_t NotifyWait(uint32_t uxBitsToWaitFor, TickType_t xTicksToWait) {
  uint32_t value;

  taskENTER_CRITICAL();
  (void)xTaskNotifyWait(0, uxBitsToWaitFor, &value, 0);
  if (!(value & uxBitsToWaitFor) && xTicksToWait) {
    TickType_t end = xTaskGetTickCount() + xTicksToWait;

    for (;;) {
      if (xTaskNotifyWait(0, uxBitsToWaitFor, &value, xTicksToWait))
        if (value & uxBitsToWaitFor)
          break;

      TickType_t now = xTaskGetTickCount();
      if (end <= now)
        break;
      xTicksToWait = end - now;
    }
  }
  taskEXIT_CRITICAL();

  return value & uxBitsToWaitFor;
}
