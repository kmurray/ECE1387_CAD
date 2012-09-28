#include <draw.h>
#include <data_structs.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

float world_dim;
int channel_spacing;
int number_of_wire_pairs;

extern t_FPGA* FPGA;

int corner_x_offset;
int corner_y_offset;

void draw_clb(t_block* block);
void draw_switchblock(t_switchblock* sb);
void draw_routing_from_sb(t_switchblock* sb);


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

    corner_x_offset = (CLB_LEN / 2) + channel_spacing;
    corner_y_offset = corner_x_offset;

    //Draw the blocks
    t_blocklist* blocklist = FPGA->blocklist;
    int block_cnt;
    for(block_cnt = 0; block_cnt < blocklist->num_blocks; block_cnt++) {
        t_block* block = blocklist->array_of_blocks[block_cnt];

        draw_clb(block);
         
    }
    
    //Draw the switchblocks
    t_switchblocklist* switchblocklist = FPGA->switchblocklist;
    int switchblock_cnt;
    for(switchblock_cnt = 0; switchblock_cnt < switchblocklist->num_switchblocks; switchblock_cnt++) {
        t_switchblock* switchblock = switchblocklist->array_of_switchblocks[switchblock_cnt];

        draw_switchblock(switchblock);

    }

    t_switchblock* sb = get_sb(0, 0);

    //Draw the routing graph
        //First do the vertical channels
    int x_coord, y_coord;
    for(x_coord = 0; x_coord < FPGA->grid_size + 1; x_coord++) {
        printf("Drawing Vertical Routing  Channel %d\n", x_coord);
        for(y_coord = 0; y_coord < FPGA->grid_size + 1; y_coord++) {
            t_switchblock* sb = get_sb(x_coord, y_coord);

            draw_routing_from_sb(sb);


        }
    }
}

