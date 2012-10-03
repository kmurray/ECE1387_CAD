#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <expansion_list.h>
#include <maze_route.h>

t_expansion_list* init_expansion_list(t_net* net_to_route);
t_adjacent_segs* find_adjacent_segs(t_wire* wire);
t_boolean commit_traceback(t_net* net_to_route);
void commit_route_seg(t_net* net_to_route, t_wire* prev_wire, t_wire* wire_to_commit);
t_switchblock* find_common_switchblock(t_wire* wire_a, t_wire* wire_b); //Should really go in util
void clean_rr_graph(void);
void clean_wire(t_wire* wire);

void route_netlist(void) {
    t_netlist* netlist = FPGA->netlist;

    int unrouted_nets = netlist->num_nets;

    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net_to_route = netlist->array_of_nets[net_index];

        if (try_route_net(net_to_route)) {
            printf("SUCCESSFULLY routed net\n");
            unrouted_nets--;
        } else {
            printf("FAILED to route net\n");
        }
    }
    printf("Finished netlist %d unrouted nets\n", unrouted_nets);
}

t_boolean try_route_net(t_net* net_to_route) {

    t_boolean found_route = FALSE;

    printf("\nRouting net %d\n", net_to_route->net_num);
    int prev_cost = 0;
    //Labels source, target
    //Initializes expansion list
    t_expansion_list* expansion_list = init_expansion_list(net_to_route);
    while(!is_empty_expansion_list(expansion_list)) {


        //Each current segment on expansion list with the lowest label
        // Note also removes from expansion list
        t_heap_node current_seg_node = get_min_expansion_list(expansion_list);
        t_wire* current_seg = current_seg_node.wire;

        //Each new segment reachable from the current segment
        // This is an expansion
        t_adjacent_segs* adjacent_segs = find_adjacent_segs(current_seg);
        printf("\tExpansion on wire %s\n", short_wire_name(current_seg));
        
        int next_cost = current_seg_node.key + 1;
        if(prev_cost < next_cost) {
            start_interactive_graphics();
        }

        int sb_index;
        for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
            int adjacent_seg_index;
            for(adjacent_seg_index = 0; adjacent_seg_index < adjacent_segs->num_segs[sb_index]; adjacent_seg_index++) {
                t_wire* adjacent_seg = adjacent_segs->array_of_segs[sb_index][adjacent_seg_index];
                
            /*printf("\t\tChecking wire %s\t", short_wire_name(adjacent_seg));*/


                //Found the target
                if(adjacent_seg->label_type == TARGET) {
                    found_route = TRUE;
                    if (adjacent_seg->label_value == -1) {
                        printf("\t\tChecking wire %s \t Found first routing path!\n", short_wire_name(adjacent_seg));
                        //First route to find it, label it
                        adjacent_seg->label_value = next_cost;
                    } else if (next_cost < adjacent_seg->label_value) {
                        printf("\t\tChecking wire %s \t Found better routing path!\n", short_wire_name(adjacent_seg));
                        adjacent_seg->label_value = next_cost;
                    } else if (next_cost == adjacent_seg->label_value){
                        printf("\t\tChecking wire %s \t Found equivalent routing path\n", short_wire_name(adjacent_seg));
                    } else {
                        printf("\t\tChecking wire %s \t Found worse routing path\n", short_wire_name(adjacent_seg));
                    }
                    continue;
                    /*break;*/
                }

                //Segment already used/labeled
                if(adjacent_seg->label_type == USED) {
                    printf("\t\tChecking wire %s \t Wire already in use\n", short_wire_name(adjacent_seg));
                    continue;
                }
                if(adjacent_seg->label_type == CURRENT_EXPANSION) {
                    printf("\t\tChecking wire %s \t Wire already in expansion\n", short_wire_name(adjacent_seg));
                    continue;
                }
                if(adjacent_seg->label_type == SOURCE) {
                    printf("\t\tChecking wire %s \t Wire is source\n", short_wire_name(adjacent_seg));
                    continue;
                }

                //Label new segment
                adjacent_seg->label_type = CURRENT_EXPANSION;
                adjacent_seg->label_value = next_cost;
                

                //Add to expansion list at current cost
                insert_expansion_list(expansion_list, adjacent_seg, adjacent_seg->label_value);
                printf("\t\tChecking wire %s \t Adding wire to expansion at cost %d\n", short_wire_name(adjacent_seg), adjacent_seg->label_value);

            }
        }
        verify_heap(expansion_list);
        prev_cost = next_cost;
    }//expansion_list

    if (found_route == TRUE) {
        found_route = commit_traceback(net_to_route);
    }

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

        target_wire->label_type = TARGET;
        target_wire->label_value = -1;

    }

    //Allocate the expansion list
    t_expansion_list* expansion_list = alloc_expansion_list();

    //Label origin wires
    int source_wire_index;
    for(source_wire_index = 0; source_wire_index < source_pin->num_adjacent_wires; source_wire_index++) {
        t_wire* source_wire = source_pin->array_of_adjacent_wires[source_wire_index];

        source_wire->label_type = SOURCE;
        source_wire->label_value = 0;
        //Source wires inserted with cost 0
        insert_expansion_list(expansion_list, source_wire, source_wire->label_value);
    }

    return expansion_list;
}

