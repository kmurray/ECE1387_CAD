#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

typedef struct s_CHIP t_CHIP;
typedef struct s_blocklist t_blocklist;
typedef struct s_netlist t_netlist;
typedef struct s_pnetlist t_pnetlist;
typedef struct s_block t_block;
typedef struct s_net t_net;
typedef struct s_pnet t_pnet;

typedef enum e_boolean {FALSE = 0, TRUE} t_boolean;
typedef enum e_block_type {PSEUDO = 1, REAL} t_block_type;

struct s_CHIP {
    int x_dim;
    int y_dim;

    t_blocklist* blocklist;

    t_netlist* netlist;

    t_pnetlist* pnetlist;

};

struct s_blocklist {
    int num_blocks;
    t_block** array_of_blocks; //[1..num_nets]
};

/*
 * 
 */
struct s_netlist {
    int num_nets;
    t_net** array_of_nets; //[1..num_nets]

    t_boolean* valid_nets; //[1..num_nets]
};

struct s_pnetlist {
    int max_len;  //The actual size of array_of_pnets
    int num_pnets;
    t_net** array_of_pnets; //[0..num_nets-1]
};

/*
 * The placement primitive, representing a
 * std cell, or pseudo cell
 */
struct s_block {
    int index;
    char* name;

    float x;
    float y;
    t_boolean is_fixed;

    int num_nets;
    t_net** associated_nets; //[0..num_blocks-1]
};

/*
 * A logical net that may fan out to multiple blocks
 */
struct s_net {
    int index;
    int num_blocks;
    t_block** associated_blocks; //[0..num_blocks-1]
};


/*
 * A 2-pin pseudo net used during analytical placement
 */
struct s_pnet {
    //The two terminal blocks
    t_block* block_a;
    t_block* block_b;
    
    float weight;
};

extern t_CHIP* g_CHIP;
#endif
