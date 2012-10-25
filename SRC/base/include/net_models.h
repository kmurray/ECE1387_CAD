#ifndef NET_MODELS_H
#define NET_MODELS_H


void generate_pnets(void (*create_net_model)(t_net*));
void clique_model(t_net* logical_net);
void bound2bound_model(t_net* logical_net);

void remove_all_pnets(void);

#endif
