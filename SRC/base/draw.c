#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <argparse.h>

//=============================================================================
// Global Vars
//=============================================================================
float world_dim_x, world_dim_y;
float block_len;

extern t_CHIP* g_CHIP;
extern t_arguments* g_args;

//=============================================================================
// Local Function Declarations
//=============================================================================


//=============================================================================
// Function Implimentations
//=============================================================================
void start_interactive_graphics(void) {
    world_dim_x = g_CHIP->x_dim;
    world_dim_y = g_CHIP->y_dim;

    init_world(-1*world_dim_x*WORLD_DIM_SCALE, world_dim_y*(1 + WORLD_DIM_SCALE), world_dim_x*(1 + WORLD_DIM_SCALE), -1*world_dim_y*WORLD_DIM_SCALE);
    update_message(g_args->netlist_file);

    block_len = BLOCK_LEN_FACTOR*world_dim_x;

    draw_screen();
    event_loop(button_press, draw_screen);
}

void draw_screen(void) {
    clearscreen();
    
    //World limits
    drawline(0.,0.,world_dim_x,0.); //Bottom
    drawline(0.,0.,0.,world_dim_y); //Left
    drawline(0.,world_dim_y,world_dim_x,world_dim_y); //Top
    drawline(world_dim_x,0.,world_dim_x,world_dim_y); //Right

    //Draw stuff
}

void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%.2f, %.2f)\n", x, y);
}