t_adjacent_segs* find_adjacent_segs(t_wire* wire) {
    //An adjacent segs structure is used to return values (see the associated .h file)
    t_adjacent_segs* adjacent_segs = my_malloc(sizeof(t_adjacent_segs));
    adjacent_segs->num_sb = wire->num_switchblocks;
    adjacent_segs->num_segs = my_calloc(sizeof(int), adjacent_segs->num_sb);
    adjacent_segs->array_of_segs = my_calloc(sizeof(t_wire*), wire->num_switchblocks);

    int sb_index;
    //Each switchblock connected to this wire
    // Grab the acutal wire references
    for(sb_index = 0; sb_index < wire->num_switchblocks; sb_index++) {
        t_switchblock* sb = wire->array_of_adjacent_switchblocks[sb_index];

        //Num adjacencies of this wire.
        // -1 since we don't count this wire being included in the switchblock
        // adjacency list
        adjacent_segs->num_segs[sb_index] = (sb->num_adjacencies[wire->wire_num] - 1);

        //Allocate appropriate space to store all the references
        adjacent_segs->array_of_segs[sb_index] = my_calloc(sizeof(t_wire*), adjacent_segs->num_segs[sb_index]);

        t_wire** adjacency_list = sb->adjacency_list[wire->wire_num];
        assert(adjacency_list != NULL);

        int adjacent_wire_index;
        int adjacent_segs_index = 0;
        for(adjacent_wire_index = 0; adjacent_wire_index < sb->num_adjacencies[wire->wire_num]; adjacent_wire_index++) {
            t_wire* adjacent_wire = adjacency_list[adjacent_wire_index];

            assert(adjacent_wire != NULL);

            //Check that the switch block isn't already occupied
            if (sb->occupancy_list[adjacent_wire_index] > 0) {
                continue;
            }
            

            //Don't re-use used wires
            if (adjacent_wire->label_type == USED) {
                continue;
            }

            //'wire' is included in the switchblock adjacency list
            // but we don't want to re-expand to the current 'wire'
            // so skip it
            if (adjacent_wire == wire) {
                continue;
            } 

            assert(adjacent_segs_index < adjacent_segs->num_segs[sb_index]);

            adjacent_segs->array_of_segs[sb_index][adjacent_segs_index] = adjacent_wire;

            assert(adjacent_segs->array_of_segs[sb_index][adjacent_segs_index] != NULL);

            adjacent_segs_index++;
        }
        assert(adjacent_segs_index <= adjacent_segs->num_segs[sb_index]);
        adjacent_segs->num_segs[sb_index] = adjacent_segs_index;
    }

    //Sanity check
    int adjacent_index;
    for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
        assert(adjacent_segs->num_segs[sb_index] >= 0);
        for(adjacent_index = 0; adjacent_index < adjacent_segs->num_segs[sb_index]; adjacent_index++) {
            assert(adjacent_segs->array_of_segs[sb_index][adjacent_index] != NULL);
        }
    }

    return adjacent_segs;
}

