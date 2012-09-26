#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include <stdlib.h>

/*
 * An FPGA complex block
 */
typedef struct s_block {
    int x_coord;
    int y_coord;

} t_block;

/*
 * A pin on a complex block
 */
typedef struct s_pin {
    t_block* block;
    int pin_num;

} t_pin;

/*
 * A net connecting two pins
 *
 * NOTE: Assuming no multi-fanout nets
 */
typedef struct s_net {
    t_pin* source_pin;
    t_pin* sink_pin;
    
} t_net;

/*
 * The netlist (actually a netlist -- the connections between all pins in the design)
 */
typedef struct s_netlist {
    t_net** array_of_nets;
    size_t num_nets;

} t_netlist;

typedef struct s_blocklist {
    t_block** array_of_blocks;
    size_t num_blocks;
} t_blocklist;

typedef struct s_FPGA {
    t_blocklist* blocklist;
    t_netlist* netlist;

    //The channel width
    int W;

    //The number of CLBs along one edge
    // assumed to be a square FPGA
    int grid_size;
} t_FPGA;

#endif
