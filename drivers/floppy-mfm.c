#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <custom.h>
#include <floppy.h>

#define DEBUG 0

/*
 * Amiga MFM track format:
 * http://lclevy.free.fr/adflib/adf_info.html#p22
 */

#define SECTOR_PAYLOAD 512

typedef struct SectorHeader {
  uint8_t format;
  uint8_t trackNum;
  uint8_t sectorNum;
  uint8_t gapDist; /* sectors until end of write */
} SectorHeader_t;

typedef struct DiskSector {
  uint32_t magic;
  uint16_t sync[2];
  SectorHeader_t info[2];
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

#define MASK 0x55555555
#define DECODE(odd, even) ((((odd)&MASK) << 1) | ((even)&MASK))

static inline DiskSector_t *Header2Sector(const uint16_t *header) {
  return (DiskSector_t *)((uintptr_t)header - offsetof(DiskSector_t, info[0]));
}

static inline void GetDecodedHeader(const DiskSector_t *sec,
                                    SectorHeader_t *hdr) {
  *(uint32_t *)hdr =
    DECODE(*(uint32_t *)&sec->info[0], *(uint32_t *)&sec->info[1]);
}

int16_t DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  int16_t secnum = SECTOR_COUNT;
  int16_t gapSecnum = -1;
  uint16_t *data = (uint16_t *)track;

  /* We always start after the DSK_SYNC word
   * but the first one may be corrupted.
   * In case we start with the sync marker
   * move to the sector header. */
  if (*data == DSK_SYNC)
    data++;

  DiskSector_t *sec = Header2Sector(data);

  do {
    SectorHeader_t info;
    GetDecodedHeader(sec, &info);

#if DEBUG
    printf("[MFM] SectorInfo: sector=%x, #sector=%d, #track=%d\n",
           (intptr_t)sec, (int)info.sectorNum, (int)info.trackNum);
#endif

    sectors[info.sectorNum] = sec++;
    /* Handle the gap. */
    if (info.gapDist == 1) {
      gapSecnum = info.sectorNum;
      /* Move to the first sector behind the gap. */
      data = FindSectorHeader((uint16_t *)sec);
      sec = Header2Sector(data);
    }
  } while (--secnum);

  return gapSecnum;
}

static uint32_t ComputeChecksumHeader(const DiskSector_t *sector) {
  uint32_t checksum = 0;
  uint32_t *lw = (uint32_t *)sector->info;
  /* The header consist of info and sectorLabel. */
  for (int16_t i = 0; i < 10; i++)
    checksum ^= *lw++ & MASK;
  return checksum;
}

void DecodeSector(const DiskSector_t *sector, uint32_t *buf) {
  uint32_t *odd = (uint32_t *)sector->data[0];
  uint32_t *even = (uint32_t *)sector->data[1];
  int16_t n = SECTOR_PAYLOAD / sizeof(uint32_t) / 2;

#if DEBUG
  /* Verify header checksum. */
  uint32_t checksumHeader =
    DECODE(sector->checksumHeader[0], sector->checksumHeader[1]);
  configASSERT(checksumHeader == ComputeChecksumHeader(sector));
  uint32_t checksum = 0;
#endif

  do {
    uint32_t odd0 = *odd++;
    uint32_t odd1 = *odd++;
    uint32_t even0 = *even++;
    uint32_t even1 = *even++;
    *buf++ = DECODE(odd0, even0);
    *buf++ = DECODE(odd1, even1);

#if DEBUG
    checksum ^= (odd0 & MASK) ^ (odd1 & MASK);
    checksum ^= (even0 & MASK) ^ (even1 & MASK);
#endif

  } while (--n);

#if DEBUG
  /* Verify data checksum. */
  configASSERT(checksum == DECODE(sector->checksum[0], sector->checksum[1]));
#endif
}

/* Realign sector pointers. */
static void RealignSectors(DiskSector_t *sectors[SECTOR_COUNT], int16_t start,
                           size_t n, size_t shift) {
  size_t i = start;
  for (size_t cnt = 0; cnt < n; cnt++) {
    sectors[i] = (void *)sectors[i] + shift;
    i = (i + 1) % SECTOR_COUNT;
  }
}

static int16_t FirstSectorNumber(const DiskTrack_t *track) {
  uint16_t *data = (uint16_t *)track;
  if (*data == DSK_SYNC)
    data++;
  DiskSector_t *sec = Header2Sector(data);
  SectorHeader_t info;
  GetDecodedHeader(sec, &info);
  return info.sectorNum;
}

#define TORANGE(d) (((d) + SECTOR_COUNT) % SECTOR_COUNT)

