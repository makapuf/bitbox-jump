#include <stdint.h>
#include <stdlib.h> // rand, RAND_MAX
#include <string.h> // memcpy, memset
#include <bitbox.h>
#ifdef EMULATED
#include <stdio.h>
#endif

#include <math.h> 

#include "blit.h"
#include "audio.h"

#include "sprite_mask.h"


#define BLACK 0
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define LOGO_WIDTH 150
#define LOGO_HEIGHT 40
#define LOGO_X (SCREEN_WIDTH-LOGO_WIDTH)/2
#define LOGO_Y (SCREEN_HEIGHT-LOGO_HEIGHT)/2

// next step
void intro_init(int, int);

extern void (*do_frame)(void);
extern void (*do_line)(void);

extern Sample sample_intro;

// define a data buffer (this is NOT the sprite pixel data) and a palettte
uint8_t fx_buffer[LOGO_WIDTH*LOGO_HEIGHT]; // 0-31 values.
uint16_t blue_palette[256]; 

static int start_frame; // when we entered the state (normally zero ?)
static int fx_current; // current blit pointer

inline int randint(int min, int max)
{
	return  min+(long long)(max-min)* rand()/RAND_MAX;
}

// duration of different periods 
#define START 3*60 // animation 
#define FADE 128 // fade out
#define FADED 2*60 // final logo

void update_palette()
{
	if (vga_frame-start_frame < START)
	{} // wait
	else if (vga_frame-start_frame < START+FADE) // 4 frames / decrease ?
	{
		int t=FADE+START-(vga_frame-start_frame); // t = FADE->0
		// fade palette out
		for (int i=0;i<16;i++)  
		{
			blue_palette[i]=(2*i*t/FADE); 
			blue_palette[i+16]=(2*i*t/FADE)*0x20 + 0x1f*(t/FADE); // NOT LINEAR !
		}
	} 
	else if (vga_frame-start_frame< START+FADE+FADED) // 2 seconds wait
	{} // wait
	else {
		// next state
		intro_init(0,0);
	}
}


// ---------------- FIRE LOGO ------------------------------------

#define COAL_HEIGHT 3
#define MIN_COAL_INTENSITY 4
#define MAX_COAL_INTENSITY 8
#define MIN_FLAME_INTENSITY 25
#define MAX_FLAME_INTENSITY 31
#define FLAME_CHANCE 10 // over 100
#define FLAME_WIDTH 20

// fire effect, see http://ionicsolutions.net/2011/12/30/demo-fire-effect/
static void flame()
{
    int p;
    int du, dd, dy, l, r;
 
    for( int x = 0; x < LOGO_WIDTH; x++ )
    {
        l = x == 0 ? 0 : x - 1;
        r = x == LOGO_WIDTH - 1 ? LOGO_WIDTH - 1 : x + 1;
 
        for ( int y = 0; y < LOGO_HEIGHT - 1; y++ )
        {
        	// index of up / down elements, with saturation
            du = (y == 0 ? 0 : y - 1)*LOGO_WIDTH;
            dy = y*LOGO_WIDTH;
            dd = (y == LOGO_HEIGHT - 1 ? LOGO_HEIGHT : y + 1)*LOGO_WIDTH;

            // filtering matrix 
            p  = fx_buffer[ du + l ];
            p += fx_buffer[ du + r ];

            p += fx_buffer[ dy + l ];
            p += fx_buffer[ dy + x ];
            p += fx_buffer[ dy + r ];
 
            p += fx_buffer[ dd + l ];
            p += fx_buffer[ dd + x ];
            p += fx_buffer[ dd + r ];
 
            fx_buffer[ du + x ] = p/8;
        }
    }
}


static void coal()
{
    int coalStart = (LOGO_HEIGHT - COAL_HEIGHT - 1)*LOGO_WIDTH;
    int coalEnd = LOGO_HEIGHT*LOGO_WIDTH;
 
    for (int position = coalStart; position<coalEnd; position++)
    {
        fx_buffer[ position ] = randint( MIN_COAL_INTENSITY, MAX_COAL_INTENSITY );
    }
 
 	for (int position=coalStart;position<coalEnd;position++) // avance de ttes façons, hors flamme
 	{
        if( randint(0,100)< FLAME_CHANCE )
        {
            int flameWidth = randint(1,FLAME_WIDTH);
            uint8_t flameIntensity = randint( MIN_FLAME_INTENSITY, MAX_FLAME_INTENSITY );
            for (int i=0;i<flameWidth && position<coalEnd;i++)
            {
	            fx_buffer[position++] = flameIntensity;
      		}
        }
    }
}

