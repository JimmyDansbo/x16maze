#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "zsmckit.h"
#include "x16maze.h"
#include "levels.h"

typedef unsigned u16;
typedef unsigned char u8;
typedef int s16;
typedef char s8;
typedef unsigned long u32;
typedef long s32;

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 30

// Global variables
u8 cursorx, cursory, bgcolor, curlvl;
u8 lvlbmp[(NUMLEVELS/8)+1];
u16 lvlindex, remflds, MoveCnt;
u32 lvltimes[NUMLEVELS];
// 32 bit value that is updated by vSync interrupt
u32 myTimer;
char tracknames[5][10] = {"title.zsm",
			  "maze1.zsm",
			  "maze2.zsm",
			  "maze3.zsm",
			  "maze4.zsm"};
u8 currtrack=1;

#define MAXTRACKS 4

void leveldone() {
	u8 bit=0x80;
	u8 lvlcnt=0;
	u8 byte=0;

	while (++lvlcnt!=curlvl) {
		bit=bit>>1;
		if (bit==0) {
			bit=0x80;
			++byte;
		}
	}
	lvlbmp[byte]=lvlbmp[byte]|bit;
}

u8 isleveldone(u8 level) {
	u8 bit=0x80;
	u8 lvlcnt=0;
	u8 byte=0;

	while (++lvlcnt!=level) {
		bit=bit>>1;
		if (bit==0) {
			bit=0x80;
			++byte;
		}
	}
	return (lvlbmp[byte]&bit);
}

/******************************************************************************
 Set sprite attributes in VRAM
 The function ensure to set the correct VERA address and then parses the 
 information in the _spriteattributes struct to place it into the correct
 bitfields in VRAM
******************************************************************************/
static void configSprite(char spriteID, struct _spriteattributes *sa) {
	u16 oldaddr;
	u8 oldaddrhi;

	// Save VERA addresses
	oldaddr=*(u16*)VERA_ADDR;
	oldaddrhi=*(u8*)VERA_ADDR_HI;

	// Set VERA address to start of spriteID
	*(u16*)VERA_ADDR=0xFC00+(spriteID*8);
	*(u8*)VERA_ADDR_HI=0x11;

	*(u8*)VERA_DATA0=sa->address>>5;
	*(u8*)VERA_DATA0=(sa->mode<<7)|(sa->address_hi<<3)|(sa->address>>13);
	*(u8*)VERA_DATA0=sa->x;
	*(u8*)VERA_DATA0=sa->x>>8;
	*(u8*)VERA_DATA0=sa->y;
	*(u8*)VERA_DATA0=sa->y>>8;
	*(u8*)VERA_DATA0=(sa->collisionmask<<4)|(sa->zdepth<<2)|(sa->vflip<<1)|(sa->hflip&0x01);
	*(u8*)VERA_DATA0=(sa->height<<6)|(sa->width<<4)|(sa->palletteoffset&0x0F);

	// Restore VERA address
	*(u8*)VERA_ADDR_HI=oldaddrhi;
	*(u16*)VERA_ADDR=oldaddr;
}


/******************************************************************************
 Load and show Crisps sprite
******************************************************************************/
static void loadnshowcrisps() {
	u16 pal[8]={0x0000,0x0410,0x0721,0x0831,0x026C,0x008F,0x0CC8,0x0FFA};
	u8 cnt;
	struct _spriteattributes sa;

	// Set address of pallette
	*(u16*)VERA_ADDR	= 0xFA20;
	*(u8*)VERA_ADDR_HI	= 0x11;
	// Set pallete one to the colors used by Crisps sprite
	for (cnt=0; cnt<8; ++cnt) {
		*(u8*)VERA_DATA0 = (char)(pal[cnt]&0xFF);
		*(u8*)VERA_DATA0 = (char)(pal[cnt]>>8);
	}

	// Load the Crisps sprite into VRAM at address 00000
	vload("crisps32.bin", 0x0000, 0);
	// Reset VERA_ADDR_HI to bank 1 and increment=1 after call to vload
	*(u8*)VERA_ADDR_HI=0x11;

	// Configure the sprite attributes
	sa.address		= 0x0000;
	sa.address_hi		= 0;
	sa.x			= 152;
	sa.y			= 195;
	sa.collisionmask	= 0;
	sa.height		= SPRITE_HEIGHT_32;
	sa.width		= SPRITE_WIDTH_32;
	sa.hflip		= 0;
	sa.vflip		= 0;
	sa.mode			= SPRITE_MODE_4BPP;
	sa.zdepth		= 3;
	sa.palletteoffset 	= 1;
	configSprite(0, &sa);

	*(u8*)VERA_DC_VIDEO	= (*(char*)VERA_DC_VIDEO|0x40); // Enable sprites
}

