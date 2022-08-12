//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_MISC_H
#define CIRCLE_ACT_MISC_H

#include "structs.h"
void handle_multi_merge(struct char_data *form);
void handle_songs(void);
void fish_update(void);
void disp_rpp_store(struct char_data *ch);
void handle_rpp_store(struct char_data *ch, int choice);
void rpp_feature(struct char_data *ch, const char *arg);
void ash_burn(struct char_data *ch);

// commands
ACMD(do_transform);
ACMD(do_follow);
ACMD(do_spoil);
ACMD(do_feed);
ACMD(do_beacon);
ACMD(do_dimizu);
ACMD(do_obstruct);
ACMD(do_warppool);
ACMD(do_fireshield);
ACMD(do_cook);
ACMD(do_adrenaline);
ACMD(do_ensnare);
ACMD(do_arena);
ACMD(do_bury);
ACMD(do_hayasa);
ACMD(do_instill);
ACMD(do_kanso);
ACMD(do_hydromancy);
ACMD(do_channel);
ACMD(do_shimmer);
ACMD(do_metamorph);
ACMD(do_amnisiac);
ACMD(do_healglow);
ACMD(do_resize);
ACMD(do_scry);
ACMD(do_runic);
ACMD(do_extract);
ACMD(do_fish);
ACMD(do_defend);
ACMD(do_lifeforce);
ACMD(do_liquefy);
ACMD(do_shell);
ACMD(do_moondust);
ACMD(do_preference);
ACMD(do_song);
ACMD(do_multiform);
ACMD(do_spiritcontrol);
ACMD(do_ashcloud);
ACMD(do_silk);

#endif //CIRCLE_ACT_MISC_H
