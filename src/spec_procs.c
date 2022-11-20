/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "spec_procs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "mail.h"
#include "act.movement.h"
#include "act.item.h"
#include "act.social.h"
#include "guild.h"
#include "races.h"
#include "act.comm.h"
#include "item_search.h"

/* local functions */


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump) {
    struct obj_data *k;
    int value = 0;

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p vanishes in a puff of smoke!", 0, 0, k, 0, TO_ROOM);
        extract_obj(k);
    }

    if (!CMD_IS("drop"))
        return false;

    do_drop(ch, argument, cmd, SCMD_DROP);

    for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
        act("$p vanishes in a puff of smoke!", 0, 0, k, 0, TO_ROOM);
        value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
        extract_obj(k);
    }

    if (value) {
        send_to_char(ch, "You are awarded for outstanding performance.\r\n");
        act("$n has been awarded for being a good citizen.", 1, ch, 0, 0, TO_ROOM);

        if (GET_LEVEL(ch) < 3)
            gain_exp(ch, value);
        else
            GET_GOLD(ch) += value;
    }
    return true;
}


SPECIAL(mayor) {
    char actbuf[MAX_INPUT_LENGTH];

    const char open_path[] =
            "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
    const char close_path[] =
            "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path = NULL;
    static int path_index;
    static bool move = 0;

    if (!move) {
        if (time_info.hours == 6) {
            move = 1;
            path = open_path;
            path_index = 0;
        } else if (time_info.hours == 20) {
            move = 1;
            path = close_path;
            path_index = 0;
        }
    }
    if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
        (GET_POS(ch) == POS_FIGHTING))
        return false;

    switch (path[path_index]) {
        case '0':
        case '1':
        case '2':
        case '3':
            perform_move(ch, path[path_index] - '0', 1);
            break;

        case 'W':
            GET_POS(ch) = POS_STANDING;
            act("$n awakens and groans loudly.", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'S':
            GET_POS(ch) = POS_SLEEPING;
            act("$n lies down and instantly falls asleep.", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'a':
            act("$n says 'Hello Honey!'", 0, ch, 0, 0, TO_ROOM);
            act("$n smirks.", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'b':
            act("$n says 'What a view!  I must get something done about that dump!'",
                0, ch, 0, 0, TO_ROOM);
            break;

        case 'c':
            act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
                0, ch, 0, 0, TO_ROOM);
            break;

        case 'd':
            act("$n says 'Good day, citizens!'", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'e':
            act("$n says 'I hereby declare the bazaar open!'", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'E':
            act("$n says 'I hereby declare Midgaard closed!'", 0, ch, 0, 0, TO_ROOM);
            break;

        case 'O':
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);    /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);    /* strcpy: OK */
            break;

        case 'C':
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);    /* strcpy: OK */
            do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);    /* strcpy: OK */
            break;

        case '.':
            move = 0;
            break;

    }

    path_index++;
    return false;
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

int num_players_in_room(room_vnum room) {
    struct descriptor_data *i;
    int num_players = 0;

    for (i = descriptor_list; i; i = i->next) {
        if (STATE(i) != CON_PLAYING)
            continue;
        if (!(i->character))
            continue;
        if (IN_ROOM(i->character) == NOWHERE || IN_ROOM(i->character) > top_of_world)
            continue;
        if (world[IN_ROOM(i->character)].number != room)
            continue;
        if ((GET_ADMLEVEL(i->character) >= ADMLVL_IMMORT) &&
            (PRF_FLAGGED(i->character, PRF_NOHASSLE))) /* Ignore Imms */
            continue;

        num_players++;
    }

    return num_players;
}

bool check_mob_in_room(mob_vnum mob, room_vnum room) {
    register struct char_data *i;
    bool found = 0;

    for (i = character_list; i; i = i->next)
        if (GET_MOB_VNUM(i) == mob)
            if (world[i->in_room].number == room) found = 1;

    return found;
}

bool check_obj_in_room(obj_vnum obj, room_vnum room) {
    return find_obj_in_list_vnum(world[real_room(room)].contents, obj);
}

static const int gauntlet_info[][3] = {  /* --mystic 26 Oct 2005 */

/* Gauntlet Room Scoring*/
/* Num  Rm Num   Direction   */
        {0,  2403, SCMD_SOUTH},  /* Waiting Room     */
        {1,  2404, SCMD_SOUTH},  /* About level 5    */
        {2,  2405, SCMD_SOUTH},  /* About level 10   */
        {3,  2406, SCMD_SOUTH},  /* About level 15   */
        {4,  2407, SCMD_SOUTH},  /* About level 20   */
        {5,  2408, SCMD_SOUTH},  /* About level 25   */
        {6,  2409, SCMD_SOUTH},  /* About level 30   */
        {7,  2410, SCMD_SOUTH},  /* About level 35   */
        {8,  2411, SCMD_SOUTH},  /* About level 40   */
        {9,  2412, SCMD_SOUTH},  /* About level 45   */
        {10, 2413, SCMD_SOUTH},  /* About level 50   */
        {11, 2414, SCMD_SOUTH},  /* About level 55   */
        {12, 2415, SCMD_SOUTH},  /* About level 60   */
        {13, 2416, SCMD_SOUTH},  /* About level 65   */
        {14, 2417, SCMD_SOUTH},  /* About level 70   */
        {15, 2418, SCMD_SOUTH},  /* About level 75   */
        {16, 2420, SCMD_SOUTH},  /* About level 80   */
        {17, 2421, SCMD_SOUTH},  /* About level 85   */
        {18, 2422, SCMD_SOUTH},  /* About level 90   */
        {19, 2423, SCMD_SOUTH},  /* About level 95   */
        {20, 2424, SCMD_SOUTH},  /* About level 100  */
        {21, 2425, SCMD_SOUTH},  /* About level 5    */
        {22, 2426, SCMD_SOUTH},  /* About level 10   */
        {23, 2427, SCMD_SOUTH},  /* About level 15   */
        {24, 2428, SCMD_SOUTH},  /* About level 20   */
        {25, 2429, SCMD_SOUTH},  /* About level 25   */
        {26, 2430, SCMD_SOUTH},  /* About level 30   */
        {27, 2431, SCMD_SOUTH},  /* About level 35   */
        {28, 2432, SCMD_SOUTH},  /* About level 40   */
        {29, 2433, SCMD_SOUTH},  /* About level 45   */
        {30, 2434, SCMD_SOUTH},  /* About level 50   */
        {31, 2435, SCMD_SOUTH},  /* About level 55   */
        {32, 2436, SCMD_SOUTH},  /* About level 60   */
        {33, 2437, SCMD_SOUTH},  /* About level 65   */
        {34, 2438, SCMD_SOUTH},  /* About level 70   */
        {35, 2439, SCMD_SOUTH},  /* About level 75   */
        {36, 2440, SCMD_SOUTH},  /* About level 80   */
        {37, 2441, SCMD_SOUTH},  /* About level 85   */
        {38, 2442, SCMD_SOUTH},  /* About level 90   */
        {39, 2443, SCMD_SOUTH},  /* About level 95   */
        {40, 2444, SCMD_SOUTH},  /* About level 100  */
        {-1, -1, -1}
};

