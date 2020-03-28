/* Linker scripts are documented at:
 * https://sourceware.org/binutils/docs/ld/Scripts.html */
OUTPUT_ARCH(m68k)
ENTRY(_start)
PHDRS
{
  text      PT_LOAD FLAGS(5);   /* read-only, executable */
  data      PT_LOAD FLAGS(6);   /* read-write */
  data_chip PT_LOAD FLAGS(6);   /* read-write */
  bss       PT_LOAD FLAGS(6);   /* read-write */
  bss_chip  PT_LOAD FLAGS(6);   /* read-write */
}
PROVIDE(ciaa = 0xbfe001);
PROVIDE(ciab = 0xbfd000);
PROVIDE(msm6242b = 0xdc0000);
PROVIDE(custom = 0xdff000);
SECTIONS
{
  .text 0: ALIGN(4) SUBALIGN(2)
  {
    PROVIDE(_text = .);
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
    PROVIDE(_etext = .);
  } :text

  .data : ALIGN(4)
  {
    PROVIDE(_data = .);
    *(.data .data.*)
    . = ALIGN(4);
    PROVIDE(_edata = .);
  } :data

  .bss : ALIGN(4)
  {
    PROVIDE(_bss = .);
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    PROVIDE(_ebss = .);
  } :bss
  
  .data.MEMF_CHIP : ALIGN(4)
  {
    PROVIDE(_datachip = .);
    *(.datachip .datachip.*)
    . = ALIGN(4);
    PROVIDE(_edatachip = .);
  } :data_chip

  .bss.MEMF_CHIP (NOLOAD) : ALIGN(4)
  {
    PROVIDE(_bsschip = .);
    *(.bsschip .bsschip.*)
    . = ALIGN(4);
    PROVIDE(_ebsschip = .);
  } :bss_chip

  /* Sections to be discarded */
  /DISCARD/ :
  {
    *(.comment)
  }
}
