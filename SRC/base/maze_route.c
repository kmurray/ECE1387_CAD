#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include <data_structs.h>
#include <util.h>
#include <draw.h>
#include <expansion_list.h>
#include <maze_route.h>
#include <pathfinder.h>

t_expansion_list* init_expansion_list(t_net* net_to_route);
t_adjacent_segs* find_adjacent_segs(t_wire* wire);
t_adjacent_segs* _find_adjacent_segs(t_wire* wire, t_boolean include_all );
t_boolean commit_traceback(t_net* net_to_route, int do_graphics);
void commit_route_seg(t_net* net_to_route, t_wire* prev_wire, t_wire* wire_to_commit, int do_graphics);
t_switchblock* find_common_switchblock(t_wire* wire_a, t_wire* wire_b); //Should really go in util
void clean_rr_graph(void);
void clean_all_nets(void);
void clean_net(t_net* net);
void clean_wire(t_wire* wire);
void clean_net_wire_list(t_net* net);
/*void clean_sb(t_switchblock* sb, t_net* net);*/

void route_netlist(void) {
    t_netlist* netlist = FPGA->netlist;

    int unrouted_nets = netlist->num_nets;

    int do_graphics = 0;
    /*start_interactive_graphics();*/

    clock_t cstart = clock();
    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net_to_route = netlist->array_of_nets[net_index];
        printf("Routing net #%d\n", net_index);

        if (try_route_net(net_to_route, do_graphics)) {
            printf("  SUCCESSFULLY routed net #%d\n", net_index);
            unrouted_nets--;
        } else {
            printf("  FAILED to route net #%d\n", net_index);
            start_interactive_graphics();
            assert(0);
        }
#if PER_NET_GRAPHICS
        /*start_interactive_graphics();*/
#endif
    }
    clock_t cend = clock();
    printf("Finished netlist %d unrouted nets\n", unrouted_nets);

    
    float nets_per_sec = ((float) netlist->num_nets) / (((double)cend - (double)cstart)* 1.0e-6);
    printf("\tProcessed %.2f nets per second\n", nets_per_sec);

    /*start_interactive_graphics();*/
}

void route_netlist_simple_reordering(void) {
    clock_t cstart = clock();
    t_netlist* netlist = FPGA->netlist;

    int routing_iteration = 0;
    int unrouted_nets = netlist->num_nets;
    int head_index = 0;
    while (unrouted_nets > 0) {
        int net_index;

        unrouted_nets = netlist->num_nets;

        //Print the re-ordered netlist
        printf("\n");
        for(net_index = 0; net_index < netlist->num_nets; net_index++) {
            t_net* net_to_route = netlist->array_of_nets[net_index];
            printf("%d ", net_to_route->net_num);
        }
        printf("\n");


        //Rip up all nets and re-try
        for(net_index = 0; net_index < netlist->num_nets; net_index++) {
            t_net* net_to_route = netlist->array_of_nets[net_index];
            clean_net(net_to_route);
        }

        routing_iteration++;

        printf("--------------- ROUTING ITERATION %d START ---------------\n", routing_iteration);
        /*start_interactive_graphics();*/
        for(net_index = 0; net_index < netlist->num_nets; net_index++) {
            t_net* net_to_route = netlist->array_of_nets[net_index];
            printf("\nRouting net %d this iteration (netlist net #%d)\n", net_index, net_to_route->net_num); 

            if (try_route_net(net_to_route, 0)) {
                printf("SUCCESSFULLY routed net #%d\n", net_index);
                unrouted_nets--;
            } else {
                printf("FAILED to route net #%d\n", net_index);
                printf("\tpushing net towards the front of the netlist\n");
                
                //Swap with net_index and head_index elements
                t_net* tmp_net = netlist->array_of_nets[head_index];
                netlist->array_of_nets[head_index] = net_to_route;
                netlist->array_of_nets[net_index] = tmp_net;
                head_index++;
                
                if(head_index >= netlist->num_nets) {
                    head_index = 0;
                }

                break;

            }
#if PER_NET_GRAPHICS
            /*start_interactive_graphics();*/
#endif
        }
        printf("---------------  ROUTING ITERATION %d END  ---------------\n", routing_iteration);
        printf("Finished netlist %d unrouted nets\n", unrouted_nets);
        
        start_interactive_graphics();


    }
    clock_t cend = clock();

    assert(unrouted_nets == 0);

    printf("\nRouting Completed after %d iterations\n", routing_iteration);
    float nets_per_sec = ((float) netlist->num_nets*routing_iteration) / (((double)cend - (double)cstart)* 1.0e-6);
    printf("\tProcessed %.2f nets per second\n", nets_per_sec);


    start_interactive_graphics();
}

