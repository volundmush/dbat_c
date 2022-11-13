//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_CLASS_H
#define CIRCLE_CLASS_H

#include "structs.h"


// global variables
extern const bool prestige_classes[NUM_CLASSES];
extern const bool class_ok_race[NUM_RACES][NUM_CLASSES];
extern const char *config_sect[NUM_CONFIG_SECTIONS+1];
extern const int class_hit_die_size[NUM_CLASSES];
extern const char *pc_class_types[NUM_CLASSES+1];
extern const char *class_names[NUM_CLASSES+1];
extern const char *class_abbrevs[NUM_CLASSES+1];
extern const bool class_ok_align[NUM_ALIGNS][NUM_CLASSES];

// functions
extern void do_start(struct char_data *ch);
extern int invalid_class(struct char_data *ch, struct obj_data *obj);
extern int level_exp(struct char_data *ch, int level);
extern int load_levels();
extern void cedit_creation(struct char_data *ch);
extern int parse_class(struct char_data *ch, int arg);
extern void advance_level(struct char_data *ch, int whichclass);
extern int8_t ability_mod_value(int abil);
extern int8_t dex_mod_capped(const struct char_data *ch);
extern char *class_desc_str(struct char_data *ch, int howlong, int wantthe);
extern int total_skill_levels(struct char_data *ch, int skill);
extern int highest_skill_value(int level, int type);
extern int calc_penalty_exp(struct char_data *ch, int gain);
extern time_t birth_age(struct char_data *ch);
extern time_t max_age(struct char_data *ch);

#endif //CIRCLE_CLASS_H
