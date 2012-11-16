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

    printf("Parsing input file: %s\n", filename);
    FILE* fp = fopen(filename, "r");

    //Temporay line
    char* line = NULL;

    //Identifies the section(s) of the file
    int file_section = 0;

    while (my_readline(fp, &line) == TRUE && file_section <= 0) {
        //Check for end of section
        if (strcmp(line, "-1") == 0) {
            file_section++;
            continue;
        }

        if (file_section == 0) {
            //The block connectivity section

            //Creates the block object, and nets as needed
            t_block* block = parse_block_conectivity(line);

            //Add this block to the global block list
            add_block_to_blocklist(block);

            DEBUG_PRINT(EXTRA_INFO,"\tAdding Block #%d \n", block->index);

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
    t_boolean seen_blk_index = FALSE;

    t_block* block = new t_block;
    //Initialization
    block->index = -1;

    //Duplicate string first, since strtok destroys
    //the input string
    char* tok = strdup(line);
    tok = strtok(tok, TOKEN_SEPS);
    while(strcmp(tok,"-1") != 0) {
        if(!seen_blk_index) {
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
    assert(seen_blk_index);

    return block;
}

/*
 * Given a net index returns the net if it already
 *   exists, or allocates it if it does not
 */
t_net* find_or_allocate_net(int net_index) {

    t_net_map::iterator it = g_netlist.find(net_index);

    if(it != g_netlist.end()) {
        return it->second;
    }

    //Allocate the actual net
    t_net* net = new t_net;

    //Initialize
    net->index = net_index;

    //Add to the netlist
    g_netlist[net_index] = net;

    return net;
}

/*
 *  Sets the cross links between a block and a net
 */
void associate_block_and_net(t_block* block, t_net* net) {
    //Add this block to the nets associated block list
    net->blocks[block->index] = block;

    //Add this net to the blocks associated net list
    block->nets[net->index] = net;
}

/*
 *  Adds a block to the global blocklist structure
 */
void add_block_to_blocklist(t_block* block) {
    
    g_blocklist[block->index] = block;

}

/*
 * Returns the block at block_index in the global 
 *  blocklist structure
 */
t_block* get_block_by_index(int block_index) {
    t_block_map::iterator it = g_blocklist.find(block_index);

    if(it == g_blocklist.end()) die("Invalid block index");

    return it->second;
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
    t_net* highest_fanout_net = NULL;
    t_block* most_connected_block = NULL;

    double avg_fanout = 0.;
    double avg_block_connectivity = 0.;

    //Net info
    t_net_map::iterator net_it;
    for(net_it = g_netlist.begin(); net_it != g_netlist.end(); net_it++) {
        t_net* net = net_it->second;

        if(highest_fanout_net == NULL || net->blocks.size() > highest_fanout_net->blocks.size()) {
            highest_fanout_net = net;
        }

        avg_fanout += net->blocks.size();
    }

    avg_fanout /= g_netlist.size();

    //Block info
    t_block_map::iterator block_it;
    for(block_it = g_blocklist.begin(); block_it != g_blocklist.end(); block_it++) {
        t_block* block = block_it->second; 

        if(most_connected_block == NULL || block->nets.size() > most_connected_block->nets.size()) {
            most_connected_block = block;
        }
        avg_block_connectivity += block->nets.size();
    }
    avg_block_connectivity /= g_blocklist.size();

    //Serach space info
    assert(g_blocklist.size() % 2 == 0); //Must be an even number of blocks to partition evenly
    //Number of possilbe left/right solutions is    : C(n,n/2)
    // But number of unique solutions is            : C(n,n/2)/2  since left and right are equivalent          
    double num_solns = binomial_coefficient((float) g_blocklist.size(), (float) g_blocklist.size() / 2) / 2;

    printf("\tAdded %zu Blocks\n", g_blocklist.size());
    printf("\tAdded %zu Nets\n", g_netlist.size());

    printf("\tHighest Fanout Net is '%d' with fanout %zu\n", highest_fanout_net->index, highest_fanout_net->blocks.size());
    printf("\tAverage Fanout is %.2f blocks\n", avg_fanout);

    printf("\tMost connected block is #%d connected to %zu nets\n", most_connected_block->index, most_connected_block->nets.size());
    printf("\tAverage connectivity is %.2f nets\n", avg_block_connectivity);

    printf("\tPossible solutions: %.2e\n", num_solns);


}
