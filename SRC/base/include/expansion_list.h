#ifndef EXPANSION_LIST_H
#define EXPANSION_LIST_H

#include <data_structs.h>


#define MAX_HEAP_ITEMS  1024
#define MIN_HEAP_INDEX  1
#define MAX_HEAP_INDEX  MAX_HEAP_ITEMS


t_expansion_list* alloc_expansion_list(void);
t_boolean is_empty_expansion_list(t_expansion_list* expansion_list);
t_heap_node get_min_expansion_list(t_expansion_list* expansion_list);
void insert_expansion_list(t_expansion_list* expansion_list, t_wire* wire, int cost);
t_boolean verify_heap(t_expansion_list* expansion_list);
#endif
