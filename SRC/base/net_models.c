//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <math.h>
#include <data_structs.h>
#include <util.h>
#include <net_models.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_pnet* create_pnet(double weight_x, double weight_y, t_block* block_a, t_block* block_b);
void add_pnet_to_block(t_pnet* pnet, t_block* block);
void remove_pnets_net(t_net* logical_net);
void remove_pnets_block(t_block* block);

//Bound2Bound Net Model Functions
void find_boundary_blocks(t_axis axis, t_net* logical_net, t_block** low_block, t_block** high_block);
t_pnet* create_b2b_pnet(t_axis axis, t_block* block_a, t_block* block_b, int fanout);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
/*
 * Generates the pnets
 *   The net model is provided as a function pointer
 */
int generate_pnets(int (*create_net_model)(t_net*)) {
    int num_pnets = 0;
    t_netlist* netlist = g_CHIP->netlist;

    int net_index;
    for(net_index = 1; net_index <= netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];

        num_pnets += create_net_model(net);
    }

    return num_pnets;
}

int clique_model(t_net* logical_net) {
    int num_pseudo_nets_added = 0;

    //Weight is 2/P for the clique model, where P is the number
    // of pins on the net
    double weight = 2.0 / logical_net->num_blocks;

    //Check that we created the correct number of pseudo_nets
    //  The correct number is P*(P-1)/2
    logical_net->num_pnets = logical_net->num_blocks*(logical_net->num_blocks - 1)/2;

    //Allocate the pnet array
    logical_net->equivalent_pnets = my_realloc(logical_net->equivalent_pnets, sizeof(t_pnet*)*logical_net->num_pnets);

    int index_a;
    for(index_a = 0; index_a < logical_net->num_blocks - 1; index_a++) {
        t_block* block_a = logical_net->associated_blocks[index_a];

        //Look only at nets beyond index_a.  This prevents double links in the clique
        int index_b;
        for(index_b = index_a + 1; index_b < logical_net->num_blocks; index_b++) {
            t_block* block_b = logical_net->associated_blocks[index_b];

            //Allocate the pnet and initialize
            t_pnet* pnet = create_pnet(weight, weight, block_a, block_b);

            //Store in net
            logical_net->equivalent_pnets[num_pseudo_nets_added] = pnet;

            //For fast access during connectivity matrix generation
            add_pnet_to_block(pnet, block_a);
            add_pnet_to_block(pnet, block_b);

            num_pseudo_nets_added++;

        }
    }

    assert(num_pseudo_nets_added == logical_net->num_pnets);
    return num_pseudo_nets_added;
}

int bound2bound_model(t_net* logical_net) {
    if(logical_net->num_blocks == 1) {
        //Net wiht no fanout, do nothing
        return 0;
    }
    
    //All blocks except the low and high blocks
    assert(logical_net->num_blocks > 1);
    int num_intermediate_pins = logical_net->num_blocks - 2;

    //The low and high blocks each add one pnet to the intermediate nodes
    // AND also add one connection between themselves
    int number_of_pnets_to_add_per_dimension = 2*num_intermediate_pins + 1;

    //Two dimensions
    int total_number_of_pnets_to_add = 2*number_of_pnets_to_add_per_dimension;

    //Reallocate the net's array of pnets
    logical_net->num_pnets = total_number_of_pnets_to_add;
    logical_net->equivalent_pnets = my_realloc(logical_net->equivalent_pnets, sizeof(t_pnet*)*logical_net->num_pnets);

    //Array index
    int num_pnets_added = 0;

    //Both axies
    for(t_axis axis = X_AXIS; axis <= Y_AXIS; axis++) {
        //First find the boundary pins
        t_block* low_block = NULL;
        t_block* high_block = NULL;

        //Returns the high and low blocks via pass-by-reference arguments
        find_boundary_blocks(axis, logical_net, &low_block, &high_block);

        assert(low_block != NULL);
        assert(high_block != NULL);

        //Create the pnet between the two boundary blocks
        t_pnet* boundary_pnet = create_b2b_pnet(axis, low_block, high_block, logical_net->num_blocks);

        //Add it to the array
        logical_net->equivalent_pnets[num_pnets_added] = boundary_pnet;
        num_pnets_added++; //Increment
        //Add pnet to each block
        add_pnet_to_block(boundary_pnet, low_block);
        add_pnet_to_block(boundary_pnet, high_block);

        //Create the low to intermediate, and high to intermediate nets for each
        // intermediate block
        for(int block_index = 0; block_index < logical_net->num_blocks; block_index++) {
            t_block* intermediate_block = logical_net->associated_blocks[block_index];

            if (intermediate_block == low_block || intermediate_block == high_block) {
                //Not an intermediate block => skip
                continue;
            }

            /*
             *  We create a pnet from both the low and high blocks to each intermediate net
             *    This involes:
             *      1) Create the pnet with correct rate
             *      2) Adding the pnet to logical_nets array of equivalent pnets
             *      3) Adding the pnet to each of its associated blocks
             */

            //Create 1st pnet (low to inter)
            t_pnet* low_to_intermediate_pnet = create_b2b_pnet(axis, low_block, intermediate_block, logical_net->num_blocks);
            //Add it to array
            logical_net->equivalent_pnets[num_pnets_added] = low_to_intermediate_pnet;
            num_pnets_added++; //Increment
            assert(num_pnets_added <= total_number_of_pnets_to_add);

            //Add the pnet to it's associated blocks
            add_pnet_to_block(low_to_intermediate_pnet, low_block);
            add_pnet_to_block(low_to_intermediate_pnet, intermediate_block);


            //Create 2nd pnet (high to inter)
            t_pnet* high_to_intermediate_pnet = create_b2b_pnet(axis, high_block, intermediate_block, logical_net->num_blocks);
            //Add it
            logical_net->equivalent_pnets[num_pnets_added] = high_to_intermediate_pnet;
            num_pnets_added++; //Increment
            assert(num_pnets_added <= total_number_of_pnets_to_add);

            //Add the pnet to it's associated blocks
            add_pnet_to_block(high_to_intermediate_pnet, high_block);
            add_pnet_to_block(high_to_intermediate_pnet, intermediate_block);
        }

    }

    //Sanity check
    assert(num_pnets_added == total_number_of_pnets_to_add);

    return num_pnets_added;
}

