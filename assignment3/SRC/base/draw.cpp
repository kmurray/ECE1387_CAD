#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <argparse.h>

//=============================================================================
// Global Vars
//=============================================================================
float world_dim_x, world_dim_y;
float vert_grid;

extern t_CHIP* g_CHIP;
extern t_arguments* g_args;

//=============================================================================
// Local Function Declarations
//=============================================================================
void draw_search_tree(t_bbnode* node, int level, float parent_x, float start_x, float end_x, int num_siblings, int sibling_num);
double vert_position(int level);


//=============================================================================
// Function Implimentations
//=============================================================================
void start_interactive_graphics(void) {
    world_dim_x = 150;
    world_dim_y = 100;

    init_world(-1*world_dim_x*WORLD_DIM_SCALE, world_dim_y*(1 + WORLD_DIM_SCALE), world_dim_x*(1 + WORLD_DIM_SCALE), -1*world_dim_y*WORLD_DIM_SCALE);
    update_message(g_args->netlist_file);

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
    int num_levels = (int) g_blocklist.size()/2;
    
    //num_levels + 2 to give spacing above and below
    vert_grid = world_dim_y / (num_levels + 1);

    draw_search_tree(g_search_root, 1, world_dim_x/2, 0., world_dim_x, 1, 1);

}


void draw_search_tree(t_bbnode* node, int level, float parent_x, float start_x, float end_x, int num_siblings, int sibling_num) {
    double node_y = vert_position(level);
    double node_x;

    //printf("BBNODE, level: %d, sibling: %d/%d\n", level, sibling_num, num_siblings);

    float sibling_ratio = (float) sibling_num / (num_siblings + 1);
    node_x = start_x + (end_x - start_x)/2;

    fillrect(node_x - BOX_LEN/2, node_y - BOX_LEN/2, node_x + BOX_LEN/2, node_y + BOX_LEN/2);
    char buf[50];
    snprintf(buf, sizeof(buf), "%d", node->max_left_index);
    drawtext(node_x - BOX_LEN, node_y, buf, 50);
    if(level != 1) {
        drawline(node_x, node_y, parent_x, vert_position(level - 1));
    }


    float child_spacing = (end_x - start_x) / node->children.size();
    float child_start_x = start_x;
    float child_end_x = child_start_x + child_spacing;

    int child_index = 1;
    for(t_bbnode_map::iterator node_iter = node->children.begin(); node_iter != node->children.end(); node_iter++) {
        t_bbnode* child_node = node_iter->second;
        int block_index = node_iter->first;


        //printf("\tLevel: %d sibling: %d/%d, Child: %d, Block#: %d\n", level, sibling_num, num_siblings, child_index, block_index);
        draw_search_tree(child_node, level+1, node_x, child_start_x, child_end_x, node->children.size(), child_index);

        child_start_x = child_end_x;
        child_end_x += child_spacing;
        child_index++;
    }
}

double vert_position(int level) {
    return world_dim_y - level*vert_grid;
}

void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%.2f, %.2f)\n", x, y);
}

