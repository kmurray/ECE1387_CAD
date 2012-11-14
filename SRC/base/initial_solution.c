//================================================================================================
// INCLUDES 
//================================================================================================
#include <data_structs.h>
#include <util.h>
#include <assert.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_block* get_random_block();
void add_block_from_freelist(t_block_map::iterator block_iter, t_side side, t_bbnode* bbnode);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

t_bbnode* find_initial_solution() {

    t_bbnode* initial_soln = new t_bbnode;

    //Start with all the blocks on the free list
    // NOTE: this is copy of g_blocklist not a reference
    initial_soln->free_blocks = g_blocklist;

    while(initial_soln->left_blocks.size() < g_blocklist.size()/2) {
        t_block* seed_block = get_random_block();

        //Skip if we have already seen this block
        if(initial_soln->left_blocks.find(seed_block->index) != initial_soln->left_blocks.end()) {
            continue;
        }

        //Put the seed block on the left side
        initial_soln->left_blocks[seed_block->index] = seed_block;
        //Remove it from the free blocks
        initial_soln->free_blocks.erase(seed_block->index);

        //Look through the nets attached to the random seed block, and greedily pack blocks connected
        // to them into one side
        for(t_net_map::iterator net_iter = seed_block->nets.begin(); net_iter != seed_block->nets.end(); net_iter++) {
            t_net* net = net_iter->second;

            for(t_block_map::iterator block_iter = net->blocks.begin(); block_iter != net->blocks.end(); block_iter++) {

                if(initial_soln->left_blocks.size() + 1 <= g_blocklist.size()/2) {
                    //Add the item, since we aren't at the half-way point yet
                    add_block_from_freelist(block_iter, LEFT, initial_soln);

                } else {
                    //At the halfway point, don't add any more
                    break;
                }
            }
            if(initial_soln->left_blocks.size() >= g_blocklist.size()/2) {
                break;
            }
        }
    }
    
    return initial_soln;
}

t_block* get_random_block() {
    //Generate a random index in range
    int index = rand_in_range(g_blocklist.size());

    return g_blocklist[index];

}

void add_block_from_freelist(t_block_map::iterator block_iter, t_side side, t_bbnode* bbnode) {
    //The block must be in the freelist
    assert(block_iter != bbnode->free_blocks.end());

    //The actual block
    t_block* block = block_iter->second;

    //Add the block to the appropriate side
    pair<t_block_map::iterator, bool> ret;
    if(side == LEFT) {
        ret = bbnode->left_blocks.insert( t_block_pair(block->index, block) );
    } else { //RIGHT
        ret = bbnode->right_blocks.insert( t_block_pair(block->index, block) );
    }
    //Fail if something was overwritten
    assert(ret.second != false);

    //Remove the block from the freelist
    bbnode->free_blocks.erase(block_iter);
}

