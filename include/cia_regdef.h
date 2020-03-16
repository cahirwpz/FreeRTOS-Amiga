#ifndef _CIA_REGDEF_H_
#define _CIA_REGDEF_H_

#ifndef __ASSEMBLER__

#include <cdefs.h>

struct CIA {
  uint8_t ciapra;
  uint8_t pad_0[255];
  uint8_t ciaprb;
  uint8_t pad_1[255];
  uint8_t ciaddra;
  uint8_t pad_2[255];
  uint8_t ciaddrb;
  uint8_t pad_3[255];
  uint8_t ciatalo;
  uint8_t pad_4[255];
  uint8_t ciatahi;
  uint8_t pad_5[255];
  uint8_t ciatblo;
  uint8_t pad_6[255];
  uint8_t ciatbhi;
  uint8_t pad_7[255];
  uint8_t ciatodlow;
  uint8_t pad_8[255];
  uint8_t ciatodmid;
  uint8_t pad_9[255];
  uint8_t ciatodhi;
  uint8_t pad_a[255];
  uint8_t unusedreg;
  uint8_t pad_b[255];
  uint8_t ciasdr;
  uint8_t pad_c[255];
  uint8_t _ciaicr; /* please use WriteICR / SampleICR procedures */
  uint8_t pad_d[255];
  uint8_t ciacra;
  uint8_t pad_e[255];
  uint8_t ciacrb;
  uint8_t pad_f[255];
};

#else

#define ciapra 0x0000
#define ciaprb 0x0100
#define ciaddra 0x0200
#define ciaddrb 0x0300

#endif

/* interrupt control register bit numbers */
#define CIAICRB_TA 0
#define CIAICRB_TB 1
#define CIAICRB_ALRM 2
#define CIAICRB_SP 3
#define CIAICRB_FLG 4
#define CIAICRB_IR 7
#define CIAICRB_SETCLR 7

/* control register A bit numbers */
#define CIACRAB_START 0
#define CIACRAB_PBON 1
#define CIACRAB_OUTMODE 2
#define CIACRAB_RUNMODE 3
#define CIACRAB_LOAD 4
#define CIACRAB_INMODE 5
#define CIACRAB_SPMODE 6
#define CIACRAB_TODIN 7

/* control register B bit numbers */
#define CIACRBB_START 0
#define CIACRBB_PBON 1
#define CIACRBB_OUTMODE 2
#define CIACRBB_RUNMODE 3
#define CIACRBB_LOAD 4
#define CIACRBB_INMODE0 5
#define CIACRBB_INMODE1 6
#define CIACRBB_ALARM 7

/* interrupt control register masks */
#define CIAICRF_TA BIT(CIAICRB_TA)
#define CIAICRF_TB BIT(CIAICRB_TB)
#define CIAICRF_ALRM BIT(CIAICRB_ALRM)
#define CIAICRF_SP BIT(CIAICRB_SP)
#define CIAICRF_FLG BIT(CIAICRB_FLG)
#define CIAICRF_IR BIT(CIAICRB_IR)
#define CIAICRF_SETCLR BIT(CIAICRB_SETCLR)
#define CIAICRF_ALL (CIAICRF_TA | CIAICRF_TB | CIAICRF_ALRM | CIAICRF_SP |     \
                     CIAICRF_FLG)

/* control register A register masks */
#define CIACRAF_START BIT(CIACRAB_START)
#define CIACRAF_PBON BIT(CIACRAB_PBON)
#define CIACRAF_OUTMODE BIT(CIACRAB_OUTMODE)
#define CIACRAF_RUNMODE BIT(CIACRAB_RUNMODE)
#define CIACRAF_LOAD BIT(CIACRAB_LOAD)
#define CIACRAF_INMODE BIT(CIACRAB_INMODE)
#define CIACRAF_SPMODE BIT(CIACRAB_SPMODE)
#define CIACRAF_TODIN BIT(CIACRAB_TODIN)

/* control register B register masks */
#define CIACRBF_START BIT(CIACRBB_START)
#define CIACRBF_PBON BIT(CIACRBB_PBON)
#define CIACRBF_OUTMODE BIT(CIACRBB_OUTMODE)
#define CIACRBF_RUNMODE BIT(CIACRBB_RUNMODE)
#define CIACRBF_LOAD BIT(CIACRBB_LOAD)
#define CIACRBF_INMODE0 BIT(CIACRBB_INMODE0)
#define CIACRBF_INMODE1 BIT(CIACRBB_INMODE1)
#define CIACRBF_ALARM BIT(CIACRBB_ALARM)

