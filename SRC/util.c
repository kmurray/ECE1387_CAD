#ifndef UTIL_H
#define UTIL_H

void* my_malloc(size_t size) {
    return malloc(size);
}

void* my_realloc(size_t size) {
    return realloc(size);
}

void* my_calloc(size_t size_of_item, size_t num_items) {
    return calloc(size_of_item, num_items);
}

int get_block_index(t_FPGA* FPGA, int x_coord, y_coord) {
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
     *      INDEX = N*x + y
     */
    
    return  FPGA->grid_size * x_coord + y_coord;
}

#endif
