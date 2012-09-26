#include <data_structs.h>
#include <stdio.h>

void* my_malloc(size_t size) {
    void* retval = malloc(size);
    if (retval == NULL) {
        printf("Error: malloc of size %u bytes failed\n", (unsigned int) size);
        exit(1);
    }
    return retval;
}

void* my_realloc(void* ptr, size_t size) {
    void* retval = realloc(ptr, size);
    if (retval == NULL) {
        printf("Error: realloc of size %u bytes failed\n", (unsigned int) size);
        exit(1);
    }
    return retval;
}

void* my_calloc(size_t size_of_item, size_t num_items) {
    void* retval = calloc(size_of_item, num_items);
    if (retval == NULL) {
        printf("Error: calloc of size %ux%u bytes failed\n", (unsigned int) size_of_item, (unsigned int) num_items);
        exit(1);
        
    }
    return retval;
}

int get_block_index(t_FPGA* FPGA, int x_coord, int y_coord) {
    /*
     *  All blocks are stored as an array.
     *  Since the FPGA is always square it is 
     *  simple to convert from a given x and y 
     *  location to the index.
     *
     *   (1,3)  (2,3) (3,3)
     *
     *   (1,2)  (2,2) (3,2)
     *
     *   (1,1)  (2,1) (3,1)
     *
     *  The blocks are stored in row major order
     *  as follows:
     *   ---------------------------------------------------------
     *   | 1,1 | 2,1 | 3,1 | 1,2 | ... | 3,2 | 1,3 | ... | 3,3 |
     *   ---------------------------------------------------------
     *   
     *  Therefore, knowing the lenght of one side of the grid 'N',
     *  and the  'x' coord and 'y' coord, the index is:
     *
     *      INDEX = N*(y-1) + (x-1)
     *  Where the '-1' along side each co-ordinate adjusts for the fact
     *  that the CLB rows/columns are indexed from 1 instead of 0.
     */
    
    return  (FPGA->grid_size * (y_coord - 1)) + (x_coord - 1);
}

