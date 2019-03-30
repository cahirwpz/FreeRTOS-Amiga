#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <event.h>

#define EV_MAXNUM 16

static QueueHandle_t EventQueue;

void EventQueueInit(void) {
  EventQueue = xQueueCreate(EV_MAXNUM, sizeof(Event_t));
}

void EventQueueKill(void) {
  vQueueDelete(EventQueue);
}

void PushEvent(Event_t *ev) {
   xQueueSendToBack(EventQueue, (void *)ev, portMAX_DELAY);
}

void PushEventFromISR(Event_t *ev) {
   xQueueSendToBackFromISR(EventQueue, (void *)ev, &xNeedRescheduleTask);
}

bool PopEvent(Event_t *ev) {
   return xQueueReceive(EventQueue, (void *)ev, portMAX_DELAY);
}
