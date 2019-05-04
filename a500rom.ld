/* Linker scripts are documented at:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_ARCH(m68k)
ENTRY(_entry)
PHDRS
{
  init PT_LOAD FLAGS(5); /* read-only, executable */
  rom  PT_LOAD FLAGS(5); /* read-only, executable */
}
MEMORY
{
  chip (rw) : ORIGIN = 0x000000, LENGTH = 512K
  slow (rw) : ORIGIN = 0xc00000, LENGTH = 512K
  kick (rx) : ORIGIN = 0xfc0000, LENGTH = 256K
}
PROVIDE(ciaa = 0xbfe001);
PROVIDE(ciab = 0xbfd000);
PROVIDE(custom = 0xdff000);
PROVIDE(_stack_size = 0x100);
/* Address range that will be copied from RAM to ROM. */
PROVIDE(_rom = LOADADDR(.text));
PROVIDE(_erom = LOADADDR(.vecs));
/* Necessary to construct boot data for FreeRTOS. */
PROVIDE(_echip = ORIGIN(chip) + LENGTH(chip));
PROVIDE(_eslow = ORIGIN(slow) + LENGTH(slow));
SECTIONS
{
  /* ROM header & initialization routine. */
  .init ORIGIN(kick):
  {
    LONG(_estack);      /* Initial SP */
    LONG(_entry);       /* Initial PC */

    *(.init .init.*)
    . = ALIGN(4);
  } :init

  /* Allocate stack at the beginning of slow memory. */
  .stack ORIGIN(slow): 
  {
    PROVIDE(_stack = .);
    . += _stack_size;
    PROVIDE(_estack = .);
  } :NONE

  /* Put text & data sections into ROM. 
   * They'll be copied to RAM by init routine. */
  .text . : AT(LOADADDR(.init) + SIZEOF(.init))
  {
    PROVIDE(_text = .);
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
    PROVIDE(_etext = .);
  } :rom

  .data . :
  {
    PROVIDE(_data = .);
    *(.data .data.*)
    . = ALIGN(4);
    PROVIDE(_edata = .);
  } :rom

  /* ROM must end with vector of 7 words for 68000 / 68010 processors.
   * Explanation here: http://eab.abime.net/showthread.php?t=87087 */
  .vecs . :
  {
    . += LENGTH(kick) - (SIZEOF(.init) + SIZEOF(.text) + SIZEOF(.data)) - 16;
    SHORT(0x4718); SHORT(0x4819); SHORT(0x4f1a); SHORT(0x531b);
    SHORT(0x541c); SHORT(0x4f1d); SHORT(0x571e); SHORT(0x4e1f);
  } :rom

  /* BSS is placed just after DATA in RAM. */
  .bss _edata : AT(_edata)
  {
    PROVIDE(_bss = .);
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    PROVIDE(_ebss = .);
  } :NONE

  /* Sections to be discarded */
  /DISCARD/ :
  {
    *(.comment)
  }
}
