//================================================================================================
// INCLUDES 
//================================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <data_structs.h>
#include <util.h>
#include <parse_input.h>
#include <assert.h>


//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_block* parse_block_conectivity(char* line);
t_net* find_or_allocate_net(int net_index);
void add_block_to_blocklist(t_block* block);
t_block* get_block_by_index(int block_index);
t_boolean my_readline(FILE* fp, char** line);
void print_netlist_stats(void);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
/*
 * Parse the input netlist file
 */
void parse_netlist(const char* filename) {
    //Identifies the three sections of the file
    int file_section = 0;

    printf("Parsing input file: %s\n", filename);
    FILE* fp = fopen(filename, "r");

    //Allocate the chip structure and initialize
    g_CHIP = (t_CHIP*) my_malloc(sizeof(t_CHIP));

    //Allocate the block list
    t_blocklist* blocklist = (t_blocklist*) my_malloc(sizeof(t_blocklist));
    blocklist->num_blocks = 0;
    blocklist->array_of_blocks = NULL;
    g_CHIP->blocklist = blocklist; //Add to global chip structure

    //Allocate the net list
    t_netlist* netlist = (t_netlist*) my_malloc(sizeof(t_netlist));
    netlist->num_nets = 0;
    netlist->array_of_nets = NULL;
    netlist->valid_nets = NULL;
    g_CHIP->netlist = netlist; //Add to global chip structure

    //Temporay line
    char* line = NULL;

    while (my_readline(fp, &line) == TRUE && file_section <= 0) {
        if (file_section == 0) {

            //Check for end of section
            if (strcmp(line, "-1") == 0) {
                file_section++;
                continue;
            }

            //The block connectivity section

            //Creates the block object, and nets as needed
            t_block* block = parse_block_conectivity(line);

            //Add this block to the global block list
            add_block_to_blocklist(block);

            DEBUG_PRINT(EXTRA_INFO,"\tAdding Block #%d '%s'\n", block->index, block->name);

        } else {
            die("Error: Invalid file section %d", file_section);
        }
    }

    print_netlist_stats();

    fclose(fp);
}

/*
 *  Parses a line in the block connectivity section
 *    Will allocate and return the associated block
 *    If a new net is found, it is also allocated
 */
t_block* parse_block_conectivity(char* line) {
    t_boolean seen_name = FALSE;
    t_boolean seen_blk_index = FALSE;

    t_block* block = (t_block*) my_malloc(sizeof(t_block));
    //Initialization
    block->index = -1;
    block->name = NULL;
    block->x = g_CHIP->x_dim/2;
    block->y = g_CHIP->y_dim/2;
    block->is_fixed = FALSE;
    block->num_nets = 0;
    block->associated_nets = NULL;
    block->num_pnets = 0;
    block->associated_pnets = NULL;
    block->set = NONE;

    //Duplicate string first, since strtok destroys
    //the input string
    char* tok = strdup(line);
    tok = strtok(tok, TOKEN_SEPS);
    while(strcmp(tok,"-1") != 0) {
        if(!seen_name && !seen_blk_index) {
            //The block name
            block->name = strdup(tok);
            seen_name = TRUE;
        } else if (seen_name && !seen_blk_index) {
            //The block index
            
            //Convert to integer
            block->index = atoi(tok);
            seen_blk_index = TRUE;

        } else {
            //An associated net
            int net_index = atoi(tok);

            //Grab the net
            t_net* net = find_or_allocate_net(net_index);

            //Add links between block and net
            associate_block_and_net(block, net);
            
        }
        tok = strtok(NULL, TOKEN_SEPS);
    }
    assert(seen_name);
    assert(seen_blk_index);

    return block;
}

/*
 * Given a net index returns the net if it already
 *   exists, or allocates it if it does not
 */
t_net* find_or_allocate_net(int net_index) {
    t_netlist* netlist = g_CHIP->netlist;

    t_net* net = NULL;

    //First check if it already exists in the netlist
    if(net_index <= netlist->num_nets && netlist->valid_nets[net_index] == TRUE) {
        net = netlist->array_of_nets[net_index];

    } else {
        //It doesn't exist, so allocate it
        net = (t_net*) my_malloc(sizeof(t_net));

        //Initialize
        net->index = net_index;
        net->num_blocks = 0;
        net->associated_blocks = NULL;
        net->num_pnets = 0;
        net->equivalent_pnets = NULL;

        //Expand the array_of_nets
        if(netlist->num_nets < net_index) {
            //Track old values to ensure a correct update
            int old_num_nets = netlist->num_nets;

            //Expand the arrays
            netlist->num_nets = net_index;
            netlist->array_of_nets = (t_net**) my_realloc(netlist->array_of_nets, sizeof(t_net*)*(netlist->num_nets + 1));
            netlist->valid_nets = (t_boolean*) my_realloc(netlist->valid_nets, sizeof(t_boolean)*(netlist->num_nets + 1));

            //Mark any new elements beyond old_num_nets as invalid
            //  This is needed to ensure unallocated nets are not referenced
            int valid_index;
            for(valid_index = old_num_nets + 1; valid_index <= netlist->num_nets; valid_index++) {
                netlist->valid_nets[valid_index] = FALSE;    
            }
        }
        //Add to the netlist
        netlist->array_of_nets[net_index] = net;
        netlist->valid_nets[net_index] = TRUE;
    }
    assert(net != NULL);

    assert(net_index == net->index);
    return net;
}

