#ifndef MAZE_ROUTE_H
#define MAZE_ROUTE_H

extern t_FPGA* FPGA;

typedef struct s_adjacent_segs t_adjacent_segs;

struct s_adjacent_segs {
    int* num_segs; //Number of segments [0..num_sb-1]
    int num_sb; //Number of switchblocks
    t_wire*** array_of_segs; //[0..num_sb-1][0..num_segs-1]
};


t_boolean try_route_net(t_net* net_to_route, int do_graphics);
void route_netlist(void);
void route_netlist_simple_reordering(void);
t_adjacent_segs* find_all_adjacent_segs(t_wire* wire);
void free_adjacent_segs(t_adjacent_segs* adjacent_segs);
void rip_up_all_nets(void);

#endif
