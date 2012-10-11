#ifndef PATHFINDER_H
#define PATHFINDER_H

#define RESERVOIR_WIRE_INCR_COST 1.0
#define RESERVOIR_WIRE_INCR_COST_PCT 0.5
#define PRES_FAC_MULT  1.1



int pathfinder_route(void);

int count_overused_resources(void);
#endif
