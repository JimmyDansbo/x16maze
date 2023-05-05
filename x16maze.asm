.import popa

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

.segment "CODE"
old_handler:	.word $0000
myTimer = $02

myIntHandler:
	inc	myTimer
	bne	skip
	inc	myTimer+1
skip:
	jmp	(old_handler)

_start_timer:
	lda	$0314
	sta	old_handler
	lda	$0315
	sta	old_handler+1

	sei
	lda	#<myIntHandler
	sta	$0314
	lda	#>myIntHandler
	sta	$0315
	cli
	lda	#0
	rts

_Setcol:
	pha
	jsr	popa
	clc
	adc	#$B0
	sta	$9F21
	jsr	popa
	asl
	inc
	sta	$9F20
	pla
	sta	$9F23
	rts

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

_Getcol:
	clc
	adc	#$B0
	sta	$9F21
	jsr	popa
	asl
	inc
	sta	$9F20
	lda	$9F23
	rts

_Getfgcol:
	jsr	_Getcol
	and	#$0F
	rts

_Getbgcol:
	jsr	_Getcol
	lsr
	lsr
	lsr
	lsr
	rts

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
	cmp	#$20
	bcc	nonprintable
	cmp	#$40
	bcc	end
	cmp	#$60
	bcc	:+
nonprintable:
	lda	#$57+$40
:	sbc	#$3F
end:
	sta	$9F23
	lda	#0
	rts

_waitVsync:
	wai
	rts

_ReadJoypad:
	jsr	$FF56
	eor	#$FF
;	ldx	#0
	rts

_screen_set:
	clc
	jsr	$FF5F 
	ldx	#0
	lda	#0
	rol
	eor	#1
	rts

