#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <data_structs.h>
#include <util.h>
#include <expansion_list.h>
#include <maze_route.h>

t_expansion_list* init_expansion_list(t_net* net_to_route);
t_adjacent_segs* find_adjacent_segs(t_wire* wire);

void route_netlist(void) {
    t_netlist* netlist = FPGA->netlist;

    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net_to_route = netlist->array_of_nets[net_index];

        if (try_route_net(net_to_route)) {
            printf("Routed net\n");
        } else {
            printf("Failed to route net\n");
        }
    }
    printf("Finishe netlist\n");
}

t_boolean try_route_net(t_net* net_to_route) {

    t_boolean found_route = FALSE;

    //Labels source, target
    //Initializes expansion list
    t_expansion_list* expansion_list = init_expansion_list(net_to_route);
    int expansion_cnt = 0;
    while(!is_empty_expansion_list(expansion_list)) {

        //Each current segment on expansion list with the lowest label
        // Note also removes from expansion list
        t_wire* current_seg = get_min_expansion_list(expansion_list);

        //Each new segment reachable from the current segment
        // This is an expansion
        t_adjacent_segs* adjacent_segs = find_adjacent_segs(current_seg);
        expansion_cnt++;

        int adjacent_seg_index;
        for(adjacent_seg_index = 0; adjacent_seg_index < adjacent_segs->num_segs; adjacent_seg_index++) {
            t_wire* adjacent_seg = adjacent_segs->array_of_segs[adjacent_seg_index];

            //Found the target
            if(adjacent_seg->label == TARGET) {
                found_route = TRUE;
                printf("Found routing path at expansion %d!\n", expansion_cnt);
                /*break;*/
            }

            //Segment already used/labeled
            if(adjacent_seg->label == USED || adjacent_seg->label == CURRENT_EXPANSION) {
                continue;
            }

            //Label new segment
            adjacent_seg->label = CURRENT_EXPANSION;

            //Add to expansion list at current cost
            insert_expansion_list(expansion_list, adjacent_seg, expansion_cnt);

        }
    }//expansion_list


    return found_route;
}

t_expansion_list* init_expansion_list(t_net* net){
    t_pin* source_pin = net->source_pin;
    t_pin* sink_pin   = net->sink_pin;
   
    //Uniform channel widths
    assert(source_pin->num_adjacent_wires == sink_pin->num_adjacent_wires);

    //Label target wires
    int target_wire_index;
    for(target_wire_index = 0; target_wire_index < sink_pin->num_adjacent_wires; target_wire_index++) {
        t_wire* target_wire = sink_pin->array_of_adjacent_wires[target_wire_index];

        target_wire->label = TARGET;

    }

    //Allocate the expansion list
    t_expansion_list* expansion_list = alloc_expansion_list();

    //Label origin wires
    int source_wire_index;
    for(source_wire_index = 0; source_wire_index < source_pin->num_adjacent_wires; source_wire_index++) {
        t_wire* source_wire = source_pin->array_of_adjacent_wires[source_wire_index];

        //Source wires inserted with cost 0
        insert_expansion_list(expansion_list, source_wire, 0);
    }

    return expansion_list;
}

t_adjacent_segs* find_adjacent_segs(t_wire* wire) {
    //An adjacent segs structure is used to return values (see the associated .h file)
    t_adjacent_segs* adjacent_segs = my_malloc(sizeof(t_adjacent_segs));

    adjacent_segs->num_segs = 0;

    int sb_index;
    //Each switchblock connected to this wire
    // First we count how many adjacnet wires there are
    // We need to do this since it is not constant for all switchblocks
    // (i.e. edges only have 1/2 to 3/4 of num of adjacent wires vs internal
    //  switchblocks)
    for(sb_index = 0; sb_index < wire->num_switchblocks; sb_index++) {
        t_switchblock* sb = wire->array_of_adjacent_switchblocks[sb_index];
        //Num adjacencies of this wire.
        // -1 since we don't count this wire being included in the switchblock
        // adjacency list
        adjacent_segs->num_segs += (sb->num_adjacencies[wire->wire_num] - 1);
    }

    //Allocate appropriate space to store all the references
    adjacent_segs->array_of_segs = my_calloc(sizeof(t_wire*), adjacent_segs->num_segs);

    int adjacent_segs_index = 0;
    //Each switchblock connected to this wire
    // Grab the acutal wire references
    for(sb_index = 0; sb_index < wire->num_switchblocks; sb_index++) {
        t_switchblock* sb = wire->array_of_adjacent_switchblocks[sb_index];

        t_wire** adjacency_list = sb->adjacency_list[wire->wire_num];
        assert(adjacency_list != NULL);

        int adjacent_wire_index;
        for(adjacent_wire_index = 0; adjacent_wire_index < sb->num_adjacencies[wire->wire_num]; adjacent_wire_index++) {
            t_wire* adjacent_wire = adjacency_list[adjacent_wire_index];

            assert(adjacent_wire != NULL);

            //'wire' is included in the switchblock adjacency list
            // but we don't want to re-expand to the current 'wire'
            // so skip it
            if (adjacent_wire != wire) {
                assert(adjacent_segs_index < adjacent_segs->num_segs);

                adjacent_segs->array_of_segs[adjacent_segs_index] = adjacent_wire;

                assert(adjacent_segs->array_of_segs[adjacent_segs_index] != NULL);

                adjacent_segs_index++;
            }
        }
    }

    //Sanity check
    int adjacent_index;
    for(adjacent_index = 0; adjacent_index < adjacent_segs->num_segs; adjacent_index++) {
        assert(adjacent_segs->array_of_segs[adjacent_index] != NULL);
    }

    return adjacent_segs;
}
