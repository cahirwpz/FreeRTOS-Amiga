#ifndef _STDDEF_H_
#define _STDDEF_H_

typedef int ptrdiff_t;
typedef unsigned int size_t;

typedef int wchar_t;

#define offsetof(type, member) ((size_t)(&((type *)0)->member))

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* !_STDDEF_H_ */
