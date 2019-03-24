#include <FreeRTOS.h>
#include <queue.h>

#include <custom.h>
#include <interrupt.h>
#include <stdio.h>

#include <serial.h>

#define CLOCK 3546895
#define QUEUELEN 64

static QueueHandle_t SendQ;
static QueueHandle_t RecvQ;

#define SendByte(byte) { custom->serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static ISR(SendIntHandler) {
  /* Signal end of interrupt. */
  ClearIRQ(TBE);

  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(SendQ, &cSend, NULL))
    SendByte(cSend);
}

static ISR(RecvIntHandler) {
  /* Signal end of interrupt. */
  ClearIRQ(RBF);

  /* Send one byte to waiting task. */
  char cRecv = custom->serdatr;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, NULL);
}

void SerialInit(unsigned baud) {
  printf("[Init] Serial port driver!\n");

  custom->serper = CLOCK / baud - 1;

  RecvQ = xQueueCreate(QUEUELEN, sizeof(char));
  SendQ = xQueueCreate(QUEUELEN, sizeof(char));

  SetIntVec(TBE, SendIntHandler);
  ClearIRQ(TBE);
  EnableINT(TBE);

  SetIntVec(RBF, RecvIntHandler);
  ClearIRQ(RBF);
  EnableINT(RBF);
}

void SerialKill(void) {
  DisableINT(TBE);
  ClearIRQ(TBE);
  ResetIntVec(TBE);

  DisableINT(RBF);
  ClearIRQ(RBF);
  ResetIntVec(RBF);

  vQueueDelete(RecvQ);
  vQueueDelete(SendQ);
}

static void TriggerSend(uint8_t cSend) {
  taskENTER_CRITICAL();
  if (uxQueueMessagesWaiting(SendQ) == 0) {
    SendByte(cSend);
  } else {
    uint8_t data = cSend;
    (void)xQueueSend(SendQ, &data, portMAX_DELAY);
  }
  taskEXIT_CRITICAL();
}

void SerialPutChar(__reg("d0") char data) {
  TriggerSend(data);
  if (data == '\n') {
    data = '\r';
    TriggerSend(data);
  }
}

void SerialPrint(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  kvprintf(SerialPutChar, fmt, ap);
  va_end(ap);
}

int SerialGetChar(void) {
  char cRecv;
  xQueueReceive(RecvQ, (void *)&cRecv, portMAX_DELAY);
  return cRecv;
}
