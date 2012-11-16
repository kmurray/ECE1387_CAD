#ifndef PARSE_INPUT_H
#define PARSE_INPUT_H

//================================================================================================
// INCLUDES 
//================================================================================================

//================================================================================================
// EXTERNAL GLOBAL VARIABLES 
//================================================================================================
extern t_CHIP* g_CHIP;

//================================================================================================
// GLOBAL FUNCTION DECLARATIONS 
//================================================================================================
void parse_netlist(const char* filename);

//================================================================================================
// PREPROCESSOR DEFINES 
//================================================================================================
#define DEFAULT_LINE_LENGTH 128
#define TOKEN_SEPS " "

void associate_block_and_net(t_block* block, t_net* net);

#endif
