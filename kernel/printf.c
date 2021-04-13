#include <stdarg.h>
#include <stdio.h>
#include <devfile.h>
#include <file.h>

#define BUFSIZ 80

typedef struct FileBuf {
  File_t *file;
  short cur;
  char buf[BUFSIZ];
} FileBuf_t;

static void FBPutChar(FileBuf_t *fb, char c) {
  if (fb->cur < BUFSIZ) {
    fb->buf[fb->cur++] = c;
  } else {
    long r;
    FileWrite(fb->file, fb->buf, BUFSIZ, &r);
    fb->cur = 0;
  }
}

static void FBFlush(FileBuf_t *fb) {
  if (fb->cur > 0) {
    long r;
    FileWrite(fb->file, fb->buf, fb->cur, &r);
  }
}

void FilePrintf(File_t *f, const char *fmt, ...) {
  FileBuf_t buf = {f, 0, ""};
  va_list ap;

  va_start(ap, fmt);
  kvprintf((putchar_t)FBPutChar, &buf, fmt, ap);
  va_end(ap);

  FBFlush(&buf);
}
