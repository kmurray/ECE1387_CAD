//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <umfpack.h>
#include <data_structs.h>
#include <util.h>
#include <solver.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_moveable_blocks* find_num_moveable_objects(void);
double get_connectivity_matrix_entry(t_axis axis, int row, int col, t_moveable_blocks* moveable_blocks);
void build_connectivity_matrix(t_axis axis, t_moveable_blocks* moveable_blocks, t_ccm* Q);
t_block* get_block_by_row_index(int row, t_moveable_blocks* moveable_blocks);
void dump_ccm(t_ccm* M);
double* build_anchoring_vector(t_axis axis, t_moveable_blocks* moveable_blocks);
double get_anchoring_vector_entry(t_axis axis, int row, t_moveable_blocks* moveable_blocks);
void call_solver(t_ccm* Q, double* x, double* b);
void update_block_locations(t_axis axis, t_moveable_blocks* moveable_blocks, double* x);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
t_boolean solve_system(void) {

    t_moveable_blocks* moveable_blocks = find_num_moveable_objects();

    t_axis axis;
    for(axis = X_AXIS; axis <= Y_AXIS; axis++) {

        if(axis == X_AXIS) {
            printf("\nSolving X-Axis System\n");
        } else {
            printf("\nSolving Y-Axis System\n");
        }

        t_ccm* Q = my_malloc(sizeof(t_ccm));
        build_connectivity_matrix(axis, moveable_blocks, Q);

        printf("Matrix Q:\n");
        dump_ccm(Q);

        double* b = build_anchoring_vector(axis, moveable_blocks);

        printf("Anchoring Vector:\n");
        int index;
        for (index = 0; index < moveable_blocks->num_blocks; index++) {
            printf("b[%d] = %.2f\n", index, b[index]);
        }

        double x[moveable_blocks->num_blocks];
        for(index = 0; index < moveable_blocks->num_blocks; index++) {
            x[index] = 0;
        }

        call_solver(Q, x, b);

        int i;
        for (i = 0 ; i < moveable_blocks->num_blocks ; i++) printf ("soln[%d] = %g\n", i, x [i]) ;

        update_block_locations(axis, moveable_blocks, x);
    }

    return TRUE;
}

void update_block_locations(t_axis axis, t_moveable_blocks* moveable_blocks, double* x){
    int block_index;
    for(block_index = 0; block_index < moveable_blocks->num_blocks; block_index++) {
        t_block* block = moveable_blocks->array_of_blocks[block_index];
        if(axis == X_AXIS) {
            block->x = x[block_index];
        } else {
            block->y = x[block_index];
        }
    }
}

