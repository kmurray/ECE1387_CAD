#include <util.h>
#include <argparse.h>


static error_t parse_opt(int key, char* arg, struct argp_state* state);
static int check_args(t_arguments* args);
static void set_default_args(t_arguments* args);

const char* argp_program_version = "kplace 0.1.0";
const char* argp_program_bug_address = "k.murray@eecg.utoronto.ca";

//Program documentation
static char doc[] = "kplace -- An Simple Analytic Standard Cell Placer for ECE1387 2012 Assignment #2";
//Non optioned argument documentation
static char args_doc[] = "";

static struct argp_option options[] = {
    /* Fields:
     * {NAME, KEY, ARG, FLAGS, DOC}
     */
    {"verbosity", 'v', "VERBOSITY", 0, "Print more detailed information"},
    {"circuit", 'c', "NETLIST_FILE", 0, "The input circuit netlist file"},
    {"draw_nets", 'n', 0, 0, "Draw net flightlines"},
    {"interactive_iteration_graphics", 'i', 0, 0, "Turn on interactive graphics at the end of each simpl stage"},
    {"interactive_spreading_graphics", 'g', 0, 0, "Turn on interactive graphics which illustrate the spreading technique"},
    {"clique", 'q', 0, 0, "Place the circuit using the clique net model"},
    {"bound2bound", 'b', 0, 0, "Place the circuit using the bound2bound net model"},
    {"simpl", 's', 0, 0, "Place the circuit using a modified version of the simPL algorithm"},
    {0}
};

static struct argp argp = {options, parse_opt, args_doc, doc};


t_arguments* parse_args(int argc, char** argv){
    t_arguments* args = (t_arguments*) my_malloc(sizeof(t_arguments));

    set_default_args(args);
    argp_parse(&argp, argc, argv, 0, 0, args);
    
    return args;
}


/*
 * Argument parser
 */
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    t_arguments* arguments = (t_arguments*) state->input;

    switch (key) {
        case 'v':
            arguments->verbosity = atoi(arg);
            break;
        case 'c':
            arguments->netlist_file = arg;
            break;
        case 'i':
            arguments->interactive_graphics = 1;
            break;
        case 'n':
            arguments->draw_nets = 1;
            break;
        case 'q':
            arguments->clique = 1;
            break;
        case 'b':
            arguments->bound2bound = 1;
            break;
        case 's':
            arguments->simpl = 1;
            break;
        case 'g':
            arguments->spreading_graphics = 1;
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

static void set_default_args(t_arguments* args) {
    args->verbosity = 100;
    args->netlist_file = NULL;
    args->interactive_graphics = 0;
    args->draw_nets = 0;
    args->clique = 0;
    args->bound2bound = 0;
    args->simpl = 0;
    args->spreading_graphics = 0;
}


static int check_args(t_arguments* args) {
    if (args->netlist_file == NULL) {
        return 0;
    }

    return 1;
}

