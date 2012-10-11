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
void parse_netlist(const char* filename) {
    FILE* fp = fopen(filename, "r");

    FPGA = my_malloc(sizeof(t_FPGA));

    //Temporary variables for global FPGA information
    int grid_size;
    int channel_width;

    //Temporary variables for net info
    int source_x_coord, source_y_coord, source_pin_num;
    int sink_x_coord, sink_y_coord, sink_pin_num;

    //First line is the FPGA grid size (e.g. 10x10)
    fscanf(fp, "%d", &grid_size);
    FPGA->grid_size = grid_size;

    //Second line is the channel width
    fscanf(fp, "%d", &channel_width);
    FPGA->W = channel_width;

    //Creates array_of_blocks
    t_blocklist* blocklist = allocate_blocklist(FPGA);
    FPGA->blocklist = blocklist;
   
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
        t_pin* source_pin = get_block_pin(source_x_coord, source_y_coord, source_pin_num); 
        assert(source_pin->block->x_coord == source_x_coord);
        assert(source_pin->block->y_coord == source_y_coord);

        t_pin* sink_pin = get_block_pin(sink_x_coord, sink_y_coord, sink_pin_num); 
        assert(sink_pin->block->x_coord == sink_x_coord);
        assert(sink_pin->block->y_coord == sink_y_coord);

        //Allocate the net structure
        t_net* new_net = my_malloc(sizeof(t_net));
        new_net->source_pin = source_pin;
        new_net->sink_pin = sink_pin;
        new_net->associated_wires = my_calloc(sizeof(t_wire*), NET_MAX_WIRES);
        new_net->num_associated_wires = 0;

        
        //Tag the pins with the nets
        source_pin->associated_net = new_net;
        sink_pin->associated_net = new_net;
        
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
        new_net->net_num = net_index;
        netlist->array_of_nets[net_index] = new_net;
    }

    FPGA->netlist = netlist;

    fclose(fp);

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
          
            //The pins of the block
            block->num_pins  = CLB_SIDES_PER_BLOCK*CLB_NUM_PINS_PER_SIDE;
            block->array_of_pins = my_calloc(sizeof(t_pin**), block->num_pins);
             


            //Clockwise around the block, starting from right side
            int pin_cnt;
            for(pin_cnt = 0; pin_cnt < block->num_pins; pin_cnt++) {
                //Allocate each pin
                t_pin* pin = my_malloc(sizeof(t_pin));

                //Put the pin in the array
                block->array_of_pins[pin_cnt] = pin;

                //Set pin values
                pin->block = block;
                pin->pin_num = pin_cnt;

                //Allocate the array
                pin->num_adjacent_wires = 0;
                pin->array_of_adjacent_wires = my_calloc(sizeof(t_wire*), FPGA->W);
            }

            //Add the block to the list
            blocklist->array_of_blocks[cnt] = block;
            cnt++;
        }
    }

    return blocklist;
}
