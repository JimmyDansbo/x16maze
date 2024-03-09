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

;zsm_init_engine(uint8_t bank);
_zsm_init_engine=zsm_init_engine
;zsm_tick(uint8_t what);
_zsm_tick=zsm_tick
;zsm_play(uint8_t priority);
_zsm_play:
	tax			; Priority is in .A, move it to .X
	jmp	zsm_play
;zsm_stop(uint8_t priority);
_zsm_stop:
	tax			; Priority is in .A, move it to .X
	jmp	zsm_stop
;zsm_rewind(uint8_t priority);
_zsm_rewind:
	tax			; Priority is in .A, move it to .X
	jmp	zsm_rewind
;zsm_close(uint8_t priority);
_zsm_close:
	tax			; Priority is in .A, move it to .X
	jmp	zsm_close
;zsm_fill_buffers();
_zsm_fill_buffers=zsm_fill_buffers
;zsm_setlfs(uint8_t lfnsa, uint8_t priority, uint8_t device);
_zsm_setlfs:
	tay			; device - in .Y
	jsr	popa
	tax			; priority - in .X
	jsr	popa		; lfn/sa - in .A
	jmp	zsm_setlfs
;zsm_setfile(uint8_t priority, char *filename);
_zsm_setfile:
	pha			; low byte of filename address
	txa
	tay			; high byte of filename address - in .Y
	jsr	popa
	tax			; priority - in .X
	pla			; low byte of filename - in .A
	jmp	zsm_setfile
;uint16_t zsm_loadpcm(uint8_t priority, uint16_t addr, uint8_t bank);
_zsm_loadpcm:
	sta	RAM_BANK	; set RAM bank
	jsr	popa
	pha			; low address
	jsr	popa
	pha			; high addres
	jsr	popa
	tax			; priority - in .X
	ply			; high address - in .Y
	pla			; low address - in .A
	jsr	zsm_loadpcm
				; low address - in .A
	phy			; high address
	plx			; high address - in .X
	; .X .A = lo hi address
	rts
;zsm_setmem(uint8_t priority, uint16_t addr, uint8_t bank);
_zsm_setmem:
	sta	RAM_BANK	; Set RAM Bank
	jsr	popa		; low address
	pha
	jsr	popa		; high address
	pha
	jsr	popa		; priority
	tax			; priority - in .X
	ply			; high address - in .Y
	pla			; low address - in .A
	jmp	zsm_setmem
;zsm_setatten(uint8_t priority, uint8_t attenuation);
_zsm_setatten:
	pha			; attenuation
	jsr	popa		; priority
	tax			; priority - in .X
	pla			; attenuation - in .A
	jmp	zsm_setatten
;zsm_setcb(uint8_t priority, zsm_callback, uint8_t bank);
_zsm_setcb:
	sta	RAM_BANK	; Set RAM Bank
	jsr	popax		; Address of callback function
	pha			; low addr
	jsr	popa		; Priority
	cmp	#0
	bne	@is1
	pla			; Here priority = 0
	; Write addr of users CB function to internal CB function
	sta	@cb0j+1		; low addr
	stx	@cb0j+2		; high addr
	ldx	#0		; priority - in .X
	lda	#<@cb0		; Set own callback function that will call
	ldy	#>@cb0		; users callback function
	jmp	zsm_setcb
@is1:	cmp	#1
	bne	@is2
	pla			; Here priority = 1
	; Write addr of users CB function to internal CB function
	sta	@cb1j+1		; low addr
	stx	@cb1j+2		; high addr
	ldx	#1		; priority - in .X
	lda	#<@cb1		; Set own callback function that will call
	ldy	#>@cb1		; users callback function
	jmp	zsm_setcb
@is2:	cmp	#2
	bne	@is3
	pla			; Here priority = 2
	; Write addr of users CB function to internal CB function
	sta	@cb2j+1		; low addr
	stx	@cb2j+2		; high addr
	ldx	#2		; priority
	lda	#<@cb2		; Set own callback function that will call
	ldy	#>@cb2		; users callback function
	jmp	zsm_setcb
@is3:
	pla			; Here priority = 3
	; Write addr of users CB function to internal CB function
	sta	@cb3j+1		; low addr
	stx	@cb3j+2		; high addr
	ldx	#3		; priority
	lda	#<@cb3		; Set own callback function that will call
	ldy	#>@cb3		; users callback function
	jmp	zsm_setcb
; ****** Callback functions that will call users callback function *******
; ****** This is done to get the return values in the correct order ******
; ****** for the C function                                        *******
;void callbackfunction(uint8_t eventtype, uint8_t priority, uint8_t paramval)
@cb0:	pha			; Save .A as it must contain value for function
	tya			; Push .Y to soft-stack
	jsr	pusha
	txa			; Push .X to soft-stack
	jsr	pusha
	pla			; Restore .A for function call
