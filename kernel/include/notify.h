#pragma once

#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

/* Types of events that a task can await for.
 * Please note that there may be more that one source of given event,
 * it's up to task implementation to distinguish between them
 * (think of waiting on more than single message port or interrupt). */
typedef enum NotifyBits {
  NB_MSGPORT = BIT(0), /* used by message ports, refer to <msgport.h> */
  NB_EVENT = BIT(1),   /* used by kernel events, refer to <event.h> */
  NB_IRQ = BIT(2),     /* use it when waiting for an interrupt to happen */
} NotifyBits_t;

/* Send notification `bits` to `task`. */
static inline void NotifySend(TaskHandle_t task, NotifyBits_t bits) {
  (void)xTaskNotify(task, (uint32_t)bits, eSetBits);
}

/* Same as above, but intended to be called in an interrupt service routine. */
static inline void NotifySendFromISR(TaskHandle_t task, NotifyBits_t bits) {
  (void)xTaskNotifyFromISR(task, bits, eSetBits, &xNeedRescheduleTask);
}

/* Uses FreeRTOS direct task notifications. Suspend task execution until one
 * of the bits in `bitsToWaitFor` is set. Leaves immediately if one of the bits
 * was set before entry. Clears `bitsToWaitFor` bits in tasks' notification
 * value before returning.
 *
 * Returns a value where one or more bits are set on success,
 * and 0 when no `bitsToWaitFor` was set within ticksToWait timespan. */
NotifyBits_t NotifyWait(NotifyBits_t bitsToWaitFor, TickType_t ticksToWait);
