/*	$NetBSD: subr_prf.c,v 1.28 2019/02/03 11:59:43 mrg Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)printf.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Scaled down version of printf(3).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef long INTMAX_T;
typedef unsigned long UINTMAX_T;
typedef intptr_t PTRDIFF_T;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef long ssize_t;

#define NBBY 8

static const char hexdigits[16] = "0123456789abcdef";

static void kprintn(putchar_t, UINTMAX_T, int, int, int);

#define LONG 0x01
#define ALT 0x04
#define SPACE 0x08
#define LADJUST 0x10
#define SIGN 0x20
#define ZEROPAD 0x40
#define NEGATIVE 0x80
#define KPRINTN(base) kprintn(put, ul, base, lflag, width)
#define RADJUSTZEROPAD()                                                       \
  {                                                                            \
    if ((lflag & (ZEROPAD | LADJUST)) == ZEROPAD) {                            \
      while (width-- > 0)                                                      \
        put('0');                                                              \
    }                                                                          \
  }
#define LADJUSTPAD()                                                           \
  {                                                                            \
    if (lflag & LADJUST) {                                                     \
      while (width-- > 0)                                                      \
        put(' ');                                                              \
    }                                                                          \
  }
#define RADJUSTPAD()                                                           \
  {                                                                            \
    if ((lflag & (ZEROPAD | LADJUST)) == 0) {                                  \
      while (width-- > 0)                                                      \
        put(' ');                                                              \
    }                                                                          \
  }

#define KPRINT(base)                                                           \
  {                                                                            \
    ul = (lflag & LONG) ? va_arg(ap, u_long) : va_arg(ap, u_int);              \
    KPRINTN(base);                                                             \
  }

void kvprintf(putchar_t put, const char *fmt, va_list ap) {
  char *p;
  int ch;
  UINTMAX_T ul;
  int lflag;
  int width;
  char *q;

  for (;;) {
    while ((ch = *fmt++) != '%') {
      if (ch == '\0')
        return;
      put(ch);
    }
    lflag = 0;
    width = 0;
  reswitch:
    switch (ch = *fmt++) {
      case '#':
        lflag |= ALT;
        goto reswitch;
      case ' ':
        lflag |= SPACE;
        goto reswitch;
      case '-':
        lflag |= LADJUST;
        goto reswitch;
      case '+':
        lflag |= SIGN;
        goto reswitch;
      case '0':
        lflag |= ZEROPAD;
        goto reswitch;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        for (;;) {
          width *= 10;
          width += ch - '0';
          ch = *fmt;
          if ((unsigned)ch - '0' > 9)
            break;
          ++fmt;
        }
      case 'l':
        lflag |= LONG;
        goto reswitch;
      case 'j':
        if (sizeof(intmax_t) == sizeof(long))
          lflag |= LONG;
        goto reswitch;
      case 't':
        if (sizeof(PTRDIFF_T) == sizeof(long))
          lflag |= LONG;
        goto reswitch;
      case 'z':
        if (sizeof(ssize_t) == sizeof(long))
          lflag |= LONG;
        goto reswitch;
      case 'c':
        ch = va_arg(ap, int);
        --width;
        RADJUSTPAD();
        put(ch & 0xFF);
        LADJUSTPAD();
        break;
      case 's':
        p = va_arg(ap, char *);
        for (q = p; *q != '\0'; ++q)
          continue;
        width -= q - p;
        RADJUSTPAD();
        while ((ch = (unsigned char)*p++))
          put(ch);
        LADJUSTPAD();
        break;
      case 'd':
        ul = (lflag & LONG) ? va_arg(ap, long) : va_arg(ap, int);
        if ((INTMAX_T)ul < 0) {
          ul = -(INTMAX_T)ul;
          lflag |= NEGATIVE;
        }
        KPRINTN(10);
        break;
      case 'o':
        KPRINT(8);
        break;
      case 'u':
        KPRINT(10);
        break;
      case 'p':
        lflag |= (LONG | ALT);
        /* FALLTHROUGH */
      case 'x':
        KPRINT(16);
        break;
      default:
        if (ch == '\0')
          return;
        put(ch);
        break;
    }
  }
}

static void kprintn(putchar_t put, UINTMAX_T ul, int base, int lflag,
                    int width) {
  /* hold a INTMAX_T in base 8 */
  char *p, buf[(sizeof(INTMAX_T) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];
  char *q;

  p = buf;
  do {
    *p++ = hexdigits[ul % base];
  } while (ul /= base);
  q = p;
  if (lflag & ALT && *(p - 1) != '0') {
    if (base == 8) {
      *p++ = '0';
    } else if (base == 16) {
      *p++ = 'x';
      *p++ = '0';
    }
  }
  if (lflag & NEGATIVE)
    *p++ = '-';
  else if (lflag & SIGN)
    *p++ = '+';
  else if (lflag & SPACE)
    *p++ = ' ';
  width -= p - buf;
  if (lflag & ZEROPAD) {
    while (p > q)
      put(*--p);
  }
  RADJUSTPAD();
  RADJUSTZEROPAD();
  do {
    put(*--p);
  } while (p > buf);
  LADJUSTPAD();
}
