#pragma once
#include <stdint.h>
#include <stddef.h>

#include <bitbox.h>

void *memcpy2 (void *dst, const void *src, size_t count); // included as memcpy in a later newlib version

typedef struct __attribute__ ((__packed__))  {
	unsigned int skip:7; // in pixels
	unsigned int data:7; // in pixels
	unsigned int rfu:1; // constant ?
	unsigned int eol:1; // end of line
} Blit;

_Static_assert(sizeof(Blit) == 2, "Blits must be 16 bits wide");

typedef struct Sprite
{
	// set by external logic
	int x, y, z, frame;

	// internal blitting data, used by blitting engine, reset each frame
	unsigned int current_blit;
	unsigned int current_data_index; // XXX real ptr ?
	unsigned int end_blit; // index of end blit of this frame (where to stop). dont blit this one.

	// const data
	int w,h;
	uint16_t *pixel_data;
	Blit * blits;

	int nb_frames;	
	int (*frames)[][2]; // pointer to array of couples
} Sprite;

inline void blit_frame(Sprite *s) // or others ?
{
	// skip to current blit
	if (s->frame==0) 
	{	
		// rewind to start
		s->current_blit = 0;
		s->current_data_index=0;
	} else {
		// start of this blit is the end of the preceding one
		s->current_blit=(*s->frames)[s->frame-1][0];
		s->current_data_index=(*s->frames)[s->frame-1][1];
	}  

	s->end_blit = (*s->frames)[s->frame][0];
}

inline void blit_line(Sprite *s)
{
	// use global line, draw_buffer
	Blit blit;

	if (vga_line<s->y) return;
	if (s->end_blit==s->current_blit) return; 
	// done - current blit will be stuck to the rest of the frame. XXX remove from blitlist

	unsigned int blit_ptr = s->x;
	do {
		blit = s->blits[s->current_blit++];
		blit_ptr+=blit.skip;

		// copy bytes. could do better : call, post update, alignment, IF instr blit
		memcpy2(&draw_buffer[blit_ptr], &s->pixel_data[s->current_data_index], 2*blit.data);
		blit_ptr += blit.data;
		s->current_data_index += blit.data;
	} while (!blit.eol);
}