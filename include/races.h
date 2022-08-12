//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_RACES_H
#define CIRCLE_RACES_H

#include "structs.h"


// global variables
extern const int race_ok_gender[NUM_SEX][NUM_RACES];
extern const char *d_race_types[NUM_RACES+1];
extern const char *race_names[NUM_RACES+1];
extern const char *pc_race_types[NUM_RACES+1];
extern const struct guild_info_type guild_info[6];
extern const int race_def_sizetable[NUM_RACES + 1];
extern const char *race_abbrevs[NUM_RACES+1];

// functions
void racial_body_parts(struct char_data *ch);
void racial_ability_modifiers(struct char_data *ch);
void set_height_and_weight_by_race(struct char_data *ch);
int invalid_race(struct char_data *ch, struct obj_data *obj);
int parse_race(struct char_data *ch, int arg);

#endif //CIRCLE_RACES_H
