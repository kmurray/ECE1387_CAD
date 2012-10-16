#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <argp.h>


/*
 * Based off of the argp tutorial at:
 *  crasseux.com/books/ctutorial/argp-example.html
 */

typedef struct s_arguments {
    int verbosity;
    char* netlist_file;     // The input netlist file
    int interactive_graphics;
    int draw_nets;
} t_arguments;






t_arguments* parse_args(int arc, char** argv);

extern t_arguments* g_args;

#endif
