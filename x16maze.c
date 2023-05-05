#include <stdio.h>
#include <stdbool.h>
#include "x16maze.h"
#include "levels.h"

typedef unsigned u16;
typedef unsigned char u8;
typedef int s16;
typedef char s8;

#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 30

#define myTimer 0x0002

u8 cursorx, cursory, bgcolor, curlvl;
u16 lvlindex, remflds, MoveCnt;

static void printstr(u8 x, u8 y, char *str) {
	u16 cnt=0;
	u8 ch;

	while (str[cnt]!=0) {
		ch = str[cnt++];
		if (ch > 0x3F) ch=ch-0x40;
		PrintChar(x++, y, ch);
	}
}
static void printstrfg(u8 x, u8 y, char *str, u8 fgcol) {
	u16 cnt=0;
	u8 ch;

	while (str[cnt]!=0) {
		ch = str[cnt++];
		Setfgcol(x, y, fgcol);
		PrintChar(x++, y, ch);
	}
}
static void printstrcol(u8 x, u8 y, char *str, u8 fgc, u8 bgc) {
	u16 cnt=0;
	u8 ch;
	char col;

	col = (bgc<<4)+fgc;

	while (str[cnt]!=0) {
		ch = str[cnt++];
		Setcol(x, y, col);
		PrintChar(x++, y, ch);
	}
}

static void resetPlayfield() {
	u8 x, y;

	for (y=1; y<SCREEN_HEIGHT-1; y++)
		for (x=1; x<SCREEN_WIDTH-1; x++) {
			PrintChar(x, y, 0x20);
			Setbgcol(x, y, WHITE);
		}
	for (y=2; y<SCREEN_HEIGHT-2; y++)
		for (x=2; x<SCREEN_WIDTH-2; x++) {
			PrintChar(x, y, 0x20);
			Setcol(x, y, (WALLCOL<<4)+BLACK);
		}

	printstrfg(2,1,"flds:000", RED);
	printstrfg((SCREEN_WIDTH/2)-(4),1,"x16maze", RED);
	printstrfg((SCREEN_WIDTH-9),1,"lvl:000", RED);
	printstrfg((SCREEN_WIDTH/2)-15,SCREEN_HEIGHT-2,"dpad=move b=next select=reset", RED);
}

static void SetBorderColor(char color) {
	u8 x, y;

	for (x=0; x<SCREEN_WIDTH; x++) {
		Setbgcol(x, 0, color);
		Setbgcol(x, SCREEN_HEIGHT-1, color);
	}
	for (y=0; y<SCREEN_HEIGHT;y++) {
		Setbgcol(0, y, color);
		Setbgcol(SCREEN_WIDTH-1, y, color);
	}
}

static void nextbgcolor() {
	switch (bgcolor) {
		case WHITE:	bgcolor=RED;	break;
		case RED:	bgcolor=CYAN;	break;
		case CYAN:	bgcolor=PURPLE;	break;
		case PURPLE:	bgcolor=GREEN;	break;
		case GREEN:	bgcolor=BLUE;	break;
		case BLUE:	bgcolor=YELLOW;	break;
		case YELLOW:	bgcolor=ORANGE; break;
		case ORANGE:	bgcolor=PINK;	break;
		case PINK:	bgcolor=LIGHTGREEN; break;
		case LIGHTGREEN:bgcolor=LIGHTBLUE; break;
		case LIGHTBLUE:	bgcolor=WHITE;	break;
	}
}

