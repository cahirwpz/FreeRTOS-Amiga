#include <FreeRTOS.h>
#include <queue.h>

#include <hardware.h>
#include <interrupt.h>
#include <libsa.h>

#include <serial.h>

#define CLOCK 3546895
#define QUEUELEN 64

#define SERDATF_RBF (1 << 14)
#define SERDATF_TBE (1 << 13)
#define SERDATF_TSRE (1 << 12)

static QueueHandle_t SendQ;
static QueueHandle_t RecvQ;

#define SendByte(byte) { custom->serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void) {
  /* Signal end of interrupt. */
  custom->intreq = INTF_TBE;

  /* Send one byte into the wire. */
  uint8_t cSend;
  if (xQueueReceiveFromISR(SendQ, &cSend, NULL))
    SendByte(cSend);
}

static void RecvIntHandler(void) {
  /* Signal end of interrupt. */
  custom->intreq = INTF_RBF;

  /* Send one byte to waiting task. */
  char cRecv = custom->serdatr;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, NULL);
}

static ISR_t oldTBE;
static ISR_t oldRBF;

void SerialInit(unsigned baud) {
  dprintf("[Init] Serial port driver!\n");

  custom->serper = CLOCK / baud - 1;

  RecvQ = xQueueCreate(QUEUELEN, sizeof(char));
  SendQ = xQueueCreate(QUEUELEN, sizeof(char));

  oldTBE = IntVec[INTB_TBE];
  IntVec[INTB_TBE] = SendIntHandler;

  oldRBF = IntVec[INTB_RBF];
  IntVec[INTB_RBF] = RecvIntHandler;

  custom->intreq = INTF_TBE | INTF_RBF;
  custom->intena = INTF_SETCLR | INTF_TBE | INTF_RBF;
}

void SerialKill(void) {
  custom->intena = INTF_TBE | INTF_RBF;
  custom->intreq = INTF_TBE | INTF_RBF;

  IntVec[INTB_TBE] = oldTBE;
  IntVec[INTB_RBF] = oldRBF;

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
