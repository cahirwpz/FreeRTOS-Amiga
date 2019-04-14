        xdef ulPortSetIPL
        xdef vPortYieldHandler
        xdef vPortStartFirstTask
        xref pxCurrentTCB

        section '.text',code

        macro portSAVE_CONTEXT
        or.w    #$0700,sr               ; mask all interrupts
        movem.l d0-a6,-(sp)             ; save D0-A6 registers
        move.l  pxCurrentTCB,a0         ; load current task pointer
        move.l  sp,(a0)                 ; save stack pointer
	endm

        macro portRESTORE_CONTEXT
        move.l  pxCurrentTCB,a0         ; load current task pointer
        move.l  (a0),sp                 ; restore stack pointer
        movem.l (sp)+,d0-a6             ; restore D0-A6 registers
        rte                             ; restore SR and PC
	endm

ulPortSetIPL:
        move.l  4(sp),d0
        move.w  sr,-(sp)
        move.w  sr,d1
        and.w   #$f8ff,d1               ; old SR with IPL bits cleared
        and.w   #$0700,d0               ; argument with IPL bits left
        or.w    d0,d1
        move.w  d1,sr                   ; set new value of IPL in SR
        move.w  (sp)+,d0
        and.w   #$0700,d0               ; return IPL bits of original SR
        rts

vPortYieldHandler:
        portSAVE_CONTEXT
        jsr     vTaskSwitchContext
        portRESTORE_CONTEXT

vPortStartFirstTask:
        portRESTORE_CONTEXT

; vim: ft=asm68k ts=8 sw=8
