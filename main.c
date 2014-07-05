/* TODO : 
2 points de contact
sons : rebond (1/brique : normal, haut, bas, chgt couleur,), depart, miaow si atteint chat.

pieces (ds le jeu) : flash array of x,y ; virer si trop bas, virer si mangées, créer si entre.
logo KO : couleur, ..
intro : plus joli

menu : credits
top line
reset KO
cote  :  score, scrolling cat messages (trigger = height)
calculer bas du sprite avec hauteur du sprite !
pb entree score touches -

titre RLE plein (blits simples)

niveau : debut tres simple
permettre de jouer avec les elements avant d'y aller
forcer le nyan (empecher de remonter plus)

pieges : mauvais trucs (pointilles, ...)
forcer a utiliser les touches  pour passer

sons : 2-mixer au moins

*/

#include "sprite_player.h"
#include "sprite_coins.h"

#ifdef EMULATED
#include "stdio.h"
#endif 

#include <string.h> // memset

#include "audio.h" // XXX !

#include "tilemaps.h"

#define BLACK 0

// external
void logo_init();
void intro_init();

uint16_t *tilemap;
int tilemap_w,tilemap_h;
int tilemap_x,tilemap_y;

void (*do_frame) (void); // pointer to frame action/level / ... 
void (*do_line) (void); // pointer to frame action/level / ... 

void audio_start_sample(Sample *s) {}


extern uint16_t *draw_buffer;

void draw_bg() // NO CLIPPING, replace with blit_tilemap
{
	int bgline = vga_line-tilemap_y;

	if (bgline<0 || bgline>=tilemap_h*16)
	    memset(draw_buffer,BLACK, tilemap_w*16*2); 
	else
	{
		for (int i=0;i<tilemap_w; i++)
		{
			int tile_id=tilemap[(bgline/16)*tilemap_w+i];
			uint32_t *dst= (uint32_t*)&draw_buffer[tilemap_x+i*16]; // was MARGIN+i*16
			const uint32_t *src= (uint32_t*)&tile_data[tile_id][bgline%16*16];
			for (int j=0;j<8;j++) // 16 pixels in 8 words
			{
				*dst++=*src++;
			}
		}
	}
}
	
	

// --------------------------- main elements

// snd : ensuite.


void game_init()
{
	logo_init(); // start
}

void game_line()
{
	if (do_line) do_line();
}

void game_frame()
{
	if (do_frame) do_frame();
}

void game_snd_buffer(uint16_t *buffer, int len) {};