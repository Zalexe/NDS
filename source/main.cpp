 



//Este juego esta hecho por Carlos Marruedo

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <nds.h>

//include pcx file (auto generated)
#include "ball_pcx.h"
#include "cielo_pcx.h"
#include "bouncer_pcx.h"
#include "playa_pcx.h"

#define NUM_SPRITES 1

SpriteEntry OAMCopySub[128];

//simple sprite 
typedef struct {
	int x,y;			// screen co-ordinates
	int dx, dy;			// velocity
	SpriteEntry* oam;	// pointer to the sprite attributes in OAM
	int gfxID; 			// graphics lovation
}Sprite;
typedef struct {
	int x, y;			// screen co-ordinates		// graphics lovation
}Point;

Point pointsList[100];


//---------------------------------------------------------------------------------
void MoveSprite(Sprite* sp) {
//---------------------------------------------------------------------------------
	int x = sp->x >> 8;
	int y = sp->y >> 8;

	sp->oam->x = x;
	sp->oam->y = y;

}



//---------------------------------------------------------------------------------
void initOAM(void) {
//---------------------------------------------------------------------------------
	int i;

	for(i = 0; i < 128; i++) {
		OAMCopySub[i].attribute[0] = ATTR0_DISABLED;
	}
}

//---------------------------------------------------------------------------------
void updateOAM(void) {
//---------------------------------------------------------------------------------
	memcpy(OAM_SUB, OAMCopySub, 128 * sizeof(SpriteEntry));
}


/* HSV to RGB conversion function with only integer
 * math */
//---------------------------------------------------------------------------------
uint16_t hsl2rgb(unsigned char hue, unsigned char sat, unsigned char lum) {
//---------------------------------------------------------------------------------
	int v;

	v = (lum < 128) ? (lum * (256 + sat)) >> 8 :
		(((lum + sat) << 8) - lum * sat) >> 8;
    if (v <= 0) {
		return RGB8(0,0,0);
	} else {
		int m;
		int sextant;
		int fract, vsf, mid1, mid2;

		m = lum + lum - v;
		hue *= 6;
		sextant = hue >> 8;
		fract = hue - (sextant << 8);
		vsf = v * fract * (v - m) / v >> 8;
		mid1 = m + vsf;
		mid2 = v - vsf;
		switch (sextant) {
			case 0: return RGB8(v,mid1,m); break;
			case 1: return RGB8(mid2,v,m); break;
			case 2: return RGB8(m,v,mid1); break;
			case 3: return RGB8(m,mid2,v); break;
			case 4: return RGB8(mid1,m,v); break;
			default: return RGB8(v,m,mid2); break;
		}
	}
}

#define WIDTH 256
#define HEIGHT 196


int x, y, w = WIDTH, h = HEIGHT;

Sprite sprites[NUM_SPRITES];

int i, delta = 0;
int ix = 0;
int iy = 0;

touchPosition touch;

