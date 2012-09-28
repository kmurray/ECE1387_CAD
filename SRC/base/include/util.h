#ifndef UTIL_H
#define UTIL_H

void* my_malloc(size_t size);

void* my_realloc(void* ptr, size_t size);

void* my_calloc(size_t size_of_item, size_t num_items);

int get_block_index(int x_coord, int y_coord);

t_block* get_block(int x_coord, int y_coord);

int get_sb_index(int x_coord, int y_coord);

t_switchblock* get_sb(int x_coord, int y_coord);

int get_starting_pin_index_for_side(t_block* block, t_SIDE block_side);

void dump_switchblock(t_switchblock* sb);

void dump_wire(t_wire* wire);

t_boolean is_vertical_wire(t_wire* wire);
t_boolean is_positive_wire(t_wire* wire, t_switchblock* sb);
//void swap(void* a, void *b);
#endif
