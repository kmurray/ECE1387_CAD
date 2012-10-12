#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <data_structs.h>
#include <pathfinder.h>
#include <maze_route.h>
#include <draw.h>

double Rfac;

void update_costs(int routing_iteration);
int resource_overuse_exists(void);
void reset_count_flag(void);
void copy_net(t_net* tmp_net, t_net* routed_net);
double calc_Pfac(int routing_iteration);
void update_cost_for_wire(t_wire* wire, int routing_iteration);
void update_costs_for_net(t_net* net, int routing_iteration);

extern int opt_reservoirs;
extern int interactive_graphics;

int pathfinder_route(void) {
    int routing_iteration = 0;
    int overused_resource_count;
    t_netlist* netlist = FPGA->netlist;
    start_interactive_graphics();

    while(routing_iteration == 0 || overused_resource_count > 0) {
        //Track the iteration number
        /*start_interactive_graphics();*/
        routing_iteration++;

        printf("--------------- ROUTING ITERATION %d START ---------------\n", routing_iteration);

        //Initial costs
        update_costs(routing_iteration);

        printf("Routing nets");

        //Start timer
        clock_t cstart = clock();

        int net_index;
        for(net_index = 0; net_index < netlist->num_nets; net_index++) {
            t_net* net_to_route = netlist->array_of_nets[net_index];
            
            //Save a copy of the previous routing
            copy_net(netlist->tmp_net, net_to_route);
             
            //Remove any previous routing
            rip_up_net(net_to_route);

            //Re-route the net
            /*printf("Routing net #%d...", net_to_route->net_num);*/
            printf(".");
            fflush(stdout);
            if (try_route_net(net_to_route, 0)) {
                /*printf("Successful\n");*/
            } else {
                printf("\n\tError: could not route net\n");
                assert(0);
            }


            //Update the costs of each node
            /*update_costs(routing_iteration);*/
            //Update costs of the wires that have changed
            update_costs_for_net(netlist->tmp_net, routing_iteration);
            update_costs_for_net(net_to_route, routing_iteration);

        }
        printf("\n");

        //End timer
        clock_t cend = clock();

        //See how much overuse exists
        overused_resource_count = count_overused_resources();
        

        printf("---------------  ROUTING ITERATION %d END  ---------------\n", routing_iteration);
        printf("\t%d resources overused\n", overused_resource_count);
#if DETAILED_INTERACTIVE_GRAPHICS   
        if(interactive_graphics) {
            start_interactive_graphics();
        }
#endif

        float nets_per_sec = ((float) netlist->num_nets) / (((double)cend - (double)cstart)* 1.0e-6);
        printf("\tProcessed %.2f nets per second\n", nets_per_sec);

    }
    printf("\nRouting Completed after %d iterations\n", routing_iteration);
    start_interactive_graphics();

    return routing_iteration;
}


//Returns the number of resources that are overused
int count_overused_resources(void) {

    reset_count_flag();

    int resource_overuse_count = 0;

    t_blocklist* blocklist = FPGA->blocklist;

    //Check every net
    int block_index;
    for(block_index = 0; block_index < blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        int pin_index;
        for(pin_index = 0; pin_index < block->num_pins; pin_index++) {
            t_pin* pin = block->array_of_pins[pin_index];
            
            int adjacent_wire_index;
            for(adjacent_wire_index = 0; adjacent_wire_index < pin->num_adjacent_wires; adjacent_wire_index++) {
                t_wire* wire = pin->array_of_adjacent_wires[adjacent_wire_index];

                if(wire->counted == 0) {
                    if(wire->occupancy > 1) {
                        resource_overuse_count++;
                        wire->counted++;
                    }
                }
    
            }
        }
    }
    return resource_overuse_count;
}

void reset_count_flag(void) {
    t_blocklist* blocklist = FPGA->blocklist;

    //Check every net
    int block_index;
    for(block_index = 0; block_index < blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        int pin_index;
        for(pin_index = 0; pin_index < block->num_pins; pin_index++) {
            t_pin* pin = block->array_of_pins[pin_index];
            
            int adjacent_wire_index;
            for(adjacent_wire_index = 0; adjacent_wire_index < pin->num_adjacent_wires; adjacent_wire_index++) {
                t_wire* wire = pin->array_of_adjacent_wires[adjacent_wire_index];

                wire->counted = 0;
            }
        }
    }
}

