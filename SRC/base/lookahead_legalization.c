//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <queue>
#include <math.h>
#include <string.h>
#include <data_structs.h>
#include <util.h>
#include <solver.h>
#include <net_models.h>
#include <parse_input.h>
#include <draw.h>
#include <lookahead_legalization.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
//Find over used gridsquares
void add_block_to_gridsquare(t_gridsquare_list* gs_list, t_block* block);
int coord_to_grid_index(double x, double y);

//Cluster overused gridsquares into clustesr
t_clustered_gridsquares* cluster_gridsquares(t_gridsquare_list* gs_list, double gamma);
t_gridsquare_list* create_new_gs_cluster(t_gridsquare_list* gs_list, int gs_index);
void add_gridsquare_to_gridsquare_list(t_gridsquare_list* gs_list, t_gridsquare* gs);
t_clusterlist* clustered_gridsquares_to_clusters(t_clustered_gridsquares* clustered_gridsquares);

//Expand over used clusters
t_cluster* expand_cluster(t_cluster* cl, double gamma);
double util_gs(t_gridsquare* gs);
double util_cluster(t_cluster* cluster);
t_region* find_maximal_region(t_gridsquare_list* cl);

//find and sort blocks in cluster
t_moveable_blocks* find_moveable_blocks(t_cluster* cluster);
void add_block_to_moveable_blocks(t_moveable_blocks* movable_blocks, t_block* block);
void sort_blocks(t_axis cut_axis, t_moveable_blocks* moveable_blocks);
void verify_sorted_order(t_axis cut_axis, t_moveable_blocks* blocks);
int compare_block_pos_x(const void* elem1, const void* elem2);
int compare_block_pos_y(const void* elem1, const void* elem2);

//Retion and cluster area functions
double calc_area_region(t_region* region);
double calc_area_cluster(t_cluster* cluster);

//Partition each level into two segments
void partition_cells(t_axis cut_axis, t_cluster* cluster,
                     t_region** region_A, t_region** region_B, t_moveable_blocks** blocks_A, 
                     t_moveable_blocks** blocks_B);
double find_cell_cut(t_axis cut_axis, t_cluster* cluster);
double find_region_cut(t_axis cut_axis, t_cluster* cluster);
void split_cells(t_axis cut_axis, t_moveable_blocks* cluster_blocks, double cut, t_moveable_blocks** blocks_A_ref, t_moveable_blocks** blocks_B_ref);
void split_region(t_axis cut_axis, t_region* region, double cut, t_region** region_A_ref, t_region** region_B_ref);
void set_block_set(t_moveable_blocks* blocks, t_set set);

//Legalize a split region
t_cluster* legalize_cells_in_region(t_axis cut_axis, t_region* region, t_moveable_blocks* blocks, double gamma, t_boolean positive);

