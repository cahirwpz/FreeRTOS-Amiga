#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <custom.h>
#include <interrupt.h>
#include <libkern.h>
#include <serial.h>
#include <device.h>

#define CLOCK 3546895
#define QUEUELEN 64

static QueueHandle_t SendQ;
static QueueHandle_t RecvQ;

static int SerialRead(Device_t *, off_t, void *, size_t, ssize_t *);
static int SerialWrite(Device_t *, off_t, const void *, size_t, ssize_t *);

static DeviceOps_t SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
};

#define SendByte(byte)                                                         \
  { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(__unused void *ptr) {
  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(SendQ, &cSend, &xNeedRescheduleTask))
    SendByte(cSend);
}

static void RecvIntHandler(__unused void *ptr) {
  /* serdatr contains both data and status bits! */
  uint16_t code = custom.serdatr;
  if ((code & SERDATF_RBF) == 0)
    return;
  /* Send one byte to waiting task. */
  char cRecv = code;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, &xNeedRescheduleTask);
}

void SerialInit(unsigned baud) {
  kprintf("[Serial] Initializing driver!\n");

  custom.serper = CLOCK / baud - 1;

  RecvQ = xQueueCreate(QUEUELEN, sizeof(char));
  SendQ = xQueueCreate(QUEUELEN, sizeof(char));

  SetIntVec(TBE, SendIntHandler, NULL);
  SetIntVec(RBF, RecvIntHandler, NULL);

  ClearIRQ(INTF_TBE | INTF_RBF);
  EnableINT(INTF_TBE | INTF_RBF);

  AddDevice("serial", &SerialOps, NULL);
}

void SerialKill(void) {
  DisableINT(INTF_TBE | INTF_RBF);
  ResetIntVec(TBE);
  ResetIntVec(RBF);

  vQueueDelete(RecvQ);
  vQueueDelete(SendQ);

  kprintf("[Serial] Driver deactivated!\n");
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

static void SerialPutChar(char data) {
  TriggerSend(data);
  if (data == '\n') {
    data = '\r';
    TriggerSend(data);
  }
}

static int SerialGetChar(void) {
  char cRecv;
  xQueueReceive(RecvQ, (void *)&cRecv, portMAX_DELAY);
  return cRecv;
}

static int SerialWrite(Device_t *dev __unused, off_t offset __unused,
                       const void *buf, size_t nbyte, ssize_t *donep) {
  const char *cbuf = buf;
  for (size_t i = 0; i < nbyte; i++)
    SerialPutChar(*cbuf++);
  *donep = nbyte;
  return 0;
}

static int SerialRead(Device_t *dev __unused, off_t offset __unused, void *buf,
                      size_t nbyte, ssize_t *donep) {
  char *cbuf = buf;
  size_t i = 0;
  while (i < nbyte) {
    cbuf[i] = SerialGetChar();
    if (cbuf[i++] == '\n')
      break;
  }
  *donep = i;
  return 0;
}