void init() {


	//uint16* map0 = (uint16*)SCREEN_BASE_BLOCK_SUB(1);
	//uint16* map1 = (uint16*)SCREEN_BASE_BLOCK_SUB(2);

	//set main display to render 256 color bitmap
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);

	//set up the sub display
	videoSetModeSub(MODE_0_2D |
		DISPLAY_SPR_1D_LAYOUT |
		DISPLAY_SPR_ACTIVE |
		DISPLAY_BG0_ACTIVE |
		DISPLAY_BG1_ACTIVE);

	//vram banks are somewhat complex
	vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_SUB_SPRITE);

	sImage ball;

	sImage bouncer;

	sImage cielo;

	sImage playa;

	//load our ball pcx file into an image
	loadPCX((u8*)ball_pcx, &ball);
	loadPCX((u8*)cielo_pcx, &cielo);
	loadPCX((u8*)bouncer_pcx, &bouncer);
	loadPCX((u8*)playa_pcx, &playa);

	//tile it so it is usefull as sprite data
	imageTileData(&ball);
	imageTileData(&cielo);
	imageTileData(&playa);
	imageTileData(&bouncer);
	// Sprite initialisation
	for (i = 0; i < 256; i++)
		SPRITE_PALETTE_SUB[i] = bouncer.palette[i];

	for (i = 0; i < 32 * 16; i++)
		SPRITE_GFX_SUB[i] = bouncer.image.data16[i];

	for (i = 0; i < 256; i++)
		SPRITE_PALETTE[i] = bouncer.palette[i];

	for (i = 0; i < 32 * 16; i++)
		SPRITE_GFX[i] = bouncer.image.data16[i];

	//turn off sprites
	initOAM();

	for (i = 0; i < NUM_SPRITES; i++) {
		//random place and speed
		sprites[i].x = rand() & 0xFFFF;
		sprites[i].y = rand() & 0x7FFF;
		sprites[i].dx = (rand() & 0xFF) + 0x100;
		sprites[i].dy = (rand() & 0xFF) + 0x100;

		if (rand() & 1)
			sprites[i].dx = -sprites[i].dx;
		//if(rand() & 1)
		sprites[i].dy = -sprites[i].dy;

		sprites[i].oam = &OAMCopySub[i];
		sprites[i].gfxID = 0;

		//set up our sprites OAM entry attributes
		sprites[i].oam->attribute[0] = ATTR0_COLOR_256 | ATTR0_SQUARE;
		sprites[i].oam->attribute[1] = ATTR1_SIZE_32;
		sprites[i].oam->attribute[2] = sprites[i].gfxID;
	}

	//set up two backgrounds to scroll around
	REG_BG0CNT_SUB = BG_COLOR_256 | (1 << MAP_BASE_SHIFT);
	REG_BG1CNT_SUB = BG_COLOR_256 | (2 << MAP_BASE_SHIFT);

	BG_PALETTE_SUB[0] = RGB15(10, 10, 10);
	BG_PALETTE_SUB[1] = RGB15(0, 16, 0);
	BG_PALETTE_SUB[2] = RGB15(0, 0, 31);

	//pintar cielo arriba y abajo
	int bg3 = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);

	dmaCopy(cielo.image.data16, bgGetGfxPtr(bg3), 256 * 256);
	dmaCopy(cielo.palette, BG_PALETTE, 256 * 2);

	dmaCopy(cielo.image.data16, BG_BMP_RAM_SUB(0), 256 * 256);
	dmaCopy(cielo.palette, BG_PALETTE_SUB, 256 * 2);
	
	//otro metodo de meter los BG, para pintar segun el ejemplo del profesor
	

	// Set up background for 8bit bitmap
	REG_BG3CNT = BG_BMP8_256x256;

	// and 1:1 scaling
	REG_BG3PA = 1 << 8;
	REG_BG3PB = 0; // BG SCALING X
	REG_BG3PC = 0; // BG SCALING Y
	REG_BG3PD = 1 << 8;
	REG_BG3X = 0;
	REG_BG3Y = 0;



	// Set up background for 8bit bitmap
	REG_BG3CNT_SUB = BG_BMP8_256x256;

	// and 1:1 scaling
	REG_BG3PA_SUB = 1 << 8;
	REG_BG3PB_SUB = 0; // BG SCALING X
	REG_BG3PC_SUB = 0; // BG SCALING Y
	REG_BG3PD_SUB = 1 << 8;
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;
}

void updateInput(touchPosition * touch) {
	//update the key registers with current values
	scanKeys();

	//update the touch screen values
	 touchRead(touch);

	 
}

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------

	init();
	while (1) {
		//scroll the background
		//REG_BG0HOFS_SUB = delta ;
		//REG_BG0VOFS_SUB = delta++ ;


		//logica pantalla tactil
		updateInput(&touch);
		
		//touch true
		if (touch.px > 0) {
			u8* bgsub = (u8*)BG_BMP_RAM_SUB(0);
			bgsub[touch.px + touch.py * 256] = 1;
		}



		//move the sprites and physics
		for(i = 0; i < NUM_SPRITES; i++) {
			sprites[i].x += sprites[i].dx;
			sprites[i].y += sprites[i].dy;

			//check for collision with the screen boundries
			if(sprites[i].x < (1<<8) || sprites[i].x > (247 << 8))
				sprites[i].dx = -sprites[i].dx;

			if(sprites[i].y < (1<<8) || sprites[i].y > (182 << 8))
				sprites[i].dy = -sprites[i].dy;

			//reposition the sprites
			MoveSprite(&sprites[i]);
		}

	

	

		swiWaitForVBlank();

		scanKeys();

		int pressed = keysDown();

		if (pressed & KEY_START) break;

		updateOAM();

	}
	return 0;
}

