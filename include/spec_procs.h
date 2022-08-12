//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_SPEC_PROCS_H
#define CIRCLE_SPEC_PROCS_H

#include "structs.h"


// functions
void npc_steal(struct char_data *ch, struct char_data *victim);
int num_players_in_room(room_vnum room);
bool check_mob_in_room(mob_vnum mob, room_vnum room);
bool check_obj_in_room(obj_vnum obj, room_vnum room);


// specials
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(auction);
SPECIAL(bank);
SPECIAL(gravity);
SPECIAL(augmenter);
SPECIAL(gauntlet_room);
SPECIAL(gauntlet_end);
SPECIAL(gauntlet_rest);
SPECIAL(magic_user_orig);
SPECIAL(healtank);
SPECIAL(augmenter);
SPECIAL(gravity);
SPECIAL(bank);
SPECIAL(cleric_marduk);
SPECIAL(cleric_ao);
SPECIAL(dziak);
SPECIAL(azimer);
SPECIAL(lyrzaxyn);

#endif //CIRCLE_SPEC_PROCS_H
