#ifndef MAZE_ROUTE_H
#define MAZE_ROUTE_H

extern t_FPGA* FPGA;

typedef struct s_adjacent_segs t_adjacent_segs;

struct s_adjacent_segs {
    int num_segs;
    t_wire** array_of_segs; //[0..num_segs-1]
};


t_boolean try_route_net(t_net* net_to_route);
void route_netlist(void);

#endif
