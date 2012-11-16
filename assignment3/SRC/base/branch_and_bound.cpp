//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <data_structs.h>
#include <util.h>
#include <branch_and_bound.h>
#include <initial_solution.h>
#include <verify.h>
#include <draw.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_bbnode* solve_bnb_subproblem(t_bbnode* root, t_bbnode* best);
int create_subproblems(t_bbnode* root);
t_bbnode* create_subproblem(t_bbnode* parent, int new_index);
t_bbnode* create_bnb_tree_root();

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

//Solves the branch and bound partitioning problem
t_bbnode* solve_bnb(t_bbnode* initial_soln){
    t_bbnode* root = create_bnb_tree_root();

    g_search_root = root;

    t_bbnode* solution = solve_bnb_subproblem(g_search_root, initial_soln);

    return solution;

}

t_bbnode* solve_bnb_subproblem(t_bbnode* root, t_bbnode* best) {
    //Generate sub-problems
    int num_new_subproblems = create_subproblems(root);

    if(num_new_subproblems > 0) {
        //start_interactive_graphics();
    }

    for(t_bbnode_map::iterator bbnode_iter = root->children.begin(); bbnode_iter != root->children.end(); bbnode_iter++) {
        t_bbnode* subproblem = bbnode_iter->second;

        //Recursion
        t_bbnode* intermediate_soln = solve_bnb_subproblem(subproblem, best);

        if(evaluate_partial_solution_fixed_only(best) > evaluate_partial_solution_fixed_only(intermediate_soln)) {
            //Intermediate is better
            best = intermediate_soln;
        }
    }
    return best;
}

/*
 *  Creates a set of sub-problems for the specified node
 */
int create_subproblems(t_bbnode* node) {
    int num_new_subproblems = 0;
    if(node->left_blocks.size() < g_blocklist.size()/2) {
        for(int i = (int) node->max_left_index + 1; i <= (int) g_blocklist.size(); i++) {
            t_bbnode* subproblem = create_subproblem(node, i);
            
            node->children[i] = subproblem;
            
            num_new_subproblems++;
        }
    }
    return num_new_subproblems;
}

/*
 * Creates a new subproblm based on the parent problem, and the next
 *   index to insert on the left side
 */
t_bbnode* create_subproblem(t_bbnode* parent, int new_index) {
    t_bbnode* subproblem = new t_bbnode;
    subproblem->left_blocks = parent->left_blocks;
    subproblem->right_blocks = parent->right_blocks;
    subproblem->free_blocks = parent->free_blocks;
    subproblem->max_left_index = parent->max_left_index;
    subproblem->max_right_index = parent->max_right_index;

    t_block_map::iterator block_iter = parent->free_blocks.find(new_index);
    assert(block_iter != parent->free_blocks.end());

    t_block* block_to_add = block_iter->second;

    add_block_from_freelist(block_to_add, LEFT, subproblem);

    return subproblem;
}

/*
 * Creates the root node of the search tree
 *
 *   Note that this simply places the first block (index 1) on the 
 *   left side. Compared to the case where we place NO blocks on
 *   either side, this case ignores solutions where the same
 *   partitionings are used, but assigned to different sides.
 *
 *   E.g. 
 *      For the set of blocks [1, 2, 3, 4, 5, 6]:
 *
 *      This treats the solutions:
 *          L = [1, 2, 3],  R = [4, 5, 6]
 *
 *          and
 *
 *          L = [4, 5, 6], R = [1, 2, 3]
 *
 *          As equivalent
 *
 *   This is valid, since we ultimately care only about
 *   the partitioning and not which side got which blocks.
 */
t_bbnode* create_bnb_tree_root() {
    t_bbnode* root = new t_bbnode;
    root->max_left_index = 0;
    root->max_right_index = 0;
    
    root->free_blocks = g_blocklist;

    assert(g_blocklist.size() >= 2);

    t_block_map::iterator blk1_iter = root->free_blocks.find(1);

    assert(blk1_iter != root->free_blocks.end());

    add_block_from_freelist(blk1_iter->second, LEFT, root);

    return root;
}
