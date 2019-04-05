        include 'exec/macros.i'
        include 'exec/io.i'
        include 'exec/execbase.i'
        include 'exec/memory.i'
        include 'exec/libraries.i'
        include 'dos/dosextens.i'
        include 'dos/doshunks.i'
        include 'devices/trackdisk.i'
        include 'hardware/custom.i'

; custom chips addresses
CUSTOM          EQU     $dff000
CIAA            EQU     $bfe001
CIAB            EQU     $bfd000

; include 'lvo/exec_lib.i'
_LVOSuperState          EQU     -150
_LVOAllocMem            EQU     -198
_LVOFreeMem             EQU     -210
_LVODoIO                EQU     -456
_LVOCopyMem             EQU     -624
_LVOAvailMem            EQU     -216
_LVOCacheControl        EQU     -648

 STRUCTURE SEG,0
	LONG	SEG_LEN
	APTR	SEG_NEXT
	LABEL	SEG_START
	LABEL	SEG_SIZE

	section	BB,code

; Boot block cannot generally be assumed to be placed in a specific kind of
; memory, or at any specific address, so all code must be completely PC
; relative, and all chip data must be explicitly copied to chip memory.

	dc.b    'DOS',0
	dc.l	0
ExecInfo:
        dc.w    0, 0    ; length / start (sector aligned)

        ; Boot block entry:
        ;  [a1] IOStdReq (trackdisk.device)
        ;  [a6] ExecBase
Entry:
        move.l  a1,a3                   ; [a3] trackdisk.device IORequest

        movem.w ExecInfo(pc),d2/d4
        lsl.l   #8,d2                   ; [d2] executable length in bytes
        lsl.l   #8,d4                   ; [d4] executable start in bytes

        ; allocate memory for executable file
        move.l  d2,d0
        move.l  #MEMF_CHIP,d1
        JSRLIB  AllocMem
        move.l  d0,d3                   ; [d3] block pointer (executable file)

        ; read executable file into memory
        move.l  a3,a1                   ; trackdisk.device IORequest
        movem.l d2-d4,IO_LENGTH(a1)     ; length / data / start 
        JSRLIB  DoIO

        ; turn off the motor
        clr.l   IO_LENGTH(a1)
        move.w  #TD_MOTOR,IO_COMMAND(a1)
        JSRLIB  DoIO

        ; disable caches if kickstart 2.0 or above
        cmp.w   #36,LIB_VERSION(a6)
        blt     .kick13
        moveq.l #0,d0
        moveq.l #-1,d1
        JSRLIB  CacheControl
.kick13

        ; move hunks around and relocate them
        move.l  d3,a0
        bsr     SetupHunkFile
        tst.l   d0
        beq     .exit                   ; oops... it failed
        move.l  d0,a4                   ; [a4] first hunk of executable file

        ; allocate all available memory for the kernel
        clr.l   -(sp)                           ; array terminator
        clr.l   -(sp) 

.chip   move.l  #MEMF_CHIP|MEMF_LARGEST,d1
        JSRLIB  AvailMem
        move.l  d0,-(sp)                        ; chip memory size
        move.l  #MEMF_CHIP,d1
        JSRLIB  AllocMem
        move.l  d0,-(sp)                        ; chip memory pointer

.fast   move.l  #MEMF_FAST|MEMF_LARGEST,d1
        JSRLIB  AvailMem
        tst.l   d0
        beq     .nofast
        move.l  d0,-(sp)                        ; fast memory size
        move.l  #MEMF_FAST,d1
        JSRLIB  AllocMem
        move.l  d0,-(sp)                        ; fast memory pointer
.nofast
        
        ; [2nd arg of _start] the address of memory areas array
        move.l  sp,-(sp)

        ; [1st arg of _start] cpu model flags
        clr.l   -(sp)
        move.w  AttnFlags(a6),2(sp)

        ; set up initial environment and jump to kernel
        move.w  #$7fff,intena+CUSTOM    ; disable all interrupts
        JSRLIB  SuperState              ; enter supervisor mode
        or.w    #$0700,sr               ; set highest priority level
        jsr     SEG_START(a4)           ; enter the kernel

.exit   bra     .exit

SetupHunkFile:
        ; executable amiga hunk file?
        cmp.l   #HUNK_HEADER,(a0)+
        beq     .setup
        moveq   #0,d0
        rts

.setup  movem.l d2-d4/a2-a3,-(sp)
        lea     4(a0),a2

        ; read number of hunks, assume there's no resident library name
        move.l  (a2)+,d2
        lsl.l   #2,d2           ; [d2] hunk array size
        sub.l   d2,sp           ; [sp] hunk array

        ; move to hunk information
        addq.l  #8,a2

        ; allocate hunks
        move.l  sp,a3
        move.l  d2,d4
.alloc  move.l  (a2)+,d3
        lsl.l   #2,d3           ; [d3] hunk size
        addq.l  #SEG_SIZE,d3
        move.l  d3,d0
        move.l  #MEMF_PUBLIC|MEMF_CLEAR,d1
        JSRLIB  AllocMem
        move.l  d0,(a3)+
        move.l  d0,a0
        move.l  d3,SEG_LEN(a0)
        subq.l  #4,d4
        bgt     .alloc

        ; link hunks
        move.l  sp,a1
        move.l  d2,d4
        subq.l  #4,d4
.link   move.l  (a1)+,a0        ; previous hunk
        move.l  (a1),SEG_NEXT(a0)
        subq.l  #4,d4
        bgt     .link

        ; parse hunks
        move.l  sp,a3
        move.l  d2,d4
.parse  move.w  (a2)+,d0        ; should always read zero
        add.w   (a2)+,d0        ; hunk type
        cmp.w   #HUNK_CODE,d0
        beq     .hdata
        cmp.w   #HUNK_DATA,d0
        beq     .hdata
        cmp.w   #HUNK_BSS,d0
        beq     .hbss
        cmp.w   #HUNK_RELOC32,d0
        beq     .hreloc
        cmp.w   #HUNK_SYMBOL,d0
        beq     .hsyms
        cmp.w   #HUNK_END,d0
        beq     .hend
        moveq   #0,d0           ; report error
        bra     .error

.hdata  move.l  (a3),a1
        lea.l   SEG_START(a1),a1
        move.l  (a2)+,d0
        lsl.l   #2,d0
        move.l  a2,a0
        add.l   d0,a2           ; move pointer to next hunk
        JSRLIB  CopyMem
        bra     .parse

.hbss   addq.l  #4,a2           ; skip bss length
        bra     .parse

.hsyms  move.l  (a2)+,d0
        beq     .parse
        addq.l  #1,d0
        lsl.l   #2,d0
        add.l   d0,a2           ; move pointer to symbol
        bra     .hsyms

.hreloc move.l  (a2)+,d3        ; number of relocations
        beq     .parse
        move.l  (a2)+,d0        ; referenced hunk number
        lsl.l   #2,d0
        move.l  (sp,d0.l),d0
        addq.l  #SEG_START,d0    ; [d0] referenced hunk data address
        move.l  (a3),a1
.reloc  move.l  (a2)+,d1
        add.l   d0,SEG_START(a1,d1.l)
        subq.l  #1,d3
        bgt     .reloc
        bra     .hreloc

.hend   addq.l  #4,a3           ; go to next hunk
        subq.l  #4,d4
        bne     .parse

.quit   move.l  (sp),d0

.error  add.l   d2,sp           ; deallocate hunk array

        movem.l (sp)+,d2-d4/a2-a3
        rts

; vim: ft=asm68k:ts=8:sw=8
