#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <custom.h>
#include <floppy.h>

#define DEBUG 1

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

static inline int16_t NextSecnum(int16_t secnum) {
  if (++secnum >= SECTOR_COUNT)
    secnum = 0;
  return secnum;
}

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

static inline bool IsSecondSyncOK(const void *track) {
  return *(uint16_t *)track == DSK_SYNC;
}

void GenDecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT],
                    int16_t *firstSecnum, int16_t *gapSecnum) {
  int16_t secnum = SECTOR_COUNT;
  uint16_t *data = (uint16_t *)track;

  /* We always start after the DSK_SYNC word
   * but the first one may be corrupted.
   * In case we start with the sync marker
   * move to the sector header. */
  if (IsSecondSyncOK(track))
    data++;

  DiskSector_t *sec = Header2Sector(data);
  SectorHeader_t info;

  do {
    GetDecodedHeader(sec, &info);

#if DEBUG
    printf("[MFM] SectorInfo: sector=%x, #sector=%d, #track=%d\n",
           (intptr_t)sec, (int)info.sectorNum, (int)info.trackNum);
#endif

    sectors[info.sectorNum] = sec++;
    /* Handle the gap. */
    if (info.gapDist == 1) {
      if (gapSecnum)
        *gapSecnum = info.sectorNum;
      /* Move to the first sector behind the gap. */
      data = FindSectorHeader((uint16_t *)sec);
      sec = Header2Sector(data);
    }
  } while (--secnum);

  if (firstSecnum)
    *firstSecnum = NextSecnum(info.sectorNum);
}

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  GenDecodeTrack(track, sectors, NULL, NULL);
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
  int16_t i = start;
  for (int16_t it = 0; it < 2; it++) {
    for (; n && i < SECTOR_COUNT; i++, n--)
      sectors[i] = (void *)sectors[i] + shift;
    i = 0;
  }
}

/* Obtain number of sectors between the two given sectors. */
static inline int16_t SectorDist(int16_t s1, int16_t s2) {
  int16_t dist = s1 - s2 + 1;
  if (dist < 0)
    dist += SECTOR_COUNT;
  return dist;
}

static inline uintptr_t PtrDiff(const void *p1, const void *p2) {
  return p1 - p2;
}

void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT],
                  int16_t firstSecnum, int16_t gapSecnum) {
  int16_t beforeGap = SectorDist(gapSecnum, firstSecnum);
  int16_t afterGap = SECTOR_COUNT - beforeGap;
  int16_t afterGapSecnum = NextSecnum(gapSecnum);
  size_t afterGapOff = PtrDiff(sectors[afterGapSecnum], track);
  size_t leftOff, leftSize, rightOff, rightSize;

  leftOff = GAP_SIZE + sizeof(uint32_t) + sizeof(uint16_t);
  if (!IsSecondSyncOK(track))
    leftOff += sizeof(uint16_t);
  leftSize = beforeGap * sizeof(DiskSector_t);

  /* Shift sectors behind the gap. */
  if (afterGap) {
    rightOff = leftSize + GAP_SIZE /*- gapSize*/;
    rightSize = afterGap * sizeof(DiskSector_t);
    memmove((void *)track + rightOff, sectors[afterGapSecnum], rightSize);
    RealignSectors(sectors, afterGapSecnum, afterGap, rightOff - afterGapOff);
  }

  /* Shift sectors before the gap. */
  memmove((void *)track + leftOff, track, leftSize);
  RealignSectors(sectors, firstSecnum, beforeGap, leftOff);

  /* Fill the gap. */
  (void)memset(track, 0xAA, leftOff);

  /* Complete the first sector. */
  DiskSector_t *firstSec = sectors[firstSecnum];
  firstSec->sync[0] = firstSec->sync[1] = DSK_SYNC;
}

#define SECTOR(track, i) ((void *)(track) + (i)*SECTOR_SIZE)

/* Encode a long word of type 0d0d...0d,
 * where d bits are data bits. */
static inline uint32_t EncodeEven(uint32_t lw, uint32_t prev) {
  uint32_t code = lw | ((lw >> 1) ^ (MASK >> 1));
  code &= ~(lw << 1);
  if (!(code & 0x40000000) && !(prev & 1))
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
  if (!(*lw & 0x40000000)) {
    *lw &= 0x7fffffff;
    if (!(prev & 1))
      *lw |= 0x80000000;
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
