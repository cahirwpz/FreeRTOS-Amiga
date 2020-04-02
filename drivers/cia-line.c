#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/list.h>

#include <cia.h>
#include <interrupt.h>

static List_t WaitingTasks;

/* All TOD registers latch on a read of MSB event and remain latched
 * until after a read of LSB event. */
static uint32_t GetCounter(void) {
  uint32_t line = 0;
  line |= ciab.ciatodhi;
  line <<= 8;
  line |= ciab.ciatodmid;
  line <<= 8;
  line |= ciab.ciatodlow;
  return line;
}

/* TOD is automatically stopped whenever a write to the register occurs. The
 * clock will not start again until after a write to the LSB event register. */
static void SetCounter(uint32_t line) {
  ciab.ciatodhi = line >> 16;
  ciab.ciatodmid = line >> 8;
  ciab.ciatodlow = line;
}

static void SetAlarm(uint32_t line) {
  volatile uint8_t *ciacrb = &ciab.ciacrb;
  BSET(*ciacrb, CIACRAB_TODIN);
  SetCounter(line);
  BCLR(*ciacrb, CIACRAB_TODIN);
}

static void LineCounterHandler(List_t *tasks) {
  /* CIA requires the interrupt to be acknowledged by the handler.
   * This is done by reading the value in Interrupt Control Register */
  (void)SampleICR(CIAB, 0x00);

  if (listLIST_IS_EMPTY(tasks))
    return;

  /* Remove all items with counter value not greater that current one.
   * Wake up corresponding tasks. */
  uint32_t curr = GetCounter();
  while (listGET_ITEM_VALUE_OF_HEAD_ENTRY(tasks) <= curr) {
    xTaskHandle task = listGET_OWNER_OF_HEAD_ENTRY(tasks);
    uxListRemove(listGET_HEAD_ENTRY(tasks));
    vTaskNotifyGiveFromISR(task, &xNeedRescheduleTask);
  }

  /* Reprogram TOD alarm if there's an item on list,
   * otherwise disable alarm interrupt. */
  if (listLIST_IS_EMPTY(tasks))
    WriteICR(CIAB, CIAICRF_ALRM);
  else
    SetAlarm(listGET_ITEM_VALUE_OF_HEAD_ENTRY(tasks));
}

INTSERVER_DEFINE(LineCounter, 0, (ISR_t)LineCounterHandler, &WaitingTasks);

void LineCounterInit(void) {
  SetCounter(0);
  vListInitialise(&WaitingTasks);
  AddIntServer(ExterChain, LineCounter);
}

void LineCounterKill(void) {
  WriteICR(CIAB, CIAICRF_ALRM);
  RemIntServer(LineCounter);
  configASSERT(WaitingTasks.uxNumberOfItems == 0);
}

void LineCounterWait(uint32_t lines) {
  taskENTER_CRITICAL();
  {
    /* If the list is empty before insertion then enable the interrupt. */
    if (WaitingTasks.uxNumberOfItems == 0)
      WriteICR(CIAB, CIAICRF_SETCLR | CIAICRF_ALRM);
    /* Calculate wakeup time. */
    uint32_t alarm = GetCounter() + lines;
    /* Insert currently running task onto waiting tasks list. */
    xTaskHandle owner = xTaskGetCurrentTaskHandle();
    ListItem_t item = { .xItemValue = alarm, .pvOwner = owner };
    vListInitialiseItem(&item);
    vListInsert(&WaitingTasks, &item);
    /* Reprogram TOD alarm if inserted task should be woken up as first. */
    if (listGET_HEAD_ENTRY(&WaitingTasks) == &item)
      SetAlarm(alarm);
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  }
  taskEXIT_CRITICAL();
}
