//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <data_structs.h>
#include <util.h>
#include <verify.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
int evaluate_partial_solution_fixed_only(t_bbnode* soln);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

int evaluate_solution(t_bbnode* soln) {
    assert(soln->free_blocks.size() == 0);
    assert(soln->left_blocks.size() == soln->right_blocks.size());
    assert(soln->left_blocks.size() + soln->right_blocks.size() == g_blocklist.size());

    int cost = evaluate_partial_solution_fixed_only(soln);

    return cost;
}

//The crossing count of blocks that have been fixed to opposite sides
int evaluate_partial_solution_fixed_only(t_bbnode* soln) {
    int cut_count = 0;

    t_net_map seen_nets;

    //Each block on the left side
    for(t_block_map::iterator left_iter = soln->left_blocks.begin(); left_iter != soln->left_blocks.end(); left_iter++) {
        t_block* left_block = left_iter->second;

        //Each net on a left block
        for(t_net_map::iterator left_net_iter = left_block->nets.begin(); left_net_iter != left_block->nets.end(); left_net_iter++) {
            t_net* left_net = left_net_iter->second;

            //Keep track of the nets we have looked at, so we don't 
            //double count
            if(seen_nets.find(left_net->index) != seen_nets.end()) {
                //Found the net, so we have already looked at it
                continue;
            }
            seen_nets[left_net->index] = left_net; //Mark this new net as seen

            //Check if any block on this left net is on the right
            for(t_block_map::iterator block_iter = left_net->blocks.begin(); block_iter != left_net->blocks.end(); block_iter++) {
                t_block* block = block_iter->second;

                //find() returns end() if the block isn't in the map
                if(soln->right_blocks.find(block->index) != soln->right_blocks.end()) {
                    cut_count++;
                    break; //No point checking further, each net contributes at most '1' to the cut count
                }
            }
        }
    }

    return cut_count;
}

void dump_solution(t_bbnode* soln) {
    printf("Left: %zu blocks\n", soln->left_blocks.size());
    dump_block_map(soln->left_blocks);

    printf("Right: %zu blocks\n", soln->right_blocks.size());
    dump_block_map(soln->right_blocks);

    printf("Free: %zu blocks\n", soln->free_blocks.size());
    dump_block_map(soln->free_blocks);
}

void dump_block_map(t_block_map blk_map) {
    int size = blk_map.size();
    int cnt = 0;
    for(t_block_map::iterator blk_iter = blk_map.begin(); blk_iter != blk_map.end(); blk_iter++) {
        t_block* blk = blk_iter->second;
        assert(blk->index == blk_iter->first);
        printf("    BLK: %d\n", blk->index);
        cnt++;
    }
    assert(size == cnt);
}

