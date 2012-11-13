#include <data_structs.h>
#include <argparse.h>
#include <parse_input.h>
#include <draw.h>

//the global CHIP data sturcture
t_CHIP* g_CHIP;

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

    start_interactive_graphics();
    printf("\nTool Finished - exiting...\n");
    return 0;
}

