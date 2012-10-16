//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <data_structs.h>
#include <util.h>
#include <net_models.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
void create_pseudo_net(float weight, t_block* block_a, t_block* block_b);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
/*
 * Generates the pnets
 *   The net model is provided as a function pointer
 */
void generate_pnets(void (*create_net_model)(t_net*)) {
    t_netlist* netlist = g_CHIP->netlist;

    int net_index;
    for(net_index = 1; net_index <= netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];

        create_net_model(net);
    }

}

void clique_model(t_net* logical_net) {
    int num_pseudo_nets = 0;

    //Weight is 2/P for the clique model, where P is the number
    // of pins on the net
    float weight = 2.0 / logical_net->num_blocks;


    int index_a;
    for(index_a = 0; index_a < logical_net->num_blocks; index_a++) {
        t_block* block_a = logical_net->associated_blocks[index_a];

        //Look only at nets beyond index_a.  This prevents double links in the clique
        int index_b;
        for(index_b = index_a + 1; index_b < logical_net->num_blocks; index_b++) {
            t_block* block_b = logical_net->associated_blocks[index_b];

            create_pseudo_net(weight, block_a, block_b);
            num_pseudo_nets++;

        }
    }

    //Check that we created the correct number of pseudo_nets
    //  The correct number is P*(P-1)/2
    assert(num_pseudo_nets == logical_net->num_blocks*(logical_net->num_blocks - 1)/2);
}

/*
 *  Creates a pseudo net of the specified weight in the global pnetlist
 */
void create_pseudo_net(float weight, t_block* block_a, t_block* block_b) {
    t_pnetlist* pnetlist = g_CHIP->pnetlist;

    pnetlist->num_pnets++;

    if(pnetlist->num_pnets > pnetlist->max_len - 1) {
        //(Re)Allocate the array of pnets
        pnetlist->max_len *= 2;

        pnetlist->array_of_pnets = my_realloc(pnetlist->array_of_pnets, sizeof(t_pnet)*pnetlist->max_len);
    }

    //Allocate the pnet
    t_pnet* pnet = my_malloc(sizeof(t_pnet));
    pnet->block_a = block_a;
    pnet->block_b = block_b;
    pnet->weight = weight;

    //Insert it into the array
    pnetlist->array_of_pnets[pnetlist->num_pnets - 1] = pnet;
}
