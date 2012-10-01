#include <rr_graph.h>
#include <data_structs.h>
#include <util.h>
#include <assert.h>
#include <stdio.h>

extern t_FPGA* FPGA;

t_switchblocklist* generate_switchblocks(void);
t_switchblock* allocate_switchblock(int x_coord, int y_coord);
void set_adjacent_switchblocks_and_pins(t_wire* wire, t_switchblock* lower_sb, t_switchblock* upper_sb);
void generate_wires(t_switchblock* lower_sb, t_switchblock* upper_sb);
void get_adjacent_block_coordinates(int top_right, t_switchblock* lower_sb, t_switchblock* upper_sb,
                                    int* block_x_coord, int* block_y_coord);
t_block* get_adjacent_block(int top_right, t_switchblock* lower_sb, t_switchblock* upper_sb);
void add_wire_to_switchblock_adjacency(t_wire* wire, t_switchblock* sb);

void generate_rr_graph(void) {
    printf("Generating Routing Resource Graph\n");

    printf("Generating Switch Blocks\n");
    t_switchblocklist* sb_list = generate_switchblocks();
    FPGA->switchblocklist = sb_list;

    //Add wires between all switchblock pairs    
    
    //First do the vertical channels
    int x_coord, y_coord;
    for(x_coord = 0; x_coord < FPGA->grid_size + 1; x_coord++) {
        printf("Generating Vertical Routing  Channel %d\n", x_coord);
        for(y_coord = 0; y_coord < FPGA->grid_size; y_coord++) {
            int lower_x_coord = x_coord;
            int lower_y_coord = y_coord;
            int upper_x_coord = x_coord;
            int upper_y_coord = y_coord+1;
            printf("Generating wires for switchblock pair: (%d,%d)<-->(%d,%d)\n", lower_x_coord, lower_y_coord, upper_x_coord, upper_y_coord);
            t_switchblock* bottom_sb = get_sb(lower_x_coord, lower_y_coord);
            t_switchblock* top_sb = get_sb(upper_x_coord, upper_y_coord);

            generate_wires(bottom_sb, top_sb);

        }
    }

    //Then the horizontal channels
    for(y_coord = 0; y_coord < FPGA->grid_size + 1; y_coord++) {
        printf("Generating Horizontal Routing  Channel %d\n", y_coord);
        for(x_coord = 0; x_coord < FPGA->grid_size; x_coord++) {

            int lower_x_coord = x_coord;
            int lower_y_coord = y_coord;
            int upper_x_coord = x_coord+1;
            int upper_y_coord = y_coord;
            printf("Generating wires for switchblock pair: (%d,%d)<-->(%d,%d)\n", lower_x_coord, lower_y_coord, upper_x_coord, upper_y_coord);
            t_switchblock* bottom_sb = get_sb(lower_x_coord, lower_y_coord);
            t_switchblock* top_sb = get_sb(upper_x_coord, upper_y_coord);

            generate_wires(bottom_sb, top_sb);

        }
    }

/*    //Uncomment for debug dump of rr_graph
 *    for(x_coord = 0; x_coord < FPGA->grid_size + 1; x_coord++) {
 *        printf("Generating Vertical Routing  Channel %d\n", x_coord);
 *        for(y_coord = 0; y_coord < FPGA->grid_size; y_coord++) {
 *            int lower_x_coord = x_coord;
 *            int lower_y_coord = y_coord;
 *            t_switchblock* bottom_sb = get_sb(lower_x_coord, lower_y_coord);
 *
 *            dump_switchblock(bottom_sb);
 *        }
 *    }
 */



/*
 *    int blk_cnt;
 *    for(blk_cnt = 0; blk_cnt < blocklist->num_blocks; blk_cnt++) {
 *        t_block* block = blocklist->array_of_blocks[blk_cnt];
 *
 *        int pin_cnt;
 *        for(pin_cnt = 0; pin_cnt < block->num_pins; pin_cnt++) {
 *            t_pin* block_pin = block->array_of_pins[pin_cnt];
 *            
 *        }
 *
 *    }
 */
}

/*
 *  Generates all the switchblocks used in the FPGA
 *  Routing Resource graph.
 *
 *  This routine only performs allocation, it does not
 *  set the connectivity of the switchblocks.
 */
