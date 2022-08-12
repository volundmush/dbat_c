//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_CLASS_H
#define CIRCLE_CLASS_H

#include "structs.h"


// global variables
extern int prestige_classes[NUM_CLASSES];
extern const int class_ok_race[NUM_RACES][NUM_CLASSES];
extern const char *config_sect[NUM_CONFIG_SECTIONS+1];
extern const int class_hit_die_size[NUM_CLASSES];
extern const char *pc_class_types[NUM_CLASSES+1];
extern const char *class_names[NUM_CLASSES+1];
extern const char *class_abbrevs[NUM_CLASSES+1];

// functions
void do_start(struct char_data *ch);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(struct char_data *ch, int level);
int load_levels();
void cedit_creation(struct char_data *ch);
int parse_class(struct char_data *ch, int arg);
void advance_level(struct char_data *ch, int whichclass);
int8_t ability_mod_value(int abil);
int8_t dex_mod_capped(const struct char_data *ch);
char *class_desc_str(struct char_data *ch, int howlong, int wantthe);
int total_skill_levels(struct char_data *ch, int skill);
int highest_skill_value(int level, int type);
int calc_penalty_exp(struct char_data *ch, int gain);
time_t birth_age(struct char_data *ch);
time_t max_age(struct char_data *ch);

#endif //CIRCLE_CLASS_H
