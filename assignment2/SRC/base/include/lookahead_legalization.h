#ifndef LOOKAHEAD_LEGALIZATION_H
#define LOOKAHEAD_LEGALIZATION_H
        
#define STRIP_WIDTH_FACTOR 10.

void lookahead_legalization(double gamma, double max_cluster_size, double max_expansion_size);

int add_legalized_position_pnets(double alpha);

t_gridsquare_list* build_gridsquare_list(void);
t_moveable_blocks* find_moveable_blocks_in_region(t_region* region);

#endif
