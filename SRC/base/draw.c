#include <draw.h>
#include <data_structs.h>
#include <stdlib.h>
#include <stdio.h>

float world_dim;
int channel_spacing;
int number_of_wire_pairs;

extern t_FPGA* FPGA;

void start_interactive_graphics(void) {
    init_graphics("Test graphics");

    printf("FPGA gridsize: %d\n", FPGA->grid_size);

    //Calculate the world size
    number_of_wire_pairs = FPGA->W / 2;
    channel_spacing = number_of_wire_pairs*PAIRED_WIRE_SPACING + (number_of_wire_pairs + 1)*NON_PAIRED_WIRE_SPACING;
    world_dim = FPGA->grid_size*CLB_LEN + (FPGA->grid_size + 1)*channel_spacing + CLB_LEN;

    printf("World Dim: %.2f\n", world_dim);

    init_world(0., world_dim, world_dim, 0.);
    update_message("Testing graphics.....");

    event_loop(button_press, draw_screen);
}

static void draw_screen(void) {
    clearscreen();

    //World limits
    drawline(0.,0.,world_dim,0.);
    drawline(0.,0.,0.,world_dim);
    drawline(world_dim,world_dim,0.,world_dim);
    drawline(world_dim,world_dim,world_dim,0.);

    int corner_x_offset = (CLB_LEN / 2) + channel_spacing;
    int corner_y_offset = corner_x_offset;

    int block_x_coord, block_y_coord;
    for(block_y_coord = 1; block_y_coord <= FPGA->grid_size; block_y_coord++) {
        for(block_x_coord = 1; block_x_coord <= FPGA->grid_size; block_x_coord++) {

            int rect_x = corner_x_offset + (block_x_coord-1)*(CLB_LEN + channel_spacing);
            int rect_y = corner_y_offset + (block_y_coord-1)*(CLB_LEN + channel_spacing);

            drawrect (rect_x, rect_y, rect_x + CLB_LEN,rect_y + CLB_LEN);

            int wire_pair_cnt;
            for (wire_pair_cnt = 0; wire_pair_cnt < number_of_wire_pairs; wire_pair_cnt++) {
                int i;
                for(i = 0; i < 2; i++) {
                   
                    //NOTE: lots of over-draw, will fix when colouring routing

                    //Left side of CLB
                    int line_x_start = rect_x - NON_PAIRED_WIRE_SPACING - i*PAIRED_WIRE_SPACING - (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire_pair_cnt;
                    int line_x_end = line_x_start;
                    int line_y_start = rect_y - WIRE_EXTENSION_BEYOND_CLB ;
                    int line_y_end = rect_y + CLB_LEN + WIRE_EXTENSION_BEYOND_CLB;
                    drawline(line_x_start, line_y_start, line_x_end, line_y_end);

                    //Right side of CLB
                    line_x_start = rect_x + CLB_LEN + NON_PAIRED_WIRE_SPACING + i*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire_pair_cnt;
                    line_x_end = line_x_start;
                    line_y_start = rect_y - WIRE_EXTENSION_BEYOND_CLB;
                    line_y_end = rect_y + CLB_LEN + WIRE_EXTENSION_BEYOND_CLB;
                    drawline(line_x_start, line_y_start, line_x_end, line_y_end);

                    //Top of CLB
                    line_x_start = rect_x - WIRE_EXTENSION_BEYOND_CLB;
                    line_x_end = rect_x + CLB_LEN + WIRE_EXTENSION_BEYOND_CLB;
                    line_y_start = rect_y + CLB_LEN + NON_PAIRED_WIRE_SPACING + i*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire_pair_cnt;
                    line_y_end = line_y_start;
                    drawline(line_x_start, line_y_start, line_x_end, line_y_end);

                    //Bottom of CLB
                    line_x_start = rect_x - WIRE_EXTENSION_BEYOND_CLB;
                    line_x_end = rect_x + CLB_LEN + WIRE_EXTENSION_BEYOND_CLB;
                    line_y_start = rect_y - NON_PAIRED_WIRE_SPACING - i*PAIRED_WIRE_SPACING - (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire_pair_cnt;
                    line_y_end = line_y_start;
                    drawline(line_x_start, line_y_start, line_x_end, line_y_end);
                }
            }

            char buf[50];
            snprintf(buf, sizeof(buf), "%d,%d", block_x_coord, block_y_coord);
            drawtext(rect_x + CLB_LEN/2, rect_y + CLB_LEN/2, buf, 30);
             
        }
    }
}

static void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%f, %f)\n", x, y);
}