SPECIAL(gauntlet_room)  /* Jamdog - 13th Feb 2006 */
{
    int i = 0;
    int proceed = 1;
    struct char_data *tch;
    char *buf2 = "$N tried to sneak past without a fight, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = 1;

    /* give player credit for making it this far */
    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        if ((!IS_NPC(ch)) && (world[IN_ROOM(ch)].number == gauntlet_info[i][1])) {
            /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
            if (GET_GAUNTLET(ch) < (gauntlet_info[i][0])) {
                //set player's gauntlet rank
                GET_GAUNTLET(ch) = (gauntlet_info[i][0]);
            }
        }
    }

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        send_to_char(ch,
                     "Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (!IS_MOVE(cmd) && !CMD_IS("surrender")) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (CMD_IS("surrender")) {
        if (FIGHTING(ch)) {
            /* OK, player has had enough - position is already stored, so throw them back to the start */
            char_from_room(ch);
            char_to_room(ch, real_room(gauntlet_info[0][1]));
            act("$n suddenly appears looking relieved after $s trial in the Gauntlet", 0, ch, 0, ch, TO_NOTVICT);
            act("You are returned to the start of the Gauntlet", 0, ch, 0, ch, TO_VICT);

            /* Hit point penalty for surrendering */
            if (GET_HIT(ch) > 2000)
                GET_HIT(ch) = GET_HIT(ch) / 5;
            else if (GET_HIT(ch) > 500)
                GET_HIT(ch) = 100;
            else
                GET_HIT(ch) = 1;

            look_at_room(IN_ROOM(ch), ch, 0);
            return true;
        } else {
            send_to_char(ch, "You can only surrender while fighting, so at least TRY to make an effort");
            return true;
        }
    }

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    /* Only Avatars may pass the 11th room
    if ((world[ch->in_room].number == gauntlet_info[GAUNTLET_AV][1] ) && (cmd == gauntlet_info[GAUNTLET_AV][2]))
    {
      if (GET_CLASS(ch) != CLASS_AVATAR)
      {
        send_to_char (ch, "Only Avatars may proceed further!\r\n");
        return TRUE;
      }
    } */
    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        if (world[ch->in_room].number == gauntlet_info[i][1]) {
            if (cmd == gauntlet_info[i][2]) {
                //don't let him proceed if mob is still alive
                for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
                    if (IS_NPC(tch) && i > 0)  /* Ignore mobs in the waiting room */
                    {
                        proceed = 0;
                        sprintf(buf, "%s wants to teach you a lesson first.\r\n", GET_NAME(tch));
                    }
                }
                /* In the case of the phoenix room, don't progress if there is 1st or 2nd ashes */
                /* TODO: (add in here when ash is created) */
                if (proceed) {
                    nomob = 1;

                    /* Check the next room for players and ensure mob is waiting */
                    for (tch = world[real_room(gauntlet_info[i + 1][1])].people; tch; tch = tch->next_in_room) {
                        if (!IS_NPC(tch)) {
                            proceed = 0;  /* There is a player there */
                            sprintf(buf, "%s is in the next room.  You must wait for them to finish.\r\n",
                                    GET_NAME(tch));
                        } else {
                            nomob = 0;
                        }
                    }
                    if (nomob == 1) {
                        proceed = 0;  /* There is no mob there in the next room */
                        sprintf(buf, "The next room is empty.  You must wait for your opponent to re-appear.\r\n");
                    }
                }

                if (proceed == 0) {
                    send_to_char(ch, buf);
                    act(buf2, 0, ch, 0, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

SPECIAL(gauntlet_end)  /* Jamdog - 20th Feb 2007 */
{
    int i = 0;

    /* give player credit for making it this far */
    if (!IS_NPC(ch)) {
        /* Check not overwriting gauntlet rank with lower value (Jamdog - 20th July 2006) */
        if (GET_GAUNTLET(ch) < GAUNTLET_END) {
            //set player's gauntlet rank
            GET_GAUNTLET(ch) = GAUNTLET_END;
        }
    }

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        if ((FIGHTING(ch)) && (GET_POS(ch) == POS_FIGHTING)) {
            send_to_char(ch,
                         "You can't flee from this fight./r/nIt's your own fault for summoning creatures into the gauntlet!\r\n");
            return true;
        } else {
            send_to_char(ch, "There is nothing here to flee from\r\n");
            return true;
        }
    }

    if (CMD_IS("surrender")) {
        send_to_char(ch, "You have completed the gauntlet, why would you need to surrender?\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (!EXIT(ch, cmd - 1) || EXIT(ch, cmd - 1)->to_room == NOWHERE)
        return false;
    if (EXIT_FLAGGED(EXIT(ch, cmd - 1), EX_CLOSED))
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        if (world[EXIT(ch, (cmd - 1))->to_room].number == gauntlet_info[i][1]) {
            send_to_char(ch, "You have completed the gauntlet, you cannot go backwards!\r\n");
            return true;
        }
    }
    return false;
}

SPECIAL(gauntlet_rest)  /* Jamdog - 20th Feb 2007 */
{
    int i = 0;
    int proceed = 1, door;
    struct char_data *tch;
    char *buf2 = "$N tried to return to the gauntlet, and got nowhere.";
    char buf[MAX_STRING_LENGTH];
    bool nomob = 1;

    if (!cmd)                        /* If no command, then nothing to do             */
        return false;

    if (CMD_IS("flee")) {
        send_to_char(ch,
                     "Fleeing is not allowed!  If you want to get out of here, type @Ysurrender@n while fighting to be returned to the start.");
        return true;
    }

    if (CMD_IS("surrender")) {
        send_to_char(ch,
                     "You are in a rest-room.  Surrender is not an option.\r\nIf you want to leave the Gauntlet, you can surrender while fighting.\r\n");
        return true;
    }

    if (!IS_MOVE(cmd)) /* Only movement commands need to be checked     */
        return false;

    if (IS_NPC(ch))                  /* Mobs can move about - Jamdog 20th July 2006   */
        return false;                  /* This also allows following pets!              */

    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) /* Imms can walk through the gauntlet unhindered */
        return false;

    for (i = 0; gauntlet_info[i][0] != -1; i++) {
        for (door = 0; door < NUM_OF_DIRS; door++) {
            if (!EXIT(ch, door) || EXIT(ch, door)->to_room == NOWHERE)
                continue;
            if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
                continue;

            if ((world[EXIT(ch, door)->to_room].number == gauntlet_info[i][1]) && (door == (cmd - 1))) {
                nomob = 1;

                /* Check the next room for players and ensure mob is waiting */
                for (tch = world[real_room(gauntlet_info[i][1])].people; tch; tch = tch->next_in_room) {
                    if (!IS_NPC(tch)) {
                        proceed = 0;  /* There is a player there */
                        sprintf(buf, "%s has moved into the next room.  You must wait for them to finish.\r\n",
                                GET_NAME(tch));
                    } else {
                        nomob = 0;
                    }
                }
/* Not needed - players can go back if the mob is dead 
        if (nomob == TRUE) 
        { 
          proceed=0;  // There is no mob there in the next room 
          sprintf(buf,"The next room is empty.  You must wait for your opponent to re-appear.\r\n"); 
        } 
*/
                if (proceed == 0) {
                    send_to_char(ch, buf);
                    act(buf2, 0, ch, 0, ch, TO_ROOM);
                    return true;
                }
            }
        }
    }
    return false;
}

void npc_steal(struct char_data *ch, struct char_data *victim) {
    int gold;

    if (IS_NPC(victim))
        return;
    if (IS_NPC(ch))
        return;
    if (ADM_FLAGGED(victim, ADM_NOSTEAL))
        return;
    if (!CAN_SEE(ch, victim))
        return;

    if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
        act("You discover that $n has $s hands in your wallet.", 0, ch, 0, victim, TO_VICT);
        act("$n tries to steal zenni from $N.", 1, ch, 0, victim, TO_NOTVICT);
    } else {
        /* Steal some gold coins */
        gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
        if (gold > 0) {
            GET_GOLD(ch) += gold;
            GET_GOLD(victim) -= gold;
        }
    }
}


/*
 * Quite lethal to low-level characters.
 */
SPECIAL(snake) {
    if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
        return false;

    if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
        return false;

    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL, NULL);
    return true;
}


