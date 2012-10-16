#include <util.h>
#include <assert.h>
#include <data_structs.h>
#include <parse_input.h>

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

void* my_calloc(size_t num_items, size_t size_of_item) {
    void* retval = calloc(num_items, size_of_item);
    if (retval == NULL) {
        printf("Error: calloc of size %ux%u bytes failed\n", (unsigned int) num_items, (unsigned int) size_of_item);
        assert(0);
        exit(1);
        
    }
    return retval;
}

void die (const char * format, ...) {
    va_list vargs;
    va_start (vargs, format);
    vfprintf (stderr, format, vargs);
    assert(0);
    exit (1);
}
