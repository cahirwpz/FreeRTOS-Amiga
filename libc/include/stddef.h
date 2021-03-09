#pragma once

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef unsigned long size_t;
typedef __WCHAR_TYPE__ wchar_t;

/* A null pointer constant. */
#define NULL ((void *)0)

/* Offset of member MEMBER in a struct of type TYPE. */
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)
