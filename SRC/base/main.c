#include <data_structs.h>
#include <argparse.h>
#include <parse_input.h>
#include <draw.h>
#include <net_models.h>
#include <solver.h>
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

    init_graphics("kplace graphics");
    //start_interactive_graphics();

    printf("Generating net Model...\n");
    generate_pnets(&clique_model);
    printf("DONE\n");

    start_interactive_graphics();

    solve_system();

    start_interactive_graphics();
    return 0;
}

