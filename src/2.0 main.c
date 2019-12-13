/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers - it's recommended to leave them included */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Shared libraries */
#include <graphx.h>
#include <fileioc.h>
#include <keypadc.h>
#include "gfx/sprites_gfx.h"


#define BACKGROUND_Y	142
#define GROUND_Y		188

#define GAP_SIZE		48 + 1

#define gfx_buff		(*(uint8_t**)(0xE30014))
#define bird_x			100
#define NUM_PIPES		3
#define GRAVITY			0.5;

const gfx_rletsprite_t *bird_sprite[3] = {bird1, bird2, bird0};

uint24_t score, hscore = 133;
uint8_t x_ground;

typedef struct {
	int24_t y;
	float velocityY;

	uint8_t animationCntr;
	uint8_t spriteNum;
} bird_t;
bird_t bird;

typedef struct {
	int24_t x;
	uint8_t y;
} pipe_t;
pipe_t pipe[NUM_PIPES];

/* function prototypes */
void update();
void render();
void menu();

/* Initializes the x positions of the pipes and y positions of the random holes */
void init_pipes(int offset) {
	uint8_t i;

	for (i = 0; i < NUM_PIPES; i++) {
		pipe[i].x = offset + 344 / 3 * i; 
		pipe[i].y = 24 + rand() % 95;
	}
}

/* Put all your code here */
void main(void) {
	ti_var_t file;
	int8_t loop = 0;
	bool prev_pressed;
	bool pressed = false;

	srand(rtc_Time());

	gfx_Begin(gfx_8bpp);
	gfx_SetPalette(sprites_gfx_pal, sizeof sprites_gfx_pal, 0);
	gfx_SetTextFGColor(0x03);
	gfx_SetDrawBuffer();

	//Open the appvar and load the highscore.
	ti_CloseAll();
	file = ti_Open("FLAPPYB", "r+");
	if (file) {
		ti_Read(&hscore, sizeof(uint24_t), sizeof(hscore) / sizeof(uint24_t), file);
	}

	init_pipes(45);

	menu();

	while (kb_ScanGroup(kb_group_6) != kb_Clear) {
		prev_pressed = pressed;
		pressed = (bool)kb_AnyKey();
		
		loop++;
		if (!prev_pressed || loop > 7) {
			if (pressed) {
				bird.velocityY = -5.0f;
				loop = 0;
			}
		}
		else {
			if (loop > 7) {
				loop = 8;
			}
		}

		update();

		gfx_Wait();

		render();
		
		gfx_SwapDraw();
	}

	ti_CloseAll();

	//Open the appvar and save the highscore.
	file = ti_Open("FLAPPYB", "w");
	if (file) {
		ti_Write(&hscore, sizeof(uint24_t), sizeof(hscore) / sizeof(uint24_t), file);
	}
	ti_SetArchiveStatus(true, file);

	gfx_End();
}

void update(){
	uint8_t i;
	
	// update the bird
	bird.velocityY += GRAVITY;
    bird.y += bird.velocityY;

	bird.animationCntr--;
	if (bird.animationCntr == 0) {
		bird.spriteNum++;
		if (bird.spriteNum >= 3) {
			bird.spriteNum = 0;
		}

		bird.animationCntr = 3;
	}
	
	// bird hit the ground
	if (bird.y > 178) {
		bird.y = 176;
		menu();
	}

	// update the ground
	x_ground += 3;
	if (!(x_ground % 21))
		x_ground = 0;

	// update the pipes
	for (i = 0; i < NUM_PIPES; i++) {
		pipe_t *this_pipe = &pipe[i];
		uint24_t pipe_x;
		uint8_t pipe_y;

		this_pipe->x -= 3;

		pipe_x = this_pipe->x;
		pipe_y = this_pipe->y;

		// Check if bird collides with pipe
		if (bird_x + bird0_width >= pipe_x && bird_x <= pipe_x + pipe_i_width && (bird.y < pipe_y || bird.y + bird0_height > pipe_y + GAP_SIZE)) {
			if (bird_x + 13 >= pipe_x && bird_x <= pipe_x + 16) {
				if (bird.y < pipe_y) {
					bird.y = pipe_y;
				} 
				else {
					bird.y = pipe_y + GAP_SIZE - bird0_height;
				}
			}
			menu();
		}

		// Pipe is offscreen
		if (this_pipe->x <= -24) {
			this_pipe->x = gfx_lcdWidth; 
			this_pipe->y = 24 + rand() % 95;
		}
		
		// Bird flew through a pipe
		if (pipe_x == 74) {
			score++;
		}
	}
}

