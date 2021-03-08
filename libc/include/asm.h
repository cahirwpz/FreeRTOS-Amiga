#ifndef _ASM_H_
#define _ASM_H_

#define __IMMEDIATE #

#define ENTRY(name)                     \
  .text;                                \
  .even;                                \
  .globl name;                          \
  .type name, @function;                \
  name:

#define END(name)                       \
  .size name, . - name

#define STRONG_ALIAS(alias, sym)        \
  .globl alias;                         \
  alias = sym

#endif /* _ASM_H_ */
