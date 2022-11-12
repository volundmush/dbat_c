//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_PLAYERS_H
#define CIRCLE_PLAYERS_H

#include "structs.h"


// global variables
extern int top_of_p_table;
extern struct player_index_element *player_table;
extern void remove_player(int pfilepos);
extern void load_imc_pfile(struct char_data *ch);

#endif //CIRCLE_PLAYERS_H
