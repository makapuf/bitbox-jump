#include <string.h>
#include <stdint.h>
#include <bitbox.h>
#include "tilemaps.h"
#include "blit.h"

#ifdef EMULATED
#include <stdio.h>
#endif

uint16_t intro_ramtilemap[40*30];
int start_frame;

int cursor;
int8_t code[4]; // 4-letters code, 0-31
int speed;
int best_score, best_coins;

extern void (*do_frame)(void);
extern void (*do_line)(void);

// do an object
extern uint16_t *tilemap;
extern int tilemap_w,tilemap_h;
extern int tilemap_x,tilemap_y;
extern Sprite piece;

extern  void level_init(int);
extern  void draw_bg();

#define NB_COINS 8
Sprite coins[NB_COINS];


void intro_line()
{
	// uses pixel_t *draw_buffer, uint32_t line uint32_t frame
	draw_bg();
	// draw some coins
	for (int i=0;i<NB_COINS;i++)
		blit_line(&coins[i]);

}

void intro_out()
{
	int level_code = code[0]<<24 | code[1]<<16 | code[2]<<8 | code[3]; 
	level_init(level_code); // start game after 3 seconds !
}

void intro_frame()
{
	if (vga_frame-start_frame == 1 )
	{
		// intro screen
		memcpy(intro_ramtilemap,tilemap_intro,40*30*sizeof(uint16_t));
	} else if (tilemap_y<0) // scroll down
	{	speed += 1;
		tilemap_y+= speed;	
		if (tilemap_y>0) tilemap_y=0;	
	} else {
		// enter code 
		if (GAMEPAD_PRESSED(0,right) && ((vga_frame & 7)==0)) {
			cursor++;
			if(cursor>3) cursor =0;
		}
		if (GAMEPAD_PRESSED(0,left) && ((vga_frame & 7)==0)) {
			cursor--;
			if(cursor<0) cursor = 3;
		}

		if (GAMEPAD_PRESSED(0,up) && ((vga_frame & 7)==0))
		{
			code[cursor]++;
			if (code[cursor]>25) code[cursor] = 0;
		}
		if (GAMEPAD_PRESSED(0,down) && ((vga_frame & 7)==0))
		{
			code[cursor]--;
			if (code[cursor]<0) code[cursor] = 25;
		}

		// update cursor
		for (int i=0;i<4;i++)
			intro_ramtilemap[29*40+4+i]=0xe4;
		intro_ramtilemap[29*40+4+cursor]=0x013d;
		// draws code
		for (int i=0;i<4;i++)
			intro_ramtilemap[28*40+4+i]=0x110+code[i]+(code[i]>13?2:0); // last term is needed because tileset is urgh.	

		// draws best score
		int n=best_score;
		int col=31;
		do {
			intro_ramtilemap[26*40+col--]=0x130+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>25;col--) intro_ramtilemap[26*40+col]=0x90;

		n=best_coins;
		col=37;
		do {
			intro_ramtilemap[26*40+col--]=0x130+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>34;col--) intro_ramtilemap[26*40+col]=0x90;
	}


	// prepare draws and advance animation XXX move ?
	for (int i=0;i<NB_COINS;i++) {
		blit_frame(&coins[i]);
	}

	for (int i=0;i<NB_COINS;i++)
	{
		coins[i].x+=3;
		if (coins[i].x>=640-16)
			coins[i].x = 0;

		if (vga_frame%6==0) 
		{
			coins[i].frame = (coins[i].frame+1)%coins[i].nb_frames;
		}
	}

	if (GAMEPAD_PRESSED(0,start)) intro_out();
}


const int deltay[8] = {-2,1,4,2,-1,3,1,0}; // spread coins randomly vertically

void intro_init(int score, int nbcoins)
{
	cursor = 0;
	speed = 0;
	code[0]=code[1]=code[2]=code[3]=0;
	if (score>best_score) best_score = score;
	if (nbcoins>best_coins) best_coins = nbcoins;

	tilemap_w = 40;
	tilemap_h = 30;
	tilemap = &intro_ramtilemap[0];
	start_frame=vga_frame;
	tilemap_y = -480; // offline
	tilemap_x = 0;

	for (int i=0;i<NB_COINS;i++)
	{
		memcpy((void*)&coins[i], &piece,sizeof(piece)); // like an assignment but without consts
		coins[i].y = 300+deltay[i];
		coins[i].x = i*640/NB_COINS;
		coins[i].frame = i%coins[i].nb_frames;
	}


	do_frame = intro_frame;
	do_line = intro_line;
}