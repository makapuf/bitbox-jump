'''TODO : 
fast, fuzzy tiling
structure regroupee : x,y,frame, ptr vers pixeldata, nbframes, frameskip, blits

'''
import sys

img_fn = sys.argv[1]
name, nframes=None, None # XXX auto
MAX_SKIPS = 127
MAX_BLITS = 127

# ------------------------------------------------------------------

from PIL import Image

if name==None : 
	basename,ext=img_fn.rsplit('.',1)
	if '/' in basename : 
		name=basename.rsplit('/',1)[1]
	else : 
		name=basename
	if name.endswith('_od') or name.endswith('fs') : 
		name=name[:-3]

	if name.startswith('anim_') :
		name = name.split('anim_',1)[1]
		if  name.endswith('frames') : 
			name, nframes=name.rsplit('_',1)
			nframes = int(nframes.rsplit('frames',1)[0])
		else : 
			nframes = None # will be calculated
	else : 
		nframes = 1


def pixel(r,g,b,a) : 
	if a<128 : return 0
	return 1<<12 | (r*31//255)<<8 | (g*31//255)<<4 | (b*31//255)
def pixel(r,g,b,a) :
    "real bitbox words : A000BBBBGGGGRRRR"
    if a<128 : return 0
    return 1<<12 | int((7+15*b)//255)<<8|int((7+15*g)//255)<<4 | ((7+15*r)//255)
    
def pixel(r,g,b,a) : 
    if a<128 : return 0
    return 1<<15 | (r>>3)<<10 | (g>>3)<<5 | b>>3


img=Image.open(img_fn).convert('RGBA')
imgdata = [pixel(*c) for c in img.getdata()]
img_w, img_h = img.size
if nframes == None :
	# assumes vertical
	nframes = img_h//img_w

frame_h = img_h//nframes
data = []
blits = [] # nb_skip, nb_blits, eol ?
frame_idx = [] # zero terminated list of blit index/data index +last

for y in range(img_h) : 
	imgline_it = iter(imgdata[y*img_w:(y+1)*img_w])
	skips=-1

	# frame indices (not for the first : always 0,0)
	# last frame : data is zero

	if y and y%frame_h==0 : 
		# remove last blank blits
		while len(blits[-1][1])==0 : del blits[-1]
		frame_idx.append((len(blits), sum(len(b[1]) for b in blits)))
	try : 
		while True : 
			blit=[]
			while True :  
				skips += 1
				c=imgline_it.next()
				if c != 0 : break
			while True : 
				blit.append(c)
				c = imgline_it.next()
				if c == 0 : break

			if blit :  
				# insert as many pure skips as needed
				while skips>=MAX_SKIPS : 
					blits.append((MAX_SKIPS,[],False))
					skips -= MAX_SKIPS
				# insert N blits, at most MAX_BLITS bytes
				while len(blit) :
					blits.append((skips, blit[:MAX_BLITS], False))
					skips=0
					blit = blit[MAX_BLITS:]
			skips=0;blit=[]
	except StopIteration : # end of line

		while len(blit) :
			blits.append((skips, blit[:MAX_BLITS], False))
			skips=0
			blit = blit[MAX_BLITS:]

		if blits : 
			b = blits.pop()
			blits.append((b[0],b[1],True))

		# if not blits : 
		# 	# end of line and no blits :  skip the line 
		# 	blits.append((0,[],True))
		# else : # non empty line, modify preceding blit
		# 	b = blits.pop()
		# 	blits.append((b[0],b[1],True))
	# remove as much empty lines at the end as possible


# Stop frame
frame_idx.append((len(blits), 0))
			

# first also ?


# frames -------------------------------------------------------------------------------------------------
tot_size = len(blits)*2+sum(len(b[1]) for b in blits)*2
print '#include "blit.h"'

print "const int %s_frames[%d][2] = {"%(name,len(frame_idx))
for frame,(blit_idx, data_idx) in enumerate(frame_idx) : 
	print '    {%d,%d}, // after frame %d'%(blit_idx, data_idx, frame)
print "};\n"

# blits -------------------------------------------------------------------------------------------------
print "const Blit %s_blits[%d] = { //  (%d bytes)"%(name,len(blits), len(blits)*2)
for b in blits : 
	print '    { .skip=%2d, .data=%2d,.eol=%d},'%(b[0],len(b[1]),1 if b[2] else 0)
print "};\n"

# data -------------------------------------------------------------------------------------------------
print "const uint16_t %s_pixeldata[%d] = {"%(name,sum(len(b[1]) for b in blits))
for b in blits :
	if b[1] : 
		print '    '+','.join('0x%04x'%x for x in b[1])+','
print "};\n"

print "// total size : %d (%d %% of 1M), raw size:%d "%(tot_size, 100*tot_size//1024//1024, img_w*img_h*2)
frame_h = img_h/nframes
print '''Sprite {name} = {{
	.frames = (int (*)[][2]) &{name}_frames,
	.pixel_data=(uint16_t *){name}_pixeldata,
	.nb_frames={nframes},
	.blits=(Blit*){name}_blits,
	.w={img_w},
	.h={frame_h}
}};'''.format(**locals())

# stats -------------------------------------------------------------------------------------------------
if True  :
	from string_tiling import tile
	tiled_data, tiled_idx = tile([b[1] for b in blits], align=1)
	print '// tiled size :', len(blits)*2+len(tiled_data)*2
	print '// 16-bit index cost:',len(blits)*2, 'tiling gain:', 2*(sum(len(b[1]) for b in blits)-len(tiled_data))
import zlib
datastr = ''.join(chr(a>>8)+chr(a&0xff) for a in imgdata)
z=zlib.compress(datastr) # XXX set window size != of 32k 
print '// gzipped ', len(z),'/',tot_size
