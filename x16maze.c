#include <stdio.h>
#include <stdbool.h>
#include "zsmplayer.h"
#include "x16maze.h"
#include "levels.h"

typedef unsigned u16;
typedef unsigned char u8;
typedef int s16;
typedef char s8;

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 30

// Global variables
u8 cursorx, cursory, bgcolor, curlvl, numlevels;
u16 lvlindex, remflds, MoveCnt;
// 16 bit value that is updated by vSync interrupt
u16 myTimer;

static void loadnshowcrisps() {
	u16 pal[15]={	0x0000,0x0112,0x0311,0x0410,
			0x0510,0x0225,0x0226,0x0710,
			0x022A,0x0239,0x023E,0x0543,
			0x027C,0x028E,0x0CC9
		    };
	u8 cnt;

	// Set address of pallette
	*(char*)0x9F20 = 32;
	*(char*)0x9F21 = 0xFA;
	*(char*)0x9F22 = 0x11;
	for (cnt=0; cnt<15; ++cnt) {
		*(char*)0x9F23 = (char)(pal[cnt]&0xFF);
		*(char*)0x9F23 = (char)(pal[cnt]>>8);
	}

	vload("crisps.bin", 0x0000, 0);

	// Set address of first sprite
	*(char*)0x9F20 = 0;
	*(char*)0x9F21 = 0xFC;
	*(char*)0x9F22 = 0x11;

	*(char*)0x9F23 = 0;	// Low Address bits
	*(char*)0x9F23 = 0;	// High Address bits
	*(char*)0x9F23 = 152;	// Low X coordinate
	*(char*)0x9F23 = 0;	// High X coordinate
	*(char*)0x9F23 = 195;	// Low Y coordinate
	*(char*)0x9F23 = 0;	// High Y coordinate

	*(char*)0x9F23 = 0x0C;	// Z-depth in front
	*(char*)0x9F23 = 0x51;	// Pallette offset = 1

	*(char*)0x9F29 = (*(char*)0x9F29|0x40); // Enable sprites

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
	u8 x, y;
	u8 btn=0;

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

	// Wait for user to press start. Use the time to "randomize" background color
	while (btn != SNES_STA) {
		waitVsync();
		nextbgcolor();
		btn=ReadJoypad(0);
	}
}

/******************************************************************************
 Find index of start of level specified in curlvl variable
******************************************************************************/
static u8 seeklevel() {
	u8 lvl=1;

	lvlindex = 0;

	// Loop while lvl != curlvl
	while (lvl++ != curlvl) {
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
				Setbgcol(curx, cury, BLACK);
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
	Setbgcol(cursorx, cursory, bgcolor);

	// remflds has been incremented once too manu
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

	// Loop until we hit a wall
	while (Getbgcol(cursorx+x, cursory+y)!=WALLCOL) {
		// Remove old cursor 
		PrintChar(cursorx, cursory, ' ');
		// Calculate new coordinate
		cursorx+=x;
		cursory+=y;
		moved=true;
		// Only update remaining fields if we overwrite
		// an empty field
		if (Getbgcol(cursorx, cursory)==BLACK) {
			remflds--;
			sprintf(str, "%03d", remflds);
			printstr(7, 1, str);
		}
		// Show cursor at new location
		PrintChar(cursorx, cursory, 0x00);  //non-printable
		Setbgcol(cursorx, cursory, bgcolor);
		waitVsync();
	}
	// Update MoveCnt if we have actually moved
	if (moved) MoveCnt++;
}

/******************************************************************************
 Show statistics for the completed level
******************************************************************************/
static void show_win() {
	u8 btn=0;
	u8 hour, minute, second;
	u16 msec, curtimer;
	char str[40];

	// Read the current jiffie time
	curtimer=myTimer;

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
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-4,  "********************************", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-3,  "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-2,  "*       level completed        *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)-1, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2), str, ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+1, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+3, "*                              *", ORANGE, BLACK);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+4, "********************************", ORANGE, BLACK);
	sprintf(str,"*          %04d moves          *",MoveCnt);
	printstrcol((SCREEN_WIDTH/2)-16, (SCREEN_HEIGHT/2)+2, str, ORANGE,BLACK);

	// Use the timer as a delay when updating the "bordercolor"
	curtimer=myTimer;
	// Wait for user to press the B button
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
	char str[6];

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
			if (--curlvl == 0) curlvl=numlevels;
			sprintf(str, "%03d", curlvl);
			printstr(19, 13, str);
		// If UP has been pressed, increment level
		} else if (btnPressed & SNES_UP) {
			if (++curlvl > numlevels) curlvl=1;
			sprintf(str, "%03d", curlvl);
			printstr(19, 13, str);
		}
		btnPrev = btnHeld;
	}
}

void petprint(char *str) {
	unsigned char cnt=0;

	while (str[cnt]!=0)
		petprintch(str[cnt++]);
}

/******************************************************************************
 Main function that initializes game and runs the main loop
******************************************************************************/
int main(){
	u8 btnPressed;
	u8 btnHeld=0;
	u8 btnPrev=0;
	curlvl=255;
	bgcolor=WHITE;


	// Switch to standard PETSCII character set
	__asm__ ("lda #$8E");
	__asm__ ("jsr $FFD2");
	petprint("loading title theme...");
	load_zsm("title.zsm", 2);


	// Set 40x30 mode
	screen_set(3);

	zsm_init();
	zsm_startmusic(2, 0xA000);

	myTimer=0;
	start_timer();

	numlevels=seeklevel();

	loadnshowcrisps();

	// Show the splash screen and wait for the user to press START
	splashscreen();

	zsm_stopmusic();		// stopmusic seems to change VERA INC value
	*(char *)0x9F22 = 0x11;		// Ensure VBANK1 and Addr INC=1

	printstrcol(9, 13, "**********************", YELLOW, BLACK);
	printstrcol(9, 14, "*                    *", YELLOW, BLACK);
	printstrcol(9, 15, "* loading assetss... *", YELLOW, BLACK);
	printstrcol(9, 16, "*                    *", YELLOW, BLACK);
	printstrcol(9, 17, "**********************", YELLOW, BLACK);

	load_zsm("maze.zsm", 2);
	zsm_startmusic(2, 0xA000);

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
			}
			btnPrev = btnHeld;
			// If there are no more fields, the maze is solved
			if (remflds==0) {
				show_win();
				curlvl++;
				btnPressed=SNES_SEL;
			}
		}
	}
	return 0;
}