void call_solver(t_ccm* Q, double* x, double* b) {
    printf("Calling solver...\n");

    assert(Q->num_rows == Q->num_cols);

    double *null = (double *) NULL ;
    void *Symbolic, *Numeric ;
    umfpack_di_symbolic (Q->num_rows, Q->num_cols, Q->col_ptrs, Q->row_indexs, Q->values, &Symbolic, null, null) ;
    umfpack_di_numeric (Q->col_ptrs, Q->row_indexs, Q->values, Symbolic, &Numeric, null, null) ;
    umfpack_di_free_symbolic (&Symbolic) ;
    if(UMFPACK_OK != umfpack_di_solve (UMFPACK_A, Q->col_ptrs, Q->row_indexs, Q->values, x, b, Numeric, null, null)) {
        printf("Error: System didn't solve!\n");
    }
    /*umfpack_di_free_numeric (&Numeric) ;*/

    printf("DONE\n");

}
/*
 *    int n = 5 ;
 *    int Ap [ ] = {0, 2, 5, 9, 10, 12} ;
 *    int Ai [ ] = { 0, 1, 0, 2, 4, 1, 2, 3, 4, 2, 1, 4} ;
 *    double Ax [ ] = {2., 3., 3., -1., 4., 4., -3., 1., 2., 2., 6., 1.} ;
 *    double b [ ] = {8., 45., -3., 3., 19.} ;
 *    double x [5] ;
 *
 *    t_ccm* M = my_malloc(sizeof(t_ccm));
 *    M->num_rows = n;
 *    M->num_cols = n;
 *    M->num_values = 12;
 *    M->col_ptr = my_calloc(sizeof(int), M->num_cols+1);
 *    M->row_index = my_calloc(sizeof(int), M->num_values);
 *    M->values = my_calloc(sizeof(int), M->num_values);
 *
 *    int col_ptr_index;
 *    for(col_ptr_index = 0; col_ptr_index < M->num_cols + 1; col_ptr_index++) {
 *        M->col_ptr[col_ptr_index] = Ap[col_ptr_index];
 *    }
 *    int val_index;
 *    for(val_index = 0; val_index < M->num_values; val_index++) {
 *        M->row_index[val_index] = Ai[val_index];  
 *        M->values[val_index]    = Ax[val_index];
 *    }
 *
 *    dump_ccm(M);
 */

    /*
     *double *null = (double *) NULL ;
     *void *Symbolic, *Numeric ;
     *umfpack_di_symbolic (n, n, Ap, Ai, Ax, &Symbolic, null, null) ;
     *umfpack_di_numeric (Ap, Ai, Ax, Symbolic, &Numeric, null, null) ;
     *umfpack_di_free_symbolic (&Symbolic) ;
     *umfpack_di_solve (UMFPACK_A, Ap, Ai, Ax, x, b, Numeric, null, null) ;
     *umfpack_di_free_numeric (&Numeric) ;
     *int i ;
     *for (i = 0 ; i < m ; i++) printf ("x [%d] = %g\n", i, x [i]) ;
     */


t_moveable_blocks* find_num_moveable_objects(void) {
    t_blocklist* blocklist = g_CHIP->blocklist;

    t_moveable_blocks* moveable_blocks = my_malloc(sizeof(t_moveable_blocks));

    //Initialize
    moveable_blocks->num_blocks = 0;
    moveable_blocks->array_of_blocks = my_calloc(sizeof(t_block*), blocklist->num_blocks); //Max size initially will shrink real num_blocks is known


    int block_index;
    for(block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        if(!block->is_fixed) {
            moveable_blocks->array_of_blocks[moveable_blocks->num_blocks] = block;
            moveable_blocks->num_blocks++;
        }
    }
    //Fix sizing
    moveable_blocks->array_of_blocks = my_realloc(moveable_blocks->array_of_blocks, sizeof(t_block*)*moveable_blocks->num_blocks); //Max size initially will shrink real num_blocks is known

    return moveable_blocks;
}

void build_connectivity_matrix(t_axis axis, t_moveable_blocks* moveable_blocks, t_ccm* Q) {

    Q->num_rows = moveable_blocks->num_blocks;
    Q->num_cols = moveable_blocks->num_blocks;

    //One more than dimension, so we don't over-run the array
    Q->col_ptrs = my_calloc(sizeof(int), Q->num_cols+1);

    //Initially assume that all elements are non-zero
    // We will re-allocate once we know the actual numbers
    Q->num_values = Q->num_rows*Q->num_cols;
    Q->row_indexs = my_calloc(sizeof(int), Q->num_values);
    Q->values = my_calloc(sizeof(double), Q->num_values);

    int val_index = 0; //Current index into Q->values

    int col;
    int row;
    for(col = 0; col < Q->num_cols; col++) {
        //Set the column pointer for this column
        Q->col_ptrs[col] = val_index;

        for(row = 0; row < Q->num_rows; row++) {
            //Grab the values of this matrix entry
            int value = get_connectivity_matrix_entry(axis, row, col, moveable_blocks);            

            //Only non-zero entries are stored
            if(value != 0.) {
                Q->values[val_index] = value;
                Q->row_indexs[val_index] = row;

                val_index++;
            }
        }
    }

    //Update the actual number of used values
    Q->num_values = val_index;

    //Shrink the arrays down to the actual size
    Q->row_indexs = my_realloc(Q->row_indexs, sizeof(int)*Q->num_values);
    Q->values = my_realloc(Q->values, sizeof(double)*Q->num_values);

    Q->col_ptrs[Q->num_cols] = Q->num_cols;

    //First entry must be zero in column pointer
    assert(Q->col_ptrs[0] == 0);
    assert(Q->col_ptrs[Q->num_cols] = Q->num_cols);
}