/******************************************************************************
 Print a 0-terminated string starting at specified coordinates 
******************************************************************************/
static void printstr(u8 x, u8 y, char *str) {
	u16 cnt=0;

	while (str[cnt]!=0)
		PrintChar(x++, y, str[cnt++]);
}

/******************************************************************************
 Print a 0-terminated string starting at specified coordinates with specified
 foreground color 
******************************************************************************/
static void printstrfg(u8 x, u8 y, char *str, u8 fgcol) {
	u16 cnt=0;

	while (str[cnt]!=0) {
		Setfgcol(x, y, fgcol);
		PrintChar(x++, y, str[cnt++]);
	}
}

/******************************************************************************
 Print a 0-terminated string starting at specified coordinates with specified
 foreground- and background-color 
******************************************************************************/
static void printstrcol(u8 x, u8 y, char *str, u8 fgc, u8 bgc) {
	u16 cnt=0;
	char col;

	col = (bgc<<4)+fgc;

	while (str[cnt]!=0) {
		Setcol(x, y, col);
		PrintChar(x++, y, str[cnt++]);
	}
}

/******************************************************************************
 Reset the playing field and ensure that text is written with correct color
******************************************************************************/
static void resetPlayfield() {
	u8 x, y;

	for (y=1; y<SCREEN_HEIGHT-1; ++y)
		for (x=1; x<SCREEN_WIDTH-1; ++x) {
			PrintChar(x, y, 0x20);
			Setbgcol(x, y, WHITE);
		}
	for (y=2; y<SCREEN_HEIGHT-2; ++y)
		for (x=2; x<SCREEN_WIDTH-2; ++x)
			Setcol(x, y, (WALLCOL<<4)+BLACK);

	printstrfg(2,1,"flds:000", RED);
	printstrfg((SCREEN_WIDTH/2)-(4),1,"x16maze", RED);
	printstrfg((SCREEN_WIDTH-9),1,"lvl:000", RED);
	printstrfg((SCREEN_WIDTH/2)-15,SCREEN_HEIGHT-2,"dpad=move b=next select=reset", RED);
}

/******************************************************************************
 Set the border color by changing background color of top and bottom line as
 well as rightmost and leftmost characters on screen.
******************************************************************************/
static void SetBorderColor(char color) {
	u8 x, y;

	for (x=0; x<SCREEN_WIDTH; ++x) {
		Setbgcol(x, 0, color);
		Setbgcol(x, SCREEN_HEIGHT-1, color);
	}
	for (y=0; y<SCREEN_HEIGHT;++y) {
		Setbgcol(0, y, color);
		Setbgcol(SCREEN_WIDTH-1, y, color);
	}
}

/******************************************************************************
 Choose next background color, dependant on previous color
 This function is used to "randomize" the border color
******************************************************************************/
static void nextbgcolor() {
	switch (bgcolor) {
		case WHITE:	bgcolor=RED;	    break;
		case RED:	bgcolor=CYAN;	    break;
		case CYAN:	bgcolor=PURPLE;	    break;
		case PURPLE:	bgcolor=GREEN;	    break;
		case GREEN:	bgcolor=BLUE;	    break;
		case YELLOW:	bgcolor=ORANGE;     break;
		case BLUE:	bgcolor=YELLOW;	    break;
		case ORANGE:	bgcolor=PINK;	    break;
		case PINK:	bgcolor=LIGHTGREEN; break;
		case LIGHTGREEN:bgcolor=LIGHTBLUE;  break;
		case LIGHTBLUE:	bgcolor=WHITE;	    break;
	}
}

