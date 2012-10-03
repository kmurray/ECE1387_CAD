#include <draw.h>
#include <data_structs.h>
#include <util.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

float world_dim;
int channel_spacing;
int number_of_wire_pairs;

extern t_FPGA* FPGA;

int corner_x_offset;
int corner_y_offset;
int clb_len;

void draw_clb(t_block* block);
void draw_switchblock(t_switchblock* sb);
void draw_routing_from_sb(t_switchblock* sb);
int midpoint(int start, int end);
t_drawing_position find_pin_to_wire(int pin_x, int pin_y, t_pin* pin, t_wire* wire, t_SIDE side);
t_wire* find_wire_by_pin_net(t_pin* pin);
t_drawing_position get_wire_drawing_coords(t_switchblock* sb, t_wire* wire);


void start_interactive_graphics(void) {

    /*printf("FPGA gridsize: %d\n", FPGA->grid_size);*/

    //Calculate the world size
    number_of_wire_pairs = FPGA->W / 2;
    channel_spacing = number_of_wire_pairs*PAIRED_WIRE_SPACING + (number_of_wire_pairs + 1)*NON_PAIRED_WIRE_SPACING;
    clb_len = 1.4*channel_spacing;
    world_dim = FPGA->grid_size*clb_len + (FPGA->grid_size + 1)*channel_spacing + clb_len;

    /*printf("World Dim: %.2f\n", world_dim);*/

    init_world(0., world_dim, world_dim, 0.);
    update_message("Testing graphics.....");

    draw_screen();
    event_loop(button_press, draw_screen);
}

static void draw_screen(void) {
    clearscreen();

    //World limits
    drawline(0.,0.,world_dim,0.);
    drawline(0.,0.,0.,world_dim);
    drawline(world_dim,world_dim,0.,world_dim);
    drawline(world_dim,world_dim,world_dim,0.);

    corner_x_offset = (clb_len / 2) + channel_spacing;
    corner_y_offset = corner_x_offset;

    //Draw the routing graph
        //First do the vertical channels
    int x_coord, y_coord;
    for(x_coord = 0; x_coord < FPGA->grid_size + 1; x_coord++) {
        for(y_coord = 0; y_coord < FPGA->grid_size + 1; y_coord++) {
            /*printf("Drawing Routing from SB (%d,%d)\n", x_coord, y_coord);*/
            t_switchblock* sb = get_sb(x_coord, y_coord);
            draw_routing_from_sb(sb);


        }
    }

    //Draw the switchblocks
    t_switchblocklist* switchblocklist = FPGA->switchblocklist;
    int switchblock_cnt;
    for(switchblock_cnt = 0; switchblock_cnt < switchblocklist->num_switchblocks; switchblock_cnt++) {
        t_switchblock* switchblock = switchblocklist->array_of_switchblocks[switchblock_cnt];

        draw_switchblock(switchblock);

    }

    //Draw the blocks
    t_blocklist* blocklist = FPGA->blocklist;
    int block_cnt;
    for(block_cnt = 0; block_cnt < blocklist->num_blocks; block_cnt++) {
        t_block* block = blocklist->array_of_blocks[block_cnt];

        draw_clb(block);
         
    }

    //Force the screen to re-draw
    flushinput();
}