//Anchor pnet creation
void create_anchor_block_pnet(t_block* block, t_block* new_legalized_anchor, double alpha);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
void lookahead_legalization(double gamma) {

    //Create a list of all possible gridsquares
    t_gridsquare_list* gridsquare_list = build_gridsquare_list();

    //Cluster overused gridsquares
    t_clustered_gridsquares* clustered_gridsquares = cluster_gridsquares(gridsquare_list, gamma);

    //Convert overused gridsquares to cluster objects
    t_clusterlist* cluster_list = clustered_gridsquares_to_clusters(clustered_gridsquares);

    //Legalize each cluster
    for(int cluster_index = 0; cluster_index < cluster_list->num_clusters; cluster_index++) {
        printf("    Cluster #%d\n", cluster_index);

        t_cluster* cluster = cluster_list->array_of_clusters[cluster_index];
        assert(cluster->blocks->num_blocks > 0);

        t_region* base_region = (t_region*) my_malloc(sizeof(t_region));
        base_region->x_min = cluster->region->x_min - 0.1;
        base_region->x_max = cluster->region->x_max + 0.1;
        base_region->y_min = cluster->region->y_min - 0.1;
        base_region->y_max = cluster->region->y_max + 0.1;
        g_CHIP->base_region = base_region;


        //Expand the cluster to a total area s.t. util < gamma
        t_cluster* expanded_cluster = expand_cluster(cluster, gamma);
        
        t_region* expanded_region = (t_region*) my_malloc(sizeof(t_region));
        expanded_region->x_min = cluster->region->x_min - 0.1;
        expanded_region->x_max = cluster->region->x_max + 0.1;
        expanded_region->y_min = cluster->region->y_min - 0.1;
        expanded_region->y_max = cluster->region->y_max + 0.1;
        g_CHIP->expanded_region = expanded_region;

        std::queue<t_cluster*> cluster_queue;

        //Enque the cluster
        cluster_queue.push(expanded_cluster);

        //Recursivley partition the cluster
        // (note done iteratively)
        while(!cluster_queue.empty()) {
            //Grab the first element from the queue
            t_cluster* cluster = cluster_queue.front();
            cluster_queue.pop(); //Make sure to remove it

            double cluster_area = calc_area_cluster(cluster);

            assert(cluster_area > 0);
            assert(cluster->blocks->num_blocks > 0);
            
            //Are we done yet?
            if(cluster_area < 4*g_CHIP->grid_area || cluster->level >= 10) {
                /*printf("        Finished at level %d\n", cluster->level);*/
                continue;
            }

            //Alternate vertical and horizonatl cuts
            t_axis cut_axis = Y_AXIS;
            if(cluster->level % 2 == 1) cut_axis = X_AXIS;

            char x_axis_name[50] = "X_AXIS cut";
            char y_axis_name[50] = "Y_AXIS cut";

            char* axis_name = x_axis_name;
            if(cut_axis == Y_AXIS) axis_name = y_axis_name;

            //Splits the blocks and cluster into two sets
            t_region* region_A;
            t_region* region_B;
            t_moveable_blocks* blocks_A;
            t_moveable_blocks* blocks_B;
            partition_cells(cut_axis, cluster, &region_A, &region_B, &blocks_A, &blocks_B);

            set_block_set(blocks_A, A);
            set_block_set(blocks_B, B);

            g_CHIP->left_region = region_A;
            g_CHIP->right_region = region_B;

            //Spread the cells in each region
            if(blocks_A->num_blocks > 0) {
                /*printf("\tShifting A at level %d (%s)\n", cluster->level, axis_name);*/
                t_cluster* cluster_A = legalize_cells_in_region(cut_axis, region_A, blocks_A, gamma, FALSE);
                cluster_A->level = cluster->level + 1;
                cluster_queue.push(cluster_A);
            }

            if(blocks_B->num_blocks > 0) {
                /*printf("\tShifting B at level %d (%s)\n", cluster->level, axis_name);*/
                t_cluster* cluster_B = legalize_cells_in_region(cut_axis, region_B, blocks_B, gamma, TRUE);
                cluster_B->level = cluster->level + 1;
                cluster_queue.push(cluster_B);
            }

            g_CHIP->left_region = NULL;
            g_CHIP->right_region = NULL;
            
            set_block_set(blocks_A, NONE);
            set_block_set(blocks_B, NONE);
        }
    }
    g_CHIP->expanded_region = NULL;
    g_CHIP->base_region = NULL;
}

