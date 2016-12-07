/* TODO : 

	2 points de contact
	RLE level / heatshrink continu
	forcer le nyan (empecher de remonter plus)

	sons, level %
	fix clean
	auto scroll
	mechantes pieces (extra param ?)
	
	intro music ?

*/
#include "bitbox.h"
#include "lib/blitter/blitter.h" 

void intro_init();
void (*do_frame) (void); // pointer to frame action/level / ... 

// --------------------------- main elements

void game_init()
{
	blitter_init();
	intro_init(0,0); // start
}

void game_frame()
{
	if (do_frame) 
		do_frame();
}