t_boolean try_route_net(t_net* net_to_route, int do_graphics) {

    t_boolean found_route = FALSE;
    t_boolean found_traceback = FALSE;

    /*printf("\nRouting net %d\n", net_to_route->net_num);*/
    float prev_cost = 0.;
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
        DEBUG_PRINT("\tExpansion on wire %s\n", short_wire_name(current_seg));
       
        double next_cost = 0.;
#if DETAILED_INTERACTIVE_GRAPHICS
        /*
         *if(prev_cost < next_cost) {
         *    start_interactive_graphics();
         *}
         */
#endif

        int sb_index;
        for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
            int adjacent_seg_index;
            for(adjacent_seg_index = 0; adjacent_seg_index < adjacent_segs->num_segs[sb_index]; adjacent_seg_index++) {
                t_wire* adjacent_seg = adjacent_segs->array_of_segs[sb_index][adjacent_seg_index];
                
            /*printf("\t\tChecking wire %s\t", short_wire_name(adjacent_seg));*/

                //The cost at this adjacent_seg
                next_cost = current_seg_node.key + incr_wire_cost(adjacent_seg);
                    //BASE_RESOURCE_COST*adjacent_seg->history_cost*adjacent_seg->present_cost;
                //*(1 + adjacent_seg->occupancy);
                assert(next_cost >= 0);

                //Found the target
                if(adjacent_seg->label_type == TARGET) {
                    found_route = TRUE;
                    if (adjacent_seg->label_value == -1) {
                        DEBUG_PRINT("\t\tChecking wire %s \t Found first routing path!\n", short_wire_name(adjacent_seg));
                        //First route to find it, label it
                        adjacent_seg->label_value = next_cost;
                    } else if (next_cost < adjacent_seg->label_value) {
                        DEBUG_PRINT("\t\tChecking wire %s \t Found better routing path!\n", short_wire_name(adjacent_seg));
                        adjacent_seg->label_value = next_cost;
                    } else if (next_cost == adjacent_seg->label_value){
                        DEBUG_PRINT("\t\tChecking wire %s \t Found equivalent routing path\n", short_wire_name(adjacent_seg));
                    } else {
                        DEBUG_PRINT("\t\tChecking wire %s \t Found worse routing path\n", short_wire_name(adjacent_seg));
                    }
                    continue;
                    /*break;*/
                }

                //Segment already used/labeled
                /*
                 *if(adjacent_seg->label_type == USED) {
                 *    DEBUG_PRINT("\t\tChecking wire %s \t Wire already in use\n", short_wire_name(adjacent_seg));
                 *    continue;
                 *}
                 */
                if(adjacent_seg->label_type == CURRENT_EXPANSION) {
                    DEBUG_PRINT("\t\tChecking wire %s \t Wire already in expansion\n", short_wire_name(adjacent_seg));
                    continue;
                }
                if(adjacent_seg->label_type == SOURCE) {
                    DEBUG_PRINT("\t\tChecking wire %s \t Wire is source\n", short_wire_name(adjacent_seg));
                    continue;
                }

                //Label new segment
                adjacent_seg->label_type = CURRENT_EXPANSION;
                adjacent_seg->label_value = next_cost;
                

                //Add to expansion list at current cost
                insert_expansion_list(expansion_list, adjacent_seg, adjacent_seg->label_value);
                DEBUG_PRINT("\t\tChecking wire %s \t Adding wire to expansion at cost %f\n", short_wire_name(adjacent_seg), adjacent_seg->label_value);

            }
        }
#if DEBUG
        verify_heap(expansion_list);
#endif
        free_adjacent_segs(adjacent_segs);
        prev_cost = next_cost;
    }//while expansion_list

    if (found_route == TRUE) {
        found_traceback = commit_traceback(net_to_route, do_graphics);
    } else {
        /*
         *printf("Found no expansion route\n");
         *start_interactive_graphics();
         *assert(0);
         */
    }

    //Remove expansion labels etc
    clean_rr_graph();

    return (found_route && found_traceback);
}