int add_legalized_position_pnets(double alpha) {
    t_blocklist* blocklist = g_CHIP->blocklist;

    t_moveable_blocks* blocks = find_num_moveable_objects();

    //We now resize the blocklist to accomodate the new pblocks which represent the fixed
    // post lookahed_legalization positions
    blocklist->pblock_start_index = blocklist->num_blocks;  //Save the stating position of the pblocks

    blocklist->num_blocks += blocks->num_blocks - 1; //Expand the list
    blocklist->array_of_blocks = (t_block**) my_realloc(blocklist->array_of_blocks, sizeof(t_block*)*blocklist->num_blocks + 1);

    int pblock_index = blocklist->pblock_start_index;
    for(int block_index = 0; block_index < blocks->num_blocks; block_index++) {
        //The values for the new plbock come from the current (post legalization) block position
        t_block* block = blocks->array_of_blocks[block_index];

        //Allocate a new pblock
        t_block* new_legalized_anchor = (t_block*) my_malloc(sizeof(t_block));
        
        //Copy over the values
        new_legalized_anchor->index = -1;
        char name[40] = "__ANCHOR__";
        new_legalized_anchor->name = strdup(name);
        new_legalized_anchor->x = block->x;
        new_legalized_anchor->y = block->y;

        //Force the object to be fixed
        new_legalized_anchor->is_fixed = TRUE;

        new_legalized_anchor->num_nets = 0;
        new_legalized_anchor->associated_nets = NULL;

        new_legalized_anchor->num_pnets = 0;
        new_legalized_anchor->associated_pnets = NULL;
        new_legalized_anchor->set = NONE;

        //Allocate the new pnet connecting the block and anchor
        create_anchor_block_pnet(block, new_legalized_anchor, alpha);

        //Add the block to the blocklist
        assert(pblock_index <= blocklist->num_blocks);
        blocklist->array_of_blocks[pblock_index] = new_legalized_anchor;
        pblock_index++;
    }

    assert(pblock_index == blocklist->num_blocks + 1);

    return 0;
}

void create_anchor_block_pnet(t_block* block, t_block* new_legalized_anchor, double alpha) {
    t_netlist* netlist = g_CHIP->netlist;

    //XXX: ugly hack, use the [0] element of the netlist to store a fake net associated with
    //     the anchoring pblocks
    t_net* fake_net;
    if(netlist->array_of_nets[0] == NULL) {
        //Allocate it
        fake_net = (t_net*) my_malloc(sizeof(t_net));
        fake_net->index = 0;
        fake_net->num_blocks = 0;
        fake_net->associated_blocks = NULL;
        fake_net->num_pnets = 0;
        fake_net->equivalent_pnets = NULL;
        
    } else {
        fake_net = netlist->array_of_nets[0];
    }

    assert(fake_net->index == 0); //No valid net has the zero index

    //Add the block to the net
    associate_block_and_net(new_legalized_anchor, fake_net);

    //Create the pnet
    t_pnet* pnet = create_pnet(alpha, alpha, block, new_legalized_anchor);

    //Add the pnet to the blocks
    add_pnet_to_block(pnet, block);
    add_pnet_to_block(pnet, new_legalized_anchor);

    //Add the pnet to the fake net
    fake_net->num_pnets++;
    fake_net->equivalent_pnets = (t_pnet**) my_realloc(fake_net->equivalent_pnets, sizeof(t_pnet*)*fake_net->num_pnets);
    fake_net->equivalent_pnets[fake_net->num_pnets - 1] = pnet;
}

