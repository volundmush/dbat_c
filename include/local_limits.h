//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_LIMITS_H
#define CIRCLE_LIMITS_H

#include "structs.h"

// functions
void timed_dt(struct char_data *ch);
void run_autowiz(void);
void reboot_wizlists(void);
void mutant_limb_regen(struct char_data *ch);
void set_title(struct char_data *ch, char *title);
void gain_level(struct char_data *ch, int whichclass);
void gain_exp(struct char_data *ch, int64_t gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
void point_update(void);
void update_innate(struct char_data *ch);

#endif //CIRCLE_LIMITS_H
