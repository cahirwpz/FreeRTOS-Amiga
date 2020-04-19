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
    uint8_t sectors;
  } info[2];
  uint8_t sectorLabel[2][16];
  uint32_t checksumHeader[2];
  uint32_t checksum[2];
  uint8_t data[2][SECTOR_PAYLOAD];
} DiskSector_t;

typedef DiskSector_t *DiskTrack_t[SECTOR_COUNT];

#define MASK 0x55555555
#define DECODE(odd, even) (((odd) & MASK) << 1) | ((even) & MASK)

static void ParseTrack(void *track, DiskTrack_t sectors) {
  int16_t secnum = SECTOR_COUNT;
  DiskSector_t *maybeSector = track;

  do {
    uint16_t *data = (uint16_t *)maybeSector;
    struct {
      uint8_t format;
      uint8_t trackNum;
      uint8_t sectorNum;
      uint8_t sectors;
    } info = {0};

    /* Find synchronization marker and move to first location after it. */
    while (*data != DSK_SYNC)
      data++;
    while (*data == DSK_SYNC)
      data++;

    DiskSector_t *sec =
        (DiskSector_t *)((uintptr_t)data - offsetof(DiskSector_t, info[0]));

    *(uint32_t *)&info =
        DECODE(*(uint32_t *)&sec->info[0], *(uint32_t *)&sec->info[1]);

#if DEBUG
    printf("[MFM] SectorInfo: sector=%x, #sector=%d, #track=%d\n",
           (intptr_t)sec, (int)info.sectorNum, (int)info.trackNum);
#endif

    sectors[info.sectorNum] = sec;
    maybeSector = sec + 1;
  } while (--secnum);
}

static void DecodeSector(DiskSector_t *sector, void *data) {
  uint32_t *dst = data;
  uint32_t *odd = (uint32_t *)sector->data[0];
  uint32_t *even = (uint32_t *)sector->data[1];
  int16_t n = SECTOR_PAYLOAD / sizeof(uint32_t) / 2;

  do {
    uint32_t odd0 = *odd++;
    uint32_t odd1 = *odd++;
    uint32_t even0 = *even++;
    uint32_t even1 = *even++;
    *dst++ = DECODE(odd0, even0);
    *dst++ = DECODE(odd1, even1);
  } while (--n);
}

void DecodeTrack(void *aTrack, void *aData) {
  DiskTrack_t sectors;

  ParseTrack(aTrack, sectors);
  for (int j = 0; j < SECTOR_COUNT; j++) {
    DecodeSector(sectors[j], aData + j * SECTOR_SIZE);
  }
}