t_gridsquare_list* build_gridsquare_list(void) {

    t_gridsquare_list* gridsquare_list = (t_gridsquare_list*) my_malloc(sizeof(t_gridsquare_list));
    gridsquare_list->num_gridsquares = g_CHIP->num_vert_grids*g_CHIP->num_horiz_grids;
    gridsquare_list->array_of_gridsquares = (t_gridsquare**) my_calloc(sizeof(t_gridsquare*), gridsquare_list->num_gridsquares);

    //Create the grid squares
    //Row major order
    int list_index = 0;
    for(int y_grid = 0; y_grid < g_CHIP->num_vert_grids; y_grid++) {
        for(int x_grid = 0; x_grid < g_CHIP->num_horiz_grids; x_grid++) {
            t_region* region = (t_region*) my_malloc(sizeof(t_region));

            region->x_min = (x_grid    )*g_CHIP->x_grid;
            region->x_max = (x_grid + 1)*g_CHIP->x_grid;
            region->y_min = (y_grid    )*g_CHIP->y_grid;
            region->y_max = (y_grid + 1)*g_CHIP->y_grid;

            //Allocate the gridsquare and initialize
            t_gridsquare* gridsquare = (t_gridsquare*) my_malloc(sizeof(t_gridsquare));
            gridsquare->region = region;
            gridsquare->num_blocks = 0;
            gridsquare->array_of_blocks = NULL;
            gridsquare->is_clustered = FALSE;

            gridsquare_list->array_of_gridsquares[list_index] = gridsquare;

            list_index++;
        }
    }

    t_blocklist* blocklist = g_CHIP->blocklist;

    for(int block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        if(!block->is_fixed) {
            //Add each block to the appropraite gridsquare
            add_block_to_gridsquare(gridsquare_list, block);
        }
    }

    return gridsquare_list;
}

void add_block_to_gridsquare(t_gridsquare_list* gs_list, t_block* block) {
    int index = coord_to_grid_index(block->x, block->y);

    t_gridsquare* gs = gs_list->array_of_gridsquares[index];

    gs->num_blocks++;
    gs->array_of_blocks = (t_block**) my_realloc(gs->array_of_blocks, sizeof(t_block*)*gs->num_blocks);
    gs->array_of_blocks[gs->num_blocks - 1] = block;
}


int coord_to_grid_index(double x, double y) {

    assert(g_CHIP->x_grid == 1.);
    assert(g_CHIP->y_grid == 1.);

    //Far edges fall into the last block as well
    if(x >= g_CHIP->x_dim) x -= 1.;
    if(y >= g_CHIP->y_dim) y -= 1.;
    int floor_x = floor(x);
    int floor_y = floor(y);

    //Row major order
    int grid_index = g_CHIP->num_vert_grids*floor_y + floor_x;

    assert(grid_index < g_CHIP->num_vert_grids*g_CHIP->num_horiz_grids);

    return grid_index;

}

t_clustered_gridsquares* cluster_gridsquares(t_gridsquare_list* gs_list, double gamma){
    //The list of clusters
    t_clustered_gridsquares* cl_gsquares = (t_clustered_gridsquares*) my_malloc(sizeof(t_clustered_gridsquares));
    cl_gsquares->num_clustered_gridsquares = 0;
    cl_gsquares->array_of_clustered_gridsquares = NULL;

    for(int gridsquare_index = 0; gridsquare_index < gs_list->num_gridsquares; gridsquare_index++) {

        //The grid square
        t_gridsquare* gs = gs_list->array_of_gridsquares[gridsquare_index];

        if(gs->is_clustered) continue; //Don't cluster grid squares that are already used

        double gs_util = util_gs(gs);
        if(gs_util <= gamma) continue; //Don't cluster under utilized bins

        assert(gs->num_blocks > 1);

        //Make a new cluster
        cl_gsquares->num_clustered_gridsquares++;
        cl_gsquares->array_of_clustered_gridsquares = (t_gridsquare_list**) my_realloc(cl_gsquares->array_of_clustered_gridsquares, sizeof(t_gridsquare_list*)*cl_gsquares->num_clustered_gridsquares);
        cl_gsquares->array_of_clustered_gridsquares[cl_gsquares->num_clustered_gridsquares -1] = create_new_gs_cluster(gs_list, gridsquare_index);
    }

    return cl_gsquares;
    
}

double util_gs(t_gridsquare* gs) {
    return (gs->num_blocks*g_CHIP->grid_area) / calc_area_region(gs->region);
}

