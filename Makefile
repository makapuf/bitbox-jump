NAME = jumper

TILEMAP = jumper3.tmx intro.tmx cote.tmx
TILESET = tiles2.png
GAME_C_FILES = main.c intro.c logo.c tilemaps.c level.c sounds.c
SPRITES_PLAYER = anim_bonh_25frames.png
SPRITES_COIN = anim_piece_4frames.png
SPRITE_MASK = logo_mask.png

include lib/bitbox.mk

sounds.c: sounds/*.wav
	python mk_sounds.py $< > $@

tilemaps.h tilemaps.c: $(TILEMAP) mk_tilemap.py $(TILESET)
	python mk_tilemap.py $(TILEMAP)

sprite_player.h: mk_sprite.py $(SPRITES_PLAYER)
	python mk_sprite.py $(SPRITES_PLAYER) > sprite_player.h

sprite_mask.h: mk_sprite.py $(SPRITE_MASK)
	python mk_sprite.py $(SPRITE_MASK) > sprite_mask.h

sprite_coins.h: mk_sprite.py $(SPRITES_COIN)
	python mk_sprite.py $(SPRITES_COIN) > sprite_coins.h

