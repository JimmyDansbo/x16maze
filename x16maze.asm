.import popa			; For retrieveing parameters from C
.import _myTimer
.import zsm_tick

.export _screen_set
.export _ReadJoypad
.export _waitVsync
.export _start_timer
.export _PrintChar
.export _Getfgcol
.export _Getbgcol
.export _Getcol
.export _Setfgcol
.export _Setbgcol
.export _Setcol
.export _load_zsm
.export _breakpoint
.export _dbg8
.export _dbg16
.export _petprintch
.export _vload

RAM_BANK = $00

.segment "CODE"
; Global variables 
old_handler:	.word $0000	; Address to original interrupt handler

; *****************************************************************************
; Use kernal to print character
; *****************************************************************************
_petprintch:
	jmp	$FFD2

; *****************************************************************************
; Introduce a breakpoint into the emulators debugger
; *****************************************************************************
_breakpoint:
_dbg8:
_dbg16:
	.byte $db
	rts


; *****************************************************************************
; Increment 16 bit jiffie counter and ensure music is playing
; *****************************************************************************
myIntHandler:
;	jsr	playmusic_IRQ
	lda	#0
	jsr	zsm_tick
	inc	_myTimer
	bne	:+
	inc	_myTimer+1
	bne	:+
	inc	_myTimer+2
	bne	:+
	inc	_myTimer+3
:	jmp	(old_handler)

; *****************************************************************************
; Load a binary file into VRAM at specified bank and address
; *****************************************************************************
_vload:
	inc			; Increment VERA bank number 2 times to make it
	inc			; fit with the LOAD call (2=0xxxx, 3=1xxxx)
	pha			; Save VERA bank on stack

	jsr	popa		; Get address and store it on stack
	pha
	jsr	popa
	pha

	lda	#1		; File number, must be unique
	ldx	#8		; Device 8, local filesystem or SD card
	ldy	#2		; Secondary command 2 = headerless load
	jsr	$FFBA		; SETLFS

	jsr	popa		; Get and save low part of address to filename
	sta	vbase
	jsr	popa		; Get and save high part of address to filenem
	sta	vbase+1
	ldx	#$FF		; Find length of filename by searching for 0
:	inx
	lda	$FFFF,x		; $FFFF will be replaced by address of string
vbase:=*-2
	bne	:-
	txa			; Length of filename in A
	ldx	vbase		; Address of filename
	ldy	vbase+1
	jsr	$FFBD		; SETNAM

	plx			; Pull address to load to from stack
	ply
	pla			; 0=load, 1=verify, 2=VRAM,0xxxx, 3=VRAM,1xxxx
	jsr	$FFD5		; LOAD

	; 8bit return value must be returned as 16 bit so X and A zeroed
	ldx	#0
	lda	#0
	; Move carry bit into A
	rol
	; Invert bit to make it compatible with C true/false
	eor	#1
	rts

; *****************************************************************************
; Load a ZSM file into banked memory
; *****************************************************************************
_load_zsm:
	sta	$00		; Set correct bank

	lda	#1		; File number, must be unique
	ldx	#8		; Device 8, local filesystem or SD card
	ldy	#2		; Secondary command 2 = headerless load
	jsr	$FFBA		; SETLFS

	jsr	popa		; Get and save low part of address to filename
	sta	base
	jsr	popa		; Get and save high part of address to filename
	sta	base+1
	ldx	#$FF		; Find length of filename by searching for 0
:	inx
	lda	$FFFF,x		; $FFFF will be replaced by address of string
base:=*-2
	bne	:-
	txa			; Length of filename in A
	ldx	base		; Address of filename
	ldy	base+1
	jsr	$FFBD		; SETNAM

	lda	#0		; 0=load, 1=verify, 2=VRAM,0xxxx, 3=VRAM,1xxxx
	ldx	#<$A000		; Address to load to
	ldy	#>$A000
	jsr	$FFD5		; LOAD

	; 8bit return value must be returned as 16 bit so X and A zeroed
	ldx	#0
	lda	#0
	; Move carry bit into A
	rol
	; Invert bit to make it compatible with C true/false
	eor	#1
	rts



; *****************************************************************************
; Install interrupt handler to update jiffie counter
; *****************************************************************************
_start_timer:
	lda	$0314		; Save address of the original interrupt handler
	sta	old_handler
	lda	$0315
	sta	old_handler+1

	sei
	lda	#<myIntHandler	; Install new interrupt handler
	sta	$0314
	lda	#>myIntHandler
	sta	$0315
	cli
	rts

; *****************************************************************************
; Set color at specified coordinates
; *****************************************************************************
_Setcol:
	pha			; Save Color value on stack
	jsr	popa		; Get Y coordinate
	clc
	adc	#$B0		; Add VERA textmode offset
	sta	$9F21		; Set Y coordinate in VERA
	jsr	popa		; Get X coordinate
	asl			; Double it for correct memory address
	inc			; Increment for Color value (not character)
	sta	$9F20		; Set X coordinate in VERA
	pla			; Restore color value from stack
	sta	$9F23		; Write it to VERA
	rts

; *****************************************************************************
; Set foreground color at specified coordinates without changing background
; color
; *****************************************************************************
_Setfgcol:
	pha			; Save foreground color on stack
	jsr	popa
	jsr	_Getcol		; Get current color value
	dec	$9F20		; Back to color value in vram
	and	#$F0		; Remove foreground color value
	sta	$9F23		; Save it to VERA
	dec	$9F20		; Back to color value in vram
	pla			; Get foreground color from stack
	ora	$9F23		; Combine it with existing bg color
	dec	$9F20		; Back to color value in vram
	sta	$9F23		; Set color
	rts

; *****************************************************************************
; Set bacground color at specified coordinates without changing foreground
; color
; *****************************************************************************
_Setbgcol:
	asl			; Move color value to high nibble
	asl
	asl
	asl
	pha			; Save background color on stack
	jsr	popa
	jsr	_Getcol		; Get current color value
	dec	$9F20		; Back to color value in vram
	and	#$0F		; Remove background color value
	sta	$9F23		; Save it to VERA
	dec	$9F20		; Back to color value in vram
	pla			; Get background color from stack
	ora	$9F23		; Combine it with existing fg color
	dec	$9F20		; Back to color value in vram
	sta	$9F23		; Set color
	rts

; *****************************************************************************
; Get color at specified coordinates
; *****************************************************************************
_Getcol:
	clc
	adc	#$B0		; Add VERA textmode offset to Y coordinate
	sta	$9F21		; Write Y coordinate to VERA
	jsr	popa		; Get X coordinate
	asl			; Double and increment to get to color memory
	inc
	sta	$9F20		; Write X coordinate to VERA
	ldx	#0		; Zero out X as X=high byte of 16bit return value
	lda	$9F23		; Read color value from VERA
	rts

; *****************************************************************************
; Get foreground color at specified coordinates
; *****************************************************************************
_Getfgcol:
	jsr	_Getcol
	and	#$0F		; Remove background color information
	ldx	#0		; Zero out X as X=high byte of 16bit return value
	rts

; *****************************************************************************
; Get background color at specified coordinates
; *****************************************************************************
_Getbgcol:
	jsr	_Getcol
	lsr			; Move background color down to low nibble
	lsr
	lsr
	lsr
	ldx	#0		; Zero out X as X=high byte of 16bit return value
	rts

; *****************************************************************************
; Write a character at specified coordinates
; This function converts PETSCII to VERA text
; *****************************************************************************
_PrintChar:
	pha			; Save character to print on stack
	jsr	popa		; Get Y coordinate
	clc
	adc	#$B0		; Add textmode offset
	sta	$9F21		; Set Y coordinate
	jsr	popa		; Get X coordinate
	asl			; Double the x coordinate
	sta	$9F20		; Set X coordinate
	pla			; Restore character 
	; Convert petscii to screencode
	cmp	#$20		; If char < $20, it is a control character
	bcc	nonprintable	; and can not be written to VERA
	cmp	#$40		; If char >= $20 & < $40 it can be written
	bcc	end		; directly to VERA without conversion
	cmp	#$60		; If char >= $40 & < $60 it needs to 
	bcc	:+		; be converted
nonprintable:
	lda	#$57+$40
:	sbc	#$3F
end:
	sta	$9F23
	lda	#0
	rts

; *****************************************************************************
; Wait for interrupt (vSync)
; *****************************************************************************
_waitVsync:
	wai
	rts

; *****************************************************************************
; Read the current value of the joypad, but invert the result so it fits
; with what is expected by the C portion of the program
; *****************************************************************************
_ReadJoypad:
	stz	tmp
	cmp	#0		; If joypad0, read both 0 and 1
	bne	onlyone
	jsr	$FF56		; Read joypad
	eor	#$FF		; Invert result
	sta	tmp		; Store in temporary location
	lda	#1		; Read joypad1 since we have just read joypad0
onlyone:
	jsr	$FF56		; Read joypad
	eor	#$FF		; Invert result
	ora	tmp		; Combine it with result from first joypad
	ldx	#0		; Zero out X as X=high byte of 16bit return value
	rts
tmp:	.byte	0

; *****************************************************************************
; Set screen mode and return status of the call
; *****************************************************************************
_screen_set:
	clc
	jsr	$FF5F 		; Set screenmode to value in .A
	lda	#0		; Empty .A
	rol			; Move carry bit to .A
	eor	#1		; Invert value so it works in C code
	ldx	#0		; Zero out X as X=high byte of 16bit return value
	rts