void flame_frame()
{
	coal();
	flame();

	update_palette();

	// reset blit index
	blit_frame(&logo_mask);
	fx_current=0;
	
}
// - NOISE LOGO ---------------------------------------------------------------------
#define NOISE_NB_POINTS 20
#define NOISE_BG 3
#define NOISE_VAL 25
void fade_out()
{
	for (int i=0;i<LOGO_WIDTH*LOGO_HEIGHT-1;i++)
	{
		fx_buffer[i]= fx_buffer[i]>NOISE_BG?fx_buffer[i]-1:NOISE_BG;		
	}
}

void noise()
{
	for (int i=0;i<NOISE_NB_POINTS;i++)
	{
		fx_buffer[randint(0,LOGO_WIDTH*LOGO_HEIGHT)]=NOISE_VAL;
	}
}

void noise_frame()
{
	fade_out();
	// essayer flame ? blur ?
	noise();
	update_palette();
	// reset blit index
	blit_frame(&logo_mask);
	fx_current=0;
}

// STARS LOGO ----------------------------------------
#define STARS_NB 350
#define STARS_BG 3
#define STARS_VALUE 22
#define STARS_SPREAD_X 2500
#define STARS_SPREAD_Y 800
#define STARS_SPREAD_Z 100
#define STARS_SPEED 0.125f

int stars[STARS_NB][3]; // x,y,z
// release ? dynamic alloc ?

void stars_frame()
{
	// clear stars
	// optimize : clear stars individually
	//memset(fx_buffer,STARS_BG,LOGO_WIDTH*LOGO_HEIGHT); 
	for (int i=0;i<LOGO_WIDTH*LOGO_HEIGHT;i++)
		if (fx_buffer[i]>4) fx_buffer[i] -=4;
	// trainée plutot ? (genre divise / deux a chaque fois)

	for (int i=0;i<STARS_NB;i++)
	{
		// move 
		stars[i][2] -= STARS_SPEED;
		if (stars[i][2]<=0) {
			// reset star
			stars[i][0]=randint(0,STARS_SPREAD_X)-STARS_SPREAD_X/2;
			stars[i][1]=randint(0,STARS_SPREAD_Y)-STARS_SPREAD_Y/2;
			stars[i][2]=randint(1,STARS_SPREAD_Z);
		}
		// display star
		int x = ( ( stars[i][0] / stars[i][2] ) + LOGO_WIDTH/2 ); // x/z + center
		int y = ( ( stars[i][1] / stars[i][2] ) + LOGO_HEIGHT/2 ); // y/z + center

		if (x>=0 && x<LOGO_WIDTH && y>=0 && y<LOGO_HEIGHT)
			fx_buffer[y*LOGO_WIDTH+x]=STARS_VALUE;
	}
	

	// reset
	fx_current = 0;
	update_palette();

	blit_frame(&logo_mask);
}
// ------------ waves


void wave_frame()
{
	for (int j=0;j<LOGO_HEIGHT;j++)
		for (int i=0;i<LOGO_WIDTH;i++)
		{
			float x=(float)(i-LOGO_WIDTH/2)/15.f ;
			float y=(float)(j-LOGO_HEIGHT/2)/15.f;
			fx_buffer[j*LOGO_WIDTH+i] = (uint8_t)(16+2.5*(x+y)*cos(x*x+y*y-vga_frame*.2f));
		}
	fx_current=0;
	blit_frame(&logo_mask);
	update_palette();
}

// ----------------------------------------------------------------------


void logo_line()
{
	// fill black start/end of line
	if (vga_line==0 || vga_line == 1 || vga_line==LOGO_Y+LOGO_HEIGHT|| vga_line==LOGO_Y+LOGO_HEIGHT+1) 
	// borders never overwritten after
	{
		memset(draw_buffer, BLACK, SCREEN_WIDTH*2+32); // was draw_buffer+margin
	}
	else if (vga_line>LOGO_Y+1 && vga_line<LOGO_Y+LOGO_HEIGHT-1) 
	// dont blit first line && last, will be overwritten anyways
	{
		// output line with palette. 
		for (int i=0;i<LOGO_WIDTH;i++)
			draw_buffer[LOGO_X+i] = blue_palette[fx_buffer[fx_current++]];
		// blit mask over it
	}
	blit_line(&logo_mask);
}

void logo_init()
{ 
	audio_start_sample(&sample_intro);

	// gen simple blue palette  0->31 black-blue-white
	for (int i=0;i<16;i++)  {
		blue_palette[i]= i<<1; 
		blue_palette[i+16]=0x1f + i*0x20; 
	}

	// place sprite
	logo_mask.x = LOGO_X;
	logo_mask.y = LOGO_Y;

	// reset buffer
	memset(fx_buffer,BLACK,LOGO_WIDTH*LOGO_HEIGHT);
	
	start_frame=vga_frame;
	switch(0) // randomize : from time ? randint(0,3)
	{
		case 0 : do_frame=flame_frame; break;
		case 1 : do_frame=noise_frame; break;
		case 2 : do_frame=stars_frame; break;
		case 3 : do_frame=wave_frame; break;
	}
	do_line = logo_line;


}
