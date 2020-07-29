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

typedef struct SectorHeader {
  uint8_t format;
  uint8_t trackNum;
  uint8_t sectorNum;
  uint8_t gapDist; /* sectors until end of write */
} SectorHeader_t;

typedef union {
  SectorHeader_t hdr;
  uint32_t lw;
} SectorHeader_u;

#define ODD 0
#define EVEN 1

typedef struct DiskSector {
  uint32_t magic;
  uint16_t sync[2];
  uint32_t info[2];
  uint32_t sectorLabel[2][4]; /* decoded value is always zeros */
  uint32_t checksumHeader[2];
  uint32_t checksum[2];
  uint32_t data[2][SECTOR_SIZE / sizeof(uint32_t)];
} DiskSector_t;

#define MASK 0x55555555
#define DECODE(odd, even) ((((odd)&MASK) << 1) | ((even)&MASK))
#define PREVLW(ptr) (((uint32_t *)&ptr)[-1])

static inline DiskSector_t *HeaderToSector(uint16_t *header) {
  return (DiskSector_t *)((uintptr_t)header - offsetof(DiskSector_t, info[0]));
}

static DiskSector_t *FindSectorHeader(void *ptr) {
  uint16_t *data = ptr;
  /* Find synchronization marker and move to first location after it. */
  while (*data != DSK_SYNC)
    data++;
  while (*data == DSK_SYNC)
    data++;
  return HeaderToSector(data);
}

static inline uint32_t DecodeLong(const uint32_t *enc) {
  return DECODE(enc[ODD], enc[EVEN]);
}

static inline SectorHeader_t DecodeHeader(const DiskSector_t *sec) {
  return ((SectorHeader_u)DecodeLong(sec->info)).hdr;
}

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  /* We always start after the DSK_SYNC word but the first one may be corrupted.
   * In case we start with the sync marker move to the sector header. */
  uint16_t *data = (uint16_t *)track;

  if (*data == DSK_SYNC)
    data++;

  DiskSector_t *sector = HeaderToSector(data);
  short secnum = SECTOR_COUNT;

  do {
    SectorHeader_t hdr = DecodeHeader(sector);

#if DEBUG
    printf("[MFM] Read: sector=%p, #sector=%d, #track=%d, #gap=%d\n", sector,
           (int)hdr.sectorNum, (int)hdr.trackNum, (int)hdr.gapDist);
    configASSERT(hdr.sectorNum <= SECTOR_COUNT);
    configASSERT(hdr.trackNum <= TRACK_COUNT);
#endif

    sectors[hdr.sectorNum] = sector++;
    /* Handle the gap. */
    if (hdr.gapDist == 1 && secnum > 1) {
      /* Move to the first sector after the gap. */
      sector = FindSectorHeader(sector);
#if DEBUG
      printf("[MFM] Gap of size %d\n",
             (intptr_t)sector - (intptr_t)(sectors[hdr.sectorNum] + 1));
#endif
    }
  } while (--secnum);
}

static uint32_t ChecksumHeader(const DiskSector_t *sector) {
  /* The header consist of info and sectorLabel. */
  const uint32_t *ptr = sector->info;
  const uint32_t *end = sector->checksumHeader;
  uint32_t checksum = 0;
  do {
    checksum ^= *ptr++ & MASK;
  } while (ptr < end);
  return checksum;
}

void DecodeSector(const DiskSector_t *sector, RawSector_t buf) {
  const uint32_t *dataOdd = &sector->data[ODD][0];
  const uint32_t *dataEven = &sector->data[EVEN][0];
  short n = SECTOR_SIZE / sizeof(uint32_t) / 2;

#if DEBUG
  /* Verify header checksum. */
  configASSERT(DecodeLong(sector->checksumHeader) == ChecksumHeader(sector));
  /* Calculate sector payload checksum. */
  uint32_t chksum = 0;
#endif

  do {
    uint32_t odd0 = *dataOdd++;
    uint32_t odd1 = *dataOdd++;
    uint32_t even0 = *dataEven++;
    uint32_t even1 = *dataEven++;
    *buf++ = DECODE(odd0, even0);
    *buf++ = DECODE(odd1, even1);

#if DEBUG
    chksum ^= odd0 ^ odd1;
    chksum ^= even0 ^ even1;
#endif
  } while (--n);

#if DEBUG
  /* Verify sector payload checksum. */
  configASSERT((chksum & MASK) == DecodeLong(sector->checksum));
#endif
}

/* Encode bits of a longword as MFM data. One bit of encoded data will become
 * two bits of encoded data as follows:
 * 0?01 -> 01
 * 0000 -> 10
 * 0100 -> 00
 * ... which gives following algorithm:
 * 0a0b -> zb where z = ~(a OR b)
 *
 * `prev` stores the least significant bit of previous encoded longword.
 */
static inline uint32_t Encode(uint32_t lw, uint32_t prev) {
  lw &= MASK;                      /* -a-b-c-d */
  uint32_t odd = lw >> 1;          /* --a-b-c- */
  if (prev & 1)                    /* get x bit from previous word */
    odd |= 0x80000000;             /* x-a-b-c- */
  odd |= lw << 1;                  /* x-a-b-c- | a-b-c-d- */
  return lw | (odd ^ (MASK << 1)); /* -a-b-c-d | ~(x-a-b-c- | a-b-c-d-) */
}

/* If the most significant encoded bit is set to zero, this procedure updates
 * preceding bit according to previous encoded longword. */