double* build_anchoring_vector(t_axis axis, t_moveable_blocks* moveable_blocks){
    double* b = my_calloc(sizeof(double), moveable_blocks->num_blocks);

    int row_index;
    for(row_index = 0; row_index < moveable_blocks->num_blocks; row_index++) {
        b[row_index] = get_anchoring_vector_entry(axis, row_index, moveable_blocks);
    }
    
    return b;
}

double get_anchoring_vector_entry(t_axis axis, int row, t_moveable_blocks* moveable_blocks) {
    //We sum up the wieghts*positions of all nets connected to the moveable object represented by
    // row, that are connected to fixed objects

    double entry = 0.;

    assert(row < moveable_blocks->num_blocks);
    t_block* block = moveable_blocks->array_of_blocks[row];

    int net_index;
    for(net_index = 0; net_index < block->num_nets; net_index++) {
        t_net* logical_net = block->associated_nets[net_index];

        int pnet_index;
        for(pnet_index = 0; pnet_index < logical_net->num_pnets; pnet_index++) {
            t_pnet* pnet = logical_net->equivalent_pnets[pnet_index];
            t_block* fixed_block;

            if (pnet->block_a != block && pnet->block_a->is_fixed == TRUE) {
                fixed_block = pnet->block_a;
                
            } else if (pnet->block_b != block && pnet->block_b->is_fixed == TRUE) {
                fixed_block = pnet->block_b;
            } else {
                //pnet not attached to a fixed position block
                continue;
            }
           
            double location;
            if(axis == X_AXIS) {
                location = fixed_block->x;
            } else { //Y_AXIS
                location = fixed_block->y;
            }
            entry += pnet->weight*location;
        }

    }
    return entry;
}

double get_connectivity_matrix_entry(t_axis axis, int row, int col, t_moveable_blocks* moveable_blocks) {
    /*
     *if(row == col) {
     *    return 1.;
     *} else {
     *    return 0.;
     *}
     */
    double entry_value = 0.;

    assert(row < moveable_blocks->num_blocks);
    assert(col < moveable_blocks->num_blocks);
    t_block* row_block = get_block_by_row_index(row, moveable_blocks);
    t_block* col_block = get_block_by_row_index(col, moveable_blocks);


    //Loop through all the nets connected
    int net_index;
    for(net_index = 0; net_index < row_block->num_nets; net_index++) {
        t_net* logical_net = row_block->associated_nets[net_index];

        //Loop through all the pnets that represent this logical net
        int pnet_index;
        for(pnet_index = 0; pnet_index < logical_net->num_pnets; pnet_index++) {
            t_pnet* pnet = logical_net->equivalent_pnets[pnet_index];

            if (row == col) {
                //On diagonal elements are the sum of all connected wieghts
                // for the pnets connected to this block

                // Sum the weights
                entry_value += pnet->weight;
            } else {
                //Off diagonal elements are the NEGATIVE sum of connected weights
                // between row_block and column_block

                if( (pnet->block_a == row_block && pnet->block_b == col_block) ||
                    (pnet->block_b == row_block && pnet->block_a == col_block) ) {
                    //Only do the negative sum of the pnet is between row_block and
                    //col_block

                    entry_value -= pnet->weight;
                }

            }
        }
    }


    return entry_value;
}

t_block* get_block_by_row_index(int row, t_moveable_blocks* moveable_blocks) {
    assert(row < moveable_blocks->num_blocks);
    return moveable_blocks->array_of_blocks[row];
}

void dump_ccm(t_ccm* M) {
    int val_index;
    int col_index = 0;

    for(val_index = 0; val_index < M->num_values; val_index++) {
        double value = M->values[val_index];
        int row_index = M->row_indexs[val_index];
        
        
        if(val_index == M->col_ptrs[col_index + 1]) {
            assert(col_index + 1 < M->num_cols + 1);
            col_index++;
        }

        printf("M[%d][%d]: %.2f\n", row_index, col_index, value);
    }
}