t_switchblocklist* generate_switchblocks(void) {
    int num_sb_per_dim = FPGA->grid_size + 1;

    t_switchblocklist* sb_list = my_malloc(sizeof(t_switchblocklist));

    sb_list->num_switchblocks = num_sb_per_dim * num_sb_per_dim;

    sb_list->array_of_switchblocks = my_calloc(sizeof(t_switchblock*), sb_list->num_switchblocks);
    
    int sb_cnt = 0;
    int sb_x_coord;
    for(sb_x_coord = 0; sb_x_coord < num_sb_per_dim; sb_x_coord++) {
        int sb_y_coord;
        for(sb_y_coord = 0; sb_y_coord < num_sb_per_dim; sb_y_coord++) {
            sb_list->array_of_switchblocks[sb_cnt] = allocate_switchblock(sb_x_coord, sb_y_coord);
            sb_cnt++;
        }
    }
    assert(sb_cnt == (sb_list->num_switchblocks));
    
    return sb_list;
}

/*
 * Allocates a switch block and its internal data structures
 *
 * Initializes x-y coords, but does not set connectivity
 */
t_switchblock* allocate_switchblock(int x_coord, int y_coord) {
    t_switchblock* switchblock = my_malloc(sizeof(t_switchblock));

    switchblock->x_coord = x_coord;
    switchblock->y_coord = y_coord;

    //One adjacency list for each wire in the channel
    switchblock->adjacency_list = my_calloc(sizeof(t_wire***), FPGA->W);
    switchblock->num_adjacencies = my_calloc(sizeof(int*), FPGA->W);
    switchblock->occupancy_list = my_calloc(sizeof(int*), FPGA->W);
   
    int channel_wire_index;
    for(channel_wire_index = 0; channel_wire_index < FPGA->W; channel_wire_index++) {
        switchblock->adjacency_list[channel_wire_index] = my_calloc(sizeof(t_wire*), FPGA->Fs + 1);
        switchblock->num_adjacencies = my_calloc(sizeof(int), FPGA->W);
        switchblock->occupancy_list = my_calloc(sizeof(int), FPGA->W);
        
        int wire_cnt;
        for(wire_cnt = 0; wire_cnt < FPGA->W; wire_cnt++) {
            switchblock->num_adjacencies[wire_cnt] = 0;
            switchblock->occupancy_list[wire_cnt] = 0;
        }
    }

    return switchblock;
}

/*
 *  Given two switch blocks, generates a wiring channel between them.
 *
 */
void generate_wires(t_switchblock* lower_sb, t_switchblock* upper_sb) {
   
    assert(FPGA->W % 2 == 0);
    /*
     * Loop over each pair of wires
     * allocate and connect each wire
     */
    int wire_pair;
    for(wire_pair = 0; wire_pair < (FPGA->W / 2); wire_pair++) {

        int pair_cnt;
        //Two wires per pair
        for(pair_cnt = 0; pair_cnt < 2; pair_cnt++) {

            //Allocate wire
            t_wire* wire = my_malloc(sizeof(t_wire));

            /*
             *  Set releveant wire parameters
             */
            //Channel coordinates
            if (lower_sb->x_coord == upper_sb->x_coord) {
                //Vertical Channel
                wire->channel_x_coord = lower_sb->x_coord;
                wire->channel_y_coord = -1;
            } else {
                //Horizontal channel
                assert(lower_sb->y_coord == upper_sb->y_coord);
                wire->channel_x_coord = -1;
                wire->channel_y_coord = lower_sb->y_coord;
            }
            
            //Initial associated_net is unused
            wire->associated_net = NULL;
            
            //The pair number
            wire->channel_pair_num = wire_pair;

            //The wire number in the channel
            wire->wire_num = 2*wire_pair + pair_cnt;
            /*printf("  Generating wire %d\n", wire->wire_num);*/

            //Set adjacencies for this wire
            //  array_of_adjacent_switches
            //  array_of_adjacent_pins
            set_adjacent_switchblocks_and_pins(wire, lower_sb, upper_sb);


            //Connect to switch blocks
            wire->label = NONE;
        }

    }
}

