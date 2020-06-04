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

#if DEBUG
#define GET_2BITS(lw, n) (((lw) >> (n)*2) & 3)

static int CheckEncodingLW(uint32_t lw, uint32_t prev) {
  uint8_t pbit = prev & 1;
  for (int16_t i = 15; i > -1; i--) {
    uint8_t triple = GET_2BITS(lw, i) | (pbit << 2);
    if (!triple || triple > 5)
      return 0;
    pbit = triple & 1;
  }
  return 1;
}

static int CheckEncoding(const uint32_t *code, uint32_t len, uint32_t prev) {
  for (uint32_t i = 0; i < len; i++) {
    uint32_t lw = *code++;
    if (!CheckEncodingLW(lw, prev))
      return 0;
    prev = lw;
  }
  return 1;
}
#endif

#define MASK 0x55555555
#define DECODE(odd, even) ((((odd)&MASK) << 1) | ((even)&MASK))

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  int16_t secnum = SECTOR_COUNT;
  DiskSector_t *maybeSector = (DiskSector_t *)track;

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

    configASSERT(CheckEncoding(sec->checksum,
                               (SECTOR_PAYLOAD / sizeof(uint32_t)) * 2 + 2,
                               sec->checksumHeader[1]));
#endif

    sectors[info.sectorNum] = sec;
    maybeSector = sec + 1;
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

void EncodeTrack(uint32_t *decodedTrack, DiskSector_t *sectors[SECTOR_COUNT]) {
  for (int16_t i = 0; i < SECTOR_COUNT; i++)
    EncodeSector(SECTOR(decodedTrack, i), sectors[i]);
}

void EncodeSector(uint32_t *decodedSector, DiskSector_t *sector) {
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

#if DEBUG
  configASSERT(
    CheckEncoding(sector->checksum, (n + 1) * 2 + 2, checksumHeader));
#endif
}
