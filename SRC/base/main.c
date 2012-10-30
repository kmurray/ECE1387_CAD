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

    if(0) {
        //Quesiton 1.
        solve_clique();

        //Question 2.
        solve_bound2bound();
    }

    //Quesiton 3.
    solve_simpl();

    printf("\nTool Finished - exiting...\n");
    return 0;
}

