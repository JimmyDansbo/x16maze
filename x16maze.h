#ifndef _x16maze_h_
#define _x16maze_h_

#define BLACK		0x00
#define WHITE		0x01
#define RED		0x02
#define CYAN		0x03
#define PURPLE		0x04
#define GREEN		0x05
#define BLUE		0x06
#define YELLOW		0x07
#define ORANGE		0x08
#define BROWN		0x09
#define PINK		0x0A
#define DARKGRAY	0x0B
#define MIDGRAY		0x0C
#define LIGHTGREEN	0x0D
#define LIGHTBLUE	0x0E
#define LIGHTGRAY	0x0F
#define WALLCOL		LIGHTGRAY

// Joystick button values
#define SNES_B		0x80
#define SNES_Y		0x40
#define SNES_SEL	0x20
#define SNES_STA	0x10
#define SNES_UP		0x08
#define SNES_DN		0x04
#define SNES_LT		0x02
#define SNES_RT		0x01
#define SNES_A		0x80
#define SNES_X		0x40
#define SNES_L		0x20
#define SNES_R		0x10

#define	RAM_BANK	0x0000
#define ROM_BANK	0x0001

#define VERA_CONFIG	0x9F29
#define VERA_ADDR	0x9F20
#define VERA_ADDR_HI	0x9F22
#define VERA_DATA0	0x9F23
#define VERA_DATA1	0x9F24

extern void __fastcall__ screen_set(char mode);
extern char __fastcall__ ReadJoypad(char num);
extern void __fastcall__ waitVsync();
extern void __fastcall__ start_timer();
extern void __fastcall__ PrintChar(char x, char y, char ch);
extern char __fastcall__ Getcol(char x, char y);
extern char __fastcall__ Getfgcol(char x, char y);
extern char __fastcall__ Getbgcol(char x, char y);
extern void __fastcall__ Setcol(char x, char y, char col);
extern void __fastcall__ Setfgcol(char x, char y, char col);
extern void __fastcall__ Setbgcol(char x, char y, char col);
extern void __fastcall__ breakpoint();
extern void __fastcall__ dbg8(unsigned char val);
extern void __fastcall__ dbg16(unsigned int val);
extern char __fastcall__ load_zsm(char *str, char bank);
extern void __fastcall__ petprintch(char ch);
extern char __fastcall__ vload(char *str, unsigned int addr, char bank);

#define SPRITE_MODE_4BPP	0
#define SPRITE_MODE_8BPP	1

#define SPRITE_WIDTH_8		0
#define SPRITE_WIDTH_16		1
#define SPRITE_WIDTH_32		2
#define SPRITE_WIDTH_64		3

#define SPRITE_HEIGHT_8		0
#define SPRITE_HEIGHT_16	1
#define SPRITE_HEIGHT_32	2
#define SPRITE_HEIGHT_64	3

struct _spriteattributes {
	unsigned int address;
	unsigned char address_hi;
	unsigned char mode;
	unsigned int x;
	unsigned int y;
	unsigned char vflip;
	unsigned char hflip;
	unsigned char zdepth;
	unsigned char collisionmask;
	unsigned char palletteoffset;
	unsigned char width;
	unsigned char height;
};

#endif