static inline void UpdateMSB(uint32_t *lwp, uint32_t prev) {
  uint32_t lw = *lwp;
  /* If encoded bit is set to one, then there's nothing to do. */
  if (lw & 0x40000000)
    return;
  /* 0000 -> 10, 0100 -> 00 */
  *lwp = (prev & 1) ? (lw & 0x7fffffff) : (lw | 0x80000000);
}

static inline void EncodeLongWord(uint32_t *enc, uint32_t lw) {
  enc[ODD] = Encode(lw >> 1, enc[-1]);
  enc[EVEN] = Encode(lw, lw >> 1);
  configASSERT(lw == DecodeLong(enc));
}

void EncodeSector(const RawSector_t buf, DiskSector_t *sector) {
  /* Before we correctly encode first longword of data we need to calculate
   * and encode checksum, thus we start encoding from second longword. */
  uint32_t *dataOdd = &sector->data[ODD][1];
  uint32_t *dataEven = &sector->data[EVEN][1];
  uint32_t first = *buf++;

  /* Calculate checksum of first longword. */
  uint32_t chksum = ((first >> 1) & MASK) ^ (first & MASK);
  /* Previous word to encode. */
  uint32_t prev = first;
  short n = SECTOR_SIZE / sizeof(uint32_t) - 1;

  do {
    uint32_t lw = *buf++;
    uint32_t odd = Encode(lw >> 1, prev >> 1);
    uint32_t even = Encode(lw, prev);
    chksum ^= odd ^ even;
    *dataOdd++ = odd;
    *dataEven++ = even;
    prev = lw;
    configASSERT(lw == DECODE(odd, even));
  } while (--n);

  /* Sector checksum is known, so let's encode it. */
  EncodeLongWord(sector->checksum, chksum & MASK);

  /* We know last bit of encoded checksum,
   * thus we can correctly encode first longword of sector payload. */
  sector->data[ODD][0] = Encode(first >> 1, chksum);
  sector->data[EVEN][0] = Encode(first, prev);
  configASSERT(first == DECODE(sector->data[ODD][0], sector->data[EVEN][0]));
}

/*
 * Floppy drive rotational speed can vary (at most +/- 5%) !!!
 *
 * It's suprising that physical track may be somewhat longer or shorter than
 * TRACK_SIZE. When physical track is shorter than TRACK_SIZE there's a real
 * danger of overwriting data at the beginning of track. Hence we reorganize
 * encoded track buffer in such a way that it begins with a zero-filled GAP
 * and ends with last sector to be recorded.
 *
 * http://amigadev.elowar.com/read/ADCD_2.1/Devices_Manual_guide/node015B.html
 *
 * During first-ever write of a track (formatting) a buffer looks like this:
 *   <GAP>|sector0|sector1|sector2|......|sector10|
 *              11      10       9      ?        1 <- gap distance
 *
 * But we can read a track into a buffer as follows:
 *   <JUNK>|sector9|sector10|<GAP>|sector0|...|sector8|<JUNK>
 *                2        1            11 ...       3 <- gap distance
 *
 * Result of track re-aligning (as peformed by the procedure below) should be:
 *   <GAP>|sector9|sector10|sector0|...|sector8|
 *              11       10       9 ...       1 <- gap distance
 *
 * ... so that when the track is rewritten, the sector offsets are adjusted to
 * match the way the data was written.
 */
void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  DiskSector_t *sector = (void *)track + GAP_SIZE;

  /* Find sector with the highest address. */
  DiskSector_t *last = NULL;
  short lastNum = -1;
  for (short i = 0; i < SECTOR_COUNT; i++) {
    if (sectors[i] > last) {
      last = sectors[i];
      lastNum = i;
    }
  }

  /* Compact sectors towards the end of track buffer. */
  for (short i = SECTOR_COUNT - 1, j = lastNum; i >= 0; i--, j--) {
    if (j < 0)
      j += SECTOR_COUNT;
    (void)memmove(&sector[i], sectors[j], sizeof(DiskSector_t));
  }

  /* Fill the gap with encoded zeros. */
  (void)memset(track, 0xAA, GAP_SIZE);

  /* Sync word of first sector and magic longword might have not been read
   * correctly. Fix them now. */
  sector->magic = 0xAAAAAAAA;
  sector->sync[1] = DSK_SYNC;

  /* Fix sector header encoding */
  short gapDist = SECTOR_COUNT;

  do {
    /* Update the gap distance and encode the header. */
    SectorHeader_t hdr = DecodeHeader(sector);
    hdr.gapDist = gapDist;

#if DEBUG
    printf("[MFM] Write: sector=%p, #sector=%d, #track=%d, #gap=%d\n", sector,
           (int)hdr.sectorNum, (int)hdr.trackNum, (int)hdr.gapDist);
#endif

    /* Encode the sector header. */
    uint32_t info = ((SectorHeader_u)hdr).lw;
    EncodeLongWord(sector->info, info);
    UpdateMSB(sector->info + 1, info);

    /* Compute and encode the header checksum. */
    uint32_t chksum = ChecksumHeader(sector);
    EncodeLongWord(sector->checksumHeader, chksum);
    UpdateMSB(sector->checksumHeader + 1, chksum);

    /* The least significant bit of last longword in previous sector
     * influences most significant bit of magic value. */
    UpdateMSB(&sector->magic, PREVLW(sector));

    sector++;
  } while (--gapDist);
}