SPECIAL(thief) {
    struct char_data *cons;

    if (IS_NPC(ch))
        return false;

    if (cmd || GET_POS(ch) != POS_STANDING)
        return false;

    for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
        if (!IS_NPC(cons) && !ADM_FLAGGED(cons, ADM_NOSTEAL) && !rand_number(0, 4)) {
            npc_steal(ch, cons);
            return true;
        }

    return false;
}


SPECIAL(magic_user_orig) {
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
        vict = FIGHTING(ch);

    /* Hm...didn't pick anyone...I'll wait a round. */
    if (vict == NULL)
        return true;

    if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
        cast_spell(ch, vict, NULL, SPELL_POISON, NULL);

    if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
        cast_spell(ch, vict, NULL, SPELL_BLINDNESS, NULL);

    if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0) {
        if (IS_EVIL(ch))
            cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN, NULL);
        else if (IS_GOOD(ch))
            cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
    }

    if (rand_number(0, 4))
        return true;

    switch (GET_LEVEL(ch)) {
        case 4:
        case 5:
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
            break;
        case 6:
        case 7:
            cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
            break;
        case 8:
        case 9:
            cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
            break;
        case 10:
        case 11:
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
            break;
        case 12:
        case 13:
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
            break;
        case 14:
        case 15:
        case 16:
        case 17:
            cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, NULL);
            break;
        default:
            cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
            break;
    }
    return true;

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard) {
    int i;
    struct char_data *guard = (struct char_data *) me;
    const char *buf = "The guard humiliates you, and blocks your way.\r\n";
    const char *buf2 = "The guard humiliates $n, and blocks $s way.";

    if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
        return false;

    if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
        return false;

    for (i = 0; guild_info[i].guild_room != NOWHERE; i++) {
        /* Wrong guild or not trying to enter. */
        if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room || cmd != guild_info[i].direction)
            continue;

        /* Allow the people of the guild through. */
        if (!IS_NPC(ch) && GET_CLASS_RANKS(ch, guild_info[i].pc_class) > 0)
            continue;

        send_to_char(ch, "%s", buf);
        act(buf2, 0, ch, 0, 0, TO_ROOM);
        return true;
    }

    return false;
}


SPECIAL(puff) {
    char actbuf[MAX_INPUT_LENGTH];

    if (cmd)
        return false;

    switch (rand_number(0, 60)) {
        case 0:
            do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);    /* strcpy: OK */
            return true;
        case 1:
            do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);    /* strcpy: OK */
            return true;
        case 2:
            do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);    /* strcpy: OK */
            return true;
        case 3:
            do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);    /* strcpy: OK */
            return true;
        default:
            return false;
    }
}

SPECIAL(fido) {
    struct obj_data *i, *temp, *next_obj;

    if (cmd || !AWAKE(ch))
        return false;

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!IS_CORPSE(i))
            continue;

        act("$n savagely devours a corpse.", 0, ch, 0, 0, TO_ROOM);
        for (temp = i->contains; temp; temp = temp->next_content) {
            obj_from_obj(temp);
            obj_to_room(temp, IN_ROOM(ch));
        }
        extract_obj(i);
        return true;
    }

    return false;
}

SPECIAL(janitor) {
    struct obj_data *i;

    if (cmd || !AWAKE(ch))
        return false;

    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
        if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
            continue;
        if (GET_OBJ_TYPE(i) == ITEM_DRINKCON || GET_OBJ_COST(i) >= 100)
            continue;
        act("$n picks up some trash.", 0, ch, 0, 0, TO_ROOM);
        obj_from_room(i);
        obj_to_char(i, ch);
        return true;
    }

    return false;
}

