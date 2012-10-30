#ifndef UTIL_H
#define UTIL_H
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <data_structs.h>
#include <argparse.h>

// Debug Printing
#define DEBUG_PRINT(LEVEL, ...) do { if(LEVEL <= g_args->verbosity) fprintf( stderr, __VA_ARGS__ ); } while (0)

// Debug levels
#define EXTRA_INFO 10
#define NORMAL_INFO 5


void* my_malloc(size_t size);

void* my_realloc(void* ptr, size_t size);

void* my_calloc(size_t size_of_item, size_t num_items);

double my_abs(double a);

double my_pct_diff(double orig_val, double new_val);

void die (const char * format, ...);

#endif
