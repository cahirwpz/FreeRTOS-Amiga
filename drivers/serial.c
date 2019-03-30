#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <custom.h>
#include <interrupt.h>
#include <stdio.h>

#include <serial.h>

#define CLOCK 3546895
#define QUEUELEN 64

static QueueHandle_t SendQ;
static QueueHandle_t RecvQ;

#define SendByte(byte) { custom->serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void *) {
  /* Signal end of interrupt. */
  ClearIRQ(INTF_TBE);

  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(SendQ, &cSend, &xNeedRescheduleTask))
    SendByte(cSend);
}

static void RecvIntHandler(void *) {
  /* Signal end of interrupt. */
  ClearIRQ(INTF_RBF);

  /* Send one byte to waiting task. */
  char cRecv = custom->serdatr;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, &xNeedRescheduleTask);
}

void SerialInit(unsigned baud) {
  printf("[Init] Serial port driver!\n");

  custom->serper = CLOCK / baud - 1;

  RecvQ = xQueueCreate(QUEUELEN, sizeof(char));
  SendQ = xQueueCreate(QUEUELEN, sizeof(char));

  SetIntVec(TBE, SendIntHandler, NULL);
  SetIntVec(RBF, RecvIntHandler, NULL);

  ClearIRQ(INTF_TBE|INTF_RBF);
  EnableINT(INTF_TBE|INTF_RBF);
}

void SerialKill(void) {
  DisableINT(INTF_TBE|INTF_RBF);
  ResetIntVec(TBE);
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
