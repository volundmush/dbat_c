//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_ITEM_H
#define CIRCLE_ACT_ITEM_H

#include "structs.h"

/* act.item.c */
// global variables
extern struct obj_data *obj_selling;
extern struct char_data *ch_selling, *ch_buying;

// functions
int check_saveroom_count(struct char_data *ch, struct obj_data *cont);
void dball_load(void);
int check_insidebag(struct obj_data *cont, double mult);
int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void weight_change_object(struct obj_data *obj, int weight);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_remove(struct char_data *ch, int pos);
int64_t max_carry_weight(struct char_data *ch);
void stop_auction(int type, struct char_data * ch);
void check_auction(void);

// commands
ACMD(do_split);
ACMD(do_auction);
ACMD(do_bid);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_twohand);
ACMD(do_deploy);
ACMD(do_pack);
ACMD(do_garden);
ACMD(do_refuel);
ACMD(do_sac);

#endif //CIRCLE_ACT_ITEM_H
