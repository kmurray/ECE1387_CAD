//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <net_models.h>
#include <solver.h>
#include <verify.h>
#include <place.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
void dump_block_positions(void);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

double solve_clique(void) {
    printf("************* Start Clique Net Solve ************\n");
    //Clean any previous operations
    remove_all_pnets();

    printf("    Generating Clique Net Model...\n");
    int num_pnets = generate_pnets(&clique_model);
    printf("    DONE\n");
    printf("    Generated %d pnets\n", num_pnets);

    start_interactive_graphics();

    //Call the solver
    double objective_val = solve_system();

    evaluate_qor();

    printf("*************  End Clique Net Solve  ************\n");

    return objective_val;
}

double solve_bound2bound(void) {
    int iter_num = 0;
    double pct_diff_obj_hpwl;
    double pct_diff_hpwl_old_hpwl;
    double objective_val;
    double hpwl;
    double old_hpwl = calc_hpwl_netlist(g_CHIP->netlist);;

#if DEBUG_MATRIX
    dump_block_positions();
#endif

    while(iter_num == 0 || pct_diff_hpwl_old_hpwl > 1) {
        printf("\n------------- Start Bound2Bound Iteration #%d ------------\n", iter_num);

        printf("    Removing Previous pnets\n");
        remove_all_pnets(); //Clean any previous operations
        printf("    Done\n");

        printf("    Re-generating Net Model...\n");
        int num_pnets = generate_pnets(&bound2bound_model);
        printf("    DONE\n");
        printf("    Generated %d pnets\n", num_pnets);

        //Call the solver
        objective_val = solve_system();

#if DEBUG_MATRIX
        dump_block_positions();
#endif

        hpwl = calc_hpwl_netlist(g_CHIP->netlist);
        pct_diff_obj_hpwl = my_pct_diff(hpwl, objective_val);
        pct_diff_hpwl_old_hpwl = my_pct_diff(hpwl, old_hpwl);

        old_hpwl = hpwl;

        printf("-------------  End Bound2Bound Iteration #%d  ------------\n", iter_num);

        evaluate_qor();
        printf("    HPWL and Objective %% diff: %.4f\n", pct_diff_obj_hpwl);
        printf("    HPWL and Prev HPWL %% diff: %.4f\n", pct_diff_hpwl_old_hpwl);
        iter_num++;
    }
    printf("\nBound2Bound Refinement Complete!\n");
    start_interactive_graphics();

    return objective_val;
}

void dump_block_positions(void) {
    printf("Block Positions:\n");
    t_blocklist* blocklist = g_CHIP->blocklist;
    for(int block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        if(!block->is_fixed) {
            printf("  Block #%d at (%f, %f)\n", block->index, block->x, block->y);
        }
    }

}