void set_adjacent_switchblocks_and_pins(t_wire* wire, t_switchblock* lower_sb, t_switchblock* upper_sb) {
        /*
         *  Two adjacent switchblocks
         */
        wire->num_switchblocks = 2;
        wire->array_of_adjacent_switchblocks = my_calloc(sizeof(t_switchblock*), wire->num_switchblocks);
        wire->array_of_adjacent_switchblocks[0] = lower_sb;
        wire->array_of_adjacent_switchblocks[1] = upper_sb;

        //Add the wires too the switchblock adjacency list
        add_wire_to_switchblock_adjacency(wire, lower_sb);
        add_wire_to_switchblock_adjacency(wire, upper_sb);

        /*
         *  Adjacent pins on blocks
         */
        t_boolean is_vertical_channel; 
        int channel_num;
        if(wire->channel_y_coord == -1) {
            //Vertical channel
            channel_num = wire->channel_x_coord;
            is_vertical_channel = TRUE;
        } else {
            //Horizontal channel
            channel_num = wire->channel_y_coord;
            is_vertical_channel = FALSE;
        }

        //Adjacent pins
        wire->num_adjacent_pins = 2*CLB_NUM_PINS_PER_SIDE; //Defaults to four pins, corrects to 2 pins for edge cases later
        wire->array_of_adjacent_pins = my_calloc(sizeof(t_pin*), wire->num_adjacent_pins);

        int wire_adjacent_pin_index = 0;

        //At least one of these conditions must be true
        assert(channel_num != 0 || channel_num != FPGA->grid_size);


        if (channel_num != 0) {
            //This is not the left or bottom edge of the FPGA
            // Therefore, there exists a block to the left or bottom of this channel
            
            t_SIDE block_side;
            //Opposite, since the side is in refernce to the
            // block and not the channel
            if(is_vertical_channel) {
                block_side = RIGHT;
            } else {
                block_side = TOP;
            }

            //Get block relative to this routing channel
            t_block* adjacent_block_left_or_down = get_adjacent_block(0, lower_sb, upper_sb);
            
            //Get index for adjacent pins
            int starting_pin_index = get_starting_pin_index_for_side(adjacent_block_left_or_down, block_side);

            int pin_cnt;
            for(pin_cnt = 0; pin_cnt < CLB_NUM_PINS_PER_SIDE; pin_cnt++) {
                t_pin* adjacent_pin = adjacent_block_left_or_down->array_of_pins[starting_pin_index + pin_cnt];

                //Add pin to wire adjacency list
                wire->array_of_adjacent_pins[wire_adjacent_pin_index] = adjacent_pin;
                wire_adjacent_pin_index++;

                //Add wire to pin adjacency list
                assert(wire->wire_num < FPGA->W);
                adjacent_pin->num_adjacent_wires++;
                adjacent_pin->array_of_adjacent_wires[wire->wire_num] = wire;
            }

        } else {
            //If we were the left or bottom edge, we don't need 4 adjacent pins, only 2 will do
            wire->num_adjacent_pins = CLB_NUM_PINS_PER_SIDE;
            wire->array_of_adjacent_pins = my_realloc(wire->array_of_adjacent_pins, wire->num_adjacent_pins);

        }

        if (channel_num != FPGA->grid_size) {
            //This is not the top or right edge of the FPGA
            // Therefore, there exists a block to the right or top of this channel
            
            t_SIDE block_side;
            //Opposite to side of block, since the side is in refernce to the
            // block and not the channel
            if(is_vertical_channel) {
                block_side = LEFT;
            } else {
                block_side = BOTTOM;
            }

            //Relative to this routing channel
            t_block* adjacent_block_right_or_up = get_adjacent_block(1, lower_sb, upper_sb);
            
            //Get index for adjacent pins
            int starting_pin_index = get_starting_pin_index_for_side(adjacent_block_right_or_up, block_side);

            int pin_cnt;
            for(pin_cnt = 0; pin_cnt < CLB_NUM_PINS_PER_SIDE; pin_cnt++) {
                t_pin* adjacent_pin = adjacent_block_right_or_up->array_of_pins[starting_pin_index + pin_cnt];

                //Add pin to wire adjacency list
                wire->array_of_adjacent_pins[wire_adjacent_pin_index] = adjacent_pin;
                wire_adjacent_pin_index++;

                //Add wire to pin adjacency list
                assert(wire->wire_num < FPGA->W);
                adjacent_pin->num_adjacent_wires++;
                adjacent_pin->array_of_adjacent_wires[wire->wire_num] = wire;
            }

        } else {
            //If we were the right or top edge, we don't need 4 adjacent pins, only 2 will do
            wire->num_adjacent_pins = CLB_NUM_PINS_PER_SIDE;
            wire->array_of_adjacent_pins = my_realloc(wire->array_of_adjacent_pins, wire->num_adjacent_pins);
        }
}

/*
 * Converts two adjacent switch blocks into the XY coordiantes of the
 * adjacent CLB
 *
 * Sets the block_x_coord and block_y_coord references
 */
