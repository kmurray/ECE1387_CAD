#include <stdio.h>
#include <stdlib.h>
#include <argparse.h>
#include <parse_input.h>
#include <draw.h>
#include <rr_graph.h>
#include <maze_route.h>


t_FPGA* FPGA;


int main (int argc, char** argv) {
    struct arguments args;

    set_default_args(&args);
    argp_parse(&argp, argc, argv, 0, 0, &args);

    if (args.netlist_file == NULL) {
        printf("No netlist file provided\n");
    } else {
        printf("Netlist file: %s\n", args.netlist_file);
    }

    //Read in the netlist file and generate the basic FPGA structures
    parse_netlist(args.netlist_file); 
    FPGA->Fs = FPGA_FS;

    //Generate the Routing Resource Graph
    generate_rr_graph();
    verify_rr_graph();

    route_netlist();

    start_interactive_graphics(); 

    return 0;
}

