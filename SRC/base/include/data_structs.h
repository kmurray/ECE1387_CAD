#ifndef DATA_STRUCTS_H
#define DATA_STRUCTS_H

typedef struct s_CHIP t_CHIP;
typedef struct s_blocklist t_blocklist;
typedef struct s_netlist t_netlist;
typedef struct s_block t_block;
typedef struct s_net t_net;
typedef struct s_pnet t_pnet;
typedef struct s_ccm t_ccm;
typedef struct s_movable_blocks t_moveable_blocks;
typedef struct s_clustered_gridsquares t_clustered_gridsquares;
typedef struct s_gridsquare t_gridsquare;
typedef struct s_gridsquare_list t_gridsquare_list;
typedef struct s_clusterlist t_clusterlist;
typedef struct s_cluster t_cluster;
typedef struct s_region t_region;

typedef enum e_boolean {FALSE = 0, TRUE} t_boolean;
typedef enum e_block_type {PSEUDO = 1, REAL} t_block_type;
typedef enum e_axies {X_AXIS = 0, Y_AXIS} t_axis;
typedef enum e_side {LEFT = 0, RIGHT, TOP, BOTTOM } t_side;

struct s_CHIP {
    int x_dim;
    int y_dim;

    t_blocklist* blocklist;

    t_netlist* netlist;

    double x_grid;
    double y_grid;
    double grid_area;
    int num_vert_grids;
    int num_horiz_grids;

};

struct s_blocklist {
    int pblock_start_index;
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
    t_net** associated_nets; //[0..num_nets-1]

    int num_pnets;
    t_pnet** associated_pnets; //[0..num_pnets-1]
};

/*
 * A logical net that may fan out to multiple blocks
 */
struct s_net {
    int index;

    int num_blocks;
    t_block** associated_blocks; //[0..num_blocks-1]

    int num_pnets;
    t_pnet** equivalent_pnets;
};


/*
 * A 2-pin pseudo net used during analytical placement
 */
struct s_pnet {
    //The two terminal blocks
    t_block* block_a;
    t_block* block_b;
    
    double weight_x;
    double weight_y;
};
/*
 * A Compressed Column Matrix
 *   i.e. a matrix stored in compressed column format
 */
struct s_ccm {
    int* col_ptrs; //[0..num_cols]
    int* row_indexs; //[0..num_values-1]
    double* values; //[0..num_values-1]
    int num_values;
    int num_rows; //The number of rows in the full matrix
    int num_cols; //The number of cols in the full matrix
};

struct s_movable_blocks {
    int num_blocks;
    t_block** array_of_blocks; //[0..num_blocks-1]
};

struct s_clusterlist {
    int num_clusters;
    t_cluster** array_of_clusters;
};

struct s_cluster {
    t_region* region;
    t_moveable_blocks* blocks;

    int level;
};

struct s_clustered_gridsquares {
    int num_clustered_gridsquares;
    t_gridsquare_list** array_of_clustered_gridsquares;
};

struct s_gridsquare_list {
    int num_gridsquares;
    t_gridsquare** array_of_gridsquares;
};
struct s_gridsquare {
    t_region* region;
    int num_blocks;
    t_block** array_of_blocks;
    t_boolean is_clustered;
};

struct s_region {
    double x_min;
    double x_max;
    double y_min;
    double y_max;
};

extern t_CHIP* g_CHIP;
#endif
