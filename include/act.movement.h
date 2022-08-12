//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_MOVEMENT_H
#define CIRCLE_ACT_MOVEMENT_H

#include "structs.h"

// global variables
extern const char *cmd_door[NUM_DOOR_CMD];

// functions
void handle_teleport(struct char_data *ch, struct char_data *tar, int location);
void dismount_char(struct char_data *ch);
void mount_char(struct char_data *ch, struct char_data *mount);
int land_location(struct char_data *ch, char *arg);
void carry_drop(struct char_data *ch, int type);
int has_o2(struct char_data *ch);
int do_simple_move(struct char_data *ch, int dir, int need_specials_check);
int perform_move(struct char_data *ch, int dir, int need_specials_check);

// commands
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_leave);
ACMD(do_stand);
ACMD(do_fly);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);
ACMD(do_follow);
ACMD(do_flee);
ACMD(do_carry);
ACMD(do_land);
ACMD(do_move);

#endif //CIRCLE_ACT_MOVEMENT_H
