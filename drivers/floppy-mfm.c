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
<<<<<<< HEAD
  void *maybeSector = (DiskSector_t *)track;

  if ((*track)[0] == DSK_SYNC)
    maybeSector += sizeof(uint16_t);

  DiskSector_t *sec =
    (DiskSector_t *)((uintptr_t)maybeSector - offsetof(DiskSector_t, info[0]));
=======
  uint16_t *data = (uint16_t *)track;

  /* We always start after the DSK_SYNC word
   * but the first one may be corrupted.
   * In case we start with the sync marker
   * move to the sector header. */
  if (*data == DSK_SYNC)
    data++;

  DiskSector_t *sec = Header2Sector(data);
>>>>>>> upstream/master

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
<<<<<<< HEAD
    if (info.sectors == 1)
      sec = (void *)sec + GAP_SIZE;
=======
    /* Handle the gap. */
    if (info.gapDist == 1 && secnum != 1) {
      /* Move to the first sector behind the gap. */
      data = FindSectorHeader((uint16_t *)sec);
      sec = Header2Sector(data);
    }
>>>>>>> upstream/master
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

#define SECTOR(track, i) ((void *)(track) + (i)*SECTOR_SIZE)

static inline uint32_t EncodeEven(uint32_t lw, uint32_t prev) {
  uint32_t code = lw | ((lw >> 1) ^ (MASK >> 1));
  code &= ~(lw << 1);
  if (!(code >> 30) && !(prev & 1))
    code |= 0x80000000;
  return code;
}

static inline uint32_t Encode(uint32_t lw, uint32_t prevOdd, uint32_t prevEven,
                              uint32_t *odd, uint32_t *even) {
  uint32_t lwOdd = (lw >> 1) & MASK;
  uint32_t lwEven = lw & MASK;
  *odd = EncodeEven(lwOdd, prevOdd);
  *even = EncodeEven(lwEven, prevEven);
  return lwOdd ^ lwEven;
}

static void EncodeSector(uint32_t *decodedSector, DiskSector_t *sector) {
  uint32_t *odd = (uint32_t *)sector->data[0];
  uint32_t *even = (uint32_t *)sector->data[1];
  uint32_t checksum = 0;
  uint32_t first = *decodedSector++;
  uint32_t prev = first;
  int16_t n = SECTOR_PAYLOAD / sizeof(uint32_t) - 1;
  odd++, even++;

  do {
    uint32_t lw = *decodedSector++;
    checksum ^= Encode(lw, prev >> 1, prev, odd++, even++);
    prev = lw;
  } while (--n);

  uint32_t checksumHeader =
    DECODE(sector->checksumHeader[0], sector->checksumHeader[1]);
  (void)Encode(checksum, checksumHeader, checksum >> 1, &sector->checksum[0],
               &sector->checksum[1]);

  (void)Encode(first, checksum, prev >> 1, (uint32_t *)sector->data[0],
               (uint32_t *)sector->data[1]);
}

static inline void UpdateMSB(uint32_t *lw, uint32_t prev) {
  if (!((*lw >> 30) & 1)) {
    *lw &= 0x7fffffff;
    *lw |= (~prev << 31);
  }
}

#define LAST_BIT(es) (*((uint8_t *)(es + 1) - 1) & 1)

void EncodeTrack(uint32_t *decodedTrack, DiskSector_t *sectors[SECTOR_COUNT]) {
  int16_t minIdx, maxIdx;
  minIdx = maxIdx = 0;

  for (int16_t i = 1; i < SECTOR_COUNT; i++)
    if (sectors[i] < sectors[minIdx])
      minIdx = i;
    else if (sectors[i] > sectors[maxIdx])
      maxIdx = i;
  EncodeSector(SECTOR(decodedTrack, minIdx), sectors[minIdx]);

  uint32_t prev = LAST_BIT(sectors[minIdx]);

  for (int16_t i = minIdx + 1; i < SECTOR_COUNT; i++) {
    DiskSector_t *sec = sectors[i];
    EncodeSector(SECTOR(decodedTrack, i), sec);
    UpdateMSB(&sec->magic, prev);
    prev = LAST_BIT(sec);
  }

  uint32_t *lastMagic = NULL;
  uint32_t *gap = (uint32_t *)(sectors[SECTOR_COUNT - 1] + 1);
  UpdateMSB(gap, prev);

  if (minIdx != 0) {
    prev = *((uint8_t *)sectors[0] - 1) & 1;
    for (int16_t i = 0; i < minIdx; i++) {
      DiskSector_t *sec = sectors[i];
      EncodeSector(SECTOR(decodedTrack, i), sec);
      UpdateMSB(&sec->magic, prev);
      prev = LAST_BIT(sec);
    }
    lastMagic = (uint32_t *)(sectors[minIdx - 1] + 1);
  } else {
    lastMagic = (void *)gap + GAP_SIZE;
    prev = *((uint8_t *)lastMagic - 1) & 1;
  }
  UpdateMSB(lastMagic, prev);
}