/******************************************************************************
 Show the splash screen
******************************************************************************/
static void splashscreen() {
	s8 x, y;	// Signed 8 bit as the loop never goes beyond 40 and 
			// x variable is borrowed later and needs to be signed
	u8 btn=0;
	u32 tmpTimer;

	for (y=0; y<SCREEN_HEIGHT; ++y)
		for (x=0; x<SCREEN_WIDTH; ++x) {
			PrintChar(x, y, ' ');
			Setbgcol(x, y, BLACK);
		}
	
	printstrfg((SCREEN_WIDTH/2)-17, (SCREEN_HEIGHT/2)-8,  "*  * ***  ****", LIGHTBLUE);
	printstrfg((SCREEN_WIDTH/2)-17, (SCREEN_HEIGHT/2)-7,  "*  *   *  *",    GREEN);
	printstrfg((SCREEN_WIDTH/2)-17, (SCREEN_HEIGHT/2)-6,  " **    *  ****", YELLOW);
	printstrfg((SCREEN_WIDTH/2)-17, (SCREEN_HEIGHT/2)-5,  "*  *   *  *  *", ORANGE);
	printstrfg((SCREEN_WIDTH/2)-17, (SCREEN_HEIGHT/2)-4,  "*  * **** ****", RED);

	printstrfg((SCREEN_WIDTH/2)-2, (SCREEN_HEIGHT/2)-8,  "*  *  **  **** ****", ORANGE);
	printstrfg((SCREEN_WIDTH/2)-2, (SCREEN_HEIGHT/2)-7,  "**** *  *   ** *",    ORANGE);
	printstrfg((SCREEN_WIDTH/2)-2, (SCREEN_HEIGHT/2)-6,  "*  * ****  **  **",   ORANGE);
	printstrfg((SCREEN_WIDTH/2)-2, (SCREEN_HEIGHT/2)-5,  "*  * *  * **   *",    ORANGE);
	printstrfg((SCREEN_WIDTH/2)-2, (SCREEN_HEIGHT/2)-4,  "*  * *  * **** ****", ORANGE);

	printstrfg((SCREEN_WIDTH/2)-12, (SCREEN_HEIGHT/2), "press start to begin game", WHITE);
	printstrfg((SCREEN_WIDTH/2)-11, (SCREEN_HEIGHT/2)+3, "created by jimmy dansbo", GREEN);
	printstrfg((SCREEN_WIDTH/2)-11, (SCREEN_HEIGHT/2)+5, "(jimmy@dansbo.dk)  2023", GREEN);
	printstrfg((SCREEN_WIDTH/2)-15, (SCREEN_HEIGHT/2)+6, "https://github.com/jimmydansbo/", LIGHTGREEN);

	printstrfg((SCREEN_WIDTH/2)-7, (SCREEN_HEIGHT/2)+8, "music by crisps", RED);

	// Set VERA address to point to Crisps sprites X coordinate
	*(u16*)VERA_ADDR=0xFC02;
	// Ensure that VERA does not increment or decrement on access
	*(u8*)VERA_ADDR_HI=0x01;
	// Borrow x variable to determine direction of sprite movement
	x=1;

	tmpTimer=myTimer;
	// Wait for user to press start. Use the time to "randomize" background color
	while (btn != SNES_STA) {
		waitVsync();
		nextbgcolor();
		// Only move sprite 30 times a second (2 jiffies delay)
		if ((myTimer-tmpTimer)==2) {
			tmpTimer=myTimer;
			if (x==1) {
				if (*(u8*)VERA_DATA0==195) x=-1;
			} else {
				if (*(u8*)VERA_DATA0==102) x=1;
			}
			*(u8*)VERA_DATA0+=x;
		}
		btn=ReadJoypad(0);
	}
}

/******************************************************************************
 Find index of start of level specified in curlvl variable
******************************************************************************/
static u8 seeklevel() {
	u8 lvl=0;

	lvlindex = 0;

	// Loop while lvl != curlvl
	while (++lvl != curlvl) {
		// If first value of level is 0, we have reached the end of levels
		if (levels[lvlindex]==0) {
			curlvl=1;	// Set current level to 1 and
			lvlindex=0;	// lvlindex to 0
			return (lvl-2);
		}
		lvlindex += levels[lvlindex];	// Set next lvlindex
	}
	// If first value of level is 0, we have reached the end of levels
	if (levels[lvlindex]==0) {
		curlvl=1;
		lvlindex=0;
		return (lvl-2);
	}
	return (curlvl);
}

