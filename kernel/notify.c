#include <notify.h>

NotifyBits_t NotifyWait(NotifyBits_t bitsToWaitFor, TickType_t ticksToWait) {
  uint32_t value;

  taskENTER_CRITICAL();
  TaskHandle_t self = xTaskGetCurrentTaskHandle();
  /* HACK: If notification has not been sent to `self` then xTaskNotifyWait
   * will not clear bits in ulNotifiedValue. */
  (void)xTaskGenericNotify(self, 0, eNoAction, NULL);
  (void)xTaskNotifyWait(0, bitsToWaitFor, &value, 0);
  if (!(value & bitsToWaitFor) && ticksToWait) {
    TickType_t end = xTaskGetTickCount() + ticksToWait;

    for (;;) {
      if (xTaskNotifyWait(0, bitsToWaitFor, &value, ticksToWait))
        if (value & bitsToWaitFor)
          break;

      TickType_t now = xTaskGetTickCount();
      if (end <= now)
        break;
      ticksToWait = end - now;
    }
  }
  taskEXIT_CRITICAL();

  return value & bitsToWaitFor;
}
