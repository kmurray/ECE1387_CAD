//================================================================================================
// INCLUDES 
//================================================================================================
#include <parse_input.h>
#include <assert.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_blocklist* allocate_blocklist(t_FPGA* FPGA);

//================================================================================================
//================================================================================================
/*
 * Parse the input netlist file
 */
t_FPGA* parse_netlist(const char* filename) {
    FILE* fp = fopen(filename, "r");

    t_FPGA* FPGA = my_malloc(sizeof(t_FPGA));

    //Temporary variables for global FPGA information
    int grid_size;
    int channel_width;

    //Temporary variables for net info
    int source_x_coord, source_y_coord, source_pin_num;
    int sink_x_coord, sink_y_coord, sink_pin_num;

    //First line is the FPGA grid size (e.g. 10x10)
    fscanf(fp, "%d", &grid_size);
    FPGA->grid_size = grid_size;

    //Creates array_of_blocks
    t_blocklist* blocklist = allocate_blocklist(FPGA);
    FPGA->blocklist = blocklist;

    //Second line is the channel width
    fscanf(fp, "%d", &channel_width);
    FPGA->W = channel_width;
   
    //Allocate space for the netlist structure
    t_netlist* netlist = my_malloc(sizeof(t_netlist));

    //Initialize the number of nets
    netlist->num_nets = 0;

    while ((fscanf(fp, "%d %d %d %d %d %d", 
                  &source_x_coord, &source_y_coord, &source_pin_num,
                  &sink_x_coord, &sink_y_coord, &sink_pin_num) != EOF)
            //Not end of pin pair list
           && (source_x_coord != -1) && (source_y_coord != -1) && (source_pin_num != -1)
           && (sink_x_coord != -1) && (sink_y_coord != -1) && (sink_pin_num != -1)) {

        /*
         *  Build the netlist structure from the bottom up
         */

        //Allocate and load the pins
        t_pin* source_pin = my_malloc(sizeof(t_pin));
        int block_index = get_block_index(FPGA, source_x_coord, source_y_coord); 
        source_pin->block = (t_block*) blocklist->array_of_blocks[block_index];
        assert(source_pin->block->x_coord == source_x_coord);
        assert(source_pin->block->y_coord == source_y_coord);
        source_pin->pin_num = source_pin_num;

        t_pin* sink_pin = my_malloc(sizeof(t_pin));
        block_index = get_block_index(FPGA, sink_x_coord, sink_y_coord); 
        sink_pin->block = (t_block*) blocklist->array_of_blocks[block_index];
        assert(sink_pin->block->x_coord == sink_x_coord);
        assert(sink_pin->block->y_coord == sink_y_coord);
        sink_pin->pin_num = sink_pin_num;

        //Allocate the net structure
        t_net* new_net = my_malloc(sizeof(t_net));
        new_net->source_pin = source_pin;
        new_net->sink_pin = sink_pin;
        
        
        //Allocate space for the net entry
        if (netlist->num_nets == 0) {
            //The first net
            netlist->num_nets++;
            netlist->array_of_nets = my_malloc(sizeof(t_net*));
        } else {
            //All subsequent nets
            netlist->num_nets++;

            //Doing a realloc for every net is quite inefficient and can lead to
            // heap fragmentation.  However since our benchmarks are small this
            // should not be a problem.
            netlist->array_of_nets = my_realloc(netlist->array_of_nets, sizeof(t_net*)*netlist->num_nets);
        }

        int net_index = netlist->num_nets - 1;
        netlist->array_of_nets[net_index] = new_net;
    }

    FPGA->netlist = netlist;

    fclose(fp);

    return FPGA;
}

t_blocklist* allocate_blocklist(t_FPGA* FPGA) {
    int x_coord;
    int y_coord;

    //The blocklist structure
    t_blocklist* blocklist = my_malloc(sizeof(t_blocklist));
    
    //Square FPGA
    blocklist->num_blocks = FPGA->grid_size * FPGA->grid_size;
    blocklist->array_of_blocks = my_calloc(sizeof(t_blocklist*), blocklist->num_blocks);

    size_t cnt = 0;
    for (y_coord = 1; y_coord <= FPGA->grid_size; y_coord++) {
        for (x_coord = 1; x_coord <= FPGA->grid_size; x_coord++) {
            t_block* block = my_malloc(sizeof(t_block));
            block->x_coord = x_coord;
            block->y_coord = y_coord;
          
            //The four sides of the block
            block->array_of_pins = my_calloc(sizeof(t_pin**), 4);

            //The CLB_NUM_PINS_PER_SIDE 
            t_CLB_SIDE side;

            //Clockwise around the block
            for(side = RIGHT; side <= TOP; side++) {
                block->array_of_pins[side] = my_calloc(sizeof(t_pin*), CLB_NUM_PINS_PER_SIDE);
                int side_pin_cnt;
                for(side_pin_cnt = 0; side_pin_cnt < CLB_NUM_PINS_PER_SIDE; side_pin_cnt++) {
                    //Allocate each pin
                    block->array_of_pins[side][side_pin_cnt] = my_malloc(sizeof(t_pin));
                }
            }


            blocklist->array_of_blocks[cnt] = block;
            cnt++;
        }
    }

    return blocklist;
}
