#pragma once

/*
 * 3 1/2 inch dual density micro floppy disk drive specifications:
 * http://www.techtravels.org/wp-content/uploads/pefiles/SAMSUNG-SFD321B-070103.pdf
 *
 * Floppy disk rotates at 300 RPM, and transfer rate is 500Kib/s - which gives
 * exactly 12800 bytes per track. With Amiga track encoding that gives a gap of
 * 832 bytes between the end of sector #10 and beginning of sector #0.
 */

#define NSECTORS 11
#define SECTOR_SIZE 512U
#define NTRACKS 160
#define TRACK_SIZE (SECTOR_SIZE * NSECTORS)
#define FLOPPY_SIZE (TRACK_SIZE * NTRACKS)

#ifdef __FLOPPY_DRIVER

#include <sys/types.h>

#define DISK_TRACK_SIZE 12800
#define DISK_GAP_SIZE 832

typedef enum SectorState {
  DECODED = 1,
  DIRTY = 2,
} __packed SectorState_t;

typedef uint16_t DiskTrack_t[TRACK_SIZE / sizeof(uint16_t)];
typedef struct DiskSector DiskSector_t;
typedef uint32_t RawSector_t[SECTOR_SIZE / sizeof(uint32_t)];

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[NSECTORS]);
void DecodeSector(const DiskSector_t *disksec, RawSector_t sec);
void EncodeSector(const RawSector_t sec, DiskSector_t *disksec);
void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[NSECTORS]);

#define FLOPPY_TASK_PRIO 3

#endif /* !_FLOPPY_DRIVER */
