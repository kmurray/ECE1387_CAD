#include <data_structs.h>
#include <util.h>
#include <argparse.h>
#include <parse_input.h>
#include <initial_solution.h>
#include <branch_and_bound.h>
#include <draw.h>

//the global data sturctures
t_block_map g_blocklist;
t_net_map g_netlist;

//Argument parsing stuff
extern struct argp argp;
t_arguments* g_args;

int main (int argc, char** argv) {
    //Parse Arguments
    g_args = parse_args(argc, argv);

    printf("Parsing Netlist...\n");
    parse_netlist(g_args->netlist_file);
    printf("DONE\n");
    
    char buf[50] = "kpartition graphics";
    init_graphics(buf);
    //start_interactive_graphics();
    

    t_bbnode* initial_solution = find_initial_solution();

    solve_bnb(initial_solution);

    //start_interactive_graphics();
    printf("\nTool Finished - exiting...\n");
    return 0;
}