t_gridsquare_list* create_new_gs_cluster(t_gridsquare_list* gs_list, int gs_index) {
    //Really stupid, don't do any clustering for now
    //TODO: Add real clustering support, time permitting

    t_gridsquare_list* gs_cluster = (t_gridsquare_list*) my_malloc(sizeof(t_gridsquare_list));
    gs_cluster->num_gridsquares = 0;
    gs_cluster->array_of_gridsquares = NULL;

    t_gridsquare* gs = gs_list->array_of_gridsquares[gs_index];

    //Initially just pass the gs through
    add_gridsquare_to_gridsquare_list(gs_cluster, gs);

    //Mark as used
    gs->is_clustered = TRUE;

    return gs_cluster;
}

void add_gridsquare_to_gridsquare_list(t_gridsquare_list* gs_list, t_gridsquare* gs) {
    gs_list->num_gridsquares++;
    gs_list->array_of_gridsquares = (t_gridsquare**) my_realloc(gs_list->array_of_gridsquares, sizeof(t_gridsquare*)*gs_list->num_gridsquares);
    gs_list->array_of_gridsquares[gs_list->num_gridsquares - 1] = gs;
}

t_clusterlist* clustered_gridsquares_to_clusters(t_clustered_gridsquares* clustered_gridsquares) {
    t_clusterlist* cl_list = (t_clusterlist*) my_malloc(sizeof(t_clusterlist));
    cl_list->num_clusters = clustered_gridsquares->num_clustered_gridsquares;
    cl_list->array_of_clusters = (t_cluster**) my_malloc(sizeof(t_cluster*)*cl_list->num_clusters);

    for(int cl_gsquare_index = 0; cl_gsquare_index < clustered_gridsquares->num_clustered_gridsquares; cl_gsquare_index++) {
        
        t_gridsquare_list* gs_list = clustered_gridsquares->array_of_clustered_gridsquares[cl_gsquare_index];

        //Init
        t_cluster* cl = (t_cluster*) my_malloc(sizeof(t_cluster));
        cl->region = find_maximal_region(gs_list);
        cl->blocks = find_moveable_blocks_in_region(cl->region);
        cl->level = 0;

        assert(cl->blocks->num_blocks > 0);
        cl_list->array_of_clusters = (t_cluster**) my_realloc(cl_list->array_of_clusters, sizeof(t_cluster*)*cl_list->num_clusters);

        cl_list->array_of_clusters[cl_gsquare_index] = cl;


    }
    return cl_list;
}

t_moveable_blocks* find_moveable_blocks_in_region(t_region* region){
    t_moveable_blocks* blocks = (t_moveable_blocks*) my_malloc(sizeof(t_moveable_blocks));
    blocks->num_blocks = 0;
    blocks->array_of_blocks = NULL;

    t_blocklist* blocklist = g_CHIP->blocklist;

    for(int blk_index = 1; blk_index <= blocklist->num_blocks; blk_index++) {
        t_block* blk = blocklist->array_of_blocks[blk_index];

        if(blk->is_fixed) continue;
        if(blk->x < region->x_min) continue;
        if(blk->x > region->x_max) continue;
        if(blk->y < region->y_min) continue;
        if(blk->y > region->y_max) continue;

        add_block_to_moveable_blocks(blocks, blk);
    }
    return blocks;
}

    


double util_cluster(t_cluster* cluster){

    return cluster->blocks->num_blocks/calc_area_region(cluster->region);
}