/******************************************************************************
 Draw the current level to screen
******************************************************************************/
static void drawlevel() {
	u8 ch, bitcnt;
	u8 offsetx, offsety, datacnt;
	u8 curx, cury;
	char str[5];

	remflds=0;

	SetBorderColor(bgcolor);

	// Calculate offset of top left corner from the size of the maze
	offsetx = (SCREEN_WIDTH/2)-(levels[lvlindex+1]/2);
	offsety = (SCREEN_HEIGHT/2)-(levels[lvlindex+2]/2);

	datacnt=0;
	bitcnt=0;

	// Draw the maze it self
	ch=levels[lvlindex+5+datacnt++];
	for (cury=offsety; cury<offsety+levels[lvlindex+2]; cury++) {
		if (bitcnt!=0) {
			ch = levels[lvlindex+5+datacnt++];
			bitcnt=0;
		}
		for (curx=offsetx; curx<offsetx+levels[lvlindex+1]; curx++) {
			if ((ch & 0x80) == 0) {
				Setbgcol(curx, cury, bgcolor);
				remflds++;
			}
			ch = ch<<1;
			if (++bitcnt == 8) {
				ch = levels[lvlindex+5+datacnt++];
				bitcnt=0;
			}
		}
	}

	// Place the cursor at the starting position
	cursorx = offsetx+levels[lvlindex+3];
	cursory = offsety+levels[lvlindex+4];

	PrintChar(cursorx, cursory, 0x00); // Non-printable character
	Setcol(cursorx, cursory, (BLACK<<4)+bgcolor);

	// remflds has been incremented once too many
	--remflds;
	sprintf(str, "%03d", remflds);
	printstr(7, 1, str);
	// Print current level
	sprintf(str, "%03d", curlvl);
	printstr(SCREEN_WIDTH-5, 1, str);
	// Reset timer variable and MoveCnt
	myTimer=0;
	MoveCnt=0;
}

/******************************************************************************
 Move cursor according to the X and Y parameters
 X and Y parametes should be either -1, 0 or +1
 -1 decrements a coordinate (making cursor go up or left)
 0 leaves coordinate alone
 +1 increments a coordinate (making cursor go down or right)
******************************************************************************/
static void do_move(s8 x, s8 y) {
	u8 moved=false;
	char str[5];

	if (Getbgcol(cursorx+x, cursory+y)!=WALLCOL) {
		zsm_rewind(2);
		zsm_rewind(1);
		zsm_play(1);
	}
	// Loop until we hit a wall
	while (Getbgcol(cursorx+x, cursory+y)!=WALLCOL) {
		// Remove old cursor 
		PrintChar(cursorx, cursory, ' ');
		Setbgcol(cursorx, cursory, BLACK);
		// Calculate new coordinate
		cursorx+=x;
		cursory+=y;
		moved=true;
		// Only update remaining fields if we overwrite
		// an empty field
		if (Getbgcol(cursorx, cursory)==bgcolor) {
			remflds--;
			sprintf(str, "%03d", remflds);
			printstr(7, 1, str);
		}
		// Show cursor at new location
		PrintChar(cursorx, cursory, 0x00);  //non-printable
		Setcol(cursorx, cursory, (BLACK<<4)+bgcolor);
		waitVsync();
	}
	// Update MoveCnt if we have actually moved
	if (moved) {
		MoveCnt++;
		zsm_rewind(1);
		zsm_rewind(2);
		zsm_play(2);
	}
}

/******************************************************************************
 Show statistics for the completed level
******************************************************************************/
static void show_win() {
	u8 btn, hour, minute, second, gamedone;
	u16 msec;
	u32 curtimer;
	char str[40];

	// Read the current jiffie time
	curtimer=myTimer;
	lvltimes[curlvl-1]=curtimer;
	leveldone();

	gamedone=1;
	for (btn=1; btn<=NUMLEVELS; ++btn) {
		if (isleveldone(btn)==0) gamedone=0; 
	}
	if (gamedone==1) { // All levels are done
		curtimer=0;
		for (btn=0; btn<NUMLEVELS; ++btn) curtimer+=lvltimes[btn];
	}

	// Calculate human readable values
	hour = curtimer/60/60/60;
	curtimer=curtimer-(hour*60*60*60);
	minute = curtimer/60/60;
	curtimer=curtimer-(minute*60*60);
	second = curtimer/60;
	curtimer=curtimer-(second*60);
	msec=curtimer*(1000/60);


	// Print the values in a formatted string
	sprintf(str, "* %02d hr %02d min %02d sec %03d msec *",hour,minute,second,msec);

	// Show the statistics
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-4, "********************************", ORANGE, BLACK);
	if (gamedone==1) {
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-3, "*   !!! Congratulations !!!    *", YELLOW, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-2, "*    all levels completed      *", YELLOW, BLACK);
	} else {
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-3, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-2, "*       level completed        *", ORANGE, BLACK);
	}
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-1, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2), str, ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+1, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+3, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+4, "********************************", ORANGE, BLACK);
	if (gamedone==1) {
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+2, "*         total time           *", ORANGE,BLACK);
	} else {
	sprintf(str,"*          %04d moves          *",MoveCnt);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+2, str, ORANGE,BLACK);
	}

	if (gamedone==1) {
		for (btn=0; btn<((NUMLEVELS/8)+1); ++btn)
			lvlbmp[btn]=0x00;
	}

	// Use the timer as a delay when updating the "bordercolor"
	curtimer=myTimer;
	// Wait for user to press the B button
	btn=0;
	while (btn != SNES_B) {
		// Cycle bgcolor's
		nextbgcolor();
		SetBorderColor(bgcolor);
		while ((myTimer != curtimer+10) && (btn != SNES_B)) {
			waitVsync();
			btn=ReadJoypad(0);
		};
		curtimer=myTimer;
	}
}

/******************************************************************************
 Show window to let the user select a specific level
******************************************************************************/
static void select_level() {
	u8 btnPressed=0;
	u8 btnHeld=0;
	u8 btnPrev=0;
	u8 lvl;
	char str[6];

	lvl=curlvl;

	while (btnPressed != SNES_Y) {
	// Show the window
	printstrcol(12, 9, "*****************", ORANGE, BLACK);
	printstrcol(12,10, "*               *", ORANGE, BLACK);
	printstrcol(12,11, "* select level: *", ORANGE, BLACK);
	printstrcol(12,12, "*               *", ORANGE, BLACK);
	printstrcol(12,13, "*               *", ORANGE, BLACK);
	printstrcol(12,14, "*               *", ORANGE, BLACK);
	printstrcol(12,15, "*****************", ORANGE, BLACK);
	// Update with the current level
	sprintf(str, "%03d", curlvl);
	printstr(19, 13, str);

	// Let the user use up- and down arrows to select level
	// Loop until user presses SELECT
	while (btnPressed != SNES_SEL) {
		waitVsync();
		// Use 3 variables to decide if a button is held
		btnHeld = ReadJoypad(0);
		btnPressed = ReadJoypad(0);
		btnPressed = btnHeld & (btnHeld ^ btnPrev);

		// If DOWN has been pressed, decrement level
		if (btnPressed & SNES_DN) {
			if (--lvl == 0) lvl=NUMLEVELS;
			sprintf(str, "%03d", lvl);
			printstr(19, 13, str);
		// If UP has been pressed, increment level
		} else if (btnPressed & SNES_UP) {
			if (++lvl > NUMLEVELS) lvl=1;
			sprintf(str, "%03d", lvl);
			printstr(19, 13, str);
		}
		btnPrev = btnHeld;
	}
	if (isleveldone(lvl)!=0) {
		printstrcol(12, 9, "*****************", ORANGE, BLACK);
		printstrcol(12,10, "* level already *", ORANGE, BLACK);
		printstrcol(12,11, "*   completed   *", ORANGE, BLACK);
		printstrcol(12,12, "*   previous    *", ORANGE, BLACK);
		printstrcol(12,13, "* time lost if  *", ORANGE, BLACK);
		printstrcol(12,14, "* you restart   *", ORANGE, BLACK);
		printstrcol(12,15, "*****************", ORANGE, BLACK);
		while ((btnPressed != SNES_Y) && (btnPressed != SNES_STA)) {
			waitVsync();
			btnPressed = ReadJoypad(0);
		}
	} else btnPressed=SNES_Y;
	}
	curlvl=lvl;
}

void petprint(char *str) {
	unsigned char cnt=0;

	while (str[cnt]!=0)
		petprintch(str[cnt++]);
}