void draw_clb(t_block* block) {
    int rect_x = corner_x_offset + (block->x_coord-1)*(CLB_LEN + channel_spacing);
    int rect_y = corner_y_offset + (block->y_coord-1)*(CLB_LEN + channel_spacing);

    drawrect (rect_x, rect_y, rect_x + CLB_LEN,rect_y + CLB_LEN);

    char buf[50];
    snprintf(buf, sizeof(buf), "%d,%d", block->x_coord, block->y_coord);
    drawtext(rect_x + CLB_LEN/2, rect_y + CLB_LEN/2, buf, 30);

    //Draw CLB pins
    t_SIDE side;
    int pin_cnt = 0;
    for(side = RIGHT; side <=TOP; side++) {
        int side_pin_num;

        for(side_pin_num = 0; side_pin_num < CLB_NUM_PINS_PER_SIDE; side_pin_num++) {
            t_pin* block_pin = block->array_of_pins[pin_cnt];

            int line_start_x, line_end_x;
            int line_start_y, line_end_y;

            if(side == RIGHT) {
                line_start_x = rect_x + CLB_LEN - PIN_LENGTH/2;
                line_end_x   = rect_x + CLB_LEN + PIN_LENGTH/2;

                line_start_y = rect_y + CLB_LEN - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*CLB_LEN/4;
                line_end_y   = line_start_y;
            } else if (side ==LEFT) {
                line_start_x = rect_x - PIN_LENGTH/2;
                line_end_x   = rect_x + PIN_LENGTH/2;

                line_start_y = rect_y + CLB_LEN - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*CLB_LEN/4;
                line_end_y   = line_start_y;

            } else if (side == TOP) {
                line_start_x = rect_x + CLB_LEN - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*CLB_LEN/4;
                line_end_x   = line_start_x;

                line_start_y = rect_y - PIN_LENGTH/2;
                line_end_y   = rect_y + PIN_LENGTH/2;
                
            } else { //side == BOTTOM
                line_start_x = rect_x + CLB_LEN - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*CLB_LEN/4;
                line_end_x   = line_start_x;

                line_start_y = rect_y + CLB_LEN - PIN_LENGTH/2;
                line_end_y   = rect_y + CLB_LEN + PIN_LENGTH/2;

            }

            drawline(line_start_x, line_start_y, line_end_x, line_end_y);
            pin_cnt++;
        }
    }


    // Temporary wires until RR graph generation is completed
    //NOTE: lots of over-draw, will fix when colouring routing
    if (0) {
        int wire_pair_cnt;
        for (wire_pair_cnt = 0; wire_pair_cnt < number_of_wire_pairs; wire_pair_cnt++) {
            int i;
            for(i = 0; i < 2; i++) {
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
    }
}

void draw_switchblock(t_switchblock* sb) {
    int sb_len = channel_spacing - 2*WIRE_EXTENSION_BEYOND_CLB;
    int rect_x = CLB_LEN/2 + (sb->x_coord)*(CLB_LEN + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int rect_y = CLB_LEN/2 + (sb->y_coord)*(CLB_LEN + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;

    drawrect (rect_x, rect_y, rect_x + sb_len, rect_y + sb_len);

    char buf[50];
    snprintf(buf, sizeof(buf), "%d,%d", sb->x_coord, sb->y_coord);
    drawtext(rect_x + sb_len/2, rect_y + sb_len/2, buf, 30);

}

void draw_routing_from_sb(t_switchblock* sb) {
    printf("  Wires: adjacent to SB (%d,%d)\n", sb->x_coord, sb->y_coord);
    //As with switchblocks
    int sb_len = channel_spacing - 2*WIRE_EXTENSION_BEYOND_CLB;
    int chan_len = CLB_LEN + 2*WIRE_EXTENSION_BEYOND_CLB;

    int sb_bottom_y = CLB_LEN/2 + (sb->y_coord)*(CLB_LEN + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int sb_top_y = sb_bottom_y + sb_len;

    int sb_left_x = CLB_LEN/2 + (sb->x_coord)*(CLB_LEN + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int sb_right_x = sb_left_x + sb_len;

    int left_channel_x, right_channel_x;
    int top_channel_y, bottom_channel_y;

    int line_x_start, line_x_end;
    int line_y_start, line_y_end;


    int track_cnt;
    for(track_cnt = 0; track_cnt < FPGA->W; track_cnt++) {
        printf("    Track %d\n", track_cnt);

        t_wire** track_wires = sb->adjacency_list[track_cnt];
        int num_wires_in_track = sb->num_adjacencies[track_cnt]; 

        int wire_cnt;
        //Each wire on track 'track_cnt' around all edges of the switchblcok 'sb'
        for(wire_cnt = 0; wire_cnt < num_wires_in_track; wire_cnt++) {
            t_wire* wire = track_wires[wire_cnt];

            dump_wire(wire);
            if(is_vertical_wire(wire)) {
                //The defaults for a positive channel
                left_channel_x = sb_left_x - WIRE_EXTENSION_BEYOND_CLB;
                right_channel_x = sb_right_x + WIRE_EXTENSION_BEYOND_CLB;
                top_channel_y = sb_top_y + chan_len;
                bottom_channel_y = sb_top_y;

                if(!is_positive_wire(wire, sb)) {
                    top_channel_y = sb_bottom_y;
                    bottom_channel_y = sb_bottom_y - chan_len;
                }

                line_x_start = left_channel_x + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
                line_x_end = line_x_start;
                line_y_start = bottom_channel_y;
                line_y_end = top_channel_y;
                
            } else { //Horizontal wire
                //The defaults for a positive channel
                top_channel_y = sb_top_y + WIRE_EXTENSION_BEYOND_CLB;
                bottom_channel_y = sb_bottom_y - WIRE_EXTENSION_BEYOND_CLB;
                left_channel_x = sb_right_x;
                right_channel_x = sb_right_x + chan_len;

                if(!is_positive_wire(wire, sb)) {
                    left_channel_x = sb_left_x - chan_len;
                    right_channel_x = sb_left_x;
                }

                line_y_start = bottom_channel_y + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
                line_y_end = line_y_start;
                line_x_start = left_channel_x;
                line_x_end = right_channel_x;

            }
            /*setcolor(GREEN);*/
            /*drawrect(left_channel_x, bottom_channel_y, right_channel_x, top_channel_y);*/
            /*setcolor(RED);*/
            drawline(line_x_start, line_y_start, line_x_end, line_y_end);
        }

        

    setcolor(BLACK);
    }
}

static void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%f, %f)\n", x, y);
}

