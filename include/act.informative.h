//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_INFORMATIVE_H
#define CIRCLE_ACT_INFORMATIVE_H

#include "structs.h"

// defines
#define MAX_PORTAL_TYPES        6
/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2

#define HIST_LENGTH 100

// global variables
extern int *cmd_sort_info;

// functions
int readIntro(struct char_data *ch, struct char_data *vict);
void introCreate(struct char_data *ch);
int check_disabled(const struct command_info *command);
void sort_commands(void);
char *find_exdesc(char *word, struct extra_descr_data *list);
void add_history(struct char_data *ch, char *str, int type);
void introWrite(struct char_data *ch, struct char_data *vict, char *name);
void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief);
int perf_skill(int skill);
int search_help(const char *argument, int level);


// commands
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_status);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_exits);
ACMD(do_autoexit);
ACMD(do_history);
ACMD(do_map);
ACMD(do_rptrans);
ACMD(do_finger);
ACMD(do_perf);
ACMD(do_nickname);
ACMD(do_table);
ACMD(do_play);
ACMD(do_post);
ACMD(do_hand);
ACMD(do_shuffle);
ACMD(do_draw);
ACMD(do_kyodaika);
ACMD(do_mimic);
ACMD(do_rpbank);
ACMD(do_rdisplay);
ACMD(do_evolve);
ACMD(do_rpbanktrans);
ACMD(do_showoff);
ACMD(do_intro);
ACMD(do_scan);
ACMD(do_toplist);
ACMD(do_whois);

#endif //CIRCLE_ACT_INFORMATIVE_H
