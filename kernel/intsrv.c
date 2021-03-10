#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>

void AddIntServer(IntChain_t *ic, IntServer_t *is) {
  taskENTER_CRITICAL();
  {
    /* If the list is empty before insertion then enable the interrupt. */
    if (ic->list.uxNumberOfItems == 0) {
      ClearIRQ(ic->flag);
      EnableINT(ic->flag);
    }
    vListInitialiseItem(&is->node);
    vListInsert(&ic->list, &is->node);
  }
  taskEXIT_CRITICAL();
}

void RemIntServer(IntServer_t *is) {
  taskENTER_CRITICAL();
  {
    IntChain_t *ic = (IntChain_t *)is->node.pxContainer;
    /* If the list is empty after removal then disable the interrupt. */
    if (uxListRemove(&is->node) == 0)
      DisableINT(ic->flag);
  }
  taskEXIT_CRITICAL();
}

void RunIntChain(IntChain_t *ic) {
  /* Call each server in turn. */
  for (ListItem_t *node = listGET_HEAD_ENTRY(&ic->list);
       node != listGET_END_MARKER(&ic->list); node = listGET_NEXT(node)) {
    IntServer_t *is = (IntServer_t *)node;
    is->code(listGET_LIST_ITEM_OWNER(node));
  }
}