SPECIAL(cityguard) {
    struct char_data *tch, *evil, *spittle;
    int max_evil, min_cha;

    if (cmd || !AWAKE(ch) || FIGHTING(ch))
        return false;

    max_evil = 1000;
    min_cha = 6;
    spittle = evil = NULL;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
        if (!CAN_SEE(ch, tch))
            continue;

        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", 0, ch, 0, 0, TO_ROOM);
            return true;
        }

        if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
            act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", 0, ch, 0, 0, TO_ROOM);
            return true;
        }

        if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
            max_evil = GET_ALIGNMENT(tch);
            evil = tch;
        }

        if (GET_CHA(tch) < min_cha) {
            spittle = tch;
            min_cha = GET_CHA(tch);
        }
    }

    if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
        act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", 0, ch, 0, 0, TO_ROOM);
        return true;
    }

    /* Reward the socially inept. */
    if (spittle && !rand_number(0, 9)) {
        static int spit_social;

        if (!spit_social)
            spit_social = find_command("spit");

        if (spit_social > 0) {
            char spitbuf[MAX_NAME_LENGTH + 1];

            strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));    /* strncpy: OK */
            spitbuf[sizeof(spitbuf) - 1] = '\0';

            do_action(ch, spitbuf, spit_social, 0);
            return true;
        }
    }

    return false;
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops) {
    char buf[MAX_STRING_LENGTH], pet_name[256];
    room_rnum pet_room;
    struct char_data *pet;

    /* Gross. */
    pet_room = IN_ROOM(ch) + 1;

    if (CMD_IS("list")) {
        send_to_char(ch, "Available pets are:\r\n");
        for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
            /* No, you can't have the Implementor as a pet if he's in there. */
            if (!IS_NPC(pet))
                continue;
            send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
        }
        return true;
    } else if (CMD_IS("buy")) {

        two_arguments(argument, buf, pet_name);

        if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
            send_to_char(ch, "There is no such pet!\r\n");
            return true;
        }
        if (GET_GOLD(ch) < PET_PRICE(pet)) {
            send_to_char(ch, "You don't have enough zenni!\r\n");
            return true;
        }
        GET_GOLD(ch) -= PET_PRICE(pet);

        pet = read_mobile(GET_MOB_RNUM(pet), REAL);
        GET_EXP(pet) = 0;
        SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

        if (*pet_name) {
            snprintf(buf, sizeof(buf), "%s %s", pet->name, pet_name);
            /* free(pet->name); don't free the prototype! */
            pet->name = strdup(buf);

            snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
                     pet->description, pet_name);
            /* free(pet->description); don't free the prototype! */
            pet->description = strdup(buf);
        }
        char_to_room(pet, IN_ROOM(ch));
        add_follower(pet, ch);
        pet->master_id = GET_IDNUM(ch);

        /* Be certain that pets can't get/carry/use/wield/wear items */
        IS_CARRYING_W(pet) = 1000;
        IS_CARRYING_N(pet) = 100;

        send_to_char(ch, "May you enjoy your pet.\r\n");
        act("$n buys $N as a pet.", 0, ch, 0, pet, TO_ROOM);

        return true;
    }

    /* All commands except list and buy */
    return false;
}

