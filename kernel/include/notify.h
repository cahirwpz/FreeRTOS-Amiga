#pragma once

#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

typedef enum NotifyBits {
  NB_MSGPORT = BIT(0),
  NB_EVENT = BIT(1),
  NB_IRQ = BIT(2),
} NotifyBits_t;

static inline void NotifySend(TaskHandle_t task, NotifyBits_t bits) {
  (void)xTaskNotify(task, (uint32_t)bits, eSetBits);
}

static inline void NotifySendFromISR(TaskHandle_t task, NotifyBits_t bits) {
  (void)xTaskNotifyFromISR(task, bits, eSetBits, &xNeedRescheduleTask);
}

NotifyBits_t NotifyWait(NotifyBits_t bitsToWaitFor, TickType_t ticksToWait);
