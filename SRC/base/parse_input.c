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
t_block* parse_fixed_positions(char* line);
t_net* find_or_allocate_net(int net_index);
void associate_block_and_net(t_block* block, t_net* net);
void add_block_to_blocklist(t_block* block);
t_block* get_block_by_index(int block_index);
t_boolean my_readline(FILE* fp, char** line);

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

    //Allocate the chip structure
    g_CHIP = my_malloc(sizeof(t_CHIP));
    g_CHIP->x_dim = 0;
    g_CHIP->y_dim = 0;

    //Allocate the block list
    t_blocklist* blocklist = my_malloc(sizeof(t_blocklist));
    blocklist->num_blocks = 0;
    blocklist->array_of_blocks = NULL;
    /*blocklist->array_of_blocks = my_malloc(sizeof(t_net*));*/
    /*blocklist->array_of_blocks[0] = 0xDEADBEEF; //Sentinel value*/
    g_CHIP->blocklist = blocklist; //Add to global chip structure

    //Allocate the net list
    t_netlist* netlist = my_malloc(sizeof(t_netlist));
    netlist->num_nets = 0;
    netlist->array_of_nets = NULL;
    /*netlist->array_of_nets = my_malloc(sizeof(t_net*));*/
    /*netlist->array_of_nets[0] = 0xDEADBEEF; //Sentinel value*/
    netlist->valid_nets = NULL;
    /*netlist->valid_nets = my_malloc(sizeof(t_boolean));*/
    /*netlist->valid_nets[0] = FALSE;*/
    g_CHIP->netlist = netlist; //Add to global chip structure

    //Allocate the pseudo net list
    /*
     *t_pnetlist* pnetlist = my_malloc(sizeof(t_pnetlist));
     *pnetlist->num_pnets = 0;
     *pnetlist->max_len = 1;
     *pnetlist->array_of_pnets = my_malloc(sizeof(t_pnet)); //Will be re-allocated as pnets are created
     *g_CHIP->pnetlist = pnetlist; //Add to global chip structure
     */

    //Temporay line
    char* line = NULL;

    while (my_readline(fp, &line) == TRUE && file_section <= 2) {
        if (file_section == 0) {

            //Check for end of section
            if (strcmp(line, "-1") == 0) {
                file_section++;
                continue;
            }

            //The chip dimensions section
            int x_dim, y_dim;

            sscanf(line, "%d %d", &x_dim, &y_dim); 

            assert(g_CHIP->x_dim == 0);
            assert(g_CHIP->y_dim == 0);

            g_CHIP->x_dim = x_dim;
            g_CHIP->y_dim = y_dim;
            printf("\tChip size %dx%d\n", x_dim, y_dim);

        } else if (file_section == 1) {

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

        } else  if (file_section == 2) {

            //Check for end of section
            if (strcmp(line, "-1") == 0) {
                file_section++;
                break; //EOF
            }
            
            //The fixed position section

            //Fix relevant block's position
            t_block* block = parse_fixed_positions(line);

            DEBUG_PRINT(EXTRA_INFO, "\tFixing Block #%d '%s' at co-ordinates %.2fx%.2f\n", block->index, block->name, block->x, block->y);

        } else {
            die("Error: Invalid file section %d", file_section);
        }
    }

    printf("\tAdded %d Blocks\n", blocklist->num_blocks);
    printf("\tAdded %d Nets\n", netlist->num_nets);

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

    t_block* block = my_malloc(sizeof(t_block));
    //Initialization
    block->index = -1;
    block->name = NULL;
    block->x = g_CHIP->x_dim/2;
    block->y = g_CHIP->y_dim/2;
    block->is_fixed = FALSE;
    block->num_nets = 0;
    block->associated_nets = NULL;

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
 * Parses a line of the fixed block positions section
 *   Sets the x/y coordinates and indicates that the
 *   block has fixed location
 */
t_block* parse_fixed_positions(char* line) {
    t_boolean seen_block_name = FALSE;
    t_boolean seen_block_index = FALSE;
    t_boolean seen_block_x_coord = FALSE;
    t_boolean seen_block_y_coord = FALSE;
    char* tmp_blk_name;
    int block_index, x_coord, y_coord;

    char* tok = strdup(line);
    tok = strtok(tok, TOKEN_SEPS);
    while(tok!=NULL) {
        if(!seen_block_name) {
            tmp_blk_name = strdup(tok);
            seen_block_name = TRUE;

        } else if (!seen_block_index) {
            block_index = atoi(tok);
            seen_block_index = TRUE;

        } else if (!seen_block_x_coord) {
            x_coord = atoi(tok);
            seen_block_x_coord = TRUE;

        } else if (!seen_block_y_coord) {
            y_coord = atoi(tok);
            seen_block_y_coord = TRUE;

        } else {
            die("Error: invalid parser state during fixed position parsing\n");
        }
        tok = strtok(NULL, TOKEN_SEPS);
            
    }
    assert(seen_block_name);
    assert(seen_block_index);
    assert(seen_block_x_coord);
    assert(seen_block_y_coord);

    t_block* block = get_block_by_index(block_index);

    //Names must match
    assert(strcmp(tmp_blk_name,block->name) == 0);
    free(tmp_blk_name);
    free(tok);

    //Fix location
    block->x = x_coord;
    block->y = y_coord;
    block->is_fixed = TRUE;
    
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
        net = my_malloc(sizeof(t_net));

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
            netlist->array_of_nets = my_realloc(netlist->array_of_nets, sizeof(t_net*)*(netlist->num_nets + 1));
            netlist->valid_nets = my_realloc(netlist->valid_nets, sizeof(t_boolean)*(netlist->num_nets + 1));

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
    net->associated_blocks = my_realloc(net->associated_blocks, sizeof(t_block*)*net->num_blocks);
    net->associated_blocks[net->num_blocks - 1] = block;

    //Add this net to the blocks associated net list
    block->num_nets++;
    block->associated_nets = my_realloc(block->associated_nets, sizeof(t_net*)*block->num_nets);
    block->associated_nets[block->num_nets - 1] = net;
}

/*
 *  Adds a block to the global blocklist structure
 */
void add_block_to_blocklist(t_block* block) {
    t_blocklist* blocklist = g_CHIP->blocklist;

    blocklist->num_blocks++;
    blocklist->array_of_blocks = my_realloc(blocklist->array_of_blocks, sizeof(t_block*)*(blocklist->num_blocks + 1));
    blocklist->array_of_blocks[blocklist->num_blocks] = block;

    assert(blocklist->array_of_blocks[block->index] == block);
}

/*
 * Returns the block at block_index in the global 
 *  blocklist structure
 */
t_block* get_block_by_index(block_index) {
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
    char* buf = my_malloc(sizeof(char)*max_len);

    char ch = getc(fp);
    int cnt = 0;
    while (ch != '\n' && ch != EOF) {
        //Expand buf if required
        if(cnt >= max_len - 1) {
            max_len *= 2;
            buf = my_realloc(buf, max_len);
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
