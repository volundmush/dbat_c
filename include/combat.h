/* ************************************************************************
*  File: combat.h                    Part of Dragonball Advent Truth      *
*  Usage: Combat utilities and common functions for act.offensive.c and   *
*  act.attack.c                                                           *
*                                                                         *
*  All rights reserved to Iovan that are not due to anyone else.          *
*                                                                         *
*  This file was first written on 2011 and aside for a few instances only *
*  contains code written by Iovan for use with the Real Dragonball Battle *
*  System (RDBS) of the MUD Dragonball Advent Truth.                      *
************************************************************************ */

#ifndef __COMBAT_H__
#define __COMBAT_H__

#include "structs.h"

/* combat.c functions */
void homing_update(void);
void damage_weapon(struct char_data *ch, struct obj_data *obj, struct char_data *vict);
int64_t advanced_energy(struct char_data *ch, int64_t dmg);
int roll_accuracy(struct char_data *ch, int skill, bool kiatt);
void handle_death_msg(struct char_data *ch, struct char_data *vict, int type);
int handle_combo(struct char_data *ch, struct char_data *vict);
int handle_parry(struct char_data *ch);
void handle_spiral(struct char_data *ch, struct char_data *vict, int skill, int first);
void handle_cooldown(struct char_data *ch, int cooldown);
void parry_ki(double attperc, struct char_data *ch, struct char_data *vict, char sname[1000], int prob, int perc, int skill, int type);
void dodge_ki(struct char_data *ch, struct char_data *vict, int type, int type2, int skill, int skill2);
void damage_eq(struct char_data *vict, int location);
void remove_limb(struct char_data *vict, int num);
int check_skill(struct char_data *ch, int skill);
int check_points(struct char_data *ch, int64_t ki, int64_t st);
void pcost(struct char_data *ch, double ki, int64_t st);
void hurt(int limb, int chance, struct char_data *ch, struct char_data *vict, struct obj_data *obj, int64_t dmg, int type);
int64_t damtype(struct char_data *ch, int type, int skill, double percent);
int can_kill(struct char_data *ch, struct char_data *vict, struct obj_data *obj, int num);
void huge_update(void);
int init_skill(struct char_data *ch, int snum);
int can_grav(struct char_data *ch);
int limb_ok(struct char_data *ch, int type);
int check_def(struct char_data *vict);
void dam_eq_loc(struct char_data *vict, int area);
void hurt_limb(struct char_data *ch, struct char_data *vict, int chance, int area, int64_t power);
int handle_speed(struct char_data *ch, struct char_data *vict);
void handle_defense(struct char_data *vict, int *pry, int *blk, int *dge);
void spar_gain(struct char_data *ch, struct char_data *vict, int type, int64_t dmg);
int chance_to_hit(struct char_data *ch);
int roll_hitloc(struct char_data *ch, struct char_data *vict, int skill);
long double calc_critical(struct char_data *ch, int loc);
void decapitate_vict(struct char_data *ch, struct char_data *vict, int64_t dmg);
void sword_chop(struct char_data *ch, struct char_data *vict, int wlvl, int loc, int type);
void armor_pierce(struct char_data *ch, struct char_data *vict, int wlvl, int type);
int backstab(struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg);
int64_t gun_dam(struct char_data *ch, int wlvl);
int physical_mastery(struct char_data *ch);
int count_physical(struct char_data *ch);
int handle_dodge(struct char_data *ch);
int handle_block(struct char_data *ch);
void update_mob_absorb(void);
void cut_limb(struct char_data *ch, struct char_data *vict, int wlvl, int hitspot);
void club_stamina(struct char_data *ch, struct char_data *vict, int wlvl, int64_t dmg);
int boom_headshot(struct char_data *ch);
void handle_knockdown(struct char_data *ch);
int roll_balance(struct char_data *ch);
int64_t combo_damage(struct char_data *ch, int64_t damage, int type);
int check_ruby(struct char_data *ch);
void combine_attacks(struct char_data *ch, struct char_data *vict);
void handle_disarm(struct char_data *ch, struct char_data *vict);
int handle_defender(struct char_data *vict, struct char_data *ch);
void handle_multihit(struct char_data *ch, struct char_data *vict);
int64_t armor_calc(struct char_data *ch, int64_t dmg, int type);
#endif