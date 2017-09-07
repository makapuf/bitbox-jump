#include <string.h>
#include <stdint.h>

#include "lib/blitter/blitter.h"
#include "lib/sampler/sampler.h"

#include "intro_tmap.h"
#define TMAP_INTRO_W 64
#define TMAP_INTRO_H 30

#define SMP(x) extern const int8_t sample_##x##_data[]; extern const int sample_##x##_len;extern const int sample_##x##_rate;
#define PLAY(x) play_sample(sample_##x##_data,sample_##x##_len, 256*sample_##x##_rate/BITBOX_SAMPLERATE,-1, 50,50 );

SMP(intro);

extern const uint16_t jumper3_tset[]; // share tileset

uint16_t intro_ramtilemap[TMAP_INTRO_H][TMAP_INTRO_W];
int start_frame;

static int cursor;
int8_t code[4]; // 4-letters code, 0-31
int speed;
int best_score, best_coins;

extern void (*do_frame)(void);

extern uint32_t piece_spr[];
#define COINS_FRAMES 4

extern  void level_init(int);

#define NB_COINS 8
static struct object *coins[NB_COINS];
struct object *bg; // tilemap

void intro_out();

void intro_frame()
{
	if (bg->y<0) { // scroll down : ease w/ overshot ? 
		speed += 1;
		bg->y+= speed;	
		if (bg->y>0) {
			bg->y=0;	
			PLAY(intro);
		}
	} else {
		if ((vga_frame & 7) == 0) {
			// enter code 
			if (GAMEPAD_PRESSED(0,right)) {
				cursor++;
				if(cursor>3) cursor =0;
			}

			if (GAMEPAD_PRESSED(0,left)) {
				cursor--;
				if(cursor<0) cursor = 3;
			}

			if (GAMEPAD_PRESSED(0,up)) {
				code[cursor]++;
				if (code[cursor]>25) code[cursor] = 0;
			}

			if (GAMEPAD_PRESSED(0,down)) {
				code[cursor]--;
				if (code[cursor]<0) code[cursor] = 25;
			}
		}

		// update cursor
		for (int i=0;i<4;i++)
			intro_ramtilemap[29][4+i]=0xe4+1;
		intro_ramtilemap[29][4+cursor]=0x013d+1;

		// draws code
		for (int i=0;i<4;i++)
			intro_ramtilemap[28][4+i]=0x111+code[i]+(code[i]>13?2:0); // last term is needed because tileset is urgh.	

		// draws best score
		int n=best_score;
		int col=31;
		do {
			intro_ramtilemap[26][col--]=0x131+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>25;col--) intro_ramtilemap[26][col]=0x91;

		n=best_coins;
		col=37;
		do {
			intro_ramtilemap[26][col--]=0x131+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>34;col--) 
			intro_ramtilemap[26][col]=0x91;
	}

	// diff speed, random y
	for (int i=0;i<NB_COINS;i++)
	{
		coins[i]->x+=3;
		if (coins[i]->x>=640-16)
			coins[i]->x = 0;

		if (vga_frame%6==0) 
		{
			coins[i]->fr = (coins[i]->fr+1)%COINS_FRAMES;
		}
	}

	if (GAMEPAD_PRESSED(0,start)) intro_out();
}

const int deltay[8] = {-2,1,4,2,-1,3,1,0}; // spread coins vertically

void intro_init(int score, int nbcoins)
{
	cursor = 0;
	speed = 0;

	code[0]=code[1]=code[2]=code[3]=0;
	
	if (score>best_score) best_score = score;
	if (nbcoins>best_coins) best_coins = nbcoins;

	bg = tilemap_new(
		jumper3_tset,640,480, // same tileset
		TMAP_HEADER(TMAP_INTRO_W,TMAP_INTRO_H,TSET_16,TMAP_U16), 
		intro_ramtilemap
		);

	bg->y = -480; // offline
	// rect noir apres/dessous

	start_frame=vga_frame;

	for (int i=0;i<NB_COINS;i++)
	{
		coins[i] = sprite_new(piece_spr, i*640/NB_COINS, 300+deltay[i], 0);
		coins[i]->fr = i%COINS_FRAMES; // nb frames ?
	}

	// intro screen
	tmap_blit(bg,0,0,intro_tmap_header,intro_tmap_tmap);

	do_frame = intro_frame;
}

void intro_out()
{
	// de-allocate objects
	for (int i=0; i<NB_COINS; i++) 
		blitter_remove(coins[i]);
	blitter_remove(bg);
	
	int level_code = code[0]<<24 | code[1]<<16 | code[2]<<8 | code[3]; 
	level_init(level_code); // start game after 3 seconds !	
}