t_cluster* expand_cluster(t_cluster* cl, double gamma) {
    assert(gamma == 1.);
    assert(cl->blocks->num_blocks > 1);

    int expansion_cnt = 0;
    int expanded_region_num_blocks = cl->blocks->num_blocks;
    double util_cluster = expanded_region_num_blocks / calc_area_region(cl->region);

    while(util_cluster > gamma) {

        if(cl->region->x_max + g_CHIP->x_grid <= g_CHIP->x_dim) {
            cl->region->x_max += g_CHIP->x_grid;
        } else {
            cl->region->x_max = g_CHIP->x_dim;
        }

        if(cl->region->y_max + g_CHIP->y_grid <= g_CHIP->y_dim) {
            cl->region->y_max += g_CHIP->y_grid;
        } else {
            cl->region->y_max = g_CHIP->y_dim;
        }

        if(cl->region->x_min - g_CHIP->x_grid >= 0) {
            cl->region->x_min -= g_CHIP->x_grid;
        } else {
            cl->region->x_min = 0.;
        }

        if(cl->region->y_min - g_CHIP->y_grid >= 0) {
            cl->region->y_min -= g_CHIP->y_grid;
        } else {
            cl->region->y_min = 0.;
        }

        expanded_region_num_blocks = find_moveable_blocks_in_region(cl->region)->num_blocks;;

        util_cluster = expanded_region_num_blocks / calc_area_region(cl->region);
        expansion_cnt++;
    }


    return cl;
}

t_region* find_maximal_region(t_gridsquare_list* gs_list) {
    t_region* maximal_region = (t_region*) my_malloc(sizeof(t_region));

    for(int gs_index = 0; gs_index < gs_list->num_gridsquares; gs_index++) {
        t_gridsquare* gs = gs_list->array_of_gridsquares[gs_index];

        if(gs_index == 0) {
            maximal_region->y_min = gs->region->y_min;
            maximal_region->x_min = gs->region->x_min;
            maximal_region->y_max = gs->region->y_max;
            maximal_region->x_max = gs->region->x_max;
        }

        if(gs->region->x_min < maximal_region->x_min) maximal_region->x_min = gs->region->x_min;
        if(gs->region->y_min < maximal_region->y_min) maximal_region->y_min = gs->region->y_min;
        if(gs->region->x_max > maximal_region->x_max) maximal_region->x_max = gs->region->x_max;
        if(gs->region->y_max > maximal_region->y_max) maximal_region->y_max = gs->region->y_max;

    }

    return maximal_region;
}


void partition_cells(t_axis cut_axis, t_cluster* cluster,
                     t_region** region_A_ref, t_region** region_B_ref,
                     t_moveable_blocks** blocks_A_ref, t_moveable_blocks** blocks_B_ref){

    sort_blocks(cut_axis, cluster->blocks);

    //Split the cluster based on cell area
    double cell_cut = find_cell_cut(cut_axis, cluster);

    //Split the cluster based on white space
    double region_cut = find_region_cut(cut_axis, cluster);

    //Generate the two sets of cells
    split_cells(cut_axis, cluster->blocks, cell_cut, blocks_A_ref, blocks_B_ref);

    //Generate sub clusters based on cluster cut
    split_region(cut_axis, cluster->region, region_cut, region_A_ref, region_B_ref);

}

int compare_block_pos_x(const void* elem1, const void* elem2) {
    t_block** block_a = (t_block**) elem1;
    t_block** block_b = (t_block**) elem2;

    double diff = (*block_b)->x - (*block_a)->x;

    int ret_val;
    if (diff == 0.) {
        ret_val = 0; 
    } else if (diff > 0.) {
        ret_val = -1;
    } else { //diff < 0.
        ret_val = 1;
    }

    return ret_val;
}
int compare_block_pos_y(const void* elem1, const void* elem2) {
    t_block** block_a = (t_block**) elem1;
    t_block** block_b = (t_block**) elem2;

    double diff = (*block_b)->y - (*block_a)->y;

    int ret_val;
    if (diff == 0.) {
        ret_val = 0; 
    } else if (diff > 0.) {
        ret_val = -1;
    } else { //diff < 0.
        ret_val = 1;
    }

    return ret_val;
}

