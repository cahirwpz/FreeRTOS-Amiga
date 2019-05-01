/* Linker scripts are documented at:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_ARCH(m68k)
ENTRY(_start)
PHDRS
{
  text   PT_LOAD FLAGS(5);      /* read-only, executable */
  data   PT_LOAD FLAGS(6);      /* read-write */
  bss    PT_LOAD FLAGS(6);      /* read-write */
}
SECTIONS
{
  .text 0: ALIGN(4)
  {
    *(.text .text.*)
    . = ALIGN(4);
  } : text

  .data : ALIGN(4)
  {
    *(.data .data.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } : data

  .bss : ALIGN(4)
  {
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
  } : bss

  /* Sections to be discarded */
  /DISCARD/ :
  {
    *(.comment)
  }
}
