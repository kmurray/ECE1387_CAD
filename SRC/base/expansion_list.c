#include <expansion_list.h>
#include <assert.h>
#include <util.h>
#include <stdio.h>
int parent_index(int current_index);
int child_left_index(int current_index);
int child_right_index(int current_index);
t_expansion_list* alloc_expansion_list(void);
void bubble_up(t_expansion_list* expansion_list, int node_index);
void bubble_down(t_expansion_list* expansion_list, int node_index);
void swap_nodes(t_heap_node* a, t_heap_node* b);


t_expansion_list* alloc_expansion_list(void) {
    t_expansion_list* expansion_list = my_malloc(sizeof(t_expansion_list));
    expansion_list->heap_size = 0;
    expansion_list->heap = my_calloc(sizeof(t_heap_node), MAX_HEAP_ITEMS + 1); // +1 since indexed from [1...MAX_HEAP_ITEMS]
    return expansion_list;
}

t_boolean is_empty_expansion_list(t_expansion_list* expansion_list) {
    return expansion_list->heap_size == 0;
}

void insert_expansion_list(t_expansion_list* expansion_list, t_wire* wire, int cost) {
    //Increment the size of the heap
    expansion_list->heap_size++;

    //Don't over run the preallocated array
    assert(expansion_list->heap_size <= MAX_HEAP_ITEMS);

    t_heap_node new_node;

    new_node.key = cost;
    new_node.wire = wire;

    //Insert into the heap
    expansion_list->heap[expansion_list->heap_size] = new_node;

    //Fix the heap
    bubble_up(expansion_list, expansion_list->heap_size);
}

t_heap_node get_min_expansion_list(t_expansion_list* expansion_list) {
    //Put the final node in the last position
    swap_nodes(&expansion_list->heap[MIN_HEAP_INDEX], &expansion_list->heap[expansion_list->heap_size]);

    //Shrink the heap by one unit
    expansion_list->heap_size--;

    //Fix the heap
    bubble_down(expansion_list, MIN_HEAP_INDEX);

    //Return the previous minimum value
    return expansion_list->heap[expansion_list->heap_size+1];
}

t_boolean verify_heap(t_expansion_list* expansion_list){
    //Returns TRUE if ok
    t_boolean heap_ok = TRUE;

    int node_index = MIN_HEAP_INDEX;

    //shorter references
    int heap_size = expansion_list->heap_size;
    t_heap_node* heap = expansion_list->heap;

    for(node_index = 0; node_index <= heap_size; node_index++) {
        int left_child_index = child_left_index(node_index);
        int right_child_index = child_right_index(node_index);

        //Left child
        if (left_child_index <= heap_size
            && heap[node_index].key > heap[left_child_index].key) {
            //node_index key should be smaller
            heap_ok = FALSE;
            assert(heap_ok);
        }

        //Right child
        if (right_child_index <= heap_size
            && heap[node_index].key > heap[right_child_index].key) {
            //node_index key should be smaller
            heap_ok = FALSE;
            assert(heap_ok);
        }
    }
    
    if(heap_ok == FALSE) {
        printf("Error: invalid heap state\n");
    }
    return heap_ok;
}


int parent_index(int current_index) {
    //Relying upon integer division to 
    // impliment floor(current_index/2)
    return current_index / 2;
}

int child_left_index(int current_index) {
    return 2*current_index;
}

int child_right_index(int current_index) {
    return 2*current_index + 1;
}


void bubble_up(t_expansion_list* expansion_list, int node_index){
    //Range check
    assert(node_index <= expansion_list->heap_size);

    while(   node_index > MIN_HEAP_INDEX  //Stop at root
          && expansion_list->heap[node_index].key < expansion_list->heap[parent_index(node_index)].key //Min heap 
          ) {
        //node_index is larger than it's parent so swap them
        swap_nodes(&expansion_list->heap[node_index], &expansion_list->heap[parent_index(node_index)]);

        //Continue propogating up the tree
        node_index = parent_index(node_index);
    }
}

void bubble_down(t_expansion_list* expansion_list, int node_index){

    while(child_left_index(node_index) <= expansion_list->heap_size) { //The child index is in range (i.e. not the bottom of the heap)
        int left_child_index = child_left_index(node_index);
        int right_child_index = child_right_index(node_index);

        int child_to_swap_index = left_child_index;

        //Swap with the smallest of node_index's children if node_index is smaller
        // NOTE: ther may not exist a right_child

        if (right_child_index <= expansion_list->heap_size) {
            //Right and left children exist
            if (expansion_list->heap[right_child_index].key < expansion_list->heap[left_child_index].key) {
                child_to_swap_index = right_child_index;
            }
        }

        if (expansion_list->heap[node_index].key > expansion_list->heap[child_to_swap_index].key) {
            //Node index larger than children so swap
            swap_nodes(&expansion_list->heap[node_index], &expansion_list->heap[child_to_swap_index]);

            //Continue propogating down the tree
            node_index = child_to_swap_index;
        } else {
            break; //Already a valid min-heap
        }
    }
}


void swap_nodes(t_heap_node* a, t_heap_node* b) {
    t_heap_node tmp = *a;
    *a = *b;
    *b = tmp;
}
