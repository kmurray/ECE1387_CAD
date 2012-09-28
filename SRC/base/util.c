#include <stdio.h>
#include <assert.h>
#include <data_structs.h>
#include <parse_input.h>

extern t_FPGA* FPGA;

void* my_malloc(size_t size) {
    void* retval = malloc(size);
    if (retval == NULL) {
        printf("Error: malloc of size %u bytes failed\n", (unsigned int) size);
        exit(1);
    }
    return retval;
}

void* my_realloc(void* ptr, size_t size) {
    void* retval = realloc(ptr, size);
    if (retval == NULL) {
        printf("Error: realloc of size %u bytes failed\n", (unsigned int) size);
        exit(1);
    }
    return retval;
}

void* my_calloc(size_t num_items, size_t size_of_item) {
    void* retval = calloc(num_items, size_of_item);
    if (retval == NULL) {
        printf("Error: calloc of size %ux%u bytes failed\n", (unsigned int) num_items, (unsigned int) size_of_item);
        exit(1);
        
    }
    return retval;
}

int get_block_index(int x_coord, int y_coord) {
    /*
     *  All blocks are stored as an array.
     *  Since the FPGA is always square it is 
     *  simple to convert from a given x and y 
     *  location to the index.
     *
     *   (1,3)  (2,3) (3,3)
     *
     *   (1,2)  (2,2) (3,2)
     *
     *   (1,1)  (2,1) (3,1)
     *
     *  The blocks are stored in row major order
     *  as follows:
     *   ---------------------------------------------------------
     *   | 1,1 | 2,1 | 3,1 | 1,2 | ... | 3,2 | 1,3 | ... | 3,3 |
     *   ---------------------------------------------------------
     *   
     *  Therefore, knowing the lenght of one side of the grid 'N',
     *  and the  'x' coord and 'y' coord, the index is:
     *
     *      INDEX = N*(y-1) + (x-1)
     *  Where the '-1' along side each co-ordinate adjusts for the fact
     *  that the CLB rows/columns are indexed from 1 instead of 0.
     */
    
    return  (FPGA->grid_size * (y_coord - 1)) + (x_coord - 1);
}

t_block* get_block(int x_coord, int y_coord) {
    int index = get_block_index(x_coord, y_coord);

    assert(index < FPGA->blocklist->num_blocks);

    return FPGA->blocklist->array_of_blocks[index];
}

t_pin* get_block_pin(int x_coord, int y_coord, int pin_num) {
    //The netlist format indexes pins from 1..8
    //  The data structures are indexed from 0..7
    //  so we need to adjust the index
    assert(pin_num > 0);
    pin_num -= 1;

    t_block* block = get_block(x_coord, y_coord);

    assert(pin_num < block->num_pins);

    return block->array_of_pins[pin_num];
}

int get_sb_index(int x_coord, int y_coord) {

    return ((FPGA->grid_size + 1)*(x_coord) + y_coord);
}


t_switchblock* get_sb(int x_coord, int y_coord) {
    int index = get_sb_index(x_coord, y_coord);

    assert(index < FPGA->switchblocklist->num_switchblocks);

    t_switchblock* sb = FPGA->switchblocklist->array_of_switchblocks[index];

    assert(sb->x_coord == x_coord);
    assert(sb->y_coord == y_coord);

    return sb;
}

int get_starting_pin_index_for_side(t_block* block, t_SIDE block_side) {
    return CLB_NUM_PINS_PER_SIDE*block_side;
}

void dump_switchblock(t_switchblock* sb) {
    printf("SB: (%d,%d)\n", sb->x_coord, sb->y_coord);
    int sb_wire_cnt;
    for(sb_wire_cnt=0; sb_wire_cnt < FPGA->W; sb_wire_cnt++) {
        printf("  W %d Connectivity\n", sb_wire_cnt);
        int chan_wire_cnt; 
        for(chan_wire_cnt=0; chan_wire_cnt < FPGA->Fs+1; chan_wire_cnt++) {
            t_wire* wire = sb->adjacency_list[sb_wire_cnt][chan_wire_cnt];
            dump_wire(wire);
        }
    }
}

void dump_wire(t_wire* wire) {
    char* intdent = "    ";
    if (wire == NULL) {
        printf("    Wire: NULL\n");
    } else {
        printf("    Wire %d (%d,%d)<-->(%d,%d) %p\n", wire->wire_num, wire->array_of_adjacent_switchblocks[0]->x_coord, wire->array_of_adjacent_switchblocks[0]->y_coord, wire->array_of_adjacent_switchblocks[1]->x_coord, wire->array_of_adjacent_switchblocks[1]->y_coord, wire);
    }
}

t_boolean is_vertical_wire(t_wire* wire) {
    t_switchblock* sb_0 = wire->array_of_adjacent_switchblocks[0];
    t_switchblock* sb_1 = wire->array_of_adjacent_switchblocks[1];

    if(sb_0->x_coord == sb_1->x_coord) {
        return TRUE;
    } else {
        return FALSE;
    }

}

t_boolean is_positive_wire(t_wire* wire, t_switchblock* sb) {
    t_switchblock* sb_0 = wire->array_of_adjacent_switchblocks[0];
    t_switchblock* sb_1 = wire->array_of_adjacent_switchblocks[1];

    if (is_vertical_wire(wire)) {
        if(sb->y_coord < sb_1->y_coord) {
            return TRUE;
        } else {
            return FALSE;
        }

    } else {
        if(sb->x_coord < sb_1->x_coord) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

/*
 *void swap(void* a, void *b) {
 *    void* tmp = a;
 *    *a = b;
 *    *b = tmp;
 *}
 */