@cb0j:	jmp	$FFFF
;void callbackfunction(uint8_t eventtype, uint8_t priority, uint8_t paramval)
@cb1:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb1j:	jmp	$FFFF
;void callbackfunction(uint8_t eventtype, uint8_t priority, uint8_t paramval)
@cb2:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb2j:	jmp	$FFFF
;void callbackfunction(uint8_t eventtype, uint8_t priority, uint8_t paramval)
@cb3:	pha
	tya
	jsr	pusha
	txa
	jsr	pusha
	pla
@cb3j:	jmp	$FFFF
;zsm_clearcb(uint8_t priority);
_zsm_clearcb:
	tax			; Priority is in .A, move it to .X
	jmp	zsm_clearcb
;struct zsm_state zsm_getstate(uint8_t priority);
_zsm_getstate:
	tax			; Priority is in .A, move it to .X
	jsr	zsm_getstate
	; ordering return values to the zsm_state structure
	;typedef struct zsm_state {
	;	uint8_t playing;
	;	uint8_t not_playable;
	;	uint16_t loopcnt;
	;};
	sta	sreg+0		; Address returned in SREG
	sty	sreg+1
	php			; Save flags, we want to use .Z and .C
	pla
	pha
	lsr			; Get value of .Z into .A
	and	#$01
	tax			; .X = value of .Z
	pla			; Get value of .C into .A
	and	#$01		; .A = value of .C
	rts
;zsm_setrate(uint8_t priority, uint16_t tickrate);
_zsm_setrate:
	pha			; Save low part of tickrate
	txa			; Move .X to .Y, using .A
	tay			; high part of tickrate - in .Y
	jsr	popa		; Get priority
	tax			; priority - in .X
	pla			; low part of tickrate - in .A
	jmp	zsm_setrate
;uint16_t zsm_getrate(uint8_t priority);
_zsm_getrate:
	tax			; Priority is in .A, move it to .X
	jsr	zsm_getrate	; low part of rate - in .A
	phy			; Move high part of rate from .Y to .X through .A
	plx			; high part of rate - in .X
	rts
;zsm_setloop(uint8_t priority, uint8_t loop);
_zsm_setloop:
	pha			; Save "do loop" value
	jsr	popa		; Get priority
	tax			; pririty - in .X
	pla			; Restore "do loop" value
	lsr			; "do loop" value - in .C
	jmp	zsm_setloop
;zsm_opmatten(uint8_t priority, uint8_t channel, uint8_t val);
_zsm_opmatten:
	pha			; Save attenuation value
	jsr	popa		; Get channel
	tay			; channel - in .Y
	jsr	popa		; Get priority
	tax			; priority - in .X
	pla			; attenuation value - in .A
	jmp	zsm_opmatten
;zsm_psgatten(uint8_t priority, uint8_t channel, uint8_t val);
_zsm_psgatten:
	pha			; Save attenuation value
	jsr	popa		; Get channel
	tay			; channel - in .Y
	jsr	popa		; Get priority
	tax			; priority - in .X
	pla			; attenuation value - in .A
	jmp	zsm_psgatten
;zsm_pcmatten(uint8_t priority, uint8_t channel, uint8_t val);
_zsm_pcmatten:
	pha			; Save attenuation value
	jsr	popa		; Get channel
	tay			; channel - in .Y
	jsr	popa		; Get priority
	tax			; priority - in .X
	pla			; attenuation value - in .A
	jmp	zsm_pcmatten
;zsm_set_int_rate(uint8_t herz, uint8_t in256th);
_zsm_set_int_rate:
	tay			; fraction - in .Y
	jsr	popa		; frequency - in .A
	jmp	zsm_set_int_rate
;zcm_setmem(uint8_t priority, uint16_t addr, uint8_t bank);
_zcm_setmem:
	sta	RAM_BANK	; Set RAM Bank
	jsr	popa		; low address
	pha
	jsr	popa		; high address
	pha
	jsr	popa		; priority
	tax			; priority - in .X
	ply			; high address - in .Y
	pla			; low address - in .A
	jmp	zcm_setmem
;zcm_play(uint8_t slot, uint8_t volume);
_zcm_play:
	pha			; Save volume
	jsr	popa		; Get slot
	tax			; slot - in .X
	pla			; volume - in .A
	jmp	zcm_play
;zcm_stop();	
_zcm_stop=zcm_stop
;zsmkit_setisr();
_zsmkit_setisr=zsmkit_setisr
;zsmkit_clearisr();
_zsmkit_clearisr=zsmkit_clearisr