void get_adjacent_block_coordinates(int top_right, t_switchblock* lower_sb, t_switchblock* upper_sb,
                                    int* block_x_coord, int* block_y_coord) {
    if (top_right) {
        *block_x_coord = lower_sb->x_coord + 1;
        *block_y_coord = lower_sb->y_coord + 1;
    } else {
        *block_x_coord = upper_sb->x_coord;
        *block_y_coord = upper_sb->y_coord;
    }

}

/*
 * Returns the CLB block adjacent to the two given switchblocks
 */
t_block* get_adjacent_block(int top_right, t_switchblock* lower_sb, t_switchblock* upper_sb) {
    int block_x_coord, block_y_coord;
    get_adjacent_block_coordinates(top_right, lower_sb, upper_sb,
                                   &block_x_coord, &block_y_coord);

    assert(block_x_coord <= FPGA->grid_size);
    assert(block_x_coord > 0);
    assert(block_y_coord <= FPGA->grid_size);
    assert(block_y_coord > 0);
    return get_block(block_x_coord, block_y_coord);
}

/*
 * Add the wire to the switchblock
 *
 * This is a DISJOINT switchblock.
 */
void add_wire_to_switchblock_adjacency(t_wire* wire, t_switchblock* sb) {
    /*dump_switchblock(sb);*/
    int search_index;
    int num_adjacencies = sb->num_adjacencies[wire->wire_num];
    for(search_index = 0; search_index < num_adjacencies; search_index++) {
        t_wire* search_wire = sb->adjacency_list[wire->wire_num][search_index];
        assert(search_wire->wire_num == wire->wire_num);
        if (search_wire == wire) {
            return; //Already in list
        } else {
            assert((search_wire->array_of_adjacent_switchblocks[0] != wire->array_of_adjacent_switchblocks[0]) || (search_wire->array_of_adjacent_switchblocks[1] != wire->array_of_adjacent_switchblocks[1]));
        }
    }
    num_adjacencies++;
    assert(num_adjacencies <= FPGA->Fs + 1);
    sb->num_adjacencies[wire->wire_num] = num_adjacencies;
    sb->adjacency_list[wire->wire_num][num_adjacencies-1] = wire;
}

void verify_rr_graph(void) {
    int block_index;
    for(block_index = 0; block_index < (FPGA->grid_size*FPGA->grid_size); block_index++) {
        t_block* block = FPGA->blocklist->array_of_blocks[block_index];
        int pin_index;
        for(pin_index = 0; pin_index < CLB_SIDES_PER_BLOCK*CLB_NUM_PINS_PER_SIDE; pin_index++) {
            t_pin* pin = block->array_of_pins[pin_index];

            assert(pin->pin_num == pin_index);
            assert(pin->num_adjacent_wires == FPGA->W);

            int adjacent_wire_index;
            for(adjacent_wire_index = 0; adjacent_wire_index < pin->num_adjacent_wires; adjacent_wire_index++) {

                t_wire* wire = pin->array_of_adjacent_wires[adjacent_wire_index];
                assert(wire != NULL);

                //Two pins per side of a block -> either two or 4 pins per wire
                assert(wire->num_adjacent_pins % CLB_NUM_PINS_PER_SIDE == 0);
                assert(wire->num_adjacent_pins > 0 && wire->num_adjacent_pins <= 2*CLB_NUM_PINS_PER_SIDE);

                //Two switchblocks per wire
                assert(wire->num_switchblocks == 2);

            }

        }
    }

    int sb_index;
    for(sb_index = 0; sb_index < FPGA->switchblocklist->num_switchblocks; sb_index++) {
        t_switchblock* sb = FPGA->switchblocklist->array_of_switchblocks[sb_index];

        int wire_index;
        for(wire_index = 0; wire_index < FPGA->W; wire_index++) {

            assert(sb->num_adjacencies[wire_index] == 2 || sb->num_adjacencies[wire_index] == 3 || sb->num_adjacencies[wire_index] == 4);

            int adjacent_index;
            for(adjacent_index = 0; adjacent_index < sb->num_adjacencies[wire_index]; adjacent_index++) {
                
                t_wire* wire = sb->adjacency_list[wire_index][adjacent_index];

                //Two pins per side of a block -> either two or 4 pins per wire
                assert(wire->num_adjacent_pins % CLB_NUM_PINS_PER_SIDE == 0);
                assert(wire->num_adjacent_pins > 0 && wire->num_adjacent_pins <= 2*CLB_NUM_PINS_PER_SIDE);

                //Two switchblocks per wire
                assert(wire->num_switchblocks == 2);
            }
        }
    }

}
