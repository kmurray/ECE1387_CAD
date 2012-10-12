#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <argp.h>

static error_t parse_opt(int key, char* arg, struct argp_state* state);

/*
 * Based off of the argp tutorial at:
 *  crasseux.com/books/ctutorial/argp-example.html
 */

const char* argp_program_version = "kroute 0.1.0";
const char* argp_program_bug_address = "k.murray@eecg.utoronto.ca";

struct arguments {
    int verbose;
    char* netlist_file;     // The input netlist file
    int interactive_graphics;
    int opt_reservoirs;
};

static struct argp_option options[] = {
    /* Fields:
     * {NAME, KEY, ARG, FLAGS, DOC}
     */
    {"verbose", 'v', 0, 0, "Print more detailed information"},
    {"circuit", 'c', "NETLIST_FILE", 0, "The input circuit netlist file"},
    {"interactive_graphics", 'i', 0, 0, "Run the graphics in interactive mode"},
    {"opt_reservoirs", 'o', 0, 0, "Turn on reservoir optimization. May impact routability and runtime."},
    {0}
};

int check_args(struct arguments* args) {
    if (args->netlist_file == NULL) {
        return 0;
    }

    return 1;
}

void set_default_args(struct arguments* args) {
    args->verbose = 0;
    args->netlist_file = NULL;
    args->interactive_graphics = 0;
    args->opt_reservoirs = 0;
}

//Non optioned argument documentation
static char args_doc[] = "";


//Program documentation
static char doc[] = "kroute -- An FPGA routing program for ECE1387 2012 Assignment #1";

static struct argp argp = {options, parse_opt, args_doc, doc};

/*
 * Argument parser
 */
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct arguments* arguments = state->input;

    switch (key) {
        case 'v':
            arguments->verbose = 1;
        case 'c':
            arguments->netlist_file = arg;
            break;
        case 'i':
            arguments->interactive_graphics = 1;
            break;
        case 'o':
            arguments->opt_reservoirs = 1;
            break;
        case ARGP_KEY_END:
            //Check for required arguments
            if(check_args(arguments) != 1) {
                argp_usage(state);
            }
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}



#endif
