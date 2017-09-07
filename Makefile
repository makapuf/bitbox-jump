NAME = jumper

GAME_C_FILES = main.c intro.c level.c sounds.c jumper3.c bitbox_icon.c lib/blitter/blitter.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites.c lib/sampler/sampler.c 
GAME_BINARY_FILES = jumper3.tmap intro_tmap.tmap cote.tmap jumper3.tset intro_tmap.tset cote.tset piece.spr bonh.spr cursor.spr


include $(BITBOX)/kernel/bitbox.mk
main.c: jumper3.tmap intro_tmap.tmap cote.tmap piece.spr bonh.spr cursor.spr

%.tset %.tmap %.c %.h: %.tmx
	python tmx.py $< > $*.h

piece.spr : piece/p_?.png
	python $(BITBOX)/lib/blitter/scripts/couples_encode.py $@ $^

bonh.spr : bonh/b_??.png
	python $(BITBOX)/lib/blitter/scripts/couples_encode.py $@ $(sort $^)

cursor.spr : cursor.png
	python $(BITBOX)/lib/blitter/scripts/couples_encode.py $@ $^

sounds.c: sounds/*.wav mk_sounds.py
	python mk_sounds.py $< > $@

bitbox_icon.c: icon.png
	python $(BITBOX)/2nd_boot/mk_ico.py $^ > $@

clean::
	rm -f *.tset *.tmap *.spr
	rm -f jumper3.[ch] intro_tmap.[ch] cote.[ch] bitbox_icon.c