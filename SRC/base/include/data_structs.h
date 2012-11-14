#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

#include <map>
#include <vector>

using std::vector;
using std::map;
using std::pair;

typedef struct s_CHIP t_CHIP;
typedef struct s_block t_block;
typedef struct s_net t_net;
typedef struct s_bbnode t_bbnode;

typedef map<int, t_block*> t_block_map;
typedef pair<int, t_block*> t_block_pair;

typedef map<int, t_net*> t_net_map;
typedef pair<int, t_net*> t_net_pair;
typedef vector<t_bbnode*> t_bbnode_vec;

typedef enum e_boolean {FALSE = 0, TRUE} t_boolean;
typedef enum e_side {LEFT = 0, RIGHT} t_side;

/*
 * The placement primitive, representing a
 * std cell, or pseudo cell
 */
struct s_block {
    int index;

    t_net_map nets;
};

/*
 * A logical net that may fan out to multiple blocks
 */
struct s_net {
    int index;

    t_block_map blocks;
};

extern t_block_map g_blocklist;
extern t_net_map g_netlist;

/*
 *  A (partial) solution in the brand and bound
 *  decision tree.
 */
struct s_bbnode {
    t_block_map left_blocks;
    t_block_map right_blocks;
    t_block_map free_blocks;
    t_bbnode_vec children;
};
#endif
