#ifndef DRAW_H
#define DRAW_H

#include <graphics.h>

#define BLOCK_LEN_FACTOR 0.005
#define WORLD_DIM_SCALE 0.06

void start_interactive_graphics(void);
void draw_screen(void);
void button_press (float x, float y);

#endif