SPECIAL(auction) {
    room_rnum auct_room;
    struct obj_data *obj, *next_obj, *obj2 = NULL;
    int found = 0;

    /* Gross. */
    auct_room = real_room(80);

    if (CMD_IS("cancel")) {

        for (obj = world[auct_room].contents; obj; obj = obj->next_content) {
            if (obj && GET_AUCTER(obj) == GET_ID(ch)) {
                obj2 = obj;
                found = 1;

                if (GET_CURBID(obj2) != -1 && GET_AUCTIME(obj2) + 518400 > time(0)) {
                    send_to_char(ch,
                                 "Unable to cancel. Someone has already bid on it and their bid license hasn't expired.\r\n");
                    time_t remain = (GET_AUCTIME(obj2) + 518400) - time(0);
                    int day = (int) ((remain % 604800) / 86400);
                    int hour = (int) ((remain % 86400) / 3600);
                    int minu = (int) ((remain % 3600) / 60);
                    send_to_char(ch, "Time Till License Expiration: %d day%s, %d hour%s, %d minute%s.\r\n", day,
                                 day > 1 ? "s" : "", hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                send_to_char(ch, "@wYou cancel the auction of %s@w and it is returned to you.@n\r\n",
                             obj2->short_description);
                struct descriptor_data *d;

                for (d = descriptor_list; d; d = d->next) {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_EQ(d->character, WEAR_EYE)) {
                        send_to_char(d->character,
                                     "@RScouter Auction News@D: @GThe auction of @w%s@G has been canceled.\r\n",
                                     obj2->short_description);
                    }
                }

                obj_from_room(obj2);
                obj_to_char(obj2, ch);
                auc_save();
            }
        }

        if (found == 0) {
            send_to_char(ch, "There are no items being auctioned by you.\r\n");
        }

        return true;
    } else if (CMD_IS("pickup")) {
        struct descriptor_data *d;
        int founded = 0;

        for (obj = world[auct_room].contents; obj; obj = obj->next_content) {
            if (obj && GET_CURBID(obj) == GET_ID(ch)) {
                obj2 = obj;
                found = 1;

                if (GET_AUCTER(obj) <= 0) {
                    continue;
                }

                if (GET_BID(obj2) > GET_GOLD(ch)) {
                    send_to_char(ch, "Unable to purchase %s, you don't have enough money on hand.\r\n",
                                 obj2->short_description);
                    continue;
                }

                if (GET_AUCTIME(obj2) + 86400 > time(0)) {
                    time_t remain = (GET_AUCTIME(obj2) + 86400) - time(0);
                    int hour = (int) ((remain % 86400) / 3600);
                    int minu = (int) ((remain % 3600) / 60);
                    send_to_char(ch,
                                 "Unable to purchase %s, minimum time to bid is 24 hours. %d hour%s and %d minute%s remain.\r\n",
                                 obj2->short_description, hour, hour > 1 ? "s" : "", minu, minu > 1 ? "s" : "");
                    continue;
                }

                GET_GOLD(ch) -= GET_BID(obj2);
                obj_from_room(obj2);
                obj_to_char(obj2, ch);
                send_to_char(ch, "You pay %s zenni and receive the item.\r\n", add_commas(GET_BID(obj2)));
                auc_save();

                for (d = descriptor_list; d; d = d->next) {
                    if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                        continue;
                    if (d->character == ch)
                        continue;
                    if (GET_IDNUM(d->character) == GET_AUCTER(obj2)) {
                        founded = 1;
                        GET_BANK_GOLD(d->character) += GET_BID(obj2);
                        if (GET_EQ(d->character, WEAR_EYE)) {
                            send_to_char(d->character,
                                         "@RScouter Auction News@D: @GSomeone has purchased your @w%s@G and you had the money put in your bank account.\r\n",
                                         obj2->short_description);
                        }
                    } else if (GET_EQ(d->character, WEAR_EYE)) {
                        send_to_char(d->character,
                                     "@RScouter Auction News@D: @GSomeone has purchased the @w%s@G that was on auction.\r\n",
                                     obj2->short_description);
                    }
                }

                if (founded == 0) {
                    struct char_data *vict = NULL;
                    int is_file = 0, player_i = 0;

                    CREATE(vict, struct char_data, 1);
                    clear_char(vict);
                    CREATE(vict->player_specials, struct player_special_data, 1);
                    char blam[50];
                    sprintf(blam, "%s", GET_AUCTERN(obj2));
                    if ((player_i = load_char(blam, vict)) > -1) {
                        is_file = 1;
                    } else {
                        free_char(vict);
                        continue;
                    }
                    GET_BANK_GOLD(vict) += GET_BID(obj2);

                    GET_PFILEPOS(vict) = player_i;
                    save_char(vict);
                    if (is_file == 1)
                        free_char(vict);
                }

            }
        }

        if (found == 0) {
            send_to_char(ch, "There are no items that you have bid on.\r\n");
        }
        return true;
    } else if (CMD_IS("auction")) {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        struct descriptor_data *d;
        int value = 0;

        two_arguments(argument, arg, arg2);

        if (!*arg || !*arg2) {
            send_to_char(ch, "Auction what item and for how much?\r\n");
            return true;
        }

        value = atoi(arg2);

        if (!(obj2 = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
            send_to_char(ch, "You don't have that item to auction.\r\n");
            return true;
        }
        if (value <= 999) {
            send_to_char(ch, "Do not auction anything for less than 1,000 zenni.\r\n");
            return true;
        }

        if (OBJ_FLAGGED(obj2, ITEM_BROKEN)) {
            act("$P is broken and we will not accept it.", 0, ch, 0, obj2, TO_CHAR);
            return true;
        }

        if (OBJ_FLAGGED(obj2, ITEM_NODONATE)) {
            act("$P is junk and we will not accept it.", 0, ch, 0, obj2, TO_CHAR);
            return true;
        }

        GET_BID(obj2) = value;
        GET_STARTBID(obj2) = 0;
        GET_AUCTER(obj2) = 0;
        if (GET_AUCTERN(obj2))
            free(GET_AUCTERN(obj2));
        GET_AUCTIME(obj2) = 0;

        GET_BID(obj2) = value;
        GET_STARTBID(obj2) = GET_BID(obj2);
        GET_AUCTER(obj2) = GET_ID(ch);
        GET_AUCTERN(obj2) = strdup(GET_NAME(ch));
        GET_AUCTIME(obj2) = time(0);
        GET_CURBID(obj2) = -1;
        obj_from_char(obj2);
        obj_to_room(obj2, auct_room);
        auc_save();
        send_to_char(ch, "You place %s on auction for %s zenni.\r\n", obj2->short_description,
                     add_commas(GET_BID(obj2)));
        log("AUCTION: %s places %s on auction for %s", GET_NAME(ch), obj2->short_description,
            add_commas(GET_BID(obj2)));

        for (d = descriptor_list; d; d = d->next) {
            if (STATE(d) != CON_PLAYING || IS_NPC(d->character))
                continue;
            if (d->character == ch)
                continue;
            if (GET_EQ(d->character, WEAR_EYE)) {
                send_to_char(d->character,
                             "@RScouter Auction News@D: @GThe item, @w%s@G, has been placed on auction for @Y%s@G zenni.@n\r\n",
                             obj2->short_description, add_commas(GET_BID(obj2)));
            }
        }
        return true;
    }

    /* All commands except list and buy */
    return false;
}

/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(healtank) {
    struct obj_data *htank = NULL, *i;
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    htank = find_obj_in_list_vnum(world[IN_ROOM(ch)].contents, 64);

    if (CMD_IS("htank")) {
        if (!htank) {
            return false;
        }

        if (!*arg) {
            send_to_char(ch, "@WHealing Tank Commands:\r\n"
                             "htank [ enter | exit | check ]@n");
            return true;
        }

        if (!strcasecmp("enter", arg)) {
            if (PLR_FLAGGED(ch, PLR_HEALT)) {
                send_to_char(ch, "You are already inside a healing tank!\r\n");
                return true;
            }
            if (ch->master && ch->master != ch) {
                send_to_char(ch, "You can't enter it while following someone!\r\n");
                return true;
            } else if (IS_ANDROID(ch)) {
                send_to_char(ch, "A healing tank will have no effect on you.\r\n");
                return true;
            } else if (HCHARGE(htank) <= 0) {
                send_to_char(ch, "That healing tank needs to recharge, wait a while.\r\n");
                return true;
            } else if (OBJ_FLAGGED(htank, ITEM_BROKEN)) {
                send_to_char(ch,
                             "It is broken! You will need to fix it yourself or wait for someone else to fix it.\r\n");
                return true;
            } else if (SITS(ch)) {
                send_to_char(ch, "You are already on something.\r\n");
                return true;
            } else if (SITTING(htank)) {
                send_to_char(ch, "Someone else is already inside that healing tank!\r\n");
                return true;
            } else {
                GET_CHARGE(ch) = 0;
                REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CHARGE);
                GET_CHARGETO(ch) = 0;
                GET_BARRIER(ch) = 0;
                act("@wYou step inside the healing tank and put on its breathing mask. A water like solution pours over your body until the tank is full.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@C$n@w steps inside the healing tank and puts on its breathing mask. A water like solution pours over $s body until the tank is full.@n",
                    1, ch, 0, 0, TO_ROOM);
                SET_BIT_AR(PLR_FLAGS(ch), PLR_HEALT);
                SITS(ch) = htank;
                SITTING(htank) = ch;
                return true;
            }

        } // End of Enter argument

        else if (!strcasecmp("exit", arg)) {
            if (!PLR_FLAGGED(ch, PLR_HEALT)) {
                send_to_char(ch, "You are not inside a healing tank.\r\n");
                return true;
            } else {
                act("@wThe healing tank drains and you exit it shortly after.", 1, ch, 0, 0, TO_CHAR);
                act("@C$n@w exits the healing tank after letting it drain.@n", 1, ch, 0, 0, TO_ROOM);
                REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_HEALT);
                SITTING(htank) = NULL;
                SITS(ch) = NULL;
                return true;
            }
        } // End of Exit argument

        else if (!strcasecmp("check", arg)) {
            if (HCHARGE(htank) < 20 && HCHARGE(htank) > 0) {
                send_to_char(ch, "The healing tank has %d bars of energy displayed on its meter.\r\n", HCHARGE(htank));
            } else if (HCHARGE(htank) <= 0) {
                send_to_char(ch, "The healing tank has no energy displayed on its meter.\r\n");
            } else {
                send_to_char(ch, "The healing tank has full energy shown on its meter.\r\n");
            }
            return true;
        } else {
            send_to_char(ch, "@WHealing Tank Commands:\r\n"
                             "htank [ enter | exit | check ]@n");
            return true;
        }

    }// End of htank command
    else {
        return false;
    }
}

