        xdef _ulPortSetInterruptMaskFromISR
        xdef _vPortClearInterruptMaskFromISR
        xdef _vPortYieldHandler
        xdef _vPortStartFirstTask
        xref _pxCurrentTCB

        section "portasm.S",text

        macro portSAVE_CONTEXT
	movem.l	d0-a6,-(sp)
	move.l	_pxCurrentTCB,a0
	move.l	sp,(a0)
	endm

        macro portRESTORE_CONTEXT
	move.l	_pxCurrentTCB,a0
	move.l	(a0),sp
	movem.l	(sp)+,d0-a6
        rte
	endm

_ulPortSetInterruptMaskFromISR:
        move.w  sr,d0
        or.w    #$0700,sr
        rts

_vPortClearInterruptMaskFromISR:
        move.w  d0,sr
        rts

_vPortYieldHandler:
        portSAVE_CONTEXT
        jsr     _vTaskSwitchContext
        portRESTORE_CONTEXT

_vPortStartFirstTask:
        portRESTORE_CONTEXT

; vim: ft=asm68k ts=8 sw=8