t_pnet* create_b2b_pnet(t_axis axis, t_block* block_a, t_block* block_b, int fanout) {
    double weight_x;
    double weight_y;
    double abs_dist;

    //Calculate the weights
    if(axis == X_AXIS) {
        abs_dist = my_abs(block_a->x - block_b->x);
        if(abs_dist < B2B_SMALL_DOUBLE) abs_dist = B2B_SMALL_DOUBLE;
        
        //weight = 1 / [(fanout - 1)*dist]
        weight_x = 1 / (((double)fanout - 1)*abs_dist);
        weight_y = 0.;

    } else if (axis == Y_AXIS) {
        abs_dist = my_abs(block_a->y - block_b->y);
        if(abs_dist < B2B_SMALL_DOUBLE) abs_dist = B2B_SMALL_DOUBLE;

        weight_x = 0.;
        weight_y = 1 / ( ((double)fanout - 1)*abs_dist );
    }

    //Weights are always positive
    assert(weight_x >= 0.);
    assert(weight_y >= 0.);
    assert(isfinite(weight_x));
    assert(isfinite(weight_y));

    //Avoid extremely small weights
    /*
     *if(weight_x < B2B_SMALL_DOUBLE) {
     *    weight_x = B2B_SMALL_DOUBLE;
     *}
     *if(weight_y < B2B_SMALL_DOUBLE) {
     *    weight_y = B2B_SMALL_DOUBLE;
     *}
     */

    t_pnet* pnet = create_pnet(weight_x, weight_y, block_a, block_b);
    return pnet;
}

void find_boundary_blocks(t_axis axis, t_net* logical_net, t_block** low_block, t_block** high_block) {
    //Returns the boundary blocks through the low_block and high_block pointers

    for(int block_index = 0; block_index < logical_net->num_blocks; block_index++) {
        t_block* block = logical_net->associated_blocks[block_index];

        //Initialize the blocks
        if(*low_block == NULL) {
            *low_block = block;
        }
        if(*high_block == NULL) {
            *high_block = block;
        }

        if(axis == X_AXIS) {
            //Check if this block is an edge block in either direction on the x-axis
            if(block->x >= (*high_block)->x) {
                *high_block = block;
            } else if (block->x <= (*low_block)->x) {
                *low_block = block;
            }
        } else { //axis == Y_AXIS
            //Check if this block is an edge block in either direction on the y-axis
            if(block->y >= (*high_block)->y) {
                *high_block = block;
            } else if (block->y <= (*low_block)->y) {
                *low_block = block;
            }

        }
    }
    assert(low_block != high_block);
}

t_pnet* create_pnet(double weight_x, double weight_y, t_block* block_a, t_block* block_b) {
    //The entry to set
    t_pnet* pnet = my_malloc(sizeof(t_pnet));

    //Set the values
    pnet->block_a = block_a;
    pnet->block_b = block_b;
    pnet->weight_x = weight_x;
    pnet->weight_y = weight_y;

    return pnet;
}

void add_pnet_to_block(t_pnet* pnet, t_block* block) {
    block->num_pnets++; 
    block->associated_pnets = my_realloc(block->associated_pnets, sizeof(t_pnet*)*block->num_pnets);

    block->associated_pnets[block->num_pnets - 1] = pnet;
}

void remove_all_pnets(void) {
    t_netlist* netlist = g_CHIP->netlist;
    t_blocklist* blocklist = g_CHIP->blocklist;

    for(int net_index = 1; net_index <= netlist->num_nets; net_index++) {
        t_net* logical_net = netlist->array_of_nets[net_index];

        remove_pnets_net(logical_net);
    }
    for(int block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        remove_pnets_block(block);
    }
}

void remove_pnets_net(t_net* logical_net) {
    
    for(int pnet_index = 0; pnet_index < logical_net->num_pnets; pnet_index++) {
        t_pnet* pnet = logical_net->equivalent_pnets[pnet_index];

        //Free each pnet structure
        free(pnet);
    }
    //Reset for pnet reallocation
    logical_net->num_pnets = 0;
    free(logical_net->equivalent_pnets);
    logical_net->equivalent_pnets = NULL;
}

void remove_pnets_block(t_block* block) {
    block->num_pnets = 0;

    //Don't need to free the actual pnets (just the pointers) since they are freed by each logical net
    free(block->associated_pnets);
    block->associated_pnets = NULL;
    
}