t_expansion_list* init_expansion_list(t_net* net){
    t_pin* source_pin = net->source_pin;
    t_pin* sink_pin   = net->sink_pin;
   
    //Uniform channel widths
    assert(source_pin->num_adjacent_wires == sink_pin->num_adjacent_wires);

    //Careful care must be taken to cover the case where the source and target are the same
    //wires.  To resolve this, first add the source wire to the expansion list (so it will be
    //expanded from), and then mark the target wires.  This will ensure the expansion will have
    //a target to find.  However, we must not re-mark the target label if it was already labelled
    //a source

    //Allocate the expansion list
    t_expansion_list* expansion_list = alloc_expansion_list();

    //Label origin wires
    int source_wire_index;
    for(source_wire_index = 0; source_wire_index < source_pin->num_adjacent_wires; source_wire_index++) {
        t_wire* source_wire = source_pin->array_of_adjacent_wires[source_wire_index];

        source_wire->label_type = SOURCE;
        source_wire->label_value = incr_wire_cost(source_wire); //source_wire->present_cost*(source_wire->occupancy+1) + source_wire->history_cost;
        //Source wires inserted with cost 0
        insert_expansion_list(expansion_list, source_wire, source_wire->label_value);
    }

    assert(expansion_list->heap_size > 0);

    //Label target wires
    t_boolean found_target_wires = FALSE;
    int target_wire_index;
    for(target_wire_index = 0; target_wire_index < sink_pin->num_adjacent_wires; target_wire_index++) {
        t_wire* target_wire = sink_pin->array_of_adjacent_wires[target_wire_index];


        if(target_wire->label_type == SOURCE) {
            target_wire->label_type = TARGET;
        } else {
            target_wire->label_type = TARGET;
            target_wire->label_value = -1;
        }

        found_target_wires = TRUE;

    }
    assert(found_target_wires == TRUE);
    return expansion_list;
}
t_adjacent_segs* find_adjacent_segs(t_wire* wire) {
    return _find_adjacent_segs(wire, FALSE);
}

t_adjacent_segs* find_all_adjacent_segs(t_wire* wire) {
    return _find_adjacent_segs(wire, TRUE);
}

t_adjacent_segs* _find_adjacent_segs(t_wire* wire, t_boolean include_all ){

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

            if (!include_all) {
                //Check that the switch block isn't already occupied
                /*
                 *if (sb->occupancy_list[wire->wire_num] > 0) {
                 *    continue;
                 *}
                 */
                

                //Don't re-use used wires
                /*
                 *if (adjacent_wire->label_type == USED) {
                 *    continue;
                 *}
                 */
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

void free_adjacent_segs(t_adjacent_segs* adjacent_segs) {
    int sb_index;

    for(sb_index = 0; sb_index < adjacent_segs->num_sb; sb_index++) {
        //Free the array of pointers NOT the values the pointers point to
        // i.e. delete pointers to wires, but not the wires
        free(adjacent_segs->array_of_segs[sb_index]);
    }

    free(adjacent_segs->array_of_segs);
    free(adjacent_segs->num_segs);
    free(adjacent_segs);
}

t_boolean commit_traceback(t_net* net_to_route, int do_graphics) {
    if (do_graphics) {
        start_interactive_graphics();
    }
#if DETAILED_INTERACTIVE_GRAPHICS
    start_interactive_graphics();
#endif
    t_boolean found_traceback = TRUE;
    DEBUG_PRINT("\tTraceback for net %d\n", net_to_route->net_num);
    t_pin* source_pin = net_to_route->source_pin;
    t_pin* sink_pin = net_to_route->sink_pin;

    //Pick the target wire to follow back
    t_wire* current_seg = NULL;
    int adjacent_wire_cnt;

    //Find the lowest cost segment adjacent to the sink pin
    for(adjacent_wire_cnt = 0; adjacent_wire_cnt < sink_pin->num_adjacent_wires; adjacent_wire_cnt++) {
        t_wire* adjacent_seg = sink_pin->array_of_adjacent_wires[adjacent_wire_cnt];

        if (adjacent_seg->label_type != TARGET) {
            //Don't pick non-target wires
            continue;
        }

        //May be a valid target wire, but it must also have a non-negative label
        // since the label will be -1 if it is not reached
        if(adjacent_seg->label_value < 0) {
            continue;
        }

        if(current_seg == NULL) {
            current_seg = adjacent_seg;
        } else if (current_seg->label_value > adjacent_seg->label_value) {
            //Adjacent seg is a lower cost path
            current_seg = adjacent_seg;
        }
    }
    if (current_seg == NULL) {
        found_traceback = FALSE;
    } else {
        DEBUG_PRINT("\t\tPicked target wire %s\n", short_wire_name(current_seg));

        //Commit this segment of routing
        commit_route_seg(net_to_route, NULL, current_seg, do_graphics);

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

                    if(tmp_seg->label_type != CURRENT_EXPANSION && tmp_seg->label_type != SOURCE && tmp_seg->label_type != TARGET) {
                        continue;
                    }

                    if(next_seg == NULL) {
                        next_seg = tmp_seg;
                    } else if (tmp_seg->label_value < next_seg->label_value) {
                        next_seg = tmp_seg;
                    }
                }
            }
            assert(next_seg != NULL);
            if(next_seg == NULL) {
                found_traceback = FALSE;
                break;
            }
            commit_route_seg(net_to_route, current_seg, next_seg, do_graphics);
            current_seg = next_seg;
            if(current_seg == source_wire) {
                DEBUG_PRINT("\t\tPicked source_wire %s\n", short_wire_name(current_seg));
            } else {
                DEBUG_PRINT("\t\tPicked wire %s\n", short_wire_name(current_seg));
            }

            free_adjacent_segs(adjacent_segs);
        }
    }

    if(found_traceback == FALSE) {
        //Revert the committed wire segments
        clean_net(net_to_route);
    }
    return found_traceback;
}

