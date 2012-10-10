#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <data_structs.h>
#include <verify.h>
#include <util.h>
#include <maze_route.h>

extern t_FPGA* FPGA;

void verify_routed_netlist(void) {
    printf("\nVerifying routed netlist...\n");
    int net_index;
    t_netlist* netlist = FPGA->netlist;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net = netlist->array_of_nets[net_index];
        verify_routed_net(net);
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

        if(tmp_wire->associated_net == net) {
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

        if(tmp_wire->associated_net == net) {
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
                
                if(tmp_seg->associated_net == net) {
                    if(found_next_seg == FALSE) {

                        //Found it
                        next_seg = tmp_seg;
                        found_next_seg = TRUE;
                    } else {
                        //Avoid looping back on itself
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
    }
}