/* control register B INMODE masks */
#define CIACRBF_IN_PHI2 0
#define CIACRBF_IN_CNT (CIACRBF_INMODE0)
#define CIACRBF_IN_TA (CIACRBF_INMODE1)
#define CIACRBF_IN_CNT_TA (CIACRBF_INMODE0 | CIACRBF_INMODE1)

/*
 * Port definitions -- what each bit in a cia peripheral register is tied to
 */

/* ciaa port A (0xbfe001) */
#define CIAB_GAMEPORT1 (7) /* gameport 1, pin 6 (fire button*) */
#define CIAB_GAMEPORT0 (6) /* gameport 0, pin 6 (fire button*) */
#define CIAB_DSKRDY (5)    /* disk ready* */
#define CIAB_DSKTRACK0 (4) /* disk on track 00* */
#define CIAB_DSKPROT (3)   /* disk write protect* */
#define CIAB_DSKCHANGE (2) /* disk change* */
#define CIAB_LED (1)       /* led light control (0==>bright) */
#define CIAB_OVERLAY (0)   /* memory overlay bit */

/* ciaa port B (0xbfe101) -- parallel port */

/* ciab port A (0xbfd000) -- serial and printer control */
#define CIAB_COMDTR (7)   /* serial Data Terminal Ready* */
#define CIAB_COMRTS (6)   /* serial Request to Send* */
#define CIAB_COMCD (5)    /* serial Carrier Detect* */
#define CIAB_COMCTS (4)   /* serial Clear to Send* */
#define CIAB_COMDSR (3)   /* serial Data Set Ready* */
#define CIAB_PRTRSEL (2)  /* printer SELECT */
#define CIAB_PRTRPOUT (1) /* printer paper out */
#define CIAB_PRTRBUSY (0) /* printer busy */

/* ciab port B (0xbfd100) -- disk control */
#define CIAB_DSKMOTOR (7) /* disk motorr* */
#define CIAB_DSKSEL3 (6)  /* disk select unit 3* */
#define CIAB_DSKSEL2 (5)  /* disk select unit 2* */
#define CIAB_DSKSEL1 (4)  /* disk select unit 1* */
#define CIAB_DSKSEL0 (3)  /* disk select unit 0* */
#define CIAB_DSKSIDE (2)  /* disk side select* */
#define CIAB_DSKDIREC (1) /* disk direction of seek* */
#define CIAB_DSKSTEP (0)  /* disk step heads* */

/* ciaa port A (0xbfe001) */
#define CIAF_GAMEPORT1 BIT(7)
#define CIAF_GAMEPORT0 BIT(6)
#define CIAF_DSKRDY BIT(5)
#define CIAF_DSKTRACK0 BIT(4)
#define CIAF_DSKPROT BIT(3)
#define CIAF_DSKCHANGE BIT(2)
#define CIAF_LED BIT(1)
#define CIAF_OVERLAY BIT(0)

/* ciaa port B (0xbfe101) -- parallel port */

/* ciab port A (0xbfd000) -- serial and printer control */
#define CIAF_COMDTR BIT(7)
#define CIAF_COMRTS BIT(6)
#define CIAF_COMCD BIT(5)
#define CIAF_COMCTS BIT(4)
#define CIAF_COMDSR BIT(3)
#define CIAF_PRTRSEL BIT(2)
#define CIAF_PRTRPOUT BIT(1)
#define CIAF_PRTRBUSY BIT(0)

/* ciab port B (0xbfd100) -- disk control */
#define CIAF_DSKMOTOR BIT(7)
#define CIAF_DSKSEL3 BIT(6)
#define CIAF_DSKSEL2 BIT(5)
#define CIAF_DSKSEL1 BIT(4)
#define CIAF_DSKSEL0 BIT(3)
#define CIAF_DSKSIDE BIT(2)
#define CIAF_DSKDIREC BIT(1)
#define CIAF_DSKSTEP BIT(0)

#endif /* !_CIA_REGDEF_H_ */
