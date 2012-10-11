#include <stdio.h>
#include <stdlib.h>
#include <argparse.h>
#include <parse_input.h>
#include <draw.h>
#include <rr_graph.h>
#include <maze_route.h>
#include <pathfinder.h>
#include <verify.h>


t_FPGA* FPGA;
int interactive_graphics;
int verbose;

int main (int argc, char** argv) {
    struct arguments args;

    set_default_args(&args);
    argp_parse(&argp, argc, argv, 0, 0, &args);

    if (args.netlist_file == NULL) {
        printf("No netlist file provided\n");
    } else {
        printf("Netlist file: %s\n", args.netlist_file);
    }

    if (args.interactive_graphics) {
        interactive_graphics = 1;
    } else {
        interactive_graphics = 0;
    }
    if (args.verbose) {
        verbose = 1;
    } else {
        verbose = 0;
    }

    //Read in the netlist file and generate the basic FPGA structures
    parse_netlist(args.netlist_file); 
    FPGA->Fs = FPGA_FS;

    init_graphics("kroute graphics");


    //Generate the Routing Resource Graph
    generate_rr_graph();

    //Verify rr graph
    verify_rr_graph();


    //Route each signal in the netlist
    /*route_netlist();*/
    /*route_netlist_simple_reordering();*/
    pathfinder_route();

    verify_routed_netlist();

    //Evaluate the QoR of the routing result
    evaluate_qor();

    return 0;
}