void render() {
	uint24_t i;
	uint8_t j;

	// Draw background
	memset_fast(gfx_buff, 0x04, LCD_WIDTH * BACKGROUND_Y);					// blue sky
	for (i = 0; i < gfx_lcdWidth; i += background_width) {
		gfx_RLETSprite(background, i, BACKGROUND_Y);						// buildings and stuff
	}
	memset_fast((gfx_buff + (BACKGROUND_Y + background_height) * LCD_WIDTH), 0x15, LCD_WIDTH * (GROUND_Y - (BACKGROUND_Y + background_height))); // bottom of bushes

	// Draw pipes
	for (j = 0; j < NUM_PIPES; j++){
		pipe_t *this_pipe = &pipe[j];
		uint8_t y;

		for (y = 0; y < GROUND_Y; y += pipe_i_height) {
			if (y < this_pipe->y - pipe_u_height || y >= this_pipe->y + GAP_SIZE + pipe_l_height - 3) {
				gfx_RLETSprite(pipe_i, this_pipe->x, y);
			}
		}
		gfx_RLETSprite(pipe_u, this_pipe->x - 1, this_pipe->y - pipe_u_height);
		gfx_RLETSprite(pipe_l, this_pipe->x - 1, this_pipe->y + GAP_SIZE);
	}

	// Draw ground
	for (i = 0; i <= gfx_lcdWidth; i += ground_width) {
		gfx_RLETSprite(ground, i - x_ground, GROUND_Y);						// ground		
	}
	memset_fast(&(*gfx_buff) + (GROUND_Y + ground_height) * LCD_WIDTH, 0x16, LCD_WIDTH * (gfx_lcdHeight - (GROUND_Y + ground_height)));	// bottom stuff

	// Draw bird
	gfx_RLETSprite(bird_sprite[bird.spriteNum], bird_x, bird.y);

	// Draw Score
	gfx_SetTextXY(148, 40);
	gfx_PrintUInt(score, 3);
}

void draw_menu() {
	gfx_SetColor(0x03); gfx_FillRectangle(126, 40, 68, 108);
	gfx_SetColor(0x16); gfx_FillRectangle(128, 42, 64, 104);
	gfx_SetColor(0x13); gfx_FillRectangle(132, 46, 56, 96);
	gfx_SetColor(0x16); gfx_FillRectangle(134, 48, 52, 92);

	gfx_PrintStringXY("SCORE", 140, 59);
	gfx_SetTextXY(148, 73); gfx_PrintUInt(score, 3);

	gfx_PrintStringXY("BEST", 144, 99);
	gfx_SetTextXY(148, 111); gfx_PrintUInt(hscore, 3);

	gfx_PrintStringXY("Press any key to start", 84, 210);
	gfx_PrintStringXY("BY RICO", 135, 230);
}

void menu() {
	if (score > hscore) {
		hscore = score;
	}

	render();
	draw_menu();
	gfx_SwapDraw();

	boot_WaitShort();
	while (!os_GetCSC());
	
	init_pipes(gfx_lcdWidth);
	score = 0; 
	
	bird.velocityY = -1.0f; 
	bird.y = 75;
	bird.spriteNum = 0;
	bird.animationCntr = 0;
}
