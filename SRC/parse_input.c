#include <stdio.h>
#include <data_structs.h>

/*
 * Parse the input netlist file
 */

t_FPGA* parse_netlist(const char* filename) {
    FILE* fp = fopen(filename, 'r');

    t_FPGA* FPGA = my_malloc(sizeof(t_FPGA));

    //Temporary variables for global FPGA information
    int grid_size;
    int channel_width;
    int num_nets = 0;

    //Temporary variables for net info
    int source_x_coord, source_y_coord, source_pin;
    int sink_x_coord, sink_y_coord, sink_pin;

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

    while (fscanf(fp, "%d %d %d %d %d %d", 
                  &source_x_coord, &source_y_coord, &source_pin,
                  &sink_x_coord, &sink_y_coord, &sink_pin) != EOF
            //Not end of pin pair list
            && source_x_coord == source_y_coord == source_pin == sink_x_coord == sink_y_coord == sink_pin !== -1) {

        /*
         *  Build the netlist structure from the bottom up
         */

        //Allocate and load the pins
        t_pin* source_pin = my_malloc(sizeof(t_pin));
        block_index = get_block_index(FPGA, source_x_coord, source_y_coord); 
        source_pin->block = blocklist[block_index];
        source_pin->pin_num = source_pin;

        t_pin* sink_pin = my_malloc(sizeof(t_pin));
        block_index = get_block_index(FPGA, sink_x_coord, sink_y_coord); 
        sink_pin->block = blocklist[block_index];
        sink_pin->pin_num = sink_pin;

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
            netlist->array_of_nets = my_realloc(sizeof(t_net*), netlist->num_nets)
        }

        int net_index = netlist->num_nets - 1;
        netlist[net_index] = new_net;
    }

    FPGA->netlist = netlist;

    flcose(fp);

    return FPGA;
???
