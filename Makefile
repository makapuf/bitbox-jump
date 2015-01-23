NAME = jumper

GAME_C_FILES = main.c intro.c level.c
#level.c 
GAME_BINARY_FILES = jumper3.tmap intro.tmap cote.tmap jumper3.tset intro.tset cote.tset piece.spr bonh.spr cursor.spr

USE_ENGINE=1
USE_SAMPLER=1

NO_SDCARD=1

include $(BITBOX)/lib/bitbox.mk
main.c: jumper3.tmap intro.tmap cote.tmap piece.spr bonh.spr cursor.spr

%.tset %.tmap %.h: %.tmx 
	python $(BITBOX)/scripts/tmx.py $? > $*.h

piece.spr : piece/p_?.png
	#python ../scripts/sprite_encode2.py $@ $? -m p4
	python $(BITBOX)/scripts/couples_encode.py $@ $^ 

bonh.spr : bonh/b_??.png
	#python ../scripts/sprite_encode2.py $@ $? -m p4
	python $(BITBOX)/scripts/couples_encode.py $@ $^ 

cursor.spr : cursor.png
	python $(BITBOX)/scripts/couples_encode.py $@ $^ 

# + tard ... 
sounds.c: sounds/*.wav
	python mk_sounds.py $< > $@

clean:: 
	rm -f *.tset *.tmap *.spr