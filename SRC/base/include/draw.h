#ifndef DRAW_H
#define DRAW_H

#include <graphics.h>

#define DETAILED_INTERACTIVE_GRAPHICS 0
#define FINISHED_NET_GRAPHICS 0
void button_press (float x, float y);
void draw_screen(void);
void start_interactive_graphics(void);

#define PAIRED_WIRE_SPACING 4
#define NON_PAIRED_WIRE_SPACING 10
#define WIRE_EXTENSION_BEYOND_CLB 7
#define PIN_LENGTH 4
#define TEXT_LIMIT 50
#define PIN_TO_WIRE_BOX_WIDTH 1.5

#define DEFAULT_LINE_WIDTH 0
#define DEFAULT_LINE_COLOUR BLACK

#define USED_WIRE_WIDTH 3
#define CURRENT_EXPANSION_WIRE_WIDTH DEFAULT_LINE_WIDTH
#define SOURCE_WIRE_WIDTH 0
#define TARGET_WIRE_WIDTH SOURCE_WIRE_WIDTH

#define USED_WIRE_COLOUR BLACK
#define CURRENT_EXPANSION_WIRE_COLOUR CYAN
#define SOURCE_WIRE_COLOUR GREEN
#define TARGET_WIRE_COLOUR YELLOW

typedef struct s_drawing_position {
    float start_x;
    float end_x;
    float start_y;
    float end_y;
} t_drawing_position;

#endif