void sort_blocks(t_axis axis, t_moveable_blocks* moveable_blocks) {
    //Very inefficient way to find the cut is to sort
    // This should really be fixed with proper data structures that allow position based indexing

    if(axis == X_AXIS) {
        // If we are splitting down the X_AXIS, we need to sort along the Y_AXIS
        qsort(moveable_blocks->array_of_blocks, moveable_blocks->num_blocks, sizeof(t_block*), compare_block_pos_y);
    } else {
        qsort(moveable_blocks->array_of_blocks, moveable_blocks->num_blocks, sizeof(t_block*), compare_block_pos_x);
    }

    /*verify_sorted_order(axis, moveable_blocks);*/

}

void verify_sorted_order(t_axis cut_axis, t_moveable_blocks* blocks) {

    assert(blocks->num_blocks >= 1);
    t_block* prev = blocks->array_of_blocks[0];
    for(int block_index = 1; block_index < blocks->num_blocks; block_index++) {
        t_block* blk = blocks->array_of_blocks[block_index];

        if(cut_axis == X_AXIS) {
            assert(blk->y >= prev->y);
        } else {
            assert(blk->x >= prev->x);
        }

        prev = blk;
    }
}

double find_cell_cut(t_axis cut_axis, t_cluster* cluster) {

    //The midpoint of the sorted array
    t_block* mid_block = cluster->blocks->array_of_blocks[cluster->blocks->num_blocks/2];

    double cut;
    if(cut_axis == X_AXIS) {
        //Choose cut point on opposite axis
        cut = mid_block->y;
    } else {
        cut = mid_block->x;
    }
    return cut;
}

double find_region_cut(t_axis axis, t_cluster* cluster) {

    //Simple choice, the midpoint of the region
    double cut;
    if(axis == X_AXIS) {
        //Choose cut point on opposite axis

        cut = cluster->region->y_min + (cluster->region->y_max - cluster->region->y_min)/2; 
    } else {
        cut = cluster->region->x_min + (cluster->region->x_max - cluster->region->x_min)/2;
    }

    return cut;
}

void add_block_to_moveable_blocks(t_moveable_blocks* moveable_blocks, t_block* block) {
    moveable_blocks->num_blocks++;
    moveable_blocks->array_of_blocks = (t_block**) my_realloc(moveable_blocks->array_of_blocks, sizeof(t_block*)*moveable_blocks->num_blocks);
    moveable_blocks->array_of_blocks[moveable_blocks->num_blocks-1] = block;
}
void split_cells(t_axis cut_axis, t_moveable_blocks* cluster_blocks, double cut, t_moveable_blocks** blocks_A_ref, t_moveable_blocks** blocks_B_ref) {

    (*blocks_A_ref) = (t_moveable_blocks*) my_malloc(sizeof(t_moveable_blocks));
    (*blocks_A_ref)->num_blocks = 0;
    (*blocks_A_ref)->array_of_blocks = NULL;

    (*blocks_B_ref) = (t_moveable_blocks*) my_malloc(sizeof(t_moveable_blocks));
    (*blocks_B_ref)->num_blocks = 0;
    (*blocks_B_ref)->array_of_blocks = NULL;

    for(int blk_index = 0; blk_index < cluster_blocks->num_blocks; blk_index++) {
        t_block* blk = cluster_blocks->array_of_blocks[blk_index];

        t_boolean in_blocks_A;
        if(cut_axis == X_AXIS) {
            //Choose sells along the axis perpendicular to cut
            in_blocks_A = (blk->y < cut) ? TRUE : FALSE; 
        } else {
            in_blocks_A = (blk->x < cut) ? TRUE : FALSE; 
        }

        if(in_blocks_A) {
            add_block_to_moveable_blocks(*blocks_A_ref, blk);
        } else {
            add_block_to_moveable_blocks(*blocks_B_ref, blk);
        }
    }
    assert((*blocks_A_ref)->num_blocks > 0 || (*blocks_B_ref)->num_blocks > 0);
}
void split_region(t_axis cut_axis, t_region* cluster_region, double cut, t_region** region_A_ref, t_region** region_B_ref) {
    (*region_A_ref) = (t_region*) my_malloc(sizeof(t_region));
    (*region_B_ref) = (t_region*) my_malloc(sizeof(t_region));
    
    (*region_A_ref)->x_min = cluster_region->x_min;
    (*region_A_ref)->y_min = cluster_region->y_min;
    (*region_A_ref)->x_max = cluster_region->x_max;
    (*region_A_ref)->y_max = cluster_region->y_max;

    (*region_B_ref)->x_min = cluster_region->x_min;
    (*region_B_ref)->y_min = cluster_region->y_min;
    (*region_B_ref)->x_max = cluster_region->x_max;
    (*region_B_ref)->y_max = cluster_region->y_max;

    if(cut_axis == X_AXIS) {
        //Split region along axis perpendicular to cut
        (*region_A_ref)->y_max = cut;
        (*region_B_ref)->y_min = cut;
    } else {
        (*region_A_ref)->x_max = cut;
        (*region_B_ref)->x_min = cut;
    }
    assert(calc_area_region(*region_A_ref) > 0.);
    assert(calc_area_region(*region_B_ref) > 0.);
}

