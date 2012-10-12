#ifndef PATHFINDER_H
#define PATHFINDER_H

#define RESERVOIR_WIRE_INCR_COST 1.0
#define RESERVOIR_WIRE_INCR_COST_PCT 0.1

#define H_FAC 0.5
#define P_FAC_INITIAL 0.5
#define P_FAC_MULT 1000
#define BASE_RESOURCE_COST 1



int pathfinder_route(void);

int count_overused_resources(void);

double incr_wire_cost(t_wire* wire);
#endif
