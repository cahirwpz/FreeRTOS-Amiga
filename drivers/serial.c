#include <FreeRTOS.h>
#include <queue.h>

#include <evec.h>
#include <hardware.h>
#include <libsa.h>

#include <serial.h>

#define CLOCK 3546895
#define QUEUELEN 64

#define SERDATR_RBF (1 << 14)
#define SERDATR_TBE (1 << 13)
#define SERDATR_TSRE (1 << 12)

static QueueHandle_t SendQ;
static QueueHandle_t RecvQ;

static ISR(SendIntHandler) {
  custom->color[0] = 0x0ff;
  BaseType_t xTaskWokenByReceive = pdFALSE;
  char cSend;
  custom->intreq = INTF_TBE;
  if (xQueueReceiveFromISR(SendQ, (void *)&cSend, &xTaskWokenByReceive))
    custom->serdat = cSend | 0x100;
}

static ISR(RecvIntHandler) {
  custom->color[0] = 0xff0;
  BaseType_t xTaskWokenByReceive = pdFALSE;
  char cRecv = custom->serdatr;
  custom->intreq = INTF_RBF;
  (void)xQueueSendFromISR(RecvQ, (void *)&cRecv, &xTaskWokenByReceive);
}

static ISR_t oldTBE;
static ISR_t oldRBF;

void SerialInit(unsigned baud) {
  custom->serper = CLOCK / baud - 1;

  RecvQ = xQueueCreate(QUEUELEN, sizeof(char));
  SendQ = xQueueCreate(QUEUELEN, sizeof(char));

  oldTBE = ExcVec[EV_INTLVL(1)];
  ExcVec[EV_INTLVL(1)] = SendIntHandler;

  oldRBF = ExcVec[EV_INTLVL(5)];
  ExcVec[EV_INTLVL(5)] = RecvIntHandler;

  custom->intreq = INTF_TBE | INTF_RBF;
  custom->intena = INTF_SETCLR | INTF_TBE | INTF_RBF;
}

void SerialKill(void) {
  custom->intena = INTF_TBE | INTF_RBF;
  custom->intreq = INTF_TBE | INTF_RBF;

  ExcVec[EV_INTLVL(1)] = oldTBE;
  ExcVec[EV_INTLVL(5)] = oldRBF;

  vQueueDelete(RecvQ);
  vQueueDelete(SendQ);
}

void SerialPutChar(__reg("d0") char data) {
  char cSend = data;
  custom->intena = INTF_TBE;
  if (custom->serdatr & SERDATR_TBE) {
    custom->serdat = cSend | 0x100;
  } else {
    (void)xQueueSend(SendQ, &cSend, portMAX_DELAY);
  }
  custom->intena = INTF_SETCLR | INTF_TBE;
  if (cSend == '\n') {
    cSend = '\r';
    (void)xQueueSend(SendQ, &cSend, portMAX_DELAY);
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