void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  bool isFirstSyncOK = (*(uint16_t *)track == DSK_SYNC);
  int16_t first = FirstSectorNumber(track);
  int16_t last = TORANGE(first + SECTOR_COUNT - 1);
  int16_t gapSecnum, gapNextSecnum;
  size_t leftSize, rightSize;
  void *gap = NULL;

  /* Last sector before the gap, and first
   * sector behind the gap. */
  gapSecnum = DecodeTrack(track, sectors);
  gapNextSecnum = (gapSecnum + 1) % SECTOR_COUNT;

  int16_t afterGapNum = TORANGE(last - gapSecnum);
  /* Move all the sectors behind the gap
   * to the gap position (e.g. remove an inner gap). */
  if (afterGapNum) {
    gap = sectors[gapSecnum] + 1;
    leftSize = afterGapNum * sizeof(DiskSector_t);
    memmove(gap, sectors[gapNextSecnum], leftSize);
  }

  /* Create a gap at the beginning of the track and
   * move all the sectors to the end. */
  size_t missingSize = sizeof(uint32_t) + sizeof(uint16_t);
  size_t shift =
    GAP_SIZE + missingSize + (isFirstSyncOK ? 0 : sizeof(uint16_t));
  rightSize = sizeof(DiskSector_t) * SECTOR_COUNT - missingSize;
  memmove((void *)track + shift, track, rightSize);

  /* Update sector pointers to reflect current sectors position. */
  RealignSectors(sectors, first, SECTOR_COUNT - afterGapNum, shift);
  if (afterGapNum)
    RealignSectors(sectors, gapNextSecnum, afterGapNum,
                   shift -
                     ((uintptr_t)sectors[gapNextSecnum] - (uintptr_t)gap));

  /* Gap is 832 bytes of 0x00 data. */
  memset(track, 0xAA, GAP_SIZE + sizeof(uint32_t));

  /* The first sector read is cut off so we have to complete it. */
  DiskSector_t *sec = sectors[first];
  sec->sync[0] = sec->sync[1] = DSK_SYNC;
}

#define SECTOR(track, i) ((void *)(track) + (i)*SECTOR_SIZE)

/* Encode a long word of type 0d0d...0d,
 * where d bits are data bits. */
static inline uint32_t EncodeEven(uint32_t lw, uint32_t prev) {
  uint32_t code = lw | ((lw >> 1) ^ (MASK >> 1));
  code &= ~(lw << 1);
  if (!(code >> 30) && !(prev & 1))
    code |= 0x80000000;
  return code;
}

/* Encode a long word. Odd bits are stored in odd and
 * even bits are stored in even. */
static inline uint32_t Encode(uint32_t lw, uint32_t prevOdd, uint32_t prevEven,
                              uint32_t *odd, uint32_t *even) {
  uint32_t lwOdd = (lw >> 1) & MASK;
  uint32_t lwEven = lw & MASK;
  *odd = EncodeEven(lwOdd, prevOdd);
  *even = EncodeEven(lwEven, prevEven);
  return lwOdd ^ lwEven;
}

void EncodeSector(uint32_t *buf, DiskSector_t *sector) {
  uint32_t *odd = (uint32_t *)sector->data[0];
  uint32_t *even = (uint32_t *)sector->data[1];
  uint32_t first = *buf++;
  uint32_t prev = first;
  uint32_t checksum = ((first >> 1) & MASK) ^ (first & MASK);
  int16_t n = SECTOR_PAYLOAD / sizeof(uint32_t) - 1;
  odd++, even++;

  do {
    uint32_t lw = *buf++;
    checksum ^= Encode(lw, prev >> 1, prev, odd++, even++);
    prev = lw;
  } while (--n);

  (void)Encode(checksum, sector->checksumHeader[1], checksum >> 1,
               &sector->checksum[0], &sector->checksum[1]);

  (void)Encode(first, checksum, prev >> 1, (uint32_t *)sector->data[0],
               (uint32_t *)sector->data[1]);
}

static inline void UpdateMSB(uint32_t *lw, uint32_t prev) {
  if (!((*lw >> 30) & 1)) {
    *lw &= 0x7fffffff;
    *lw |= ~prev << 31;
  }
}

static inline void EncodeHeader(DiskSector_t *sec, const SectorHeader_t *hdr) {
  uint32_t checksumHeader;
  uint32_t infoLW = *(uint32_t *)hdr;

  /* Encode the sector header. */
  (void)Encode(infoLW, DSK_SYNC, infoLW >> 1, (uint32_t *)&sec->info[0],
               (uint32_t *)&sec->info[1]);

  UpdateMSB((uint32_t *)sec->sectorLabel, infoLW);

  /* Compute and encode the header checksum. */
  checksumHeader = ComputeChecksumHeader(sec);

  (void)Encode(checksumHeader, sec->sectorLabel[1][15], checksumHeader >> 1,
               &sec->checksumHeader[0], &sec->checksumHeader[1]);

  UpdateMSB(sec->checksum, checksumHeader);
}

void FixTrackEncoding(DiskTrack_t *track) {
  int16_t secnum = SECTOR_COUNT;
  DiskSector_t *sec = (DiskSector_t *)((void *)track + GAP_SIZE);

  do {
    /* Update the gap distance and
     * encode the header. */
    SectorHeader_t info;
    GetDecodedHeader(sec, &info);
    info.gapDist = secnum--;
    EncodeHeader(sec, &info);

    /* Update the sector's magic value. */
    UpdateMSB(&sec->magic, *((uint8_t *)sec - 1));

    sec++;
  } while (secnum);

  /* Update the gap's MSB. */
  UpdateMSB((uint32_t *)track, *((uint8_t *)sec - 1));
}
