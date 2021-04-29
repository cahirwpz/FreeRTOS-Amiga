#pragma once

/*
 * Window/terminal size structure.  This information is stored by the kernel
 * in order to provide a consistent interface, but is not used by the kernel.
 */
struct winsize {
  unsigned short ws_row;    /* rows, in characters */
  unsigned short ws_col;    /* columns, in characters */
  unsigned short ws_xpixel; /* horizontal size, pixels */
  unsigned short ws_ypixel; /* vertical size, pixels */
};

#define TIOCGETA _IOR('t', 19, struct termios)    /* get termios struct */
#define TIOCSETA _IOW('t', 20, struct termios)    /* set termios struct */
#define TIOCSETAW _IOW('t', 21, struct termios)   /* drain output, set */
#define TIOCSETAF _IOW('t', 22, struct termios)   /* drn out, fls in, set */
#define TIOCGWINSZ _IOR('t', 104, struct winsize) /* get window size */
#define TIOCSWINSZ _IOW('t', 103, struct winsize) /* set window size */
