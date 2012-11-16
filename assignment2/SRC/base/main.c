#include <data_structs.h>
#include <argparse.h>
#include <parse_input.h>
#include <draw.h>
#include <net_models.h>
#include <solver.h>
#include <verify.h>
#include <place.h>
//the global CHIP data sturcture
t_CHIP* g_CHIP;

//Argument parsing things
extern struct argp argp;
t_arguments* g_args;

int main (int argc, char** argv) {
    //Parse Arguments
    g_args = parse_args(argc, argv);

    printf("Parsing Netlist...\n");
    parse_netlist(g_args->netlist_file);
    printf("DONE\n");
    
    char buf[50] = "kplace graphics";
    init_graphics(buf);
    //start_interactive_graphics();

    if(g_args->clique) {
        //Quesiton 1.
        solve_clique();
    }

    if(g_args->bound2bound) {
        //Question 2.
        solve_clique();
        solve_bound2bound(1.);
    }

    //Quesiton 3.
    if(g_args->simpl) {
        solve_simpl();
    }

    start_interactive_graphics();
    printf("\nTool Finished - exiting...\n");
    return 0;
}

