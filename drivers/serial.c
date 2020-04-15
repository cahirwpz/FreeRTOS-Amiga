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

#define SendByte(byte) { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(__unused void *ptr) {
  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(SendQ, &cSend, &xNeedRescheduleTask))
    SendByte(cSend);
}

static void RecvIntHandler(__unused void *ptr) {
  /* Send one byte to waiting task. */
  char cRecv = custom.serdatr;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, &xNeedRescheduleTask);
}

void SerialInit(unsigned baud) {
  printf("[Init] Serial port driver!\n");

  custom.serper = CLOCK / baud - 1;

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
  /* Checking if sending queue and serdat register are empty. */
  if (uxQueueMessagesWaiting(SendQ) == 0 && custom.serdatr & SERDATF_TBE) {
    SendByte(cSend);
  } else {
    uint8_t data = cSend;
    (void)xQueueSend(SendQ, &data, portMAX_DELAY);
  }
  taskEXIT_CRITICAL();
}

void SerialPutChar(char data) {
  TriggerSend(data);
  if (data == '\n') {
    data = '\r';
    TriggerSend(data);
  }
}

int SerialGetChar(void) {
  char cRecv;
  xQueueReceive(RecvQ, (void *)&cRecv, portMAX_DELAY);
  return cRecv;
}
