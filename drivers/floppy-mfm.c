#include <stdint.h>
#include <stdio.h>

#include <custom.h>
#include <floppy.h>

#define DEBUG 0

/*
 * Amiga MFM track format:
 * http://lclevy.free.fr/adflib/adf_info.html#p22
 */

#define SECTOR_PAYLOAD 512

typedef struct DiskSector {
  uint32_t magic;
  uint16_t sync[2];
  struct {
    uint8_t format;
    uint8_t trackNum;
    uint8_t sectorNum;
    uint8_t gapDist; /* sectors until end of write */
  } info[2];
  uint8_t sectorLabel[2][16];
  uint32_t checksumHeader[2];
  uint32_t checksum[2];
  uint8_t data[2][SECTOR_PAYLOAD];
} DiskSector_t;

static uint16_t *FindSectorHeader(uint16_t *data) {
  /* Find synchronization marker and move to first location after it. */
  while (*data != DSK_SYNC)
    data++;
  while (*data == DSK_SYNC)
    data++;
  return data;
}

static inline DiskSector_t *Header2Sector(uint16_t *header) {
  return (DiskSector_t *)((uintptr_t)header - offsetof(DiskSector_t, info[0]));
}

#define MASK 0x55555555
#define DECODE(odd, even) ((((odd)&MASK) << 1) | ((even)&MASK))

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  int16_t secnum = SECTOR_COUNT;
  uint16_t *data = (uint16_t *)track;

  /* We always start after the DSK_SYNC word
   * but the first one may be corrupted.
   * In case we start with the sync marker
   * move to the sector header. */
  if (*data == DSK_SYNC)
    data++;

  DiskSector_t *sec = Header2Sector(data);

  do {
    struct {
      uint8_t format;
      uint8_t trackNum;
      uint8_t sectorNum;
      uint8_t gapDist;
    } info = {0};

    *(uint32_t *)&info =
      DECODE(*(uint32_t *)&sec->info[0], *(uint32_t *)&sec->info[1]);

#if DEBUG
    printf("[MFM] SectorInfo: sector=%x, #sector=%d, #track=%d\n",
           (intptr_t)sec, (int)info.sectorNum, (int)info.trackNum);
#endif

    sectors[info.sectorNum] = sec++;
    /* Handle the gap. */
    if (info.gapDist == 1 && secnum != 1) {
      /* Move to the first sector behind the gap. */
      data = FindSectorHeader((uint16_t *)sec);
      sec = Header2Sector(data);
    }
  } while (--secnum);
}

void DecodeSector(DiskSector_t *sector, uint32_t *buf) {
  uint32_t *odd = (uint32_t *)sector->data[0];
  uint32_t *even = (uint32_t *)sector->data[1];
  int16_t n = SECTOR_PAYLOAD / sizeof(uint32_t) / 2;

  do {
    uint32_t odd0 = *odd++;
    uint32_t odd1 = *odd++;
    uint32_t even0 = *even++;
    uint32_t even1 = *even++;
    *buf++ = DECODE(odd0, even0);
    *buf++ = DECODE(odd1, even1);
  } while (--n);
}
