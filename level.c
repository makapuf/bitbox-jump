// level.c

#include <bitbox.h>

#include "audio.h"

#include "blit.h"
#include "tilemaps.h"

#ifdef EMULATED
#include "stdio.h"
#endif 
#include <string.h>

extern Sample sample_dead, sample_jump, sample_high, sample_low, sample_color, sample_coin;

extern Sprite bonh, piece;
#define DEAD_FREEZE 60*2 // frozen frames after dead

void draw_bg();
extern void (*do_frame)(void);
extern void (*do_line)(void);

unsigned int allowed_colors=1; // mask of allowed colors
extern uint16_t *tilemap;
extern int tilemap_w,tilemap_h, tilemap_x, tilemap_y;

uint16_t tilemap_cote_ram[300];


const float DECELERATION = 0.3f; // pixel per frame^2 ? positive because y axis is down  
const int HORIZONTAL_SPEED = 2; // pixel per frame^2 ? 
const int max_player_height =480/3; // scroll bg up at this height
const int MAX_SPEED= 15.0f; // prevents going through a tile
const int lower_bound_y= 480;
const int BUMP=7.f; // pixels per frame initially on a standard jump
const int ANIM_SPEED=8;

// top of the drawing on this frame (modulo )
const int player_nb_frames = 6; // real frames
const int player_frame_base[] = {5,7,16,20,26,8};

int prev_player_tile; // previous frame player 

// le joueur demarre noir = match all. Gagne des couleurs au fur et a mesure 
typedef enum TileType { 
	tile_empty=0,   // don't block or anything
	tile_standard,  // standard, colored or invisible. color match = jump/score, else : jump petit
	tile_superjump, // color match : super+, else : normal
	tile_light,     // color match : jump, else : don't
	tile_blocking,  // color match : passe, sinon bloque
	tile_new_color, // add a color to possible colors
	tile_restart_point, // color match: yes, else : no!
	tile_left,      // color : normal, sinon : envoie a G
	tile_right, 	   // color : normal, sinon : envoie a D
	tile_killing   // color : normal, sinon : tue !	
//	tile_crumbling, + tard : necessite une VRAM
//	tile_faked : no need, just different graphically
//	tile_invisible : 
} TileType;

typedef enum Color { black, red, green, blue } Color;

int bg_y;

float vy; // bonh.y is the float value of the height. transferred to the sprite as is each frame
int color=black; // current player color
int anim_start_frame;
int dead; // 0 if not dead, nb frames lmeft freezing if dead

#define nb_coins 16
Sprite coins[nb_coins];void intro_init(int,int);
int next_coin;
int coins_collected;

void handle_gamepad()
{
	// update with keyboard input
	if (GAMEPAD_PRESSED(0,left) && bonh.x>HORIZONTAL_SPEED){
		bonh.x -= HORIZONTAL_SPEED;
	}
	if (GAMEPAD_PRESSED(0,right) && bonh.x<16*tilemap_fond_w-bonh.w) 
		bonh.x += HORIZONTAL_SPEED;

	// switch to color
	if (GAMEPAD_PRESSED(0,A)  && (allowed_colors & (1<<blue))) 
	{
		color = blue;
	}
	if (GAMEPAD_PRESSED(0,B) && (allowed_colors & (1<<red))) 
	{
		color = red;
	}
	if (GAMEPAD_PRESSED(0,X) && (allowed_colors & (1<<green))) 
	{
		color = green;
	}
	if (GAMEPAD_PRESSED(0,Y)) 
	{ // retour au noir tjs possible
		color = black;
	}
	#ifdef EMULATED
	if (GAMEPAD_PRESSED(0,A))
		printf("bg_y = %d\n",bg_y);
	#endif 
}

inline TileType tile_type_from_id(uint16_t tile_id)
{
	if (tile_id>=tile_killing*16) 
		return tile_empty;
	else if (1+tile_id/16 == 6)
		// fakes !
		return tile_empty;
	else 
		return 1+(tile_id/16); 
}

void update_score(void)
{
		// draws best score
		int n=tilemap_fond_h*16-480+bg_y;
		int col=7;
		do {
			tilemap_cote_ram[20*10+col--]=0x130+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>2;col--) tilemap_cote_ram[20*10+col]=19*4*4;

		// coins
		n=coins_collected;
		col=5;
		do {
			tilemap_cote_ram[22*10+col--]=0x130+n%10; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>2;col--) tilemap_cote_ram[20*10+col]=19*4*4;
}

