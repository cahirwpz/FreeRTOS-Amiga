#pragma once

#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

static inline void NotifySend(TaskHandle_t xTaskToNotify,
                              uint32_t uxBitsToSet) {
  (void)xTaskNotify(xTaskToNotify, uxBitsToSet, eSetBits);
}

static inline void NotifySendFromISR(TaskHandle_t xTaskToNotify,
                                     uint32_t uxBitsToSet) {
  (void)xTaskNotifyFromISR(xTaskToNotify, uxBitsToSet, eSetBits,
                           &xNeedRescheduleTask);
}

uint32_t NotifyWait(uint32_t uxBitsToWaitFor, TickType_t xTicksToWait);
