#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

//For size_t
#include <stdlib.h>

//There are two pins per side of the CLB
#define CLB_SIDES_PER_BLOCK 4
#define CLB_NUM_PINS_PER_SIDE 2
#define NUM_SB_PER_WIRE 2
/*
 * Prototypes for typdef structures
 */
typedef struct s_block t_block;
typedef struct s_pin t_pin;
typedef struct s_net t_net;
typedef struct s_netlist t_netlist;
typedef struct s_blocklist t_blocklist;
typedef struct s_switchblocklist t_switchblocklist;
typedef struct s_wire t_wire;
typedef struct s_switchblock t_switchblock;
typedef struct s_rr_graph t_rr_graph;
typedef struct s_expansion_list t_expansion_list;
typedef struct s_heap_node t_heap_node;
typedef struct s_traceback t_traceback;
typedef struct s_FPGA t_FPGA;

typedef enum e_boolean {FALSE = 0, TRUE} t_boolean;

/*
 * An FPGA complex block
 */
typedef enum e_SIDE {RIGHT = 0, TOP, LEFT, BOTTOM} t_SIDE;

struct s_block {
    int x_coord;
    int y_coord;
    
    t_pin** array_of_pins; //The pins of this block from [0..num_pins-1]
                           //  NOTE: the pins are indexed first by the side they reside
                           //        upon.  Zero corresponds to the right side, and moves
                           //        clockwise around the CLB.
    int num_pins; //The total number of pins on this block

};

struct s_switchblock {
    //XY coordinates of the switchblock
    // NOTE: these are indexed using channel coordinates not block coordinates
    int x_coord;
    int y_coord;

    t_wire*** adjacency_list; //A two dimensional list describing the connections available in this switchblock
                              // Indexed as  [source_wire][destination_wire]
                              // Valid range [0..W-1][0..num_adjacencies[source_wire_index]]

    int* num_adjacencies;    //The number of valid entries in the adjacency list
                             // Indexed from [0..W-1], need a seperate bound for each adjacency_list when building the lists
                             // After construction is complete should all have the same bounds
    int* occupancy_list;    //The number of nets utilizing a specific connectin in the switchblock.  Indexed by channel wire number
};

/*
 * A pin on a complex block
 */
struct s_pin {
    t_block* block;
    int pin_num;

    t_wire** array_of_adjacent_wires; //The wires accessible from this pin [0..W-1], since Fc=1
    int num_adjacent_wires; //Usually equal W

    t_net* associated_net;

};

/*
 * A net connecting two pins
 *
 * NOTE: Assuming no multi-fanout nets
 */
#define NET_MAX_WIRES 120
struct s_net {
    t_pin* source_pin;
    t_pin* sink_pin;

    int net_num;
    
    t_wire** associated_wires; //Array of pins from [0..num_associated_wires]  Ordered from sink (index 0) to source (index num_associated_wires-1)
    int num_associated_wires; //A value greater than zero means the wire has been routed
};

/*
 * The netlist (actually a netlist -- the connections between all pins in the design)
 */
struct s_netlist {
    t_net** array_of_nets; //[0..num_nets-1]
    size_t num_nets;

    t_net* tmp_net;
};

/*
 * The CLBs of the FPGA
 */
struct s_blocklist {
    t_block** array_of_blocks; //[0..num_blocks-1]
    size_t num_blocks;
};

/*
 * The SwitchBlocks of the FPGA
 */
struct s_switchblocklist {
    t_switchblock** array_of_switchblocks; //[0..num_switchblocks-1]
    size_t num_switchblocks;
};

/*
 * Labels used during maze routing
 */
typedef enum e_ROUTING_LABEL {NONE = -1, TARGET, SOURCE, USED, CURRENT_EXPANSION} t_ROUTING_LABEL;


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

    t_switchblock** array_of_adjacent_switchblocks; //[0..num_switchblocks-1] Array of the switchblocks this wire connects to
    int num_switchblocks;  //The number of switch blocks

    t_pin** array_of_adjacent_pins; //Pins accessible from this wire [0..num_adjacent_pins-1]
    int num_adjacent_pins;

    int channel_pair_num; //Which pair of wires in the channel does this wire belong to
    t_wire* reservoir_wire;
    int wire_num; //Wire number in the channel

    double occupancy;
    double present_cost;
    double history_cost;

    t_ROUTING_LABEL label_type; //The label applied by the maze routing algorithm
    double label_value;

    int counted; //Flag to indicate whether this wire has been counted
};


struct s_expansion_list {
    int heap_size;
    t_heap_node* heap; // array of nodes[1..heap_size]
};

struct s_heap_node {
    double key;

    t_wire* wire;
};

struct s_traceback {
    int num_segs;
    t_wire** array_of_segs;
};


struct s_FPGA {
    t_blocklist* blocklist;
    t_switchblocklist* switchblocklist;
    t_netlist* netlist;

    //The channel width
    int W;

    //The number of CLBs along one edge
    // assumed to be a square FPGA
    int grid_size;

    int Fs; //The degree of the switch block
};

#endif
