#pragma once

#include <sys/ttycom.h>

/*
 * Special Control Characters
 *
 * Index into c_cc[] character array.
 *
 *	Name	     Subscript	Enabled by
 */
#define VEOF 0      /* ICANON */
#define VEOL 1      /* ICANON */
#define VEOL2 2     /* ICANON */
#define VERASE 3    /* ICANON */
#define VWERASE 4   /* ICANON */
#define VKILL 5     /* ICANON */
#define VREPRINT 6  /* ICANON */
#define VINTR 8     /* ISIG */
#define VQUIT 9     /* ISIG */
#define VSUSP 10    /* ISIG */
#define VDSUSP 11   /* ISIG */
#define VSTART 12   /* IXON, IXOFF */
#define VSTOP 13    /* IXON, IXOFF */
#define VLNEXT 14   /* IEXTEN */
#define VDISCARD 15 /* IEXTEN */
#define VMIN 16     /* !ICANON */
#define VTIME 17    /* !ICANON */
#define VSTATUS 18  /* ICANON */
#define NCCS 20

typedef unsigned int tcflag_t;
typedef unsigned char cc_t;

struct termios {
  tcflag_t c_iflag; /* input flags */
  tcflag_t c_oflag; /* output flags */
  tcflag_t c_cflag; /* control flags */
  tcflag_t c_lflag; /* local flags */
  cc_t c_cc[NCCS];  /* control chars */
};

/* Commands passed to tcsetattr() for setting the termios structure. */
#define TCSANOW 0   /* make change immediate */
#define TCSADRAIN 1 /* drain output, then change */
#define TCSAFLUSH 2 /* drain output, flush input */

int tcgetattr(int, struct termios *);
int tcsetattr(int, int, const struct termios *);

void cfmakeraw(struct termios *);
