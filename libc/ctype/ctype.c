#include <ctype.h>

int isspace(int ch) {
  return (ch == ' ' || (ch >= '\t' && ch <= '\r'));
}

int isascii(int ch) {
  return ((ch & ~0x7f) == 0);
}

int isupper(int ch) {
  return (ch >= 'A' && ch <= 'Z');
}

int islower(int ch) {
  return (ch >= 'a' && ch <= 'z');
}

int isalpha(int ch) {
  return (isupper(ch) || islower(ch));
}

int isalnum(int ch) {
  return (isalpha(ch) || isdigit(ch));
}

int isdigit(int ch) {
  return (ch >= '0' && ch <= '9');
}

int isxdigit(int ch) {
  return (isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'));
}

int iscntrl(int ch) {
  return ((ch >= 0x00 && ch <= 0x1F) || ch == 0x7F);
}

int isgraph(int ch) {
  return (ch != ' ' && isprint(ch));
}

int isprint(int ch) {
  return (ch >= 0x20 && ch <= 0x7E);
}

int ispunct(int ch) {
  return (isprint(ch) && ch != ' ' && !isalnum(ch));
}

int toupper(int ch) {
  if (islower(ch))
    return (ch - 0x20);
  return (ch);
}

int tolower(int ch) {
  if (isupper(ch))
    return (ch + 0x20);
  return (ch);
}