void switchtrack(){
	u8 x, y, cnt;
	u16 cell[110];

	zsm_rewind(0);
	*(u8*)VERA_ADDR_HI = 0x11;	// Ensure VBANK1 and Addr INC=1
	cnt=0;
	for (y=13; y<=17; ++y) {
		for (x=9; x<=30; ++x) {
			cell[cnt++]=Getcell(x, y);
		}
	}

	printstrcol(9, 13, "**********************", YELLOW, BLACK);
	printstrcol(9, 14, "*                    *", YELLOW, BLACK);
	printstrcol(9, 15, "* loading next track *", YELLOW, BLACK);
	printstrcol(9, 16, "*                    *", YELLOW, BLACK);
	printstrcol(9, 17, "**********************", YELLOW, BLACK);

	if (++currtrack > MAXTRACKS) currtrack=0;
	load_zsm(tracknames[currtrack], 2);
	zsm_setmem(0, 0xA000, 2);
	zsm_play(0);
	cnt=0;
	for (y=13; y<=17; ++y) {
		for (x=9; x<=30; ++x) {
			Putcell(x, y, cell[cnt++]);
		}
	}

}

/******************************************************************************
 Main function that initializes game and runs the main loop
******************************************************************************/
int main(){
	u8 btnPressed;
	u8 btnHeld=0;
	u8 btnPrev=0;
	u8 playmusic=1;
	curlvl=1;
	bgcolor=WHITE;

	// Switch to standard PETSCII character set
	__asm__ ("lda #$8E");
	__asm__ ("jsr $FFD2");

	for (btnPressed=0; btnPressed<((NUMLEVELS/8)+1); ++btnPressed)
		lvlbmp[btnPressed]=0x00;

	petprint("loading title theme...");
	load_zsm(tracknames[0], 2);
	load_zsm("swoosh.zsm", 38);
	load_zsm("bonk.zsm",39);

	// Set 40x30 mode

	screen_set(3);

	zsm_init_engine(1);
	zsm_setmem(0, 0xA000, 2);
	zsm_setmem(1, 0xA000, 38);	// Swoosh
	zsm_setmem(2, 0xA000, 39);	// Bonk
	zsm_setloop(1, NO_LOOP);
	zsm_setloop(2, NO_LOOP);
	zsm_setatten(0, 0x1F);
	zsm_setatten(2, 0x2F);
	zsm_play(0);

	myTimer=0;
	start_timer();

	loadnshowcrisps();

	// Show the splash screen and wait for the user to press START
	splashscreen();

	zsm_rewind(0);
	*(u8*)VERA_ADDR_HI = 0x11;	// Ensure VBANK1 and Addr INC=1

	printstrcol(9, 13, "**********************", YELLOW, BLACK);
	printstrcol(9, 14, "*                    *", YELLOW, BLACK);
	printstrcol(9, 15, "* loading assets...  *", YELLOW, BLACK);
	printstrcol(9, 16, "*                    *", YELLOW, BLACK);
	printstrcol(9, 17, "**********************", YELLOW, BLACK);

	load_zsm(tracknames[1], 2);
	zsm_setmem(0, 0xA000, 2);
	zsm_play(0);

	*(u8*)VERA_DC_VIDEO = (*(u8*)VERA_DC_VIDEO&0xBF); // Disable sprites

	while (1) {
		// Find curlvl
		seeklevel();
		resetPlayfield();
		// Draw curlvl
		drawlevel();
		btnPressed=0;
		while (btnPressed != SNES_SEL) {
			waitVsync();

			btnHeld = ReadJoypad(0);
			btnPressed = btnHeld & (btnHeld ^ btnPrev);

			if (btnPressed & SNES_RT) 
				do_move(1, 0);
			else if (btnPressed & SNES_LT)
				do_move(-1, 0);
			else if (btnPressed & SNES_UP)
				do_move(0, -1);
			else if (btnPressed & SNES_DN)
				do_move(0, 1);
			else if (btnPressed & SNES_Y) {
				select_level();
				btnPressed=SNES_SEL;
			} else if (btnPressed & SNES_B) {
				switchtrack();
			} else if (btnPressed & SNES_STA) {
				if (playmusic==1) {
					playmusic=0;
					zsm_stop(0);
					*(u8*)VERA_ADDR_HI = 0x11;	// Ensure VBANK1 and Addr INC=1
				} else {
					playmusic=1;
					zsm_play(0);
				}
			}
			btnPrev = btnHeld;
			// If there are no more fields, the maze is solved
			if (remflds==0) {
				show_win();
				++curlvl;
				while (isleveldone(curlvl)!=0) ++curlvl;
				btnPressed=SNES_SEL;
				btnHeld=SNES_B;
				btnPrev=SNES_B;
			}
		}
	}
	return 0;
}