/* This controls stat augmenter functions */
SPECIAL(augmenter) {
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (CMD_IS("augment")) {
        int strength = ch->real_abils.str;
        int intel = ch->real_abils.intel;
        int wisdom = ch->real_abils.wis;
        int speed = ch->real_abils.cha;
        int consti = ch->real_abils.con;
        int agility = ch->real_abils.dex;

        int strcost = strength * 1200;
        int intcost = intel * 1200;
        int concost = consti * 1200;
        int wiscost = wisdom * 1200;
        int agicost = agility * 1200;
        int specost = speed * 1200;

        if (!*arg) {
            send_to_char(ch, "@D                        -----@WBody Augmentations@D-----@n\r\n");
            send_to_char(ch, "@RStrength    @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n",
                         strength, add_commas(strcost));
            send_to_char(ch, "@BIntelligence@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", intel,
                         add_commas(intcost));
            send_to_char(ch, "@CWisdom      @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", wisdom,
                         add_commas(wiscost));
            send_to_char(ch, "@GConstitution@y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", consti,
                         add_commas(concost));
            send_to_char(ch, "@mAgility     @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", agility,
                         add_commas(agicost));
            send_to_char(ch, "@YSpeed       @y: @WCurrently measured at @w%d@W, cost to augment @Y%s@W.@n\r\n", speed,
                         add_commas(specost));
            send_to_char(ch, "\r\n");
            return true;
        } else if (!strcasecmp("strength", arg) || !strcasecmp("str", arg)) {
            if (strength >= 100)
                send_to_char(ch, "Your strength is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < strcost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.str += 1;
                GET_GOLD(ch) -= strcost;
                save_char(ch);
            }
        } else if (!strcasecmp("intelligence", arg) || !strcasecmp("int", arg)) {
            if (intel >= 100)
                send_to_char(ch, "Your intelligence is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < intcost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.intel += 1;
                GET_GOLD(ch) -= intcost;
                save_char(ch);
            }
        } else if (!strcasecmp("constitution", arg) || !strcasecmp("con", arg)) {
            if (consti >= 100)
                send_to_char(ch, "Your constitution is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < concost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.con += 1;
                GET_GOLD(ch) -= concost;
                save_char(ch);
            }
        } else if (!strcasecmp("speed", arg) || !strcasecmp("spe", arg)) {
            if (speed >= 100)
                send_to_char(ch, "Your speed is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < specost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.cha += 1;
                GET_GOLD(ch) -= specost;
                save_char(ch);
            }
        } else if (!strcasecmp("agility", arg) || !strcasecmp("agi", arg)) {
            if (agility >= 100)
                send_to_char(ch, "Your agility is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < agicost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.dex += 1;
                GET_GOLD(ch) -= agicost;
                save_char(ch);
            }
        } else if (!strcasecmp("wisdom", arg) || !strcasecmp("wis", arg)) {
            if (wisdom >= 100)
                send_to_char(ch, "Your wisdom how somehow been measured is already as high as it can possibly go.\r\n");
            else if (GET_GOLD(ch) < wiscost)
                send_to_char(ch, "You can not afford the price!\r\n");
            else { /* They can augment it! */
                act("@WThe machine's arm moves out and quickly augments your body with microscopic attachments.@n",
                    1, ch, 0, 0, TO_CHAR);
                act("@WThe Augmenter 9001 moves its arm over to @C$n@W and quickly operates on $s body.@n", 1, ch, 0,
                    0, TO_ROOM);
                ch->real_abils.wis += 1;
                GET_GOLD(ch) -= wiscost;
                save_char(ch);
            }
        } else {
            send_to_char(ch, "Syntax: augment [str | con | int | wis | agi | spe]\r\n");
        }
        return true;
    } else { /* They are not using the right command, ignore them. */
        return false;
    }

}

/* This controls gravity generator functions */
SPECIAL(gravity) {
    struct obj_data *i, *obj = NULL;
    char arg[MAX_INPUT_LENGTH];
    int match = 0;

    one_argument(argument, arg);
    obj = find_obj_in_list_vnum(world[IN_ROOM(ch)].contents, 11);

    if (CMD_IS("gravity") || CMD_IS("generator")) {
        if (!*arg) {
            send_to_char(ch, "@WGravity Commands:@n\r\n");
            send_to_char(ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                             "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
            return true;
        }
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "It's broken!\r\n");
            return true;
        }
        if ((!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) && GET_OBJ_WEIGHT(obj) == 0) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("N", arg) || !strcasecmp("n", arg) || !strcasecmp("0", arg)) {
            send_to_char(ch,
                         "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITYX10)) {
                ROOM_GRAVITY(IN_ROOM(ch)) = 10;
                GET_OBJ_WEIGHT(obj) = 0;
            } else {
                ROOM_GRAVITY(IN_ROOM(ch)) = 0;
                GET_OBJ_WEIGHT(obj) = 0;
            }
            match = 1;
        }
        if (!strcasecmp("10", arg) && GET_OBJ_WEIGHT(obj) == 10) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("10", arg) && ROOM_GRAVITY(IN_ROOM(ch)) == 10 &&
                   (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITYX10))) {
            send_to_char(ch, "The gravity around you is already at that.\r\n");
            return true;
        } else if (!strcasecmp("10", arg) && ROOM_GRAVITY(IN_ROOM(ch)) != 10 &&
                   (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_GRAVITYX10))) {
            send_to_char(ch,
                         "You punch in normal gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 10;
            GET_OBJ_WEIGHT(obj) = 0;
            match = 1;
        } else if (!strcasecmp("10", arg)) {
            send_to_char(ch,
                         "You punch in ten times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            ROOM_GRAVITY(IN_ROOM(ch)) = 10;
            GET_OBJ_WEIGHT(obj) = 10;
            match = 1;
        }
        if (!strcasecmp("20", arg) && GET_OBJ_WEIGHT(obj) == 20) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("20", arg)) {
            send_to_char(ch,
                         "You punch in twenty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 20;
            GET_OBJ_WEIGHT(obj) = 20;
            match = 1;
        }
        if (!strcasecmp("30", arg) && GET_OBJ_WEIGHT(obj) == 30) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("30", arg)) {
            send_to_char(ch,
                         "You punch in thirty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 30;
            GET_OBJ_WEIGHT(obj) = 30;
            match = 1;
        }
        if (!strcasecmp("40", arg) && GET_OBJ_WEIGHT(obj) == 40) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("40", arg)) {
            send_to_char(ch,
                         "You punch in fourty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 40;
            GET_OBJ_WEIGHT(obj) = 40;
            match = 1;
        }
        if (!strcasecmp("50", arg) && GET_OBJ_WEIGHT(obj) == 50) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("50", arg)) {
            send_to_char(ch,
                         "You punch in fifty times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 50;
            GET_OBJ_WEIGHT(obj) = 50;
            match = 1;
        }
        if (!strcasecmp("100", arg) && GET_OBJ_WEIGHT(obj) == 100) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("100", arg)) {
            send_to_char(ch,
                         "You punch in one hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 100;
            GET_OBJ_WEIGHT(obj) = 100;
            match = 1;
        }
        if (!strcasecmp("200", arg) && GET_OBJ_WEIGHT(obj) == 200) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("200", arg)) {
            send_to_char(ch,
                         "You punch in two hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 200;
            GET_OBJ_WEIGHT(obj) = 200;
            match = 1;
        }
        if (!strcasecmp("300", arg) && GET_OBJ_WEIGHT(obj) == 300) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("300", arg)) {
            send_to_char(ch,
                         "You punch in three hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 300;
            GET_OBJ_WEIGHT(obj) = 300;
            match = 1;
        }
        if (!strcasecmp("400", arg) && GET_OBJ_WEIGHT(obj) == 400) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("400", arg)) {
            send_to_char(ch,
                         "You punch in four hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 400;
            GET_OBJ_WEIGHT(obj) = 400;
            match = 1;
        }
        if (!strcasecmp("500", arg) && GET_OBJ_WEIGHT(obj) == 500) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("500", arg)) {
            send_to_char(ch,
                         "You punch in five hundred times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 500;
            GET_OBJ_WEIGHT(obj) = 500;
            match = 1;
        }
        if (!strcasecmp("1000", arg) && GET_OBJ_WEIGHT(obj) == 1000) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("1000", arg)) {
            send_to_char(ch,
                         "You punch in one thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 1000;
            GET_OBJ_WEIGHT(obj) = 1000;
            match = 1;
        }
        if (!strcasecmp("5000", arg) && GET_OBJ_WEIGHT(obj) == 5000) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("5000", arg)) {
            send_to_char(ch,
                         "You punch in five thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 5000;
            GET_OBJ_WEIGHT(obj) = 5000;
            match = 1;
        }
        if (!strcasecmp("10000", arg) && GET_OBJ_WEIGHT(obj) == 10000) {
            send_to_char(ch, "The gravity generator is already set to that.\r\n");
            return true;
        } else if (!strcasecmp("10000", arg)) {
            send_to_char(ch,
                         "You punch in ten thousand times gravity on the generator. It hums for a moment\r\nbefore you feel the pressure on your body change.\r\n");
            act("@W$n@w pushes some buttons on the gravity generator, and you feel a change in pressure on your body.@n",
                1, ch, 0, 0, TO_ROOM);
            if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
                REMOVE_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
                send_to_room(IN_ROOM(ch), "The increased gravity forces the aura to disappear.\r\n");
            }
            ROOM_GRAVITY(IN_ROOM(ch)) = 10000;
            GET_OBJ_WEIGHT(obj) = 10000;
            match = 1;
        } else if (match == 0) {
            send_to_char(ch, "That is not a proper command for this device.\r\n");
            send_to_char(ch, "@WGravity Commands:@n\r\n");
            send_to_char(ch, "@Wgravity [ 0 | N | 10 | 20 | 30 | 40 | 50 | 100 | 200 ]\r\n"
                             "          [  300 | 400 | 500 | 1,000 | 5,000 | 10,000  ]@n\r\n");
            return true;
        }
        return true;
    } else {
        return false;
    }
}

