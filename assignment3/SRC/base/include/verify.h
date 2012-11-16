#ifndef VERIFY_H
#define VERIFY_H
//================================================================================================
// INCLUDES 
//================================================================================================

//================================================================================================
// EXTERNAL GLOBAL VARIABLES 
//================================================================================================

//================================================================================================
// GLOBAL FUNCTION DECLARATIONS 
//================================================================================================
int evaluate_solution(t_bbnode* soln);
int evaluate_partial_solution_fixed_only(t_bbnode* soln);

void dump_solution(t_bbnode* soln);
void dump_block_map(t_block_map blk_map);

//================================================================================================
// PREPROCESSOR DEFINES 
//================================================================================================

#endif