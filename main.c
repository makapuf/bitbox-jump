/* TODO : 
2 points de contact
sons : rebond (1/brique : normal, haut, bas, chgt couleur,), depart, miaow si atteint chat.

pieces (ds le jeu) : flash array of x,y ; virer si trop bas, virer si mangées, créer si entre.
logo KO : couleur, ..
intro : plus joli

reset KO

titre RLE plein (blits simples)

niveau : debut tres simple
forcer le nyan (empecher de remonter plus)

pieges : mauvais trucs (pointilles, ...)
forcer a utiliser les touches  pour passer

sons : 2-mixer au moins


TMX : exporter object files (faire simple)
exporter sprites ? 
exporter externs bins, yc ds ss reps !

*/
#include "bitbox.h"
#include "blitter.h" 

void intro_init();
void (*do_frame) (void); // pointer to frame action/level / ... 

// --------------------------- main elements

void game_init()
{
	blitter_init();
	intro_init(); // start
}

void game_frame()
{
	kbd_emulate_gamepad();
	if (do_frame) 
		do_frame();
}

