//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_FIGHT_H
#define CIRCLE_FIGHT_H

#include "structs.h"
#include "spells.h"

// global variables
extern struct char_data *combat_list, *next_combat_list;
extern struct attack_hit_type attack_hit_text[NUM_ATTACK_TYPES];

// functions
void death_cry(struct char_data *ch);
int group_bonus(struct char_data *ch, int type);
void die(struct char_data *ch, struct char_data * killer);
void remove_limb(struct char_data *vict, int num);
void impact_sound(struct char_data *ch, char *mssg);
void fight_stack(void);
void appear(struct char_data *ch);
void raw_kill(struct char_data *ch, struct char_data * killer);
void	set_fighting(struct char_data *ch, struct char_data *victim);
void	stop_fighting(struct char_data *ch);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
void mutant_limb_regen(struct char_data *ch);

// commands
ACMD(do_kousengan);
ACMD(do_heal);
ACMD(do_trip);
ACMD(do_koteiru);
ACMD(do_razor);
ACMD(do_spike);
ACMD(do_ddslash);
ACMD(do_kakusanha);
ACMD(do_psyblast);
ACMD(do_punch);
ACMD(do_powerup);
ACMD(do_srepair);
ACMD(do_absorb);
ACMD(do_kaioken);
ACMD(do_strike);
ACMD(do_kick);
ACMD(do_elbow);
ACMD(do_knee);
ACMD(do_uppercut);
ACMD(do_roundhouse);
ACMD(do_heeldrop);
ACMD(do_slam);
ACMD(do_tailwhip);
ACMD(do_head);
ACMD(do_bite);
ACMD(do_ram);
ACMD(do_breath);
ACMD(do_kiball);
ACMD(do_kiblast);
ACMD(do_beam);
ACMD(do_renzo);
ACMD(do_tsuihidan);
ACMD(do_shogekiha);
ACMD(do_kamehameha);
ACMD(do_galikgun);
ACMD(do_masenko);
ACMD(do_hellspear);
ACMD(do_hellflash);
ACMD(do_dualbeam);
ACMD(do_honoo);
ACMD(do_pbarrage);
ACMD(do_tslash);
ACMD(do_pslash);
ACMD(do_crusher);
ACMD(do_eraser);
ACMD(do_spiral);
ACMD(do_tribeam);
ACMD(do_dodonpa);
ACMD(do_hass);
ACMD(do_zanzoken);
ACMD(do_deathball);
ACMD(do_deathbeam);
ACMD(do_kienzan);
ACMD(do_bigbang);
ACMD(do_final);
ACMD(do_sbc);
ACMD(do_scatter);
ACMD(do_nova);
ACMD(do_breaker);
ACMD(do_seishou);
ACMD(do_ensnare);
ACMD(do_barrier);
ACMD(do_attack);
ACMD(do_stand);
ACMD(do_fly);
ACMD(do_wake);
ACMD(do_flee);
ACMD(do_get);
ACMD(do_split);
ACMD(do_sac);
ACMD(do_transform);
ACMD(do_kaioken);
ACMD(do_escape);
ACMD(do_balefire);
ACMD(do_blessedhammer);

#endif //CIRCLE_FIGHT_H
