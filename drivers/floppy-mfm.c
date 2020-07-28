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
#define NEXTLW(ptr) (((uint32_t *)&ptr)[1])

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

static inline SectorHeader_t DecodeHeader(const DiskSector_t *sec) {
  return ((SectorHeader_u)DECODE(sec->info[ODD], sec->info[EVEN])).hdr;
}

void DecodeTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  short secnum = SECTOR_COUNT;
  uint16_t *data = (uint16_t *)track;

  /* We always start after the DSK_SYNC word but the first one may be corrupted.
   * In case we start with the sync marker move to the sector header. */
  if (*data == DSK_SYNC)
    data++;

  DiskSector_t *sec = HeaderToSector(data);

  do {
    SectorHeader_t hdr = DecodeHeader(sec);

#if DEBUG
    printf("[MFM] SectorInfo: sector=%x, #sector=%d, #track=%d\n",
           (intptr_t)sec, (int)hdr.sectorNum, (int)hdr.trackNum);
    configASSERT(hdr.sectorNum <= SECTOR_COUNT);
    configASSERT(hdr.trackNum <= TRACK_COUNT);
#endif

    sectors[hdr.sectorNum] = sec++;
    /* Handle the gap. */
    if (hdr.gapDist == 1 && secnum > 1) {
      /* Move to the first sector after the gap. */
      sec = FindSectorHeader(sec);
#if DEBUG
      printf("[MFM] Gap of size %d\n",
             (intptr_t)sec - (intptr_t)(sectors[hdr.sectorNum] + 1));
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
  uint32_t chksum;
  /* Verify header checksum. */
  chksum = DECODE(sector->checksumHeader[ODD], sector->checksumHeader[EVEN]);
  configASSERT(chksum == ChecksumHeader(sector));
  /* Calculate sector payload checksum. */
  chksum = 0;
#endif

  do {
    uint32_t odd0 = *dataOdd++;
    uint32_t odd1 = *dataOdd++;
    uint32_t even0 = *dataEven++;
    uint32_t even1 = *dataEven++;
    *buf++ = DECODE(odd0, even0);
    *buf++ = DECODE(odd1, even1);

#if DEBUG
    chksum ^= (odd0 & MASK) ^ (odd1 & MASK);
    chksum ^= (even0 & MASK) ^ (even1 & MASK);
#endif
  } while (--n);

#if DEBUG
  /* Verify sector payload checksum. */
  configASSERT(chksum == DECODE(sector->checksum[ODD], sector->checksum[EVEN]));
#endif
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
 *
 * WARNING: It does not recalculate checksum nor does it take care of fixing
 *          most significant encoded bit.
 */
void RealignTrack(DiskTrack_t *track, DiskSector_t *sectors[SECTOR_COUNT]) {
  DiskSector_t *sector = (void *)track + GAP_SIZE;

  /* Compact sectors at the end of track buffer. */
  for (short i = SECTOR_COUNT - 1; i >= 0; i--) {
    (void)memmove(&sector[i], sectors[i], sizeof(DiskSector_t));
    sectors[i] = &sector[i];
  }

  /* Fill the gap with encoded zeros. */
  (void)memset(track, 0xAA, GAP_SIZE);

  /* Sync word of first sector might have not been read correctly. */
  sector->sync[0] = sector->sync[1] = DSK_SYNC;
}

/* Encode bits of a longword as MFM data. Assumes odd bits are masked out.
 * One bit of encoded data will become two bits of encoded data as follows:
 * 0?01 -> 01
 * 0000 -> 10
 * 0100 -> 00
 * ... which gives following algorithm:
 * 0a0b -> zb where z = ~(a OR b)
 *
 * `prev` stores the least significant bit of previous encoded longword.
 */
static inline uint32_t Encode(uint32_t lw, uint32_t prev) {
  uint32_t odd = lw >> 1;
  if (prev & 1)
    odd |= 0x80000000;
  odd |= lw << 1;
  return lw | (odd ^ MASK);
}

/* If the most significant encoded bit is set to zero, this procedure updates
 * preceding bit according to previous encoded longword. */
static inline void UpdateMSB(uint32_t *lwp, uint32_t prev) {
  uint32_t lw = *lwp;
  /* If encoded bit is set to one, then there's nothing to do. */
  if (lw & 0x40000000)
    return;
  *lwp = (prev & 1) ? (lw | 0x80000000) : (lw & 0x7fffffff);
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
    uint32_t odd = Encode(lw & MASK, prev);
    uint32_t even = Encode((lw >> 1) & MASK, prev >> 1);
    chksum ^= even ^ odd;
    *dataOdd++ = odd;
    *dataEven++ = even;
    prev = lw;
  } while (--n);

  /* Sector checksum is known, so let's encode it. */
  sector->checksum[ODD] = Encode(chksum, PREVLW(sector->checksum[ODD]));
  sector->checksum[EVEN] = Encode(chksum >> 1, chksum);

  /* We know last bit of encoded checksum,
   * thus we can correctly encode first longword of sector payload. */
  sector->data[ODD][0] = Encode(first, chksum >> 1);
  sector->data[EVEN][0] = Encode(first >> 1, prev);
}

void FixTrackEncoding(DiskTrack_t *track) {
  short secnum = SECTOR_COUNT;
  DiskSector_t *sec = (void *)track + GAP_SIZE;

  do {
    /* Update the gap distance and encode the header. */
    SectorHeader_t hdr = DecodeHeader(sec);
    hdr.gapDist = secnum;

    /* Encode the sector header. */
    uint32_t info = ((SectorHeader_u)hdr).lw;

    sec->info[ODD] = Encode(info, DSK_SYNC);
    sec->info[EVEN] = Encode(info >> 1, info);
    UpdateMSB(&NEXTLW(sec->info[EVEN]), info);

    /* Compute and encode the header checksum. */
    uint32_t chksum = ChecksumHeader(sec);

    sec->checksumHeader[ODD] = Encode(chksum, PREVLW(sec->checksumHeader[ODD]));
    sec->checksumHeader[EVEN] = Encode(chksum >> 1, chksum);
    UpdateMSB(&NEXTLW(sec->checksumHeader[EVEN]), chksum);

    /* The least significant bit of last longword in previous sector
     * influences most significant bit of magic value. */
    UpdateMSB(&sec->magic, PREVLW(sec));

    sec++;
  } while (--secnum);

  /* Update the gap's MSB. */
  UpdateMSB((uint32_t *)track, PREVLW(sec));
}