static void splashscreen() {
	u8 x, y;
	u8 btn=0;

	for (y=0;y<SCREEN_HEIGHT;y++)
		for (x=0;x<SCREEN_WIDTH;x++) {
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
	printstrfg((SCREEN_WIDTH/2)-15, (SCREEN_HEIGHT/2)+7, "https://github.com/jimmydansbo/", LIGHTGREEN);

	while (btn != SNES_STA) {
		waitVsync();
		nextbgcolor();
		btn=ReadJoypad(0);
	}
}

static void seeklevel() {
	u8 lvl=1;

	lvlindex = 0;

	while (lvl++ != curlvl) {
		if (levels[lvlindex]==0) {
			curlvl=1;
			lvlindex=0;
			return;
		}
		lvlindex += levels[lvlindex];
	}
	if (levels[lvlindex]==0) {
		curlvl=1;
		lvlindex=0;
		return;
	}
}

static void drawlevel() {
	u8 ch, bitcnt;
	u8 offsetx, offsety, datacnt;
	u8 curx, cury;
	char str[5];

	remflds=0;

	SetBorderColor(bgcolor);

	offsetx = (SCREEN_WIDTH/2)-(levels[lvlindex+1]/2);
	offsety = (SCREEN_HEIGHT/2)-(levels[lvlindex+2]/2);

	datacnt=0;
	bitcnt=0;
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

	cursorx = offsetx+levels[lvlindex+3];
	cursory = offsety+levels[lvlindex+4];

	PrintChar(cursorx, cursory, 0x00); // Non-printable character
	Setbgcol(cursorx, cursory, bgcolor);
	remflds--;
	sprintf(str, "%03d", remflds);
	printstr(7, 1, str);
	sprintf(str, "%03d", curlvl);
	printstr(SCREEN_WIDTH-5, 1, str);
	*(unsigned *)myTimer=0;
	MoveCnt=0;
}

static void do_move(s8 x, s8 y) {
	u8 moved=false;
	char str[5];

	while (Getbgcol(cursorx+x, cursory+y)!=WALLCOL) {
		PrintChar(cursorx, cursory, ' ');
		cursorx+=x;
		cursory+=y;
		moved=true;
		if (Getbgcol(cursorx, cursory)==BLACK) {
			remflds--;
			sprintf(str, "%03d", remflds);
			printstr(7, 1, str);
		}
		PrintChar(cursorx, cursory, 0x00);  //non-printable
		Setbgcol(cursorx, cursory, bgcolor);
		waitVsync();
	}
	if (moved) MoveCnt++;
}

static void show_win() {
	u8 btn=0;
	u8 hour, minute, second;
	u16 msec, curtimer;
	char str[40];

	curtimer=*(unsigned *)myTimer;

	hour = curtimer/60/60/60;
	curtimer=curtimer-(hour*60*60*60);
	minute = curtimer/60/60;
	curtimer=curtimer-(minute*60*60);
	second = curtimer/60;
	curtimer=curtimer-(second*60);
	msec=curtimer*(1000/60);

	sprintf(str, "* %02d hr %02d min %02d sec %03d msec *",hour,minute,second,msec);

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

	curtimer=*(unsigned *)myTimer;	
	while (btn != SNES_B) {
		nextbgcolor();
		SetBorderColor(bgcolor);
		while ((*(unsigned *)myTimer != curtimer+10) && (btn != SNES_B)) {
			waitVsync();
			btn=ReadJoypad(0);
		};
		curtimer=*(unsigned *)myTimer;	
	}
}

static void select_level() {
	u8 btnPressed=0;
	u8 btnHeld=0;
	u8 btnPrev=0;
	char str[6];

	printstrcol(12, 9, "*****************", ORANGE, BLACK);
	printstrcol(12,10, "*               *", ORANGE, BLACK);
	printstrcol(12,11, "* select level: *", ORANGE, BLACK);
	printstrcol(12,12, "*               *", ORANGE, BLACK);
	printstrcol(12,13, "*               *", ORANGE, BLACK);
	printstrcol(12,14, "*               *", ORANGE, BLACK);
	printstrcol(12,15, "*****************", ORANGE, BLACK);
	sprintf(str, "%03d", curlvl);
	printstr(19, 13, str);

	while (btnPressed != SNES_SEL) {
		waitVsync();
		btnHeld = ReadJoypad(0);
		btnPressed = ReadJoypad(0);
		btnPressed = btnHeld & (btnHeld ^ btnPrev);

		if (btnPressed & SNES_DN) {
			if (--curlvl == 0) curlvl=MAX_LEVELS;
			sprintf(str, "%03d", curlvl);
			printstr(19, 13, str);
		} else if (btnPressed & SNES_UP) {
			if (++curlvl > MAX_LEVELS) curlvl=1;
			sprintf(str, "%03d", curlvl);
			printstr(19, 13, str);
		}
		btnPrev = btnHeld;
	}
}

int main(){
	u8 btnPressed;
	u8 btnHeld=0;
	u8 btnPrev=0;
	curlvl=1;
	bgcolor=WHITE;

	// Switch to standard PETSCII character set
	__asm__ ("lda #$8E");
	__asm__ ("jsr $FFD2");
	// Set 40x30 mode
	screen_set(3);

	*(unsigned *)myTimer = 0;
	start_timer();

	splashscreen();

	while (1) {
		seeklevel();
		resetPlayfield();
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
			if (remflds==0) {
				show_win();
				curlvl++;
				btnPressed=SNES_SEL;
			}
		}
	}
	return 0;
}
