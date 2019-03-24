#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include <FreeRTOSConfig.h>
#include <stdint.h>

/*
 * 3 1/2 inch dual density micro floppy disk drive specifications:
 * http://www.techtravels.org/wp-content/uploads/pefiles/SAMSUNG-SFD321B-070103.pdf
 *
 * Floppy disk rotates at 300 RPM, and transfer rate is 500Kib/s - which gives
 * exactly 12800 bytes per track. With Amiga track encoding that gives a gap of
 * 832 bytes between the end of sector #10 and beginning of sector #0.
 */

#define TRACK_NSECTORS 11
#define TRACK_SIZE 12800

void FloppyInit(unsigned aFloppyIOTaskPrio);
void FloppyKill(void);

#define AllocFloppyTrack() pvPortMallocChip(TRACK_SIZE)

void ReadFloppyTrack(void *aTrack, uint16_t aTrackNum);

#endif /* !_FLOPPY_H_ */