/*
 *  Sets the cross links between a block and a net
 */
void associate_block_and_net(t_block* block, t_net* net) {
    //Add this block to the nets associated block list
    net->num_blocks++;
    net->associated_blocks = (t_block**) my_realloc(net->associated_blocks, sizeof(t_block*)*net->num_blocks);
    net->associated_blocks[net->num_blocks - 1] = block;

    //Add this net to the blocks associated net list
    block->num_nets++;
    block->associated_nets = (t_net**) my_realloc(block->associated_nets, sizeof(t_net*)*block->num_nets);
    block->associated_nets[block->num_nets - 1] = net;
}

/*
 *  Adds a block to the global blocklist structure
 */
void add_block_to_blocklist(t_block* block) {
    t_blocklist* blocklist = g_CHIP->blocklist;

    blocklist->num_blocks++;
    blocklist->array_of_blocks = (t_block**) my_realloc(blocklist->array_of_blocks, sizeof(t_block*)*(blocklist->num_blocks + 1)); // +1 b/c indexed from 1..num_blocks
    blocklist->array_of_blocks[blocklist->num_blocks] = block;

    assert(blocklist->array_of_blocks[block->index] == block);
}

/*
 * Returns the block at block_index in the global 
 *  blocklist structure
 */
t_block* get_block_by_index(int block_index) {
    t_blocklist* blocklist = g_CHIP->blocklist;

    //Range check
    assert(block_index <= blocklist->num_blocks);

    return blocklist->array_of_blocks[block_index];
}

t_boolean my_readline(FILE* fp, char** line) {
    int max_len = DEFAULT_LINE_LENGTH;
    if(*line != NULL) {
        free(*line);
    }
    char* buf = (char*) my_malloc(sizeof(char)*max_len);

    char ch = getc(fp);
    int cnt = 0;
    while (ch != '\n' && ch != EOF) {
        //Expand buf if required
        if(cnt >= max_len - 1) {
            max_len *= 2;
            buf = (char*) my_realloc(buf, max_len);
        }

        buf[cnt] = ch;
        cnt++;

        ch = getc(fp);
    }
    //End the string
    buf[cnt] = '\0';
   
    //Pass the buffer back through the reference
    *line = buf;

    if(ch == EOF) {
        free(buf);
        return FALSE;
    }
    return TRUE;
}

void print_netlist_stats(void){
    t_blocklist* blocklist = g_CHIP->blocklist;
    t_netlist* netlist = g_CHIP->netlist;

    t_net* highest_fanout_net = NULL;
    float avg_fanout = 0.;
    for(int net_index = 1; net_index <= netlist->num_nets; net_index++) {
        if(netlist->valid_nets[net_index]) {

            t_net* net = netlist->array_of_nets[net_index];

            if(highest_fanout_net == NULL || net->num_blocks > highest_fanout_net->num_blocks) {
                highest_fanout_net = net;
            }

            avg_fanout += net->num_blocks;
        } else {
            printf("Warn: net index '%d' is invalid\n", net_index);
        }
    }

    avg_fanout /= netlist->num_nets;

    int moveable_blk_cnt = 0;
    t_block* most_connected_block = NULL;
    float avg_block_connectivity = 0.;
    for(int block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        if(!block->is_fixed) moveable_blk_cnt++;

        if(most_connected_block == NULL || block->num_nets > most_connected_block->num_nets) {
            most_connected_block = block;
        }
        avg_block_connectivity += block->num_nets;
    }

    printf("\tAdded %d Blocks (%d moveable)\n", blocklist->num_blocks, moveable_blk_cnt);
    printf("\tAdded %d Nets\n", netlist->num_nets);

    printf("\tHighest Fanout Net is '%d' with fanout %d\n", highest_fanout_net->index, highest_fanout_net->num_blocks);
    printf("\tAverage Fanout is %.2f blocks\n", avg_fanout);

    avg_block_connectivity /= blocklist->num_blocks;
    printf("\tMost connected block is #%d '%s' connected to %d nets\n", most_connected_block->index, most_connected_block->name, most_connected_block->num_nets);
    printf("\tAverage connectivity is %.2f nets\n", avg_block_connectivity);
}
