#pragma once

#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>
#include <stdint.h>

/*
 * 3 1/2 inch dual density micro floppy disk drive specifications:
 * http://www.techtravels.org/wp-content/uploads/pefiles/SAMSUNG-SFD321B-070103.pdf
 *
 * Floppy disk rotates at 300 RPM, and transfer rate is 500Kib/s - which gives
 * exactly 12800 bytes per track. With Amiga track encoding that gives a gap of
 * 832 bytes between the end of sector #10 and beginning of sector #0.
 */

#define SECTOR_COUNT 11
#define SECTOR_SIZE 512
#define TRACK_COUNT 160
#define TRACK_SIZE 12800
#define GAP_SIZE 832
#define FLOPPY_SIZE (SECTOR_SIZE * SECTOR_COUNT * TRACK_COUNT)

typedef uint16_t DiskTrack_t[TRACK_SIZE / sizeof(uint16_t)];
typedef struct DiskSector DiskSector_t;
typedef uint32_t RawSector_t[SECTOR_SIZE / sizeof(uint32_t)];

#define CMD_READ 1
#define CMD_WRITE 2

typedef struct FloppyIO {
  uint16_t cmd;            /* command code */
  uint16_t track;          /* track number to transfer */
  DiskTrack_t *buffer;     /* chip memory buffer */
  xQueueHandle replyQueue; /* after request is handled it'll be replied here */
} FloppyIO_t;

void FloppyInit(unsigned aFloppyIOTaskPrio);
void FloppyKill(void);

#define AllocTrack() pvPortMallocChip(TRACK_SIZE)

void FloppySendIO(FloppyIO_t *io);
void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]);
void DecodeSector(const DiskSector_t *disksec, RawSector_t sec);
void EncodeSector(const RawSector_t sec, DiskSector_t *disksec);
void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]);
