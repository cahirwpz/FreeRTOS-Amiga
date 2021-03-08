#include "ff.h"

#if FF_USE_LFN == 3 /* Dynamic memory allocation */

/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/

void *ff_memalloc(UINT msize) {
  return pvPortMalloc(msize);
}

/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree(void *mblock) {
  vPortFree(mblock);
}

#endif

#if FF_FS_REENTRANT /* Mutal exclusion */

/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
/  synchronization object for the volume, such as semaphore and mutex.
/  When a 0 is returned, the f_mount() function fails with FR_INT_ERR.
*/

int ff_cre_syncobj(__unused BYTE vol, FF_SYNC_t *sobj) {
  *sobj = xSemaphoreCreateMutex();
  return (int)(*sobj != NULL);
}

/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj() function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj(FF_SYNC_t sobj) {
  vSemaphoreDelete(sobj);
  return 1;
}

/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant(FF_SYNC_t sobj) {
  return (int)(xSemaphoreTake(sobj, FF_FS_TIMEOUT) == pdTRUE);
}

/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
 */

void ff_rel_grant(FF_SYNC_t sobj) {
  xSemaphoreGive(sobj);
}

#endif