SPECIAL(bank) {
    int amount, num = 0;

    struct obj_data *obj = find_obj_in_list_vnum(world[IN_ROOM(ch)].contents, 3034);

    if (CMD_IS("balance")) {
        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return true;
        }

        if (GET_BANK_GOLD(ch) > 0)
            send_to_char(ch, "Your current balance is %d zenni.\r\n", GET_BANK_GOLD(ch));
        else
            send_to_char(ch, "You currently have no money deposited.\r\n");
        return true;
    } else if (CMD_IS("wire")) {
        char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
        struct char_data *vict = NULL;

        two_arguments(argument, arg, arg2);

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return true;
        }

        if ((amount = atoi(arg)) <= 0) {
            send_to_char(ch, "How much do you want to transfer?\r\n");
            return true;
        }
        if (GET_BANK_GOLD(ch) < amount + (amount / 100)) {
            send_to_char(ch, "You don't have that much zenni in the bank (plus 1%s charge)!\r\n", "%");
            return true;
        }
        if (!*arg2) {
            send_to_char(ch, "You want to transfer it to who?!\r\n");
            return true;
        }
        if (!(vict = get_player_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
            int is_file = 0, player_i = 0;
            char name[MAX_INPUT_LENGTH];

            CREATE(vict, struct char_data, 1);
            clear_char(vict);
            CREATE(vict->player_specials, struct player_special_data, 1);

            sprintf(name, "%s", rIntro(ch, arg2));

            if ((player_i = load_char(name, vict)) > -1) {
                is_file = 1;
            } else {
                free_char(vict);
                send_to_char(ch, "That person doesn't exist.\r\n");
                return true;
            }
            if (ch->desc->user == NULL) {
                send_to_char(ch, "There is an error. Report to Iovan.");
                return true;
            }
            if (!strcasecmp(GET_NAME(vict), ch->desc->tmp1) || !strcasecmp(GET_NAME(vict), ch->desc->tmp2) ||
                !strcasecmp(GET_NAME(vict), ch->desc->tmp3) || !strcasecmp(GET_NAME(vict), ch->desc->tmp4) ||
                !strcasecmp(GET_NAME(vict), ch->desc->tmp5)) {
                send_to_char(ch, "You can not transfer money to your own offline characters...");
                if (is_file == 1)
                    free_char(vict);
                return true;
            }
            GET_BANK_GOLD(vict) += amount;
            GET_BANK_GOLD(ch) -= amount + (amount / 100);
            GET_PFILEPOS(vict) = player_i;
            mudlog(NRM, MAX(ADMLVL_IMPL, GET_INVIS_LEV(ch)), 1, "EXCHANGE: %s gave %s zenni to user %s",
                   GET_NAME(ch), add_commas(amount), GET_NAME(vict));
            save_char(vict);
            if (is_file == 1)
                free_char(vict);
        } else {
            GET_BANK_GOLD(vict) += amount;
            GET_BANK_GOLD(ch) -= amount + (amount / 100);
            send_to_char(vict, "@WYou have just had @Y%s@W zenni wired into your bank account.@n\r\n",
                         add_commas(amount));
        }
        send_to_char(ch, "You transfer %s zenni to them.\r\n", add_commas(amount));
        act("$n makes a bank transaction.", 1, ch, 0, 0, TO_ROOM);
        return true;
    } else if (CMD_IS("deposit")) {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return true;
        }

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to deposit?\r\n");
            return true;
        }
        if (GET_GOLD(ch) < amount) {
            send_to_char(ch, "You don't have that much zenni!\r\n");
            return true;
        }
        GET_GOLD(ch) -= amount;
        GET_BANK_GOLD(ch) += amount;
        send_to_char(ch, "You deposit %d zenni.\r\n", amount);
        act("$n makes a bank transaction.", 1, ch, 0, 0, TO_ROOM);
        return true;
    } else if (CMD_IS("withdraw")) {

        if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
            send_to_char(ch, "The ATM is broken!\r\n");
            return true;
        }

        if ((amount = atoi(argument)) <= 0) {
            send_to_char(ch, "How much do you want to withdraw?\r\n");
            return true;
        }
        if (GET_BANK_GOLD(ch) < amount) {
            send_to_char(ch, "You don't have that much zenni!\r\n");
            return true;
        }
        if (GET_BANK_GOLD(ch) - (amount + (1 + amount / 100)) < 0) {
            if (amount >= 100) {
                amount = amount + (amount / 100);
            } else if (amount < 100) {
                amount = amount + 1;
            }
            send_to_char(ch, "You need at least %s in the bank with the 1 percent withdraw fee.\r\n",
                         add_commas(amount));
            return true;
        }
        if (GET_GOLD(ch) + amount > GOLD_CARRY(ch)) {
            send_to_char(ch, "You can only carry %s zenni, you left the rest.\r\n", add_commas(GOLD_CARRY(ch)));
            int diff = (GET_GOLD(ch) + amount) - GOLD_CARRY(ch);
            GET_GOLD(ch) = GOLD_CARRY(ch);
            amount -= diff;
            if (amount >= 100) {
                num = amount / 100;
                GET_BANK_GOLD(ch) -= amount + num;
            } else if (amount < 100) {
                GET_BANK_GOLD(ch) -= amount + 1;
            }
            send_to_char(ch, "You withdraw %s zenni,  and pay %s in withdraw fees.\r\n.\r\n", add_commas(amount),
                         add_commas(num));
            act("$n makes a bank transaction.", 1, ch, 0, 0, TO_ROOM);
            return true;
        }
        GET_GOLD(ch) += amount;
        if (amount >= 100) {
            num = amount / 100;
            GET_BANK_GOLD(ch) -= amount + num;
        } else if (amount < 100) {
            GET_BANK_GOLD(ch) -= amount + 1;
        }
        send_to_char(ch, "You withdraw %s zenni, and pay %s in withdraw fees.\r\n", add_commas(amount),
                     add_commas(num));
        act("$n makes a bank transaction.", 1, ch, 0, 0, TO_ROOM);
        return true;
    } else
        return false;
}

