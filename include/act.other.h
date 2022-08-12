//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_OTHER_H
#define CIRCLE_ACT_OTHER_H

#include "structs.h"

// variables
extern const room_vnum freeres[NUM_ALIGNS];

// functions
void log_imm_action(char *messg, ...);
void hint_system(struct char_data *ch, int num);
int dball_count(struct char_data *ch);
void log_custom(struct descriptor_data *d, struct obj_data *obj);
void wishSYS(void);
void bring_to_cap(struct char_data *ch);
void base_update(void);
void load_shadow_dragons();
void innate_remove(struct char_data * ch, struct innate_node * inn);
void innate_add(struct char_data * ch, int innate, int timer);
int is_innate(struct char_data *ch, int spellnum);
int is_innate_ready(struct char_data *ch, int spellnum);
void add_innate_timer(struct char_data *ch, int spellnum);
void add_innate_affects(struct char_data *ch);
void update_innate(struct char_data *ch);

// commands
ACMD(do_gen_comm);
ACMD(do_charge);
ACMD(do_wear);
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_value);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
ACMD(do_scribe);
ACMD(do_pagelength);
ACMD(do_scouter);
ACMD(do_snet);
ACMD(do_spar);
ACMD(do_pushup);
ACMD(do_situp);
ACMD(do_transform);
ACMD(do_summon);
ACMD(do_instant);
ACMD(do_barrier);
ACMD(do_heal);
ACMD(do_solar);
ACMD(do_eyec);
ACMD(do_zanzoken);
ACMD(do_eavesdrop);
ACMD(do_disguise);
ACMD(do_appraise);
ACMD(do_forgery);
ACMD(do_plant);
ACMD(do_kaioken);
ACMD(do_focus);
ACMD(do_regenerate);
ACMD(do_escape);
ACMD(do_absorb);
ACMD(do_ingest);
ACMD(do_upgrade);
ACMD(do_srepair);
ACMD(do_recharge);
ACMD(do_form);
ACMD(do_spit);
ACMD(do_majinize);
ACMD(do_potential);
ACMD(do_telepathy);
ACMD(do_fury);
ACMD(do_pose);
ACMD(do_implant);
ACMD(do_hass);
ACMD(do_suppress);
ACMD(do_drag);
ACMD(do_stop);
ACMD(do_future);
ACMD(do_candy);
ACMD(do_kura);
ACMD(do_taisha);
ACMD(do_paralyze);
ACMD(do_infuse);
ACMD(do_rip);
ACMD(do_train);
ACMD(do_trip);
ACMD(do_grapple);
ACMD(do_willpower);
ACMD(do_commune);
ACMD(do_rpp);
ACMD(do_meditate);
ACMD(do_aura);
ACMD(do_think);
ACMD(do_block);
ACMD(do_visible);
ACMD(do_compare);
ACMD(do_break);
ACMD(do_fix);
ACMD(do_resurrect);
ACMD(do_clan);
ACMD(do_aid);

#endif //CIRCLE_ACT_OTHER_H
