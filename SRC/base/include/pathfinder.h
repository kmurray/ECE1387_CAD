#ifndef PATHFINDER_H
#define PATHFINDER_H


#define H_FAC 0.5

#define P_FAC_INITIAL 1
#define P_FAC_MULT 2

#define R_FAC_INITIAL 1.00
#define R_FAC_MULT 0.1

#define BASE_RESOURCE_COST 1



int pathfinder_route(void);

int count_overused_resources(void);

inline double incr_wire_cost(t_wire* wire);
#endif
