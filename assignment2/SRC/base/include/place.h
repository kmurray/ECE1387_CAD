#ifndef PLACE_H
#define PLACE_H

#define SIMPL_GAMMA         1.
#define SIMPL_ALPHA_FAC     1.00
#define SIMPL_RHO_LIMIT 0.2
#define SMALL_FLOAT 0.01
#define GRID_EXPANSION_AMNT 0.01

double solve_clique(void);
double solve_bound2bound(double target_pct_diff);
double solve_simpl(void);
#endif