SPECIAL(cleric_marduk) {
    int tmp, num_used = 0;
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 12;

    tmp = rand_number(1, 10);

    if ((tmp == 7) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);
        if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
            cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
            return true;
        }
        if ((tmp == 2) && ((GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
            return true;
        }
        if ((tmp == 3) && (GET_LEVEL(ch) > 4)) {
            cast_spell(ch, vict, NULL, SPELL_BESTOW_CURSE, NULL);
            return true;
        }
        if ((tmp == 4) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
            return true;
        }
        if ((tmp == 5) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
            cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
            return true;
        }
        if ((tmp == 6) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
            cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
            return true;
        }
        if (tmp == 7) {
            cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
            return true;
        }
        if ((tmp == 8) && (GET_LEVEL(ch) > 6) && (!IS_UNDEAD(vict))) {
            cast_spell(ch, vict, NULL, SPELL_POISON, NULL);
            return true;
        }
        if (tmp == 9 && GET_LEVEL(ch) > 8) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
            return true;
        }
        if ((tmp == 10) && (GET_LEVEL(ch) > 10)) {
            cast_spell(ch, vict, NULL, SPELL_HARM, NULL);
            return true;
        }
        if (tmp == 11) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
            return true;
        }
        if (tmp == 12 && GET_LEVEL(ch) > 8) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
            return true;
        }
    }
    return false;
}

SPECIAL(cleric_ao) {
    int tmp, num_used = 0;
    struct char_data *vict;
    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 8;

    tmp = rand_number(1, 10);

    if ((tmp == 7) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);
        if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
            cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
            return true;
        }
        if ((tmp == 2) && ((GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
            return true;
        }
        if ((tmp == 3) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
            return true;
        }
        if ((tmp == 4) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
            cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
            return true;
        }
        if ((tmp == 5) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
            cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
            return true;
        }
        if (tmp == 6) {
            cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
            return true;
        }
        if (tmp == 7 && GET_LEVEL(ch) > 8) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
            return true;
        }
        if (tmp == 8 && GET_LEVEL(ch) > 10) {
            cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
            return true;
        }
        if (tmp == 9) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
            return true;
        }
        if (tmp == 10 && GET_LEVEL(ch) > 8) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
            return true;
        }
    }
    return false;
}

SPECIAL(dziak) {
    int tmp, num_used = 0;
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;
    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 9;

    tmp = rand_number(3, 10);

    if ((tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);

        if (tmp == 2 || tmp == 1) {
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
            return true;
        }
        if (tmp == 3) {
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
            return true;
        }
        if (tmp == 4) {
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
            return true;
        }
        if (tmp == 5) {
            cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
            return true;
        }
        if (tmp == 6) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
            return true;
        }
        if (tmp == 7) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
            return true;
        }
        if ((tmp == 8) && (IS_GOOD(vict))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
            return true;
        }
        if (tmp == 9) {
            cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
            return true;
        }
    }
    return false;
}

SPECIAL(azimer) {
    int tmp, num_used = 0;
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 8;

    tmp = rand_number(3, 10);

    if ((tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);

        if (tmp == 2 || tmp == 1) {
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
            return true;
        }
        if (tmp == 3) {
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
            return true;
        }
        if (tmp == 4) {
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
            return true;
        }
        if (tmp == 5) {
            cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
            return true;
        }
        if (tmp == 6) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
            return true;
        }
        if (tmp == 7) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
            return true;
        }
        if ((tmp == 8) && (IS_GOOD(vict))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
            return true;
        }
    }
    return false;
}

SPECIAL(lyrzaxyn) {
    int tmp, num_used = 0;
    struct char_data *vict;

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 8;

    tmp = rand_number(3, 10);

    if ((tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);

        if (tmp == 2 || tmp == 1) {
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
            return true;
        }
        if (tmp == 3) {
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
            return true;
        }
        if (tmp == 4) {
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
            return true;
        }
        if (tmp == 5) {
            cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
            return true;
        }
        if (tmp == 6) {
            cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
            return true;
        }
        if (tmp == 7) {
            cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
            return true;
        }
        if ((tmp == 8) && (IS_GOOD(vict))) {
            cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
            return true;
        }
    }
    return false;
}

SPECIAL(magic_user) {
    int tmp, num_used = 0;
    struct char_data *vict;

    if (IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_KABITO) {
        if (!affected_by_spell(ch, SPELL_MAGE_ARMOR)) {
            cast_spell(ch, ch, NULL, SPELL_MAGE_ARMOR, NULL);
            return true;
        }
    }

    if (cmd || GET_POS(ch) != POS_FIGHTING)
        return false;

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
        if (FIGHTING(vict) == ch && !rand_number(0, 4))
            break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
        vict = FIGHTING(ch);

    num_used = 6;

    tmp = rand_number(2, 10);

    if ((tmp == 8) || (tmp == 9) || (tmp == 10)) {
        tmp = rand_number(1, num_used);

        if ((tmp == 1) && GET_LEVEL(ch) > 1) {
            cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
            return true;
        }
        if ((tmp == 2) && !affected_by_spell(ch, SPELL_MAGE_ARMOR)) {
            cast_spell(ch, ch, NULL, SPELL_MAGE_ARMOR, NULL);
            return true;
        }
        if ((tmp == 3) && GET_LEVEL(ch) > 1) {
            cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
            return true;
        }
        if ((tmp == 4) && GET_LEVEL(ch) > 1) {
            cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
            return true;
        }
        if ((tmp == 5) && GET_LEVEL(ch) > 5) {
            cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
            return true;
        }
        if ((tmp == 6) && GET_LEVEL(ch) > 9) {
            cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
            return true;
        }
    }
    return false;
}
