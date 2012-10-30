#include <data_structs.h>
#include <util.h>
#include <lookahead_legalization.h>
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
void draw_block(t_block* block);
void draw_pnet(t_pnet* pnet);
void draw_flightline(t_block* block_from, t_block* block_to);
void draw_gridsquare(t_gridsquare* gs);



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
    
    //the grid
    t_gridsquare_list* gs_list = build_gridsquare_list();
    for(int gs_index = 0; gs_index < gs_list->num_gridsquares; gs_index++) {
        t_gridsquare* gs = gs_list->array_of_gridsquares[gs_index];

        draw_gridsquare(gs);
    }

    //World limits
    drawline(0.,0.,world_dim_x,0.); //Bottom
    drawline(0.,0.,0.,world_dim_y); //Left
    drawline(0.,world_dim_y,world_dim_x,world_dim_y); //Top
    drawline(world_dim_x,0.,world_dim_x,world_dim_y); //Right


    //Draw blocks
    t_blocklist* blocklist = g_CHIP->blocklist;
    int block_index;
    for(block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];
        draw_block(block);
    }



}

void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%.2f, %.2f)\n", x, y);
}

/*
 *  Draws a block location
 */
void draw_block(t_block* block) {
    float rect_l, rect_r, rect_b, rect_t;
    rect_l = block->x - block_len/2.;
    rect_r = block->x + block_len/2.;
    rect_b = block->y - block_len/2.;
    rect_t = block->y + block_len/2.;

    if(g_args->draw_nets) {
        for(int pnet_index = 0; pnet_index < block->num_pnets; pnet_index++) {
            t_pnet* pnet = block->associated_pnets[pnet_index];
            
            if(pnet->block_a->is_fixed || pnet->block_b->is_fixed) {
                setcolor(GREEN);
            } else {
                setcolor(LIGHTGREY);
            }

            draw_flightline(pnet->block_a, pnet->block_b);
        }
        setcolor(BLACK);
    }

    if(block->is_fixed) {
        fillrect(rect_l, rect_b, rect_r, rect_t);
    } else {
        drawrect(rect_l, rect_b, rect_r, rect_t);
    }

    char buf[30];
    snprintf(buf, sizeof(buf), "%d", block->index);

#if DRAW_BLOCK_NUM
    drawtext(rect_r+block_len, (rect_t + rect_b)/2, buf, 50); 
#endif
}


void draw_flightline(t_block* from_block, t_block* to_block) {
    drawline(from_block->x, from_block->y, to_block->x, to_block->y);
}

void draw_gridsquare(t_gridsquare* gs){
    setcolor(DARKGREEN);
    drawrect(gs->region->x_min, gs->region->y_min, gs->region->x_max, gs->region->y_max);
    setcolor(BLACK);
}