inline Color tile_color_from_id(uint16_t tile_id)
{
	if (tile_id>=tile_killing*16) 
		return black; // whatever, they're empty')
	else {
		//printf("tile %d col %d\n", tile_id,(tile_id%16)/4 );
		return (tile_id%16)/4;
	}
}

static void update_positions()
{

	vy += DECELERATION; 

	if (vy>MAX_SPEED) vy = MAX_SPEED;
	bonh.y += vy; // float to int


	// change player frame according to color & status
	if ((vga_frame-anim_start_frame)<5*ANIM_SPEED)
	{ 
		bonh.frame=color*6+5-(vga_frame-anim_start_frame)/ANIM_SPEED;
	}
	else // fin de l'animation
	{
		bonh.frame=color*6+1;
	}


	// scroll background and coins down accordingly + descroll user
	if (bonh.y < max_player_height)
	{
		bg_y += max_player_height-bonh.y;
		for (int i=0;i<nb_coins;i++)
		{
			coins[i].y += max_player_height-bonh.y;
			// remove past coins
			if (coins[i].y>lower_bound_y)
				coins[i].x = -1;
		}
		bonh.y = max_player_height;

	}


	// touche pieces
	for (int i=0;i<nb_coins;i++)
	{
		if (coins[i].x>=bonh.x &&  // no need to check if active, already in this check
			coins[i].x<=bonh.x+bonh.w && 
			coins[i].y>=bonh.y && 
			coins[i].y<=bonh.y+bonh.h)
		{
			// son ting
			audio_start_sample(&sample_coin);
			coins_collected ++;
			coins[i].x = -1; // remove it
		}
	}

	//  setup number display with coins / score
	update_score();

	// touche bas
	if (bonh.y+bonh.h-player_frame_base[bonh.frame%player_nb_frames] > lower_bound_y)
	{
		// lost
		bonh.frame = 24; // dead
		dead=DEAD_FREEZE;
		audio_start_sample(&sample_dead);
	}

	// where is the base of the guy vertically relatively to the background, in pixels
	int pos_y=bonh.y+bonh.h-bg_y-player_frame_base[bonh.frame%player_nb_frames]; 

	// touch a tile : from up or down ?
	if (vy>0 && pos_y/16 != prev_player_tile) // going down and through a cell top
	{
		//printf("posy:%d vy:%d posy-vy:%d; %d->%d\n", pos_y, (int)vy,pos_y-(int)vy,pos_y/16,(pos_y-(int)vy)/16 );
		int dest_tile = tilemap_fond[pos_y/16*tilemap_fond_w+(bonh.x+bonh.w/2)/16]; // get tile ref from id 
		int tilecolor = tile_color_from_id(dest_tile);
		switch(tile_type_from_id(dest_tile))
		{
			case tile_empty : break;
			case tile_standard : 
				audio_start_sample(&sample_jump);
				if (tilecolor==black || tilecolor == color) vy=-BUMP;
				break;
			case tile_superjump : 
				audio_start_sample(&sample_high);
				vy=-BUMP;
				if (tilecolor==black || tilecolor == color) vy-=BUMP/2; // higher only if same color
				break;
			case tile_light: 
				audio_start_sample(&sample_low);
				if (tilecolor==black || tilecolor == color) vy=-5*BUMP/8;
				break;
			
			case tile_new_color:
				audio_start_sample(&sample_color);
				allowed_colors |= (1<<tilecolor);
				color = tilecolor;
				vy=-BUMP;
				break;

			default :
				#ifdef EMULATED
				printf("unknown type : %d\n",tile_type_from_id(dest_tile)); 
				#endif

				vy = -BUMP;
				break;
		}
		// check if we just jumped, if so rebase player / coins, & trigger animation
		if (vy<0) 
		{
			bonh.y = (pos_y/16)*16+bg_y-bonh.h-player_frame_base[bonh.frame%player_nb_frames];
			anim_start_frame = vga_frame;
		}
	}
	prev_player_tile = pos_y/16;
}



