#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <data_structs.h>
#include <verify.h>
#include <util.h>
#include <maze_route.h>
#include <pathfinder.h>

extern t_FPGA* FPGA;

int print_wire_seg_stats(void);
t_wire* get_reservoir_wire(t_wire* wire);
float evaluate_phi(int total_num_segs);




void verify_routed_netlist(void) {
    printf("\nVerifying routed netlist connectivity...\n");
    int net_index;
    t_netlist* netlist = FPGA->netlist;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];
        verify_routed_net(net);
    }
    printf("DONE\n");

    printf("\vVerifying no resource sharing occurs...\n");
    int overused_resource_count = count_overused_resources();
    if (overused_resource_count > 0) {
        printf("\tError: %d resources overused\n", overused_resource_count);
    }
    printf("DONE\n");
}


void verify_routed_net(t_net* net) {
    //Traverse the net from source to sink, verifying connectivity
    t_pin* source_pin = net->source_pin;
    t_pin* sink_pin = net->sink_pin;

    t_wire* source_wire = NULL;
    t_wire* sink_wire = NULL;

    //Find the source net from the source pin
    int source_net_index;
    for(source_net_index = 0; source_net_index < source_pin->num_adjacent_wires; source_net_index++) {
        t_wire* tmp_wire = source_pin->array_of_adjacent_wires[source_net_index];

        if(tmp_wire == net->associated_wires[net->num_associated_wires - 1]) {
            //Found it
            if(source_wire == NULL) {
                source_wire = tmp_wire;
            } else {
                //Error net assoicated with multiple source channel wires
                assert(0);
            }
        }
    }
    if (source_wire == NULL) {
        printf("\tError: Could not find source wire segment for net '%d'\n", net->net_num);
        return;
    }

    //Find the sink net from the sink pin
    int sink_net_index;
    for(sink_net_index = 0; sink_net_index < sink_pin->num_adjacent_wires; sink_net_index++) {
        t_wire* tmp_wire = sink_pin->array_of_adjacent_wires[sink_net_index];

        if(tmp_wire == net->associated_wires[0]) {
            //Found it
            if(sink_wire == NULL) {
                sink_wire = tmp_wire;
            } else {
                //Error net assoicated with multiple sink channel wires
                assert(0);
            }
        }
    }
    if (source_wire == NULL) {
        printf("\tError: Could not find sink wire segment for net '%d'\n", net->net_num);
        return;
    }


    //Start at the source wire
    int associated_wire_index = net->num_associated_wires - 2; //2nd segment from source pin
    t_wire* prev_seg = NULL;
    t_wire* current_seg = source_wire;
    t_wire* next_seg = NULL;

    while(current_seg != sink_wire) {
        t_boolean found_next_seg = FALSE;
        t_adjacent_segs* adjacent_segs = find_all_adjacent_segs(current_seg);

        int sb_index;
        int adjacent_wire_cnt;
        //The adjacent tmp_seg with the same associated net as current_seg
        for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
            for(adjacent_wire_cnt = 0; adjacent_wire_cnt < adjacent_segs->num_segs[sb_index]; adjacent_wire_cnt++) {
                t_wire* tmp_seg = adjacent_segs->array_of_segs[sb_index][adjacent_wire_cnt];
                
                if(tmp_seg == prev_seg) {
                    continue;
                }
                if(tmp_seg == current_seg) {
                    continue;
                }
                if(tmp_seg == next_seg) {
                    continue;
                }
                
                if(tmp_seg == net->associated_wires[associated_wire_index]) {
                    if(found_next_seg == FALSE) {

                        //Found it
                        next_seg = tmp_seg;
                        found_next_seg = TRUE;

                        assert(associated_wire_index - 1 >= -1); //Allow the counter to go one under for the actual match
                        associated_wire_index--;
                    } else {
                        printf("\n\tError, found multiple adjacent wires '%s' & '%s' with the same associated net\n", short_wire_name(tmp_seg), short_wire_name(next_seg));
                        assert(0);
                    }
                }
            }
        }
        if(found_next_seg) {
            prev_seg = current_seg;
            current_seg = next_seg;
            next_seg = NULL;

        } else {
            printf("\n\tError: could not find next routing segment for net %d\n", net->net_num);
            assert(0);
        }

        free_adjacent_segs(adjacent_segs);
    }
}

void evaluate_qor(void) {
    printf("\nQOR RESULTS:\n");
    int total_num_segs = print_wire_seg_stats();

    float phi = evaluate_phi(total_num_segs);
    printf("  Reservoir Optimization:\n");
    printf("\tPHI: %.1f segs\t(%.1f of total # segs)\n", phi, (phi/total_num_segs));
}

//Returns the total number of segments used
int print_wire_seg_stats(void) {
    t_netlist* netlist = FPGA->netlist;

    int max_num_segs = 0;
    int total_num_segs = 0;

    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];

        int net_length = net->num_associated_wires;

        total_num_segs += net_length;

        if (net_length > max_num_segs) {
            max_num_segs = net_length;
        }
    }

    printf("  Segment Utilization\n");
    printf("\tTotal # Segs: %d segs\n", total_num_segs);
    printf("\tAVG   # Segs: %.1f segs\n", total_num_segs / (float) netlist->num_nets);
    printf("\tMAX   # Segs: %d segs\n", max_num_segs);

    return total_num_segs;

}

float evaluate_phi(int total_num_segs) {
    int num_reservoir_segs;
    t_netlist* netlist = FPGA->netlist;
    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];

        int seg_index;
        for(seg_index = 0; seg_index < net->num_associated_wires; seg_index++) {
            t_wire* net_wire = net->associated_wires[seg_index];

            t_wire* reservoir_wire = get_reservoir_wire(net_wire);
            
            // For non even channel widths there will be some wires
            // with no matching reservoir
            if(FPGA->W % 2 != 0 && reservoir_wire == NULL) {
                continue;
            }

            if (reservoir_wire->occupancy == 0) {
                num_reservoir_segs++;
            }
        }
    }

    float phi = (float) total_num_segs - (0.3 * (float) num_reservoir_segs);

    return phi;
}

t_wire* get_reservoir_wire(t_wire* wire) {
    t_switchblock* sb = wire->array_of_adjacent_switchblocks[0];

    /*printf("Wire to match: %s\n", short_wire_name(wire));*/
    //Look for a net with the same switchblocks, and the same channel_pair_num
    int track_index;
    for(track_index = 0; track_index <= FPGA->W; track_index++) {
        int wire_index;
        for(wire_index = 0; wire_index < sb->num_adjacencies[track_index]; wire_index++) {
            t_wire* new_wire = sb->adjacency_list[track_index][wire_index];
            /*printf("\tChecking Wire: %s\n", short_wire_name(new_wire));*/

            assert(new_wire->num_switchblocks == 2);
            assert(wire->num_switchblocks == 2);

            if(new_wire == wire) {
                /*printf("\t\tSkipping since same wire\n");*/
                continue;
            }

            if(new_wire->channel_pair_num != wire->channel_pair_num ) {
                /*printf("\t\tSkipping since not the same wire pair\n");*/
                continue;
            }
    
            if(new_wire->array_of_adjacent_switchblocks[0] != wire->array_of_adjacent_switchblocks[0]) {
                /*printf("\t\tSkipping swithcblock[0] doesn't match\n");*/
                continue;
            }

            if(new_wire->array_of_adjacent_switchblocks[1] != wire->array_of_adjacent_switchblocks[1]){
                /*printf("\t\tSkipping swithcblock[1] doesn't match\n");*/
                continue;
            }

            /*printf("\t\tFound Match!\n");*/
            return new_wire;
        }
    }
    /*printf("\t\tFound NO Match\n");*/
    assert(FPGA->W % 2 == 0);
    assert(0);
    return NULL; 
}
