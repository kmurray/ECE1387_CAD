//================================================================================================
// INCLUDES 
//================================================================================================
#include <assert.h>
#include <time.h>
#include <umfpack.h>
#include <data_structs.h>
#include <util.h>
#include <solver.h>

//================================================================================================
// INTERNAL FUNCTION DECLARTAIONS 
//================================================================================================
t_moveable_blocks* find_num_moveable_objects(void);
double get_connectivity_matrix_entry(int row, int col, t_moveable_blocks* moveable_blocks);
void build_connectivity_matrix(t_moveable_blocks* moveable_blocks, t_ccm* Q);
t_block* get_block_by_row_index(int row, t_moveable_blocks* moveable_blocks);
void dump_ccm(t_ccm* M);
double* build_anchoring_vector(t_axis axis, t_moveable_blocks* moveable_blocks);
double get_anchoring_vector_entry(t_axis axis, int row, t_moveable_blocks* moveable_blocks);
void call_solver(t_ccm* Q, double* x, double* b);
void update_block_locations(t_axis axis, t_moveable_blocks* moveable_blocks, double* x);
double get_entry(t_ccm* M, int row, int col);

//================================================================================================
// INTERNAL FUCTION IMPLIMENTATIONS
//================================================================================================
t_boolean solve_system(void) {

    t_moveable_blocks* moveable_blocks = find_num_moveable_objects();

    //The connectivity matrix is valid for both X and Y solves
    DEBUG_PRINT(EXTRA_INFO, "Generating Connectivity Matrix...\n");
    t_ccm* Q = my_malloc(sizeof(t_ccm));
    build_connectivity_matrix(moveable_blocks, Q);
    DEBUG_PRINT(EXTRA_INFO, "DONE\n");

    t_axis axis;
    for(axis = X_AXIS; axis <= Y_AXIS; axis++) {

        if(axis == X_AXIS) {
            DEBUG_PRINT(EXTRA_INFO, "\nSolving X-Axis System\n");
        } else {
            DEBUG_PRINT(EXTRA_INFO, "\nSolving Y-Axis System\n");
        }


        /*printf("Matrix Q:\n");*/
        /*dump_ccm(Q);*/

        double* b = build_anchoring_vector(axis, moveable_blocks);

        /*printf("Anchoring Vector:\n");*/
        /*for (int index = 0; index < moveable_blocks->num_blocks; index++) printf("b[%d] = %.2f\n", index, b[index]);*/

        double x[moveable_blocks->num_blocks];
        for(int index = 0; index < moveable_blocks->num_blocks; index++) {
            x[index] = 0.;
        }

        DEBUG_PRINT(EXTRA_INFO, "Calling solver...\n");
        call_solver(Q, x, b);
        DEBUG_PRINT(EXTRA_INFO, "DONE\n");


        /*for (int i = 0 ; i < moveable_blocks->num_blocks ; i++) printf ("soln[%d] = %g\n", i, x [i]) ;*/

        update_block_locations(axis, moveable_blocks, x);

        free(b);
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

    assert(Q->num_rows == Q->num_cols);

    double *null = (double *) NULL ;
    void *Symbolic, *Numeric ;

    double Control [UMFPACK_CONTROL];

    umfpack_di_defaults(Control);
    Control[UMFPACK_PRL] = 5.;
    /*printf("--\n");*/
    /*umfpack_di_report_matrix (Q->num_rows, Q->num_cols, Q->col_ptrs, Q->row_indexs, Q->values, 1, Control) ;*/
    /*printf("--\n");*/

    umfpack_di_symbolic (Q->num_rows, Q->num_cols, Q->col_ptrs, Q->row_indexs, Q->values, &Symbolic, null, null) ;
    umfpack_di_numeric (Q->col_ptrs, Q->row_indexs, Q->values, Symbolic, &Numeric, null, null) ;
    umfpack_di_free_symbolic (&Symbolic) ;
    double Info [UMFPACK_INFO];
    if(UMFPACK_OK != umfpack_di_solve (UMFPACK_A, Q->col_ptrs, Q->row_indexs, Q->values, x, b, Numeric, null, Info)) {
        printf("Error: System didn't solve!\n");
    }

    /*umfpack_di_report_control(Control);*/
    /*umfpack_di_report_info(Control, Info);*/
    //UMFPACK stats:
    /*
     *printf("UMFPACK Returned status: %.2f\n", Info[UMFPACK_STATUS]);
     *printf("UMFAPCK solve took %.2f FLOPs\n", Info[UMFPACK_SOLVE_FLOPS]);
     *printf("UMFAPCK solve took %.2f s\n", Info[UMFPACK_SOLVE_TIME]);
     */
    if(Info[UMFPACK_STATUS] == UMFPACK_WARNING_singular_matrix) {
        printf("UMFAPCK Singular matrix, divide by zero occured\n");
    }
    if(Info[UMFPACK_STATUS] == UMFPACK_ERROR_out_of_memory) {
        printf("UMFPACK out of memory\n");
    }
    if(Info[UMFPACK_STATUS] == UMFPACK_ERROR_argument_missing) {
        printf("UMFAPCK argument missing\n");
    }
    if(Info[UMFPACK_STATUS] == UMFPACK_ERROR_invalid_system) {
        printf("UMFPACK invalid system\n");
    }
    if(Info[UMFPACK_STATUS] == UMFPACK_ERROR_invalid_Numeric_object) {
        printf("UMFPACK invalid Numeric object\n");
    }
    umfpack_di_free_numeric (&Numeric) ;


}


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

void build_connectivity_matrix(t_moveable_blocks* moveable_blocks, t_ccm* Q) {

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

        // The Q matrix is symmetric around the diagonal, so 
        // reference the already calculated lower triangular elements
        // to buid the upper triangular (rather than re-calculating) 
        for(row = 0; row < col; row++) {
            double value = get_entry(Q, col, row);
            if(value != 0.) {
                Q->values[val_index] = value;
                Q->row_indexs[val_index] = row;

                val_index++;
            }
            
        }

        //Calculate the lower triangular (including diagonal)
        for(row = col; row < Q->num_rows; row++) {
            //Grab the values of this matrix entry
            double value = get_connectivity_matrix_entry(row, col, moveable_blocks);            

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

    //col_ptr is indexed from [0..Q->num_cols]
    // where j is the column #:
    //      col_ptr[j] = starting value_index of this column
    //      
    //      if j==Q->num_cols: We have reached the end of the array
    //
    // col_ptr[Q->num_cols] must be one greater than the total number
    // of values stored in Q->values. Since Q->values is index from
    // [0..Q->num_values-1], this value should be Q->num_values
    Q->col_ptrs[Q->num_cols] = Q->num_values;

    //First entry must be zero in column pointer
    assert(Q->col_ptrs[0] == 0);
    assert(Q->col_ptrs[Q->num_cols] = Q->num_values);
}

double get_entry(t_ccm* M, int row, int col) {
    int col_ptr = M->col_ptrs[col];
    int next_col_ptr = M->col_ptrs[col+1];

    int val_index;
    for(val_index = col_ptr; val_index < next_col_ptr; val_index++) {
        if(M->row_indexs[val_index] == row) {
            return M->values[val_index];
        }

    }

    //Not found, must be zero
    return 0.;

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

    t_block* row_block = get_block_by_row_index(row, moveable_blocks);

    for(int pnet_index = 0; pnet_index < row_block->num_pnets; pnet_index++) {
        t_pnet* pnet = row_block->associated_pnets[pnet_index];
        t_block* fixed_block;

        if (pnet->block_a != row_block && pnet->block_b == row_block && pnet->block_a->is_fixed == TRUE) {
            fixed_block = pnet->block_a;
            
        } else if (pnet->block_b != row_block && pnet->block_a == row_block && pnet->block_b->is_fixed == TRUE) {
            fixed_block = pnet->block_b;
        } else {
            //pnet not attached to a fixed position block
            continue;
        }
       
        double fixed_location;
        if(axis == X_AXIS) {
            fixed_location = fixed_block->x;
        } else { //Y_AXIS
            fixed_location = fixed_block->y;
        }

        //The entry is the sum of all weights*fixed_locations
        entry += pnet->weight*fixed_location;
    }

    return entry;
}

double get_connectivity_matrix_entry(int row, int col, t_moveable_blocks* moveable_blocks) {
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

    
    //Faster direct look-up code
    //       Old               New              New2
    // ap_1: 1375 block/sec    16500 block/sec  
    // ap_2: 63.17 blk/sec     622.32 blk/sec   
    // ap_3:
    //
    for(int pnet_index = 0; pnet_index < row_block->num_pnets; pnet_index++) {
        t_pnet* pnet = row_block->associated_pnets[pnet_index];

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

                //Negative sum of the weight
                entry_value -= pnet->weight;
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