static void handle_coins()
{

	// add new ones
	while (pieces_pos[next_coin][1]+bg_y>=0)
	{
		for (int i=0;i<nb_coins;i++)
			if (coins[i].x<0) // a free one.
			{
				coins[i].x = pieces_pos[next_coin][0];
				coins[i].y = pieces_pos[next_coin][1]+bg_y-piece.h;
				break; // stop searching
			} // not found OK dont put it.
		next_coin++;
	} 

	// make them turn, rewind them
	for (int i=0;i<nb_coins;i++)
	{
		coins[i].frame = (vga_frame>>3)%piece.nb_frames;
	}


}


void level_frame() 
{
	if (!dead)
	{		
		handle_gamepad();
		update_positions();
	} 
	else if (dead-- == 1)
	{
		// dead & tile out
		int score = tilemap_fond_h*16-480+bg_y;
		intro_init(score, coins_collected);
	} 

	handle_coins();

	// prepare draws
	blit_frame(&bonh);
	for (int i=0;i<nb_coins;i++) 
		if (coins[i].x>=0) 
			blit_frame(&coins[i]);
}

void level_line()
{
	// fond
	tilemap_y = bg_y;
	tilemap_x = 0;
	tilemap = (uint16_t *)tilemap_fond; // discard const
	tilemap_h = tilemap_fond_h;
	tilemap_w = tilemap_fond_w;
	draw_bg();

	// now blit cote
	tilemap = (uint16_t *)tilemap_cote_ram; 
	tilemap_x = 16*tilemap_fond_w;
	tilemap_y = 0;
	tilemap_h = tilemap_cote_h;
	tilemap_w = tilemap_cote_w;
	draw_bg();

	// blit barre progression cote (en fn de line)
	int dymax = tilemap_fond_h*16+480;
	int height = 480*(bg_y+dymax)/(2*dymax); // 0-479 de progression
	draw_buffer[tilemap_fond_w*16]=(vga_line<=480-height)?RGB(0,0,128):RGB(0,255,255);
	draw_buffer[tilemap_fond_w*16+1]=(vga_line<=480-height)?RGB(0,0,128):RGB(0,255,255);

	// draw active coins 
	for (int i=0;i<nb_coins;i++)
		if (coins[i].x>=0)
			blit_line(&coins[i]);

	blit_line(&bonh); // draws the player

}


void level_init(int level_code)
{
	#ifdef EMULATED
	printf("level code : %x\n",level_code);
	#endif 


    // copy tilemap cote in RAM
	memcpy(tilemap_cote_ram,tilemap_cote,sizeof(tilemap_cote_ram));


	tilemap_w=tilemap_fond_w;
	tilemap_h=tilemap_fond_h;
	tilemap=(uint16_t *) &tilemap_fond[0]; // discards const

	bonh.x=300;
	bonh.y=480-64;
	bonh.frame=0;
	color = 0;
	
	vy = -BUMP;


	// check level_codes 
	switch (level_code+ ('A'<<24|'A'<<16|'A'<<8|'A')) 
	{
		case 'N'<<24|'Y'<<16|'A'<<8|'N' : bg_y = -46209;	break; // NYAN
		case 'M'<<24|'E'<<16|'O'<<8|'W' : bg_y = -2824*16+480-64 ; break;
		case 'C'<<24|'E'<<16|'C'<<8|'E' : bg_y = -2813*16+480-64 ; break;
		case 'S'<<24|'T'<<16|'A'<<8|'R' : bg_y = -2694*16+480-64 ; break;
		case 'C'<<24|'L'<<16|'U'<<8|'D' : bg_y = -2632*16+480-64 ; break;
		case 'B'<<24|'L'<<16|'U'<<8|'E' : bg_y = -2455*16+480-64 ; break;
		case 'G'<<24|'R'<<16|'E'<<8|'G' : bg_y = -2305*16+480-64 ; break;
		
		
		default : // at start !
			bg_y=-tilemap_fond_h*16+480;
			break;
	}

	// init all coins
	for (int i=0;i<nb_coins;i++) 
	{
		coins[i] = piece;
		coins[i].x = -1;
	}
	// skip coins
	for (next_coin=0;pieces_pos[next_coin][1]+bg_y>480;next_coin++); // skip all passed coins 

	allowed_colors=1;
	coins_collected= 0;

	do_frame = level_frame;
	do_line = level_line;
}

