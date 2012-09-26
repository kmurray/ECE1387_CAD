#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include <stdlib.h>

//There are two pins per side of the CLB
#define CLB_NUM_PINS_PER_SIDE 2

/*
 * Prototypes for typdef structures
 */
typedef struct s_block t_block;
typedef struct s_pin t_pin;
typedef struct s_net t_net;
typedef struct s_netlist t_netlist;
typedef struct s_blocklist t_blocklist;
typedef struct s_wire t_wire;
typedef struct s_switchblock t_switchblock;
typedef struct s_rr_graph t_rr_graph;
typedef struct s_FPGA t_FPGA;

/*
 * An FPGA complex block
 */
typedef enum e_CLB_SIDE {RIGHT = 0, BOTTOM, LEFT, TOP} t_CLB_SIDE;

struct s_block {
    int x_coord;
    int y_coord;
    
    t_pin*** array_of_pins; //The pins of this block from [0..3][0..CLB_NUM_PINS_PER_SIDE-1]
                           //  NOTE: the pins are indexed first by the side they reside
                           //        upon.  Zero corresponds to the right side, and moves
                           //        clockwise around the CLB.

};

/*
 * A pin on a complex block
 */
struct s_pin {
    t_block* block;
    int pin_num;

    t_wire** array_of_adjacent_wires; //The wires accessible from this pin [0..W-1], since Fc=1

};

/*
 * A net connecting two pins
 *
 * NOTE: Assuming no multi-fanout nets
 */
struct s_net {
    t_pin* source_pin;
    t_pin* sink_pin;
    
};

/*
 * The netlist (actually a netlist -- the connections between all pins in the design)
 */
struct s_netlist {
    t_net** array_of_nets; //[0..num_nets-1]
    size_t num_nets;

};

/*
 * The CLBs of the FPGA
 */
struct s_blocklist {
    t_block** array_of_blocks; //[0..num_blocks-1]
    size_t num_blocks;
};

/*
 * A wire the routing resource graph
 */
struct s_wire {
    //XY coordinates of the channels
    // NOTE: these do not correspond directly to those used in the t_block type
    //       Channels are index from 0 to W+1.
    //       e.g. channel_x_coord == 0 -> the far left vertical routing channel
    int channel_x_coord;
    int channel_y_coord;

    t_net* associated_net;  //The net this wire is part of

    t_switchblock** array_of_adjacent_switchblocks; //[0..num_switchblocks-1] Array of the switchblocks this wire connects to
    int num_switchblocks;  //The number of switch blocks

    t_pin** array_of_adjacent_pins; //Pins accessible from this wire [0..num_adjacent_pins-1]
    int num_adjacent_pins;

    int channel_pair_num; //Which pair of wires in the channel does this wire belong to
    int wire_num; //Wire number in the channel

    int routing_label; //The label applied and quiried by the maze routing algorithm
};


struct s_switchblock {
    //XY coordinates of the switchblock
    // NOTE: these are indexed using channel coordinates not block coordinates
    int x_coord;
    int y_coord;

    t_wire*** adjacency_list; //A two dimensional list describing the connections available in this switchblock
                              // Indexed as  [source_wire][destination_wire]
                              // Valid range [0..W-1][0..Fs-1], where W is the FPGA channel width
    int Fs; //The degree of the switch block
};

struct s_rr_graph {
    t_net* first_net;

};

struct s_FPGA {
    t_blocklist* blocklist;
    t_netlist* netlist;

    //The channel width
    int W;

    //The number of CLBs along one edge
    // assumed to be a square FPGA
    int grid_size;
};

#endif