void commit_route_seg(t_net* net_to_route, t_wire* prev_wire, t_wire* wire_to_commit, int do_graphics) {
    assert(net_to_route != NULL);
    assert(wire_to_commit != NULL);
    /*assert(wire_to_commit->label_type != USED);*/
    assert(wire_to_commit->label_type == CURRENT_EXPANSION || wire_to_commit->label_type == SOURCE || wire_to_commit->label_type == TARGET);
    if (prev_wire != NULL) {
        assert(prev_wire->wire_num == wire_to_commit->wire_num);

        //Need to mark the switchblock as used
        t_switchblock* sb_to_mark = find_common_switchblock(prev_wire, wire_to_commit);

        sb_to_mark->occupancy_list[prev_wire->wire_num]++;
    }
    
    wire_to_commit->occupancy++;
    wire_to_commit->label_type = USED;
    wire_to_commit->label_value = -1;

    //Add the wire to the net traceback path
    //Don't overflow array
    assert(net_to_route->num_associated_wires + 1 < NET_MAX_WIRES);
    net_to_route->num_associated_wires++;
    net_to_route->associated_wires[net_to_route->num_associated_wires - 1] = wire_to_commit;

    if (do_graphics) {
        start_interactive_graphics();
    }
#if DETAILED_INTERACTIVE_GRAPHICS
    start_interactive_graphics();
#endif
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

void clean_all_nets(void) {
    t_netlist* netlist = FPGA->netlist;

    int net_index;
    for(net_index = 0; net_index < netlist->num_nets; net_index++) {
        t_net* net_to_clean = netlist->array_of_nets[net_index];

        clean_net(net_to_clean);

    }
}

void clean_net(t_net* net) {

    int net_seg_index;
    for(net_seg_index = 0; net_seg_index < net->num_associated_wires; net_seg_index++) {
        t_wire* wire = net->associated_wires[net_seg_index];

        clean_wire(wire);

        assert(wire->occupancy - 1 >= 0);
        wire->occupancy--;

    }

    clean_net_wire_list(net);

}

void clean_net_wire_list(t_net* net) {
    //Reset wire list of net
    net->num_associated_wires = 0;
}

void clean_wire(t_wire* wire) {
    wire->label_type = NONE;
    wire->label_value = -1;
}

void rip_up_net(t_net* net) {
    clean_net(net);
}

void rip_up_all_nets(void) {
    clean_all_nets();
    clean_rr_graph();
}
