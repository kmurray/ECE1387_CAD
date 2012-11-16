//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <data_structs.h>
#include <util.h>
#include <initial_solution.h>
#include <verify.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_block* get_random_block();

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

t_bbnode* find_initial_solution() {

    t_bbnode* initial_soln = new t_bbnode;

    //Start with all the blocks on the free list
    // NOTE: this is copy of g_blocklist not a reference
    initial_soln->free_blocks = g_blocklist;

    //Unitl we have met the even split constraint
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
        // to them into the same (LEFT) side
        for(t_net_map::iterator net_iter = seed_block->nets.begin(); net_iter != seed_block->nets.end(); net_iter++) {
            t_net* net = net_iter->second;

            //Blocks on this net
            for(t_block_map::iterator block_iter = net->blocks.begin(); block_iter != net->blocks.end(); block_iter++) {
                t_block* block = block_iter->second;

                //Skip this block if it is already in the left list
                if(initial_soln->left_blocks.find(block->index) != initial_soln->left_blocks.end()) {
                    continue;
                }

                if(initial_soln->left_blocks.size() + 1 <= g_blocklist.size()/2) {
                    //Add the item, since we aren't at the half-way point yet
                    add_block_from_freelist(block, LEFT, initial_soln);

                } else {
                    //At the halfway point, don't add any more
                    break;
                }
            }
            if(initial_soln->left_blocks.size() >= g_blocklist.size()/2) {
                //At the halfway point, don't add any more
                break;
            }
        }
    }

    //Move all the remaining blocks to the right, and empty the free list
    initial_soln->right_blocks = initial_soln->free_blocks;
    initial_soln->free_blocks.erase(initial_soln->free_blocks.begin(), initial_soln->free_blocks.end());

    assert(initial_soln->free_blocks.size() == 0);
    assert(initial_soln->left_blocks.size() == initial_soln->right_blocks.size());
    assert(initial_soln->left_blocks.size() == g_blocklist.size()/2);
    
    return initial_soln;
}

t_block* get_random_block() {
    //Generate a random index in range
    int index = rand_in_range(g_blocklist.size());

    return g_blocklist[index];

}

void add_block_from_freelist(t_block* block, t_side side, t_bbnode* bbnode) {
    //The block must be in the freelist
    t_block_map::iterator block_iter = bbnode->free_blocks.find(block->index);
    assert(block_iter != bbnode->free_blocks.end());

    //Add the block to the appropriate side
    pair<t_block_map::iterator, bool> ret;
    if(side == LEFT) {
        ret = bbnode->left_blocks.insert( t_block_pair(block->index, block) );
        //Update the max value
        if(bbnode->max_left_index < block->index) {
            bbnode->max_left_index = block->index;
        }
    } else { //RIGHT
        ret = bbnode->right_blocks.insert( t_block_pair(block->index, block) );
        //Update the max value
        if(bbnode->max_right_index < block->index) {
            bbnode->max_right_index = block->index;
        }
    }
    //Fail if something was overwritten
    assert(ret.second != false);

    //Remove the block from the freelist
    bbnode->free_blocks.erase(block->index);

}

