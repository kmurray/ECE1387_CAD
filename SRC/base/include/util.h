#ifndef UTIL_H
#define UTIL_H

void* my_malloc(size_t size);

void* my_realloc(void* ptr, size_t size);

void* my_calloc(size_t size_of_item, size_t num_items);

int get_block_index(t_FPGA* FPGA, int x_coord, int y_coord);
#endif
