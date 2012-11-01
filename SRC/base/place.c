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
#include <lookahead_legalization.h>
#include <argparse.h>
#include <place.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
void dump_block_positions(void);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================

double solve_simpl(void) {
    printf("\n\n@@@@@@@@@@@@@ Start SimPL Solve @@@@@@@@@@@@\n\n");
    //Initial Placement
    solve_clique();
    solve_bound2bound(1.);

    int ps_cnt = 0;

    char buf[100];
    snprintf(buf, sizeof(buf), "%s.%d.ps", g_args->netlist_file, ps_cnt);
    init_postscript(buf);
    draw_screen();
    close_postscript();
    ps_cnt++;

    //Global Placement
    int iter_num = 0;
    double pct_overlap;
    double pct_hpwl_gap;
    double objective_value;

    //Remove the pnets from previous iterations
    remove_all_pnets(); 
    remove_all_pblocks();

    double gamma = SIMPL_GAMMA;
    while((iter_num == 0 || pct_overlap > 20.) && iter_num < 15) {
        printf("\n############# Start SimPL Iteration #%d ############\n", iter_num);
        double alpha = SIMPL_ALPHA_FAC*(1+iter_num);

        
        //Quickly legalize cell positions
        double pct_legal_overlap = evaluate_overlap();
        printf("Initial overlap: %.2f\n", pct_legal_overlap);
        int legalizer_iterations = 0;
        double max_cluster_size = g_CHIP->x_dim*g_CHIP->y_dim;
        double max_expansion_size = g_CHIP->x_dim*g_CHIP->y_dim;
        while(pct_legal_overlap > 20. && legalizer_iterations < 20) {
            lookahead_legalization(gamma, max_cluster_size, max_expansion_size);
            pct_legal_overlap = evaluate_overlap();
            printf("Lookahead legalization iteration %d overlap: %.2f (max_cluster: %.2f max_expansion: %.2f)\n", legalizer_iterations, pct_legal_overlap, max_cluster_size, max_expansion_size);

            if(legalizer_iterations > 5 && legalizer_iterations %3 == 0) {
                max_cluster_size /= 2;
                if(max_cluster_size < 1.) max_cluster_size = 1;

                max_expansion_size /= 2;
                if(max_expansion_size < 150.) max_expansion_size = 150;
            }

            legalizer_iterations++;

        }
        double legalized_hpwl = calc_hpwl_netlist(g_CHIP->netlist);
        
        //Create new bound2bound net model pnets
        int num_b2b_pnets = generate_pnets(&bound2bound_model);
        
        //Add new fixed pnets that link cells to legalized positions
        int num_legalization_pnets = add_legalized_position_pnets(alpha);

        //Re-solve the system
        objective_value = solve_system();
        double solved_hpwl = calc_hpwl_netlist(g_CHIP->netlist);


        pct_hpwl_gap = my_pct_diff(legalized_hpwl, solved_hpwl);
    
        /*if(pct_hpwl_gap > 50.) solve_bound2bound(10.);*/

        pct_overlap = evaluate_overlap();

        printf(  "############## End SimPL Iteration #%d #############\n", iter_num);
        printf("QOR:\n");
        printf("    Legal  HPWL: %.4f\n", legalized_hpwl);
        printf("    Solved HPWL: %.4f\n", solved_hpwl);
        printf("    %% HPWL Gap: %.4f\n", pct_hpwl_gap);
        printf("    %% Overlap : %.4f\n", pct_overlap);

        //Remove the pnets from previous iterations
        remove_all_pnets(); 
        remove_all_pblocks();

        draw_screen();
        if(g_args->interactive_graphics) {
            start_interactive_graphics();
        }
        iter_num++;

        snprintf(buf, sizeof(buf), "%s.%d.ps", g_args->netlist_file, ps_cnt);
        init_postscript(buf);
        draw_screen();
        close_postscript();
        ps_cnt++;
    }
    remove_all_pnets(); 
    remove_all_pblocks();
    
    printf("Finished SimPL Placement\n");
    start_interactive_graphics();

    printf("\n\n@@@@@@@@@@@@@ End SimPL Solve @@@@@@@@@@@@\n\n");
    return objective_value;
}



double solve_bound2bound(double target_pct_diff) {
    int iter_num = 0;
    double pct_diff_obj_hpwl;
    double pct_diff_hpwl_old_hpwl;
    double objective_val;
    double hpwl;
    double old_hpwl = calc_hpwl_netlist(g_CHIP->netlist);

#if DEBUG_MATRIX
    dump_block_positions();
#endif

    while(iter_num == 0 || pct_diff_hpwl_old_hpwl > target_pct_diff) {
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

        printf("-------------- End Bound2Bound Iteration #%d -------------\n", iter_num);

        evaluate_qor();
        printf("    HPWL and Objective %% diff: %.4f\n", pct_diff_obj_hpwl);
        printf("    HPWL and Prev HPWL %% diff: %.4f\n", pct_diff_hpwl_old_hpwl);
        iter_num++;
    }
    printf("\nBound2Bound Refinement Complete!\n");
    /*start_interactive_graphics();*/

    return objective_val;
}


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

    printf("************** End Clique Net Solve *************\n");

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
