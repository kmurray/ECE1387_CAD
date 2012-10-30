//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <math.h>
#include <data_structs.h>
#include <util.h>
#include <lookahead_legalization.h>
#include <verify.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
double calc_hpwl_netlist(t_netlist* netlist);
double calc_hpwl_net(t_net* net);
double evaluate_pnet_objective(t_pnet* pnet);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
void evaluate_qor(void) {
    t_netlist* netlist = g_CHIP->netlist;

    double objective_value = evaluate_objective();
    double hpwl = calc_hpwl_netlist(netlist);



    printf("\nQoR:\n");
    printf("    Objective Fn: %.2f\n", objective_value);
    printf("    Total HPWL  : %.2f\n", hpwl);
}

double calc_hpwl_netlist(t_netlist* netlist) {
    double hpwl = 0.;

    for(int net_index = 1; net_index <= netlist->num_nets; net_index++) {
        if(netlist->valid_nets[net_index]) {
            t_net* net = netlist->array_of_nets[net_index];

            hpwl += calc_hpwl_net(net);
        }
    }

    return hpwl;
}

double calc_hpwl_net(t_net* net) {
    //Bounding box dimensions
    double bb_left;
    double bb_right;
    double bb_top;
    double bb_bot;

    for(int block_index = 0; block_index < net->num_blocks; block_index++) {
        t_block* block = net->associated_blocks[block_index];

        if(block_index == 0) {
            bb_left = block->x;
            bb_right = block->x;
            bb_top = block->y;
            bb_bot = block->y;
        }

        if(block->x < bb_left) bb_left = block->x;
        if(block->x > bb_right) bb_right = block->x;
        if(block->y > bb_top) bb_top = block->y;
        if(block->y < bb_bot) bb_bot = block->y;
    }

    assert(bb_left <= bb_right);
    assert(bb_bot <= bb_top);

    //The HPWL
    double hpwl = (bb_right - bb_left) + (bb_top - bb_bot);
    assert(hpwl >= 0);
    assert(hpwl <= g_CHIP->x_dim + g_CHIP->y_dim); //At most the length of the chip

    return hpwl;
}

double evaluate_objective(void) {
    double objective_value = 0.;
    t_netlist* netlist = g_CHIP->netlist;

    for(int net_index = 1; net_index <= netlist->num_nets; net_index++) {
        t_net* logical_net = netlist->array_of_nets[net_index];

        for(int pnet_index = 0; pnet_index < logical_net->num_pnets; pnet_index++) {
            t_pnet* pnet = logical_net->equivalent_pnets[pnet_index];
           
            objective_value += evaluate_pnet_objective(pnet);

        }
    }
    return objective_value;
}

double evaluate_pnet_objective(t_pnet* pnet) {
    double x_objective, y_objective;
    double dist;

    //X axis
    dist = pnet->block_a->x - pnet->block_b->x;
    x_objective = pnet->weight_x*pow(dist, 2);

    //Y axis
    dist = pnet->block_a->y - pnet->block_b->y;
    y_objective = pnet->weight_y*pow(dist, 2);

    return x_objective + y_objective;
}

double evaluate_overlap(void) {

    t_gridsquare_list* gridsquare_list = build_gridsquare_list();

    int total_num_moveable_blocks = 0;
    int numerator = 0;
    for(int gs_index = 0; gs_index < gridsquare_list->num_gridsquares; gs_index++) {
        t_gridsquare* gs = gridsquare_list->array_of_gridsquares[gs_index];

        t_moveable_blocks* moveable_blocks =  find_moveable_blocks_in_region(gs->region);

        total_num_moveable_blocks += moveable_blocks->num_blocks;

        if(moveable_blocks->num_blocks > 0) {
            numerator += moveable_blocks->num_blocks - 1;
        } else {
            numerator += 0;
        }

    }

    double pct_overlap = (float) numerator / total_num_moveable_blocks * 100;
    return pct_overlap;
}
