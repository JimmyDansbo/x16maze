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

#endif