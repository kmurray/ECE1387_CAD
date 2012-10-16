#include <umfpack.h>
#include <data_structs.h>
#include <util.h>
#include <solver.h>


t_boolean solve_system(void) {

    int m = find_num_moveable_objects();

    //Assume all entries are non-zero
    /*
     *int Qp [m+1]; //The column pointer
     *int Qi [m*m]; //The row indexes
     *int Qx [m*m]; //The matrix values
     */

    int n = 5 ;
    int Ap [ ] = {0, 2, 5, 9, 10, 12} ;
    int Ai [ ] = { 0, 1, 0, 2, 4, 1, 2, 3, 4, 2, 1, 4} ;
    double Ax [ ] = {2., 3., 3., -1., 4., 4., -3., 1., 2., 2., 6., 1.} ;
    double b [ ] = {8., 45., -3., 3., 19.} ;
    double x [5] ;

    double *null = (double *) NULL ;
    void *Symbolic, *Numeric ;
    umfpack_di_symbolic (n, n, Ap, Ai, Ax, &Symbolic, null, null) ;
    umfpack_di_numeric (Ap, Ai, Ax, Symbolic, &Numeric, null, null) ;
    umfpack_di_free_symbolic (&Symbolic) ;
    umfpack_di_solve (UMFPACK_A, Ap, Ai, Ax, x, b, Numeric, null, null) ;
    umfpack_di_free_numeric (&Numeric) ;
    
    int i ;
    for (i = 0 ; i < n ; i++) printf ("x [%d] = %g\n", i, x [i]) ;

    return TRUE;
}


int find_num_moveable_objects(void) {
    t_blocklist* blocklist = g_CHIP->blocklist;
    int num_moveable_objects = 0;

    int block_index;
    for(block_index = 1; block_index <= blocklist->num_blocks; block_index++) {
        t_block* block = blocklist->array_of_blocks[block_index];

        if(!block->is_fixed) {
            num_moveable_objects++;
        }
    }
    return num_moveable_objects;
}