double calc_area_region(t_region* region) {
    return (region->x_max - region->x_min)*(region->y_max - region->y_min);
}

double calc_area_cluster(t_cluster* cluster) {
    return calc_area_region(cluster->region);
}

void set_block_set(t_moveable_blocks* blocks, t_set set) {
    for(int blk_index = 0; blk_index < blocks->num_blocks; blk_index++) {
        t_block* blk = blocks->array_of_blocks[blk_index];
        blk->set = set;
    }
}

t_cluster* legalize_cells_in_region(t_axis cut_axis, t_region* region, t_moveable_blocks* blocks, double gamma, t_boolean positive) {
    t_cluster* legal_cl = (t_cluster*) my_malloc(sizeof(t_cluster));

    legal_cl->region = region;
    legal_cl->blocks = blocks;

    assert(g_CHIP->x_grid == 1);
    assert(g_CHIP->y_grid == 1);

    double strip_width;
    double strip_height;
    double strip_area;
    double region_width;
    double region_height;
    if(cut_axis == X_AXIS) {
        region_height = region->x_max - region->x_min;
        region_width = region->y_max - region->y_min;
    } else {
        region_width = region->x_max - region->x_min;
        region_height = region->y_max - region->y_min;
    }

    /*strip_width = region_width/STRIP_WIDTH_FACTOR;*/
    strip_width = region_width/legal_cl->blocks->num_blocks;
    strip_height = region_height;
    strip_area = strip_width*strip_height;

    int strip_num = 0;
    double strip_filled_area = 0.;
    int blk_index = 0;
    while(blk_index < blocks->num_blocks) {
        //Already sorted in order
        t_block* blk = blocks->array_of_blocks[blk_index];

        if(cut_axis == X_AXIS) {
            double new_y_coord;
            if(positive) {
                new_y_coord = region->y_max - strip_num*strip_width;
            } else {
                new_y_coord = region->y_min + strip_num*strip_width;
            }

            assert(new_y_coord >= region->y_min);
            assert(new_y_coord <= region->y_max);

            //Set no coordinate
            blk->y = new_y_coord;

            //Increment area by one cell size
            strip_filled_area += g_CHIP->grid_area;

        } else {
            double new_x_coord;
            if(positive) {
                new_x_coord = region->x_max - strip_num*strip_width;
            } else {
                new_x_coord = region->x_min + strip_num*strip_width;
            }

            assert(new_x_coord >= region->x_min);
            assert(new_x_coord <= region->x_max);

            //Set no coordinate
            blk->x = new_x_coord;

            //Increment area by one cell size
            strip_filled_area += g_CHIP->grid_area;
        }

        strip_num++;

        blk_index++;
        /*start_interactive_graphics();*/
    }

    return legal_cl; 
}
