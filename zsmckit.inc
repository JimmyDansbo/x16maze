.import popa
.import pusha
.import popax
.importzp sreg
.include "zsmkit.inc"

.export _zsm_init_engine
.export _zsm_tick
.export _zsm_play
.export _zsm_stop
.export _zsm_rewind
.export _zsm_close
.export _zsm_fill_buffers
.export _zsm_setlfs
.export _zsm_setfile
.export _zsm_loadpcm
.export _zsm_setmem
.export _zsm_setatten
.export _zsm_setcb
.export _zsm_clearcb
.export _zsm_getstate
.export _zsm_setrate
.export _zsm_getrate
.export _zsm_setloop
.export _zsm_opmatten
.export _zsm_psgatten
.export _zsm_pcmatten
.export _zsm_set_int_rate

.export _zcm_setmem
.export _zcm_play
.export _zcm_stop
.export _zsmkit_setisr
.export _zsmkit_clearisr

RAM_BANK = $00

.segment "CODE"

_zsm_init_engine=zsm_init_engine
_zsm_tick=zsm_tick
_zsm_play:
	tax
	jmp	zsm_play
_zsm_stop:
	tax
	jmp	zsm_stop
_zsm_rewind:
	tax
	jmp	zsm_rewind
_zsm_close:
	tax
	jmp	zsm_close
_zsm_fill_buffers=zsm_fill_buffers
_zsm_setlfs:
	tay			; device
	jsr	popa
	tax			; priority
	jsr	popa		; lfn/sa
	jmp	zsm_setlfs
_zsm_setfile:
	pha			; low byte of filename
	txa
	tay			; high byte of filename
	jsr	popa
	tax			; priority
	pla			; low byte of filename	
	jmp	zsm_setfile
_zsm_loadpcm:
	sta	RAM_BANK	; set RAM bank
	jsr	popa
	pha			; low address
	jsr	popa
	pha			; high addres
	jsr	popa		; priority
	tax
	ply
	pla
	jsr	zsm_loadpcm
	phy
	plx
	rts
_zsm_setmem:
	sta	RAM_BANK	; Set RAM Bank
	jsr	popa		; low address
	pha
	jsr	popa		; high address
	pha
	jsr	popa		; priority
	tax
	ply
	pla
	jmp	zsm_setmem
_zsm_setatten:
	pha
	jsr	popa
	tax
	pla
	jmp	zsm_setatten
_zsm_setcb:
	sta	RAM_BANK
	jsr	popax		; Address of callback function
	pha			; low addr
	jsr	popa		; Priority
	cmp	#0
	bne	@is1
	pla
	sta	@cb0j+1		; low addr
	stx	@cb0j+2		; high addr
	ldx	#0		; priority
	lda	#<@cb0		; Set own callback function that will call
	ldy	#>@cb0		; users callback function
	jmp	zsm_setcb
@is1:	cmp	#1
	bne	@is2
	pla
	sta	@cb1j+1		; low addr
	stx	@cb1j+2		; high addr
	ldx	#1		; priority
	lda	#<@cb1		; Set own callback function that will call
	ldy	#>@cb1		; users callback function
	jmp	zsm_setcb
@is2:	cmp	#2
	bne	@is3
	pla
	sta	@cb2j+1		; low addr
	stx	@cb2j+2		; high addr
	ldx	#2		; priority
	lda	#<@cb2		; Set own callback function that will call
	ldy	#>@cb2		; users callback function
	jmp	zsm_setcb
@is3:
	pla
	sta	@cb3j+1		; low addr
	stx	@cb3j+2		; high addr
	ldx	#3		; priority
	lda	#<@cb3		; Set own callback function that will call
	ldy	#>@cb3		; users callback function
	jmp	zsm_setcb
; ****** Callback functions that will call users callback function *******
@cb0:	pha			; Save .A as it must contain value for function
	tya			; Push .Y to soft-stack
	jsr	pusha
	txa			; Push .X to soft-stack
	jsr	pusha
	pla			; Restore .A for function call
@cb0j:	jmp	$FFFF
@cb1:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb1j:	jmp	$FFFF
@cb2:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb2j:	jmp	$FFFF
@cb3:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb3j:	jmp	$FFFF
_zsm_clearcb:
	tax
	jmp	zsm_clearcb
_zsm_getstate:
	tax
	jsr	zsm_getstate
	sta	sreg+0
	sty	sreg+1
	php
	pla
	pha
	lsr
	and	#$01
	tax
	pla
	and	#$01
	rts
_zsm_setrate:
	pha
	txa
	tay
	jsr	popa
	tax
	pla
	jmp	zsm_setrate
_zsm_getrate:
	tax
	jsr	zsm_getrate
	phy
	plx
	rts
_zsm_setloop:
	pha
	jsr	popa
	tax
	pla
	lsr
	jmp	zsm_setloop
_zsm_opmatten:
	pha
	jsr	popa
	tay
	jsr	popa
	tax
	pla
	jmp	zsm_opmatten
_zsm_psgatten:
	pha
	jsr	popa
	tay
	jsr	popa
	tax
	pla
	jmp	zsm_psgatten
_zsm_pcmatten:
	pha
	jsr	popa
	tay
	jsr	popa
	tax
	pla
	jmp	zsm_pcmatten
_zsm_set_int_rate:
	tay
	jsr	popa
	jmp	zsm_set_int_rate
_zcm_setmem:
	sta	RAM_BANK	; Set RAM Bank
	jsr	popa		; low address
	pha
	jsr	popa		; high address
	pha
	jsr	popa		; priority
	tax
	ply
	pla
	jmp	zcm_setmem
_zcm_play:
	pha
	jsr	popa
	tax
	pla
	jmp	zcm_play
_zcm_stop=zcm_stop
_zsmkit_setisr=zsmkit_setisr
_zsmkit_clearisr=zsmkit_clearisr