inline double incr_wire_cost(t_wire* wire) {
    /*double incr_cost = BASE_RESOURCE_COST*wire->history_cost*wire->present_cost*wire->reservoir_cost;*/

    double local_Rfac = 1.0;
    /*
     *t_wire* reservoir_wire = get_reservoir_wire(wire);
     *if(reservoir_wire->occupancy > 0) {
     *    local_Rfac = Rfac;
     *} else {
     *    //The adjacent wire is not occupied
     *    local_Rfac = 1.0;
     *}
     */
    if(opt_reservoirs) {
        if(wire->wire_num % 2 == 1) {
            local_Rfac = Rfac;
        }
    }
    double incr_cost = BASE_RESOURCE_COST*wire->history_cost*wire->present_cost*local_Rfac;
    return incr_cost;
}

double calc_Pfac(int routing_iteration) {
    double Pfac;
    if(routing_iteration == 1) {
        Pfac = P_FAC_INITIAL;    
    } else {
        Pfac = P_FAC_INITIAL * ((routing_iteration - 1)*P_FAC_MULT); 
    }
    return Pfac;
}

double calc_Rfac(int routing_iteration) {
    double Rfac;
    Rfac = 1.0 + R_FAC_INITIAL*pow(R_FAC_MULT, routing_iteration-1);
    return Rfac;
}

void update_cost_for_wire(t_wire* wire, int routing_iteration) {
    double Hfac = H_FAC; //Constant
    double Pfac = calc_Pfac(routing_iteration);
    double Rfac = calc_Rfac(routing_iteration);

    //Initial State
    if (routing_iteration == 1) {

        //Set defaults
        wire->present_cost = 1.0;
        wire->history_cost = 1.0;
    } else {
        //All subsequent routing iterations
            
        /*
         *  Pathfinder cost functions as defined in
         *    'Architecture and CAD for Deep-Submicron FPGAs'
         *         by V. Betz, J. Rose, A. Marquardt
         */
        
        //Present cost is multiplied
        wire->present_cost = 1 + (wire->occupancy + 1)*Pfac;

        //History cost increments by previous present_cost
        wire->history_cost = wire->history_cost + wire->occupancy*Hfac;
    }

    //Optimize for adjacnet wire pairs having one wire unoccupied
    /*if(wire->wire_num % 2 == 1) {*/
        /*wire->present_cost += RESERVOIR_WIRE_INCR_COST_PCT * wire->present_cost;*/
    /*}*/
    
    assert(wire->history_cost >= 0);
    assert(wire->present_cost >= 0);

}

void update_costs_for_net(t_net* net, int routing_iteration) {
    int wire_index;
    for(wire_index = 0; wire_index < net->num_associated_wires; wire_index++) {
        t_wire* wire = net->associated_wires[wire_index];

        update_cost_for_wire(wire, routing_iteration);
    }
}

void update_costs(int routing_iteration) {
    double Hfac = H_FAC; //Constant
    double Pfac = calc_Pfac(routing_iteration);
    Rfac = calc_Rfac(routing_iteration);
    printf("Updating cost for wire: Hfac: %.2f Pfac: %.2f Rfac: %.2f\n", Hfac, Pfac, Rfac);

    t_switchblocklist* sb_list = FPGA->switchblocklist;

    /*printf("Updating costs Hfac: %.2f, Pfac: %.2f\n", Hfac, Pfac);*/
    //Every net
    int sb_index;
    for(sb_index = 0; sb_index < sb_list->num_switchblocks; sb_index++) {
        t_switchblock* sb = sb_list->array_of_switchblocks[sb_index];

        int track_index;
        for(track_index = 0; track_index < FPGA->W; track_index++) {
            
            int adjacent_wire_index;
            for(adjacent_wire_index = 0; adjacent_wire_index < sb->num_adjacencies[track_index]; adjacent_wire_index++) {
                t_wire* wire = sb->adjacency_list[track_index][adjacent_wire_index];
                
                update_cost_for_wire(wire, routing_iteration);
            }
        }
    }
    
}

void copy_net(t_net* tmp_net, t_net* routed_net) {
    tmp_net->source_pin = routed_net->source_pin;
    tmp_net->sink_pin = routed_net->sink_pin;
    tmp_net->net_num = routed_net->net_num;
    tmp_net->num_associated_wires = routed_net->num_associated_wires;

    int wire_index;
    for(wire_index = 0; wire_index < tmp_net->num_associated_wires; wire_index++) {
        tmp_net->associated_wires[wire_index] = routed_net->associated_wires[wire_index];
    }
}
