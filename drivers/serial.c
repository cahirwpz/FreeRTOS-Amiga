#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <custom.h>
#include <interrupt.h>
#include <stdio.h>

#include <serial.h>

#define CLOCK 3546895
#define QUEUELEN 64

typedef struct SerPort {
  File_t file;
  QueueHandle_t sendQ;
  QueueHandle_t recvQ;
} SerPort_t;

static int SerialRead(SerPort_t *ser, char *buf, size_t nbyte);
static int SerialWrite(SerPort_t *ser, const char *buf, size_t nbyte);
static void SerialClose(SerPort_t *ser);

static FileOps_t SerOps = {
  .read = (FileRead_t)SerialRead,
  .write = (FileWrite_t)SerialWrite,
  .close = (FileClose_t)SerialClose
};

#define SendByte(byte) { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void *ptr) {
  SerPort_t *ser = ptr;
  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(ser->sendQ, &cSend, &xNeedRescheduleTask))
    SendByte(cSend);
}

static void RecvIntHandler(void *ptr) {
  SerPort_t *ser = ptr;
  /* Send one byte to waiting task. */
  char cRecv = custom.serdatr;
  (void)xQueueSendFromISR(ser->recvQ, (void *)&cRecv, &xNeedRescheduleTask);
}

File_t *SerialOpen(unsigned baud) {
  static SerPort_t ser;
  File_t *f = &ser.file;

  if (++f->usecount > 1)
    return f;

  f->ops = &SerOps;

  custom.serper = CLOCK / baud - 1;

  ser.recvQ = xQueueCreate(QUEUELEN, sizeof(char));
  ser.sendQ = xQueueCreate(QUEUELEN, sizeof(char));

  SetIntVec(TBE, SendIntHandler, &ser);
  SetIntVec(RBF, RecvIntHandler, &ser);

  ClearIRQ(INTF_TBE|INTF_RBF);
  EnableINT(INTF_TBE|INTF_RBF);

  return f;
}

static void SerialClose(SerPort_t *ser) {
  if (--ser->file.usecount > 0)
    return;

  DisableINT(INTF_TBE|INTF_RBF);
  ResetIntVec(TBE);
  ResetIntVec(RBF);

  vQueueDelete(ser->recvQ);
  vQueueDelete(ser->sendQ);
}

static void TriggerSend(SerPort_t *ser, uint8_t cSend) {
  taskENTER_CRITICAL();
  /* Checking if sending queue and serdat register are empty. */
  if (uxQueueMessagesWaiting(ser->sendQ) == 0 && custom.serdatr & SERDATF_TBE) {
    SendByte(cSend);
  } else {
    uint8_t data = cSend;
    (void)xQueueSend(ser->sendQ, &data, portMAX_DELAY);
  }
  taskEXIT_CRITICAL();
}

static int SerialWrite(SerPort_t *ser, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++) {
    char data = *buf++;
    TriggerSend(ser, data);
    if (data == '\n') {
      data = '\r';
      TriggerSend(ser, data);
    }
  }
  return nbyte;
}

static int SerialRead(SerPort_t *ser, char *buf, size_t nbyte) {
  size_t i = 0;
  while (i < nbyte) {
    xQueueReceive(ser->recvQ, (void *)&buf[i], portMAX_DELAY);
    if (buf[i++] == '\n')
      break;
  }
  return i;
}
