#ifndef NET_MODELS_H
#define NET_MODELS_H

#define B2B_SMALL_DOUBLE 1e-6

int generate_pnets(int (*create_net_model)(t_net*));
int clique_model(t_net* logical_net);
int bound2bound_model(t_net* logical_net);

void remove_all_pnets(void);

#endif