t_boolean commit_traceback(t_net* net_to_route) {
    t_boolean found_traceback = TRUE;
    printf("\tTraceback for net %d\n", net_to_route->net_num);
    t_pin* source_pin = net_to_route->source_pin;
    t_pin* sink_pin = net_to_route->sink_pin;

    //Pick the target wire to follow back
    t_wire* current_seg = NULL;
    int adjacent_wire_cnt;
    //Find the lowest cost segment adjacent to the sink pin
    for(adjacent_wire_cnt = 0; adjacent_wire_cnt < sink_pin->num_adjacent_wires; adjacent_wire_cnt++) {
        t_wire* adjacent_seg = sink_pin->array_of_adjacent_wires[adjacent_wire_cnt];

        if(current_seg == NULL) {
            current_seg = adjacent_seg;
        } else if (current_seg->label_value > adjacent_seg->label_value) {
            //Adjacent seg is a lower cost path
            current_seg = adjacent_seg;
        }
    }
    printf("\t\tPicked target wire %s\n", short_wire_name(current_seg));

    //Commit this segment of routing
    commit_route_seg(net_to_route, NULL, current_seg);

    //Once we know what track is being used (from current_seg), we know what track is used at the source
    // since all switchblocks are disjoint!
    int track_num = current_seg->wire_num;
    assert(track_num < source_pin->num_adjacent_wires);
    t_wire* source_wire = source_pin->array_of_adjacent_wires[track_num];
    assert(source_wire->wire_num == track_num);

    //Follow the wire back to the source
    while(current_seg != source_wire) {
        t_adjacent_segs* adjacent_segs = find_adjacent_segs(current_seg);
        t_wire* next_seg = NULL;
        int sb_index;
        //find the minimum label adjacent to current_seg
        for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
            for(adjacent_wire_cnt = 0; adjacent_wire_cnt < adjacent_segs->num_segs[sb_index]; adjacent_wire_cnt++) {
                t_wire* tmp_seg = adjacent_segs->array_of_segs[sb_index][adjacent_wire_cnt];

                if(next_seg == NULL) {
                    next_seg = tmp_seg;
                } else if (tmp_seg->label_value < next_seg->label_value) {
                    next_seg = tmp_seg;
                }
            }
        }
        if(next_seg == NULL) {
            found_traceback = FALSE;
            break;
        }
        commit_route_seg(net_to_route, current_seg, next_seg);
        current_seg = next_seg;
        if(current_seg == source_wire) {
            printf("\t\tPicked source_wire %s\n", short_wire_name(current_seg));
        } else {
            printf("\t\tPicked wire %s\n", short_wire_name(current_seg));
        }
    }
    clean_rr_graph();
    return found_traceback;
}

void commit_route_seg(t_net* net_to_route, t_wire* prev_wire, t_wire* wire_to_commit) {
    assert(net_to_route != NULL);
    assert(wire_to_commit != NULL);
    assert(wire_to_commit->label_type != USED);
    if (prev_wire != NULL) {
        assert(prev_wire->wire_num == wire_to_commit->wire_num);

        //Need to mark the switchblock as used
        t_switchblock* sb_to_mark = find_common_switchblock(prev_wire, wire_to_commit);

        sb_to_mark->occupancy_list[prev_wire->wire_num]++;
    }
    wire_to_commit->associated_net = net_to_route;
    wire_to_commit->label_type = USED;
    wire_to_commit->label_value = -1;
    start_interactive_graphics();
}

t_switchblock* find_common_switchblock(t_wire* wire_a, t_wire* wire_b) {
    int a_index, b_index;
    for(a_index = 0; a_index < wire_a->num_switchblocks; a_index++) {
        t_switchblock* sb_a = wire_a->array_of_adjacent_switchblocks[a_index];
        for(b_index = 0; b_index < wire_b->num_switchblocks; b_index++) {
            t_switchblock* sb_b = wire_b->array_of_adjacent_switchblocks[b_index];

            if(sb_a == sb_b) {
                return sb_a;
            }
        }
    }
    return NULL;
}

void clean_rr_graph(void) {
    t_blocklist* blocklist = FPGA->blocklist;
    int block_cnt;
    for(block_cnt = 0; block_cnt < blocklist->num_blocks; block_cnt++) {
        t_block* block = blocklist->array_of_blocks[block_cnt];

        int pin_cnt;
        for(pin_cnt = 0; pin_cnt < block->num_pins; pin_cnt++) {
            t_pin* pin = block->array_of_pins[pin_cnt];

            int wire_cnt;
            for(wire_cnt = 0; wire_cnt < pin->num_adjacent_wires; wire_cnt++) {
                t_wire* wire = pin->array_of_adjacent_wires[wire_cnt];

                if (wire->label_type != USED) {
                    clean_wire(wire);
                }
            }
        }
    }
}

void clean_wire(t_wire* wire) {
    wire->label_type = NONE;
    wire->label_value = -1;
}
