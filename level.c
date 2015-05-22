// level.c
// TODO : sons, ramtile cote, musique, avance tt seul, pb de super saut imprte quand, core si rejoue, best score ?
// si meurt, anim parabolique ?

#include <bitbox.h>
#include "blitter.h"

#include <string.h>

#include "jumper3.h"
#include "cote.h"

extern char bonh_spr[];
extern char piece_spr[];
extern char cursor_spr[];


extern char * sample_dead, sample_jump, sample_high, sample_low, sample_color, sample_coin;

#define DEAD_FREEZE 60*2 // frozen frames after dead
#define PIECE_NBFRAMES 4

extern void (*do_frame)(void);

unsigned int allowed_colors; // mask of allowed colors
/*
	extern uint16_t *tilemap;
	extern int tilemap_w,tilemap_h, tilemap_x, tilemap_y;
*/
uint16_t tilemap_cote_ram[300];

void audio_start_sample(void *toto) {} // XXX

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
enum TileType { 
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
//	tile_start_autodown ... 
};

enum Color { black, red, green, blue };


float vy; // bonh.y is the float value of the height. transferred to the sprite as is each frame
int color=black; // current player color
int anim_start_frame;
int dead; // 0 if not dead, nb frames lmeft freezing if dead

int autoscroll; 

void intro_init(int,int);

#define NB_COINS 16

struct object *coins[NB_COINS], *bg, *cote, *bonh, *cursor;

int next_coin;
int coins_collected;

void handle_gamepad()
{
	// update with keyboard input
	if (GAMEPAD_PRESSED(0,left) && bonh->x>HORIZONTAL_SPEED)
		bonh->x -= HORIZONTAL_SPEED;
	
	if (GAMEPAD_PRESSED(0,right) && bonh->x<bg->w-bonh->w) 
		bonh->x += HORIZONTAL_SPEED;

	// switch to color
	if (GAMEPAD_PRESSED(0,A)  && (allowed_colors & (1<<blue))) 
		color = blue;

	if (GAMEPAD_PRESSED(0,B) && (allowed_colors & (1<<red))) 
		color = red;

	if (GAMEPAD_PRESSED(0,X) && (allowed_colors & (1<<green))) 
		color = green;

	if (GAMEPAD_PRESSED(0,Y))  // retour au noir tjs possible
		color = black;

	#ifdef EMULATED
	if (GAMEPAD_PRESSED(0,A))
		printf("bg->y = %d\n",bg->y);
	#endif 
}

void update_score(void)
{
		// draws best score
		int n=bg->h-480+bg->y;
		int col=7;
		do {
			tilemap_cote_ram[20*10+col--]=0x130+n%10+1; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>2;col--) tilemap_cote_ram[20*10+col]=19*4*4+1;

		// coins
		n=coins_collected;
		col=5;
		do {
			tilemap_cote_ram[22*10+col--]=0x130+n%10+1; // last term is needed because tileset is urgh.	
			n /= 10;
		} while (n);

		for (;col>2;col--) tilemap_cote_ram[20*10+col]=19*4*4+1;
}


inline enum TileType tile_type_from_id(uint16_t tile_id)
{
	if (tile_id>=tile_killing*16) 
		return tile_empty;
	else if (1+(tile_id-1)/16 == 6)
		// fakes !
		return tile_empty;
	else 
		return 1+((tile_id-1)/16); 
}


inline enum Color tile_color_from_id(uint16_t tile_id)
{
	if (tile_id>tile_killing*16) 
		return black; // whatever, they're empty
	else {
		//printf("tile %d col %d\n", tile_id,(tile_id%16)/4 );
		return ((tile_id-1)%16)/4;
	}
}

static void update_positions()
{

	vy += DECELERATION; 

	if (vy>MAX_SPEED) vy = MAX_SPEED;
	bonh->y += vy; // float to int

	// change player frame according to color & status
	if ((vga_frame-anim_start_frame)<5*ANIM_SPEED) { 
		bonh->fr=color*6+5-(vga_frame-anim_start_frame)/ANIM_SPEED;
	} else { // fin de l'animation
		bonh->fr=color*6+1;
	}

	// autoscroll
	if (autoscroll && vga_frame%4 == 0)  {
		bg->y += 1; 
		for (int i=0;i<NB_COINS;i++) 
			if (coins[i]->y<4096) 
				coins[i]->y++;
	}

	// scroll background and coins down accordingly + descroll user
	if (bonh->y < max_player_height)
	{
		bg->y += max_player_height-bonh->y;
		
		for (int i=0;i<NB_COINS;i++)
		{
			coins[i]->y += max_player_height-bonh->y;
			// remove past coins
			if (coins[i]->y>lower_bound_y)
				coins[i]->y = 4096;
		}

		bonh->y = max_player_height;
	}


	// touche pieces
	for (int i=0;i<NB_COINS;i++)
	{
		if (coins[i]->x>=bonh->x &&  // no need to check if active, already in this check
			coins[i]->x<=bonh->x+bonh->w && 
			coins[i]->y>=bonh->y && 
			coins[i]->y<=bonh->y+bonh->h)
		{
			// son ting
			audio_start_sample(&sample_coin);
			coins_collected ++;
			coins[i]->y = 4096; // remove it
		}
	}

	//  setup number display with coins / score
	update_score();

	// touche bas
	if (bonh->y+bonh->h-player_frame_base[bonh->fr%player_nb_frames] > lower_bound_y)
	{
		// lost
		bonh->fr = 24; // dead
		dead=DEAD_FREEZE;
		audio_start_sample(&sample_dead);
	}

	// non zero if modify speed (means we jumped this frame)
	float newv=0; 
	
	// where is the base of the guy vertically relatively to the background, in pixels
	int pos_y=bonh->y+bonh->h-bg->y-player_frame_base[bonh->fr%player_nb_frames]; 

	// touch a tile : from up or down ?
	if (vy>0 && pos_y/16 != prev_player_tile) // going down and through a cell top
	{
		int dest_tile = jumper3_tmap[0][pos_y/16*bg->w/16+(bonh->x+bonh->w/2)/16]; // get tile ref from id 
		int tilecolor = tile_color_from_id(dest_tile);
		switch(tile_type_from_id(dest_tile))
		{
			case tile_empty : 
				break;
			case tile_standard : 
				audio_start_sample(&sample_jump);
				if (tilecolor==black || tilecolor == color) newv=-BUMP;
				break;

			case tile_superjump : 
				audio_start_sample(&sample_high);
				newv = -BUMP;
				if (tilecolor==black || tilecolor == color) newv=-3*BUMP/2; // higher only if same color
				break;

			case tile_light: 
				audio_start_sample(&sample_low);
				if (tilecolor==black || tilecolor == color) newv=-6*BUMP/8;
				break;
			
			case tile_new_color:
				audio_start_sample(&sample_color);
				allowed_colors |= (1<<tilecolor);
				color = tilecolor;
				newv=-BUMP;
				break;

			default :
				#ifdef EMULATED
				printf("unknown type : %d\n",tile_type_from_id(dest_tile)); 
				#endif

				newv = -BUMP;
				break;
		}

		// check if we just jumped, if so rebase player / coins, & trigger animation
		if (newv<0) {
			vy=newv;
			bonh->y = (pos_y/16)*16+bg->y-bonh->h-player_frame_base[bonh->fr%player_nb_frames];
			anim_start_frame = vga_frame;
		}
	}
	prev_player_tile = pos_y/16;

	// cursor 

	int dymax = bg->h+480;
	cursor->y = 480-(480*(bg->y+dymax)/(2*dymax)) - cursor->h; // 0-479 progression

}


static void handle_coins()
{

	// add new ones
	while (jumper3_pieces[next_coin][1]+bg->y>=0) {
		for (int i=0;i<NB_COINS;i++)
			if (coins[i]->y==4096) // a free one.
			{
				coins[i]->x = jumper3_pieces[next_coin][0];
				coins[i]->y = jumper3_pieces[next_coin][1]+bg->y-coins[0]->h;
				break; // stop searching
			} // not found OK dont put it.
		next_coin++;
	} 

	// make them turn, rewind them
	for (int i=0;i<NB_COINS;i++) {
		coins[i]->fr = (vga_frame>>3)%PIECE_NBFRAMES;
	}
}

void level_frame() 
{
	if (!dead) {		
		handle_gamepad();
		update_positions();
	} else if (dead-- == 1)	{
		// dead & tile out -> out !

		int score = bg->h-480+bg->y;

		// de-allocate objects
		for (int i=0; i<NB_COINS; i++) 
			blitter_remove(coins[i]);

		blitter_remove(bonh);
		blitter_remove(cote);
		blitter_remove(bg);
		blitter_remove(cursor);

		// 
		intro_init(score, coins_collected);
	} 


	handle_coins();
}

void fast_fwd(int level_code)
{
	// check level_codes 
	switch (level_code+ ('A'<<24|'A'<<16|'A'<<8|'A')) 
	{
		case 'N'<<24|'Y'<<16|'A'<<8|'N' : bg->y = -46209;	break; // NYAN
		case 'M'<<24|'E'<<16|'O'<<8|'W' : bg->y = -2824*16+480-64 ; break;
		case 'C'<<24|'E'<<16|'C'<<8|'E' : bg->y = -2813*16+480-64 ; break;
		case 'S'<<24|'T'<<16|'A'<<8|'R' : bg->y = -2694*16+480-64 ; break;
		case 'C'<<24|'L'<<16|'U'<<8|'D' : bg->y = -2632*16+480-64 ; break;
		case 'B'<<24|'L'<<16|'U'<<8|'E' : bg->y = -2455*16+480-64 ; break;
		case 'G'<<24|'R'<<16|'E'<<8|'G' : bg->y = -2305*16+480-64 ; break;
		
		default : // at start !
			bg->y=-bg->h+480;
			break;
	}
}

void level_init(int level_code)
{
	// create pieces, bonh, cote, bg, progression line (--> replace with small sprite)

	bg   = tilemap_new (jumper3_tset, 0, 0, jumper3_header, jumper3_tmap);
	cote = tilemap_new (cote_tset, 0, 0, cote_header, tilemap_cote_ram);
	memcpy(tilemap_cote_ram,cote_tmap,sizeof(tilemap_cote_ram));
	cote->x = bg->w;

	bonh = sprite_new(bonh_spr, 300, 480-64,0);
	bonh->fr=0;

	cursor = sprite_new(cursor_spr,bg->w-6,0,0);

	color = 0;

    // copy tilemap cote in RAM

	vy = -BUMP;

	fast_fwd(level_code); // sets bg->y

	// init all coins
	for (int i=0;i<NB_COINS;i++) {
		coins[i] = sprite_new(piece_spr, 0,4096,0); // invisible now
	}

	// skip coins
	for (next_coin=0;jumper3_pieces[next_coin][1]+bg->y>480;next_coin++); // skip all passed coins 

	allowed_colors=1;
	coins_collected= 0;
	autoscroll=0;

	do_frame = level_frame;
}