void draw_clb(t_block* block) {
    int rect_x = corner_x_offset + (block->x_coord-1)*(clb_len + channel_spacing);
    int rect_y = corner_y_offset + (block->y_coord-1)*(clb_len + channel_spacing);

    drawrect (rect_x, rect_y, rect_x + clb_len,rect_y + clb_len);

    char buf[50];
    snprintf(buf, sizeof(buf), "CLB: %d,%d", block->x_coord, block->y_coord);
    drawtext(rect_x + clb_len/2, rect_y + clb_len/2, buf, TEXT_LIMIT);

    //Draw CLB pins
    t_SIDE side;
    int pin_cnt = 0;
    for(side = RIGHT; side < CLB_SIDES_PER_BLOCK; side++) {
        int side_pin_num;

        for(side_pin_num = 0; side_pin_num < CLB_NUM_PINS_PER_SIDE; side_pin_num++) {
            t_pin* pin = block->array_of_pins[pin_cnt];
            int pin_x, pin_y;
            int text_x, text_y;
            int line_start_x, line_end_x;
            int line_start_y, line_end_y;

            if(side == RIGHT) {
                pin_x = rect_x + clb_len;
                line_end_x   = pin_x + PIN_LENGTH/2;
                line_start_x = pin_x - PIN_LENGTH/2;
                line_end_x   = pin_x + PIN_LENGTH/2;
                text_x = pin_x - PIN_LENGTH;;

                pin_y = rect_y + (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*clb_len/8;
                line_start_y = pin_y;
                line_end_y   = line_start_y;
                text_y = pin_y;
            } else if (side == LEFT) {
                pin_x = rect_x;
                line_start_x = pin_x - PIN_LENGTH/2;
                line_end_x   = pin_x + PIN_LENGTH/2;
                text_x = pin_x + PIN_LENGTH;

                pin_y = rect_y + clb_len - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*clb_len/8;
                line_start_y = pin_y;
                line_end_y   = line_start_y;
                text_y = pin_y;

            } else if (side == TOP) {
                pin_x = rect_x + clb_len - (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*clb_len/8;
                line_start_x = pin_x;
                line_end_x   = line_start_x;
                text_x = pin_x;

                pin_y = rect_y + clb_len;
                line_start_y = pin_y - PIN_LENGTH/2;
                line_end_y   = pin_y + PIN_LENGTH/2;
                text_y = pin_y - PIN_LENGTH;
                
            } else { //side == BOTTOM
                pin_x = rect_x + (1 + CLB_NUM_PINS_PER_SIDE*side_pin_num)*clb_len/8;
                line_start_x = pin_x;
                line_end_x   = line_start_x;
                text_x = pin_x;

                pin_y = rect_y;
                line_start_y = pin_y - PIN_LENGTH/2;
                line_end_y   = pin_y + PIN_LENGTH/2;
                text_y = pin_y + PIN_LENGTH;

            }

            //The pin marker and text
            drawline(line_start_x, line_start_y, line_end_x, line_end_y);
            char buf[10];
            snprintf(buf, sizeof(buf), "P%d", pin->pin_num + 1);
            drawtext(text_x, text_y, buf, TEXT_LIMIT);

            //The pin connection
            if (pin->associated_net != NULL) {
                /*printf("Pin (%d,%d) #%d with associated net net_%d\n", pin->block->x_coord, pin->block->y_coord, pin->pin_num, pin->associated_net->net_num);*/
                t_wire* wire = find_wire_by_pin_net(pin);
                if (wire != NULL) {
                    t_drawing_position pos = find_pin_to_wire(pin_x, pin_y, pin, wire, side);
                    /*printf("Drawing line: (%d,%d) to (%d,%d)\n", pos.start_x, pos.start_y, pos.end_x, pos.end_y);*/
                    setlinewidth(USED_WIRE_WIDTH);
                    drawline(pos.start_x, pos.start_y, pos.end_x, pos.end_y);
                    setlinewidth(DEFAULT_LINE_WIDTH);
                    fillrect(pos.end_x - PIN_TO_WIRE_BOX_WIDTH/2, pos.end_y - PIN_TO_WIRE_BOX_WIDTH/2,
                             pos.end_x + PIN_TO_WIRE_BOX_WIDTH/2, pos.end_y + PIN_TO_WIRE_BOX_WIDTH/2);
                }
            }
                     

            pin_cnt++;
        }
    }
}

void draw_switchblock(t_switchblock* sb) {
    int sb_len = channel_spacing - 2*WIRE_EXTENSION_BEYOND_CLB;
    int rect_x = clb_len/2 + (sb->x_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int rect_y = clb_len/2 + (sb->y_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;

    drawrect (rect_x, rect_y, rect_x + sb_len, rect_y + sb_len);

    /*//Draw co-ordinates
     *char buf[50];
     *snprintf(buf, sizeof(buf), "SB: %d,%d", sb->x_coord, sb->y_coord);
     *drawtext(rect_x + sb_len/2, rect_y + sb_len/2, buf, TEXT_LIMIT);
     */

}

void draw_routing_from_sb(t_switchblock* sb) {
    /*printf("  Wires: adjacent to SB (%d,%d)\n", sb->x_coord, sb->y_coord);*/
    //As with switchblocks


    int track_cnt;
    for(track_cnt = 0; track_cnt < FPGA->W; track_cnt++) {

        //Intermediate pointers
        t_wire** track_wires = sb->adjacency_list[track_cnt];
        int num_wires_in_track = sb->num_adjacencies[track_cnt]; 

        int wire_cnt;
        //Each wire on track 'track_cnt' around all edges of the switchblcok 'sb'
        for(wire_cnt = 0; wire_cnt < num_wires_in_track; wire_cnt++) {
            t_wire* wire = track_wires[wire_cnt];

            //The coordinates used to draw each line
            t_drawing_position pos = get_wire_drawing_coords(sb, wire);

            //Set colours and optional text based on wire label
            if(wire->label_type == CURRENT_EXPANSION) {
                char buf[10];
                snprintf(buf, sizeof(buf), "%d", wire->label_value);
                drawtext(midpoint(pos.start_x, pos.end_x), midpoint(pos.start_y, pos.end_y), buf, TEXT_LIMIT);
                setlinewidth(CURRENT_EXPANSION_WIRE_WIDTH);
                setcolor(CURRENT_EXPANSION_WIRE_COLOUR);
            } else if (wire->label_type == SOURCE) {
                drawtext(midpoint(pos.start_x, pos.end_x), midpoint(pos.start_y, pos.end_y), "S0", TEXT_LIMIT);
                setlinewidth(SOURCE_WIRE_WIDTH);
                setcolor(SOURCE_WIRE_COLOUR);
            } else if (wire->label_type == TARGET) {
                char buf[10];
                if(wire->label_value == -1) {
                    snprintf(buf, sizeof(buf), "T");
                } else {
                    snprintf(buf, sizeof(buf), "T%d", wire->label_value);
                }
                drawtext(midpoint(pos.start_x, pos.end_x), midpoint(pos.start_y, pos.end_y), buf, TEXT_LIMIT);
                setlinewidth(TARGET_WIRE_WIDTH);
                setcolor(TARGET_WIRE_COLOUR);
            } else if (wire->label_type == USED) {
                setlinewidth(USED_WIRE_WIDTH);
                setcolor(USED_WIRE_COLOUR);
            } else {
                setlinewidth(DEFAULT_LINE_WIDTH);
                setcolor(DEFAULT_LINE_COLOUR);
            }

            //Actually draw it
            drawline(pos.start_x, pos.start_y, pos.end_x, pos.end_y);
            setcolor(DEFAULT_LINE_COLOUR);
            setlinewidth(DEFAULT_LINE_WIDTH);
            
            
            //Draw wire index indicators beside  the lowest and leftest switchblock
            if(sb->x_coord == 0 && sb->y_coord == 0) {
                char buf[10];
                snprintf(buf, sizeof(buf), "W%d", wire->wire_num);
                int text_x, text_y;
                //Ugly haardcode
                int sb_bottom_y = clb_len/2 + (sb->y_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
                int sb_left_x = clb_len/2 + (sb->x_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;

                
                if(is_vertical_wire(wire)) {
                    text_x = pos.start_x;
                    text_y = sb_bottom_y - PAIRED_WIRE_SPACING;
                } else {
                    text_y = pos.start_y;
                    text_x = sb_left_x - PAIRED_WIRE_SPACING;
                }
                drawtext(text_x, text_y, buf, TEXT_LIMIT);
            }
        }
    }
    /*flushinput();*/
}

t_drawing_position get_wire_drawing_coords(t_switchblock* sb, t_wire* wire) {
    int sb_len = channel_spacing - 2*WIRE_EXTENSION_BEYOND_CLB;
    int chan_len = clb_len + 2*WIRE_EXTENSION_BEYOND_CLB;

    int sb_bottom_y = clb_len/2 + (sb->y_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int sb_top_y = sb_bottom_y + sb_len;

    int sb_left_x = clb_len/2 + (sb->x_coord)*(clb_len + channel_spacing) + WIRE_EXTENSION_BEYOND_CLB;
    int sb_right_x = sb_left_x + sb_len;

    int left_channel_x, right_channel_x;
    int top_channel_y, bottom_channel_y;

    t_drawing_position pos;
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

        pos.start_x = left_channel_x + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.end_x = pos.start_x;
        pos.start_y = bottom_channel_y;
        pos.end_y = top_channel_y;
        
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

        pos.start_y = bottom_channel_y + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.end_y = pos.start_y;
        pos.start_x = left_channel_x;
        pos.end_x = right_channel_x;

    }

    return pos;
}

int midpoint(int start, int end) {
    int midpoint;
    if (start > end) {
        midpoint = end + (start - end) / 2; 
    } else {
        midpoint = start + (end - start) / 2; 
    }
    
    //Slight offset
    return midpoint + 1;
}

static void button_press (float x, float y) {
    /* Called whenever event_loop gets a button press in the graphics *
     * area.  Allows the user to do whatever he/she wants with button *
     * clicks.                                                        */

     printf("User clicked at coordinates (%f, %f)\n", x, y);
}

t_drawing_position find_pin_to_wire(int pin_x, int pin_y, t_pin* pin, t_wire* wire, t_SIDE side) {
    int chan_len = clb_len + 2*WIRE_EXTENSION_BEYOND_CLB;
   
    t_drawing_position pos;



    if(side == RIGHT) {
        pos.start_x = pin_x;
        pos.end_x = pin_x + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.start_y = pin_y;
        pos.end_y = pin_y;
        
    } else if(side == BOTTOM) { 
        pos.start_y = pin_y;
        pos.end_y = pin_y - channel_spacing + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.start_x = pin_x;
        pos.end_x = pin_x;
        
    } else if(side == LEFT) { 
        pos.start_x = pin_x;
        pos.end_x = pin_x - channel_spacing + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.start_y = pin_y;
        pos.end_y = pin_y;

    } else if(side == TOP) { 
        pos.start_y = pin_y;
        pos.end_y = pin_y + NON_PAIRED_WIRE_SPACING + (wire->wire_num - wire->channel_pair_num*2)*PAIRED_WIRE_SPACING + (NON_PAIRED_WIRE_SPACING + PAIRED_WIRE_SPACING)*wire->channel_pair_num;
        pos.start_x = pin_x;
        pos.end_x = pin_x;

    }
        return pos;
}

t_wire* find_wire_by_pin_net(t_pin* pin) {
    int wire_index;
    for(wire_index = 0; wire_index < pin->num_adjacent_wires; wire_index++) {
        t_wire* wire = pin->array_of_adjacent_wires[wire_index];
        if(wire->associated_net == pin->associated_net) {
            return wire;
        }
    }
    return NULL;
}
