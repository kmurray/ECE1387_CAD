#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <argparse.h>

//=============================================================================
// Global Vars
//=============================================================================
float world_dim_x, world_dim_y;

extern t_CHIP* g_CHIP;
extern t_arguments* g_args;

//=============================================================================
// Local Function Declarations
//=============================================================================
void draw_block(t_block* block);
void draw_net(t_net* net);
void draw_pnet(t_pnet* pnet);
void draw_flightline(t_block* block_from, t_block* block_to);



//=============================================================================
// Function Implimentations
//=============================================================================
void start_interactive_graphics(void) {
    world_dim_x = g_CHIP->x_dim;
    world_dim_y = g_CHIP->y_dim;

    init_world(-1*world_dim_x*WORLD_DIM_SCALE, world_dim_y*(1 + WORLD_DIM_SCALE), world_dim_x*(1 + WORLD_DIM_SCALE), -1*world_dim_y*WORLD_DIM_SCALE);
    /*update_message("Testing graphics.....");*/

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

    //Draw blocks
    t_blocklist* blocklist = g_CHIP->blocklist;
    int block_index;
    for(block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];
        draw_block(block);
    }

    //Draw nets
    if(g_args->draw_nets) {
        t_netlist* netlist = g_CHIP->netlist;
        int net_index;
        for(net_index = 1; net_index <= netlist->num_nets; net_index++) {
            t_net* net = netlist->array_of_nets[net_index];
            draw_net(net);
        }
    }

    /*
     *setcolor(LIGHTGREY);
     *t_pnetlist* pnetlist = g_CHIP->pnetlist;
     *int net_index;
     *for(net_index = 0; net_index < pnetlist->num_pnets; net_index++) {
     *    t_net* pnet = pnetlist->array_of_pnets[net_index];
     *    draw_pnet(pnet);
     *}
     *setcolor(BLACK);
     */
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
    rect_l = block->x - BLOCK_LEN/2.;
    rect_r = block->x + BLOCK_LEN/2.;
    rect_b = block->y - BLOCK_LEN/2.;
    rect_t = block->y + BLOCK_LEN/2.;

    if(block->is_fixed) {
        fillrect(rect_l, rect_b, rect_r, rect_t);
    } else {
        drawrect(rect_l, rect_b, rect_r, rect_t);
    }

    char buf[30];
    snprintf(buf, sizeof(buf), "%d", block->index);

    drawtext(rect_r+BLOCK_LEN, (rect_t + rect_b)/2, buf, 50); 
}

/*
 * Draws all the flightline net connections for
 * the specified net
 */
void draw_net(t_net* net) {

    if(net->equivalent_pnets == NULL) {
        //Haven't generated pnets yet
        int from_block_index;
        for(from_block_index = 0; from_block_index < net->num_blocks; from_block_index++) {
            t_block* from_block = net->associated_blocks[from_block_index];
            int to_block_index;
            for(to_block_index = 0; to_block_index < net->num_blocks; to_block_index++) {
                t_block* to_block = net->associated_blocks[to_block_index];

                draw_flightline(from_block, to_block);
            }
        }
    } else {
        //Draw the pnets
        int pnet_index;
        for(pnet_index = 0; pnet_index < net->num_pnets; pnet_index++) {
            t_pnet* pnet = net->equivalent_pnets[pnet_index];

            setcolor(LIGHTGREY);
            draw_flightline(pnet->block_a, pnet->block_b);
            setcolor(BLACK);
        }

    }
}

void draw_flightline(t_block* from_block, t_block* to_block) {
    drawline(from_block->x, from_block->y, to_block->x, to_block->y);
}
