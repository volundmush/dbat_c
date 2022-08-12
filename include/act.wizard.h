//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_WIZARD_H
#define CIRCLE_ACT_WIZARD_H

#include "structs.h"

/* global variables */


// functions
void search_replace(char *string, const char *find, const char *replace);
void update_space(void);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);
void copyover_check(void);

// commands
ACMD(do_echo);
ACMD(do_send);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
ACMD(do_stat);
ACMD(do_shutdown);
ACMD(do_recall);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
ACMD(do_show);
ACMD(do_set);
ACMD(do_saveall);
ACMD(do_wizupdate);
ACMD(do_chown);
ACMD(do_zpurge);
ACMD(do_zcheck);
ACMD(do_checkloadstatus);
ACMD(do_spells);
ACMD(do_finddoor);
ACMD(do_interest);
ACMD(do_transobj);
ACMD(do_permission);
ACMD(do_reward);
ACMD(do_approve);
ACMD(do_newsedit);
ACMD(do_news);
ACMD(do_lag);
ACMD(do_rbank);
ACMD(do_hell);
ACMD(do_varstat);
ACMD(do_handout);
ACMD(do_ginfo);
ACMD(do_plist);
ACMD(do_peace);
ACMD(do_raise);
ACMD(do_boom);

#endif //CIRCLE_ACT_WIZARD_H
