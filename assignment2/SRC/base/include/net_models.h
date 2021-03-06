#ifndef NET_MODELS_H
#define NET_MODELS_H

#define B2B_SMALL_DOUBLE 1e-6

int generate_pnets(int (*create_net_model)(t_net*));
int clique_model(t_net* logical_net);
int bound2bound_model(t_net* logical_net);

t_pnet* create_pnet(double weight_x, double weight_y, t_block* block_a, t_block* block_b);
void add_pnet_to_block(t_pnet* pnet, t_block* block);
void remove_all_pnets(void);
void remove_all_pblocks(void);

#endif
