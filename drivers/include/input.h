#pragma once

#include <sys/types.h>

typedef struct IoReq IoReq_t;
typedef struct QueueDefinition *QueueHandle_t;

typedef enum IeKind {
  IE_UNKNOWN,
  IE_MOUSE_UP,
  IE_MOUSE_DOWN,
  IE_MOUSE_DELTA_X,
  IE_MOUSE_DELTA_Y,
  IE_KEYBOARD_UP,
  IE_KEYBOARD_DOWN,
} __packed IeKind_t;

typedef struct InputEvent {
  IeKind_t kind;
  int16_t value;
} InputEvent_t;

QueueHandle_t InputEventQueueCreate(void);

int InputEventInjectFromISR(QueueHandle_t q, const InputEvent_t *iev, size_t n);
int InputEventRead(QueueHandle_t q, IoReq_t *io);
