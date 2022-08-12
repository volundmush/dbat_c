/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "random.h"
#include "spells.h"
#include "db.h"
#include "config.h"
#include "fight.h"
#include "races.h"
#include "class.h"
#include "feats.h"
#include "genzon.h"
#include "constants.h"
#include "act.informative.h"

/* local functions */
char commastring[MAX_STRING_LENGTH];


void dispel_ash(struct char_data *ch)
{
 struct obj_data *obj, *next_obj, *ash = NULL;
 int there = FALSE;

 for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
     next_obj = obj->next_content;
  if (GET_OBJ_VNUM(obj) == 1306) {
   there = TRUE;
   ash = obj;
  }
 }

 if (ash) {
  int roll = axion_dice(0);
  if (GET_OBJ_COST(ash) == 3) {
   if (GET_INT(ch) > roll) {
    act("@GYou clear the air with the shockwaves of your power!@n", TRUE, ch, ash, 0, TO_CHAR);
    act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE, ch, ash, 0, TO_ROOM);
    extract_obj(ash);
   }
  } else if (GET_OBJ_COST(ash) == 2) {
   if (GET_INT(ch) + 10 > roll) {
    act("@GYou clear the air with the shockwaves of your power!@n", TRUE, ch, ash, 0, TO_CHAR);
    act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE, ch, ash, 0, TO_ROOM);
    extract_obj(ash);
   }
  } else if (GET_OBJ_COST(ash) == 1) {
   if (GET_INT(ch) + 20 > roll) {
    act("@GYou clear the air with the shockwaves of your power!@n", TRUE, ch, ash, 0, TO_CHAR);
    act("@C$n@G clears the air with the shockwaves of $s power!@n", TRUE, ch, ash, 0, TO_ROOM);
    extract_obj(ash);
   }
  }
 }

}

int has_group(struct char_data *ch)
{

  struct follow_type *k, *next;

 if (!AFF_FLAGGED(ch, AFF_GROUP))
  return (FALSE);

 if (ch->followers) {
  for (k = ch->followers; k; k = next) {
   next = k->next;
   if (!AFF_FLAGGED(k->follower, AFF_GROUP)) {
    continue;
   } else {
    return (TRUE);
   }
  }
 } else if (ch->master) {
  if (!AFF_FLAGGED(ch->master, AFF_GROUP))
   return (FALSE);
  else
   return (TRUE);
 }

 return (FALSE);
}

const char *report_party_health(struct char_data *ch)
{

  if (!AFF_FLAGGED(ch, AFF_GROUP))
   return ("");

  if (!ch->followers && !ch->master)
   return ("");

  struct follow_type *k, *next;
  int count = 0, stam1 = 8, stam2 = 8, stam3 = 8, stam4 = 8, plc1 = 4, plc2 = 4, plc3 = 4, plc4 = 4;
  struct char_data *party1 = NULL, *party2 = NULL, *party3 = NULL, *party4 = NULL;
  int64_t plperc1 = 0, plperc2 = 0, plperc3 = 0, plperc4 = 0;
  int64_t kiperc1 = 0, kiperc2 = 0, kiperc3 = 0, kiperc4 = 0;
  char result_party_health[MAX_STRING_LENGTH], result1[MAX_STRING_LENGTH], result2[MAX_STRING_LENGTH], result3[MAX_STRING_LENGTH], result4[MAX_STRING_LENGTH], result5[MAX_STRING_LENGTH];

  const char *plcol[5] = {"@r",
                          "@y",
                          "@Y",
                          "@G",
                          ""
                         };

  const char *exhaust[9] = {"Exhausted", /* 0/7 */
                            "Strained", /* 1/7 */
                            "Very Tired", /* 2/7 */
                            "Tired", /* 3/7 */
                            "Kinda Tired", /* 4/7 */
                            "Very Winded", /* 5/7 */
                            "Winded", /* 6/7 */
                            "Energetic",  /* 7/7 */
                            "?????????"
                           };

  const char *excol[9] = {"@r", /* 0/7 */
                          "@R", /* 1/7 */
                          "@R", /* 2/7 */
                          "@M", /* 3/7 */
                          "@M", /* 4/7 */
                          "@M", /* 5/7 */
                          "@G", /* 6/7 */
                          "@g",  /* 7/7 */
                          "@w"
                         };

 if (ch->followers) {
  for (k = ch->followers; k; k = next) {
   next = k->next;
   if (!AFF_FLAGGED(k->follower, AFF_GROUP))
    continue;
   if (k->follower != ch) {
    count += 1;
    if (count == 1) {
     party1 = k->follower;
     plperc1 = (GET_HIT(party1) * 100) / GET_MAX_HIT(party1);
     kiperc1 = (GET_CHARGE(party1) * 100) / GET_MAX_MANA(party1);

     if (plperc1 >= 80)
      plc1 = 3;
     else if (plperc1 >= 50)
      plc1 = 2;
     else if (plperc1 >= 30)
      plc1 = 1;
     else
      plc1 = 0;

     if (GET_MOVE(party1) >= GET_MAX_MOVE(party1)) {
      stam1 = 7;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .9) {
      stam1 = 6;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .8) {
      stam1 = 5;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .7) {
      stam1 = 4;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .5) {
      stam1 = 3;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .4) {
      stam1 = 2;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .2) {
      stam1 = 1;
     } else {
      stam1 = 0;
     }
    } else if (count == 2) {
     party2 = k->follower;
     plperc2 = (GET_HIT(party2) * 100) / GET_MAX_HIT(party2);
     kiperc2 = (GET_CHARGE(party2) * 100) / GET_MAX_MANA(party2);

     if (plperc2 >= 80)
      plc2 = 3;
     else if (plperc2 >= 50)
      plc2 = 2;
     else if (plperc2 >= 30)
      plc2 = 1;
     else
      plc2 = 0;

     if (GET_MOVE(party2) >= GET_MAX_MOVE(party2)) {
      stam2 = 7;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .9) {
      stam2 = 6;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .8) {
      stam2 = 5;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .7) {
      stam2 = 4;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .5) {
      stam2 = 3;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .4) {
      stam2 = 2;
     } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .2) {
      stam2 = 1;
     } else {
      stam2 = 0;
     }
    } else if (count == 3) {
     party3 = k->follower;
     plperc3 = (GET_HIT(party3) * 100) / GET_MAX_HIT(party3);
     kiperc3 = (GET_CHARGE(party3) * 100) / GET_MAX_MANA(party3);

     if (plperc3 >= 80)
      plc3 = 3;
     else if (plperc3 >= 50)
      plc3 = 2;
     else if (plperc3 >= 30)
      plc3 = 1;
     else
      plc3 = 0;

     if (GET_MOVE(party3) >= GET_MAX_MOVE(party3)) {
      stam3 = 7;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .9) {
      stam3 = 6;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .8) {
      stam3 = 5;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .7) {
      stam3 = 4;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .5) {
      stam3 = 3;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .4) {
      stam3 = 2;
     } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .2) {
      stam3 = 1;
     } else {
      stam3 = 0;
     }
    } else if (count == 4) {
     party4 = k->follower;
     plperc4 = (GET_HIT(party4) * 100) / GET_MAX_HIT(party4);
     kiperc4 = (GET_CHARGE(party4) * 100) / GET_MAX_MANA(party4);

     if (plperc4 >= 80)
      plc4 = 3;
     else if (plperc4 >= 50)
      plc4 = 2;
     else if (plperc4 >= 30)
      plc4 = 1;
     else
      plc4 = 0;

     if (GET_MOVE(party4) >= GET_MAX_MOVE(party4)) {
      stam4 = 7;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .9) {
      stam4 = 6;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .8) {
      stam4 = 5;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .7) {
      stam4 = 4;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .5) {
      stam4 = 3;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .4) {
      stam4 = 2;
     } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .2) {
      stam4 = 1;
     } else {
      stam4 = 0;
     }
    }
   }
  }
  sprintf(result1, "@D[@BG@D]-------@mF@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D] <@RV@Y%s@R>@n\n", add_commas(GET_GROUPKILLS(ch)));
  sprintf(result2, "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty", party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty", party4 ? get_i_name(ch, party4) : "Empty");
  sprintf(result3, "@D[@BO@D]@RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s@D[@BO@D]@n\n", plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4, "%");
  sprintf(result4, "@D[@BU@D]@cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s@D[@BU@D]@n\n", kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
  sprintf(result5, "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s@D[@BP@D]@n", excol[stam1], exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4], exhaust[stam4]);
  sprintf(result_party_health, "%s%s%s%s%s\n", result1, result2, result3, result4, result5);
  ch->temp_prompt = strdup(result_party_health);
  return (ch->temp_prompt);
 } else if (ch->master && AFF_FLAGGED(ch->master, AFF_GROUP)) {
     party1 = ch->master;
     plperc1 = (GET_HIT(party1) * 100) / GET_MAX_HIT(party1);
     kiperc1 = (GET_CHARGE(party1) * 100) / GET_MAX_MANA(party1);

     if (plperc1 >= 80)
      plc1 = 3;
     else if (plperc1 >= 50)
      plc1 = 2;
     else if (plperc1 >= 30)
      plc1 = 1;
     else
      plc1 = 0;

     if (GET_MOVE(party1) >= GET_MAX_MOVE(party1)) {
      stam1 = 7;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .9) {
      stam1 = 6;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .8) {
      stam1 = 5;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .7) {
      stam1 = 4;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .5) {
      stam1 = 3;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .4) {
      stam1 = 2;
     } else if (GET_MOVE(party1) >= GET_MAX_MOVE(party1) * .2) {
      stam1 = 1;
     } else {
      stam1 = 0;
     }
   count = 1;

   for (k = party1->followers; k; k = next) {
    next = k->next;
    if (!AFF_FLAGGED(k->follower, AFF_GROUP))
     continue;
    if (k->follower != ch) {
     count += 1;
     if (count == 2) {
      party2 = k->follower;
      plperc2 = (GET_HIT(party2) * 100) / GET_MAX_HIT(party2);
      kiperc2 = (GET_CHARGE(party2) * 100) / GET_MAX_MANA(party2);

      if (plperc2 >= 80)
       plc2 = 3;
      else if (plperc2 >= 50)
       plc2 = 2;
      else if (plperc2 >= 30)
       plc2 = 1;
      else
       plc2 = 0;

      if (GET_MOVE(party2) >= GET_MAX_MOVE(party2)) {
       stam2 = 7;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .9) {
       stam2 = 6;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .8) {
       stam2 = 5;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .7) {
       stam2 = 4;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .5) {
       stam2 = 3;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .4) {
       stam2 = 2;
      } else if (GET_MOVE(party2) >= GET_MAX_MOVE(party2) * .2) {
       stam2 = 1;
      } else {
       stam2 = 0;
      }
     } else if (count == 3) {
      party3 = k->follower;
      plperc3 = (GET_HIT(party3) * 100) / GET_MAX_HIT(party3);
      kiperc3 = (GET_CHARGE(party3) * 100) / GET_MAX_MANA(party3);

      if (plperc3 >= 80)
       plc3 = 3;
      else if (plperc3 >= 50)
       plc3 = 2;
      else if (plperc3 >= 30)
       plc3 = 1;
      else
       plc3 = 0;

      if (GET_MOVE(party3) >= GET_MAX_MOVE(party3)) {
       stam3 = 7;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .9) {
       stam3 = 6;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .8) {
       stam3 = 5;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .7) {
       stam3 = 4;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .5) {
       stam3 = 3;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .4) {
       stam3 = 2;
      } else if (GET_MOVE(party3) >= GET_MAX_MOVE(party3) * .2) {
       stam3 = 1;
      } else {
       stam3 = 0;
      }
     } else if (count == 4) {
      party4 = k->follower;
      plperc4 = (GET_HIT(party4) * 100) / GET_MAX_HIT(party4);
      kiperc4 = (GET_CHARGE(party4) * 100) / GET_MAX_MANA(party4);

      if (plperc4 >= 80)
       plc4 = 3;
      else if (plperc4 >= 50)
       plc4 = 2;
      else if (plperc4 >= 30)
       plc4 = 1;
      else
       plc4 = 0;

      if (GET_MOVE(party4) >= GET_MAX_MOVE(party4)) {
       stam4 = 7;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .9) {
       stam4 = 6;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .8) {
       stam4 = 5;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .7) {
       stam4 = 4;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .5) {
       stam4 = 3;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .4) {
       stam4 = 2;
      } else if (GET_MOVE(party4) >= GET_MAX_MOVE(party4) * .2) {
       stam4 = 1;
      } else {
       stam4 = 0;
      }
     }
    } /* Is follower */
   } /* End for */

  sprintf(result1, "@D[@BG@D]-------@YL@D------- -------@mF@D------- -------@mF@D------- -------@mF@D-------[@BG@D]@n\n");
  sprintf(result2, "@D[@BR@D]@C%-15s %-15s %-15s %-15s@D[@BR@D]@n\n", party1 ? get_i_name(ch, party1) : "Empty", party2 ? get_i_name(ch, party2) : "Empty", party3 ? get_i_name(ch, party3) : "Empty", party4 ? get_i_name(ch, party4) : "Empty");
  sprintf(result3, "@D[@BO@D]@RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s @RPL@D:%s%11"I64T"@w%s@D[@BO@D]@n\n", plcol[plc1], plperc1, "%", plcol[plc2], plperc2, "%", plcol[plc3], plperc3, "%", plcol[plc4], plperc4, "%");
  sprintf(result4, "@D[@BU@D]@cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s @cCharge@D:@B%7"I64T"@w%s@D[@BU@D]@n\n", kiperc1, "%", kiperc2, "%", kiperc3, "%", kiperc4, "%");
  sprintf(result5, "@D[@BP@D]@gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s @gSt@D:%s%12s@D[@BP@D]@n", excol[stam1], exhaust[stam1], excol[stam2], exhaust[stam2], excol[stam3], exhaust[stam3], excol[stam4], exhaust[stam4]);
  sprintf(result_party_health, "%s%s%s%s%s\n", result1, result2, result3, result4, result5);
  ch->temp_prompt = strdup(result_party_health);
  return (ch->temp_prompt);
 } else {
  return ("");
 }

}


/* Check to see if anything interferes with their "knowing" the skill */
int know_skill(struct char_data *ch, int skill)
{

 int know = 0;

 if (GET_SKILL(ch, skill) > 0)
  know = 1;
 if (GET_STUPIDKISS(ch) == skill)
  know = 2;

 if (know == 0) {
  send_to_char(ch, "You do not know how to perform %s.\r\n", spell_info[skill].name);
  know = 0;
 } else if (know == 2) {
  send_to_char(ch, "@WYou try to use @M%s@W but lingering thoughts of a certain kiss distracts you!@n\r\n", spell_info[skill].name);
  send_to_char(ch, "You must sleep in order to cure this.\r\n");
  know = 0;
 }

 return (know);
}

/* Add should be set to the amount you want to add to whatever is rolled. */
int roll_aff_duration(int num, int add)
{
 int start = num / 20;
 int finish = num / 10;
 int outcome = add;


 outcome += rand_number(start, finish);

 return (outcome);
}

void null_affect(struct char_data *ch, int aff_flag)
{

  struct affected_type *af, *next_af;

  for (af = ch->affected; af; af = next_af) {
    next_af = af->next;
    if (af->location == APPLY_NONE && af->bitvector == aff_flag)
      affect_remove(ch, af);
  }
}

void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd)
{
  struct affected_type af[6];
  int num = 0;

  if (dur <= 0)
   dur = 1;

  if (str == 0 && con == 0 && wis == 0 && intel == 0 && agl == 0 && spd == 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = 0;
   af[num].location = APPLY_NONE;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (str != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = str;
   af[num].location = APPLY_STR;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (con != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = con;
   af[num].location = APPLY_CON;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (intel != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = intel;
   af[num].location = APPLY_INT;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (agl != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = agl;
   af[num].location = APPLY_DEX;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (spd != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = spd;
   af[num].location = APPLY_CHA;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
  if (wis != 0) {
   af[num].type = skill;
   af[num].duration = dur;
   af[num].modifier = wis;
   af[num].location = APPLY_WIS;
   af[num].bitvector = aff_flag;
   affect_join(ch, &af[num], FALSE, FALSE, FALSE, FALSE);
   num += 1;
  }
}

int sec_roll_check(struct char_data *ch)
{

 int figure = 0, chance = 0, outcome = 0;

 figure = 10 + (GET_LEVEL(ch) * 1.6);

 chance = axion_dice(0) + axion_dice(0) + rand_number(0, 20);

 if (figure >= chance)
  outcome = 1;

 return (outcome);

}

int get_measure(struct char_data *ch, int height, int weight)
{
  int amt = 0;
  if (!PLR_FLAGGED(ch, PLR_OOZARU) && (!IS_ICER(ch) || !IS_TRANSFORMED(ch)) && GET_GENOME(ch, 0) < 9) {
   if (height > 0) {
    amt = height;
   } else if (weight > 0) {
    amt = weight;
   }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
   if (height > 0) {
    amt = height * 3;
   } else if (weight > 0) {
    amt = weight * 4;
   }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
   if (height > 0) {
    amt = height * 3;
   } else if (weight > 0) {
    amt = weight * 5;
   }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
   if (height > 0) {
    amt = height * 1.5;
   } else if (weight > 0) {
    amt = weight * 2;
   }
  } else if (IS_ICER(ch) && PLR_FLAGGED(ch, PLR_TRANS4)) {
   if (height > 0) {
    amt = height * 2;
   } else if (weight > 0) {
    amt = weight * 3;
   }
  } else if (PLR_FLAGGED(ch, PLR_OOZARU) || GET_GENOME(ch, 0) == 9) {
   if (height > 0) {
    amt = height * 10;
   } else if (weight > 0) {
    amt = weight * 50;
   }
  }

 return (amt);
}


int64_t physical_cost(struct char_data *ch, int skill)
{

 int64_t result = 0;

 if (skill == SKILL_PUNCH)
  result = GET_MAX_HIT(ch) / 500;
 else if (skill == SKILL_KICK)
  result = GET_MAX_HIT(ch) / 350;
 else if (skill == SKILL_ELBOW)
  result = GET_MAX_HIT(ch) / 400;
 else if (skill == SKILL_KNEE)
  result = GET_MAX_HIT(ch) / 300;
 else if (skill == SKILL_UPPERCUT)
  result = GET_MAX_HIT(ch) / 200;
 else if (skill == SKILL_ROUNDHOUSE)
  result = GET_MAX_HIT(ch) / 150;
 else if (skill == SKILL_HEELDROP)
  result = GET_MAX_HIT(ch) / 80;
 else if (skill == SKILL_SLAM)
  result = GET_MAX_HIT(ch) / 90;

 int cou1 = 1 + rand_number(1, 20), cou2 = cou1 + rand_number(1, 6);

 result += rand_number(cou1, cou2);

 if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 100) {
  result -= result * 0.4;
 } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 75) {
  if (IS_TSUNA(ch)) {
   result -= result * 0.40;
  } else if (IS_TAPION(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK)) {
   result -= result * 0.35;
  } else if (IS_JINTO(ch)) {
   if (GET_SKILL_BASE(ch, skill) >= 100) {
    result -= result * 0.45;
   } else {
    result -= result * 0.25;
   }
  } else {
   result -= result * 0.25;
  }
 } else if (GET_SKILL_BASE(ch, SKILL_STYLE) >= 50) {
  result -= result * 0.25;
 }

 if (IS_ANDROID(ch)) {
  result *= 0.25;
 }

 return (result);
}

int android_can(struct char_data *ch)
{

 struct obj_data *obj = GET_EQ(ch, WEAR_BACKPACK);

 if (!obj)
  return (0);
 else if (GET_OBJ_VNUM(obj) == 1806)
  return (1);
 else if (GET_OBJ_VNUM(obj) == 1807)
  return (2);
 else
  return (0);

}

int trans_cost(struct char_data *ch, int trans)
{

 if (GET_TRANSCOST(ch, trans) == 0) {
  return (50);
 } else {
  return (0);
 }
}

/* This returns the trans requirement for the player based on their trans class */
/* 3 = Great, 2 = Average, 1 = Terrible */
/* Rillao: transloc, add new transes here */
int trans_req(struct char_data *ch, int trans)
{

 int requirement = 0;

 if (IS_HUMAN(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1500000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1800000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 2100000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 37500000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 35000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 32500000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 200000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 190000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 185000000;
    }
    break;
   case 4:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1400000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1200000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1100000000;
    }
    break;
  }
 } /* End Human Requirement */

 if (IS_HALFBREED(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1500000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1400000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1200000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 60000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 55000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 50000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1200000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1050000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 950000000;
    }
    break;
  }
 } /* End Halfbreed Requirement */

 if (IS_SAIYAN(ch)) {
  if (PLR_FLAGGED(ch, PLR_LSSJ)) {
   switch (trans)
   {
    case 1:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 600000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 500000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 450000;
     }
     break;
     case 2:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 300000000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 250000000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 225000000;
     }
     break;
   }
  } else {
   switch (trans)
   {
    case 1:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 1400000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 1200000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 1100000;
     }
     break;
    case 2:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 60000000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 55000000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 50000000;
     }
     break;
    case 3:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 160000000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 150000000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 140000000;
     }
     break;
    case 4:
     if (GET_TRANSCLASS(ch) == 1) {
      requirement = 1800000000;
     } else if (GET_TRANSCLASS(ch) == 2) {
      requirement = 1625000000;
     } else if (GET_TRANSCLASS(ch) == 3) {
      requirement = 1400000000;
     }
    break;
   }
  }
 } /* End Saiyan Requirement */

 if (IS_NAMEK(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 400000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 360000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 335000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 10000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 9500000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 8000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 240000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 220000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 200000000;
    }
    break;
   case 4:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 950000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 900000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 875000000;
    }
    break;
  }
 } /* End Namek Requirement */

 if (IS_ICER(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 550000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 500000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 450000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 20000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 17500000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 15000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 180000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 150000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 125000000;
    }
    break;
   case 4:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 880000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 850000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 820000000;
    }
    break;
  }
 } /* End Icer Requirement */

 if (IS_MAJIN(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 2400000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 2200000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 2000000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 50000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 45000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 40000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1800000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1550000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1300000000;
    }
    break;
  }
 } /* End Majin Requirement */

 if (IS_TRUFFLE(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 3800000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 3600000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 3500000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 400000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 300000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 200000000;
    }
    break;
    case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1550000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1450000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1250000000;
    }
    break;
  }
 } /* End Truffle Requirement */

 if (IS_MUTANT(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 200000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 180000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 160000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 30000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 27500000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 25000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 750000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 700000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 675000000;
    }
    break;
  }
 } /* End Mutant Requirement */

 if (IS_KAI(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 3250000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 3000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 2850000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 700000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 650000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 625000000;
    }
    break;
    case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1500000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1300000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1250000000;
    }
    break;
  }
 } /* End Kai Requirement */

 if (IS_KONATSU(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 2000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1800000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1600000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 250000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 225000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 200000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1600000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1400000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1300000000;
    }
    break;
  }
 } /* End Konatsu Requirement */

 if (IS_ANDROID(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1200000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 850000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 8500000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 8000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 7750000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 55000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 50000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 40000000;
    }
    break;
   case 4:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 325000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 300000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 275000000;
    }
    break;
   case 5:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 900000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 800000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 750000000;
    }
    break;
   case 6:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1300000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1200000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1100000000;
    }
    break;
  }
 } /* End Android Requirement */

 if (IS_BIO(ch)) {
  switch (trans)
  {
   case 1:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 2000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1800000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1700000;
    }
    break;
   case 2:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 30000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 25000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 20000000;
    }
    break;
   case 3:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 235000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 220000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 210000000;
    }
    break;
   case 4:
    if (GET_TRANSCLASS(ch) == 1) {
     requirement = 1500000000;
    } else if (GET_TRANSCLASS(ch) == 2) {
     requirement = 1300000000;
    } else if (GET_TRANSCLASS(ch) == 3) {
     requirement = 1150000000;
    }
    break;
  }
 } /* End Bioandroid Requirement */

 return (requirement);
}

void customWrite(struct char_data *ch, struct obj_data *obj)
{

 if (IS_NPC(ch))
  return;

 char fname[40], line[256], prev[256];
 char buf[MAX_STRING_LENGTH];
 FILE *fl, *file;

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(file = fopen(fname, "r"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 while (!feof(file)) {
  get_line(file, line);
  if (strcasecmp(prev, line))
   sprintf(buf+strlen(buf), "%s\n", line);
  *prev = '\0';
  sprintf(prev, line);
 }

 fclose(file);

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(fl = fopen(fname, "w"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 sprintf(buf+strlen(buf), "%s\n", obj->short_description);
 fprintf(fl, "%s\n", buf);
 

 fclose(fl);
}

void customRead(struct descriptor_data *d, int type, char *name)
{

 char fname[40], line[256], filler[256];
 FILE *fl;
 char buf[MAX_STRING_LENGTH];

 if (type == 1) {

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, name)) {
   log("ERROR: Custom unable to be read from user file!");
   return;
  }

  if (!(fl = fopen(fname, "r"))) {
   log("ERROR: Custom file unable to be read!");
   return;
  }

  char buf[MAX_STRING_LENGTH];

  while (!feof(fl)) {
   get_line(fl, line);
   if (strcasecmp(filler, line))
    sprintf(buf+strlen(buf), "%s\n", line);
   *filler = '\0';
   *line = '\0';
   sprintf(filler, line);
  }

  send_to_char(d->character, buf);

  fclose(fl);    
  return;
 } else {

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user)) {
   log("ERROR: Custom unable to be read from user file!");
   return;
  }

  if (!(fl = fopen(fname, "r"))) {
   log("ERROR: Custom file unable to be read!");
   return;
  }

  while (!feof(fl)) {
   get_line(fl, line);
   if (strcasecmp(filler, line))
    sprintf(buf+strlen(buf), "%s\n", line);
   *filler = '\0';
   sprintf(filler, line);
  }

  write_to_output(d, buf);

  fclose(fl);
 }
}

void customCreate(struct descriptor_data *d)
{

  if (!d)
   return;

  if (d->customfile == 1)
   return;

  char fname[40];
  FILE *fl;

  /* Create Their Custom File */

  if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, d->user))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not create custom file.");
    return;
  }

 fprintf(fl, "@D--@RUser @GC@gustom@Gs@D--@n\n");
 d->customfile = 1;

 fclose(fl);
}

int64_t show_softcap(struct char_data *ch)
{
 int64_t capamt = 0;
 if ((IS_ANDROID(ch) && PLR_FLAGGED(ch, PLR_ABSORB)) || (!IS_ANDROID(ch) && !IS_BIO(ch) && !IS_MAJIN(ch))) {
   if (GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    capamt = GET_LEVEL(ch) * 1500000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 2000000;
    }
   }
   if (GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    capamt = GET_LEVEL(ch) * 800000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 1000000;
    }
   }
   if (GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    capamt = GET_LEVEL(ch) * 250000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 300000;
    }
   }
   if (GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    capamt = GET_LEVEL(ch) * 200000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 250000;
    }
   }
   if (GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    capamt = GET_LEVEL(ch) * 80000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 100000;
    }
   }
   if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    capamt = GET_LEVEL(ch) * 20000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 40000;
    }
   }
   if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    capamt = GET_LEVEL(ch) * 15000;
    if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
     capamt = GET_LEVEL(ch) * 25000;
    }
   }
   if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    capamt = GET_LEVEL(ch) * 5000;
   }
   if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    capamt = GET_LEVEL(ch) * 1500;
   }
   if (GET_LEVEL(ch) <= 10) {
    capamt = GET_LEVEL(ch) * 500;
   }
 } else {
   if (GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    capamt = GET_LEVEL(ch) * 4500000;
   }
   if (GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    capamt = GET_LEVEL(ch) * 2400000;
   }
   if (GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    capamt = GET_LEVEL(ch) * 750000;
   }
   if (GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    capamt = GET_LEVEL(ch) * 600000;
   }
   if (GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    capamt = GET_LEVEL(ch) * 240000;
   }
   if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    capamt = GET_LEVEL(ch) * 60000;
   }
   if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    capamt = GET_LEVEL(ch) * 45000;
   }
   if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    capamt = GET_LEVEL(ch) * 15000;
   }
   if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    capamt = GET_LEVEL(ch) * 4500;
   }
   if (GET_LEVEL(ch) <= 10) {
    capamt = GET_LEVEL(ch) * 1500;
   }
 }
 return (capamt);
}

const char *disp_align(struct char_data *ch)
{
 int align;

 if (GET_ALIGNMENT(ch) < -800) { // Horrifically Evil
  align = 8;
 } else if (GET_ALIGNMENT(ch) < -600) { // Terrible
  align = 7;
 } else if (GET_ALIGNMENT(ch) < -300) { // Villain
  align = 6;
 } else if (GET_ALIGNMENT(ch) < -50) {  // Crook
  align = 5;
 } else if (GET_ALIGNMENT(ch) < 51) {   // Neutral
  align = 4;
 } else if (GET_ALIGNMENT(ch) < 300) {  // Do-gooder
  align = 3;
 } else if (GET_ALIGNMENT(ch) < 600) {  // Hero
  align = 2;
 } else if (GET_ALIGNMENT(ch) < 800) {  // Valiant
  align = 1;
 } else {                               // Saintly
  align = 0;
 }

 return (alignments[align]);
}


/* This creates a player's sense memory file for the first time. */
void senseCreate(struct char_data *ch)
{
  char fname[40];
  FILE *fl;
  /* Write Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch)))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save sense memory of, %s, to filename, %s.", GET_NAME(ch), fname);
    return;
  }

  fprintf(fl, "0\n");

  fclose(fl);
  return;
}

int read_sense_memory(struct char_data *ch, struct char_data *vict) {
  char fname[40], line[256];
  int known = FALSE, idnums = -1337;
  FILE *fl;

  /* Read Sense File */
  if (vict == NULL) {
    log("Noone.");
    return 0;
  }

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch))) {
    senseCreate(ch);
  }
  if (!(fl = fopen(fname, "r"))) {
    return 2;
  }
  if (vict == ch) {
    fclose(fl);
    return 0;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%d\n", &idnums);
    if (IS_NPC(vict)) {
     if (idnums == GET_MOB_VNUM(vict)) {
      known = TRUE;
     }
    } else {
     if (idnums == GET_ID(vict)) {
      known = TRUE;
     }
    }
  }
   fclose(fl);

   if (known == TRUE)
    return 1;
   else
    return 0;
}

/* This writes a player's sense memory to file. */
void sense_memory_write(struct char_data *ch, struct char_data *vict)
{
  FILE *file;
  char fname[40], line[256];
  int idnums[500] = {0};
  FILE *fl;
  int count = 0, x = 0, id_sample;

  /* Read Sense File */
  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch))) {
    senseCreate(ch);
  }
  if (!(file = fopen(fname, "r"))) {
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%d\n", &id_sample);
    idnums[count] = id_sample;
    count++;
  }
  fclose(file);

  /* Write Sense File */

  if (!get_filename(fname, sizeof(fname), SENSE_FILE, GET_NAME(ch)))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save sense memory file, %s, to filename, %s.", GET_NAME(ch), fname);
    return;
  }

  while(x < count) {
    if (x == 0 || idnums[x-1] != idnums[x]) {
     if (!IS_NPC(vict)) {
      if (idnums[x] != GET_ID(vict)) {
       fprintf(fl, "%d\n", idnums[x]);
      }
     } else {
      if (idnums[x] != GET_MOB_VNUM(vict)) {
       fprintf(fl, "%d\n", idnums[x]);
      }
     }
    }
    x++;
  }

  if (!IS_NPC(vict)) {
   fprintf(fl, "%d\n", GET_ID(vict));
  } else {
   fprintf(fl, "%d\n", GET_MOB_VNUM(vict));
  }

  fclose(fl);
  return;
}

/* Will they manage to pursue a fleeing enemy? */
int roll_pursue(struct char_data *ch, struct char_data *vict)
{

 int skill, perc = axion_dice(0);

 if (ch == NULL || vict == NULL)
  return (FALSE);

 if (!IS_NPC(ch)) {
  skill = GET_SKILL(ch, SKILL_PURSUIT);
 } else if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL)) {
  skill = GET_LEVEL(ch);
  if (ROOM_FLAGGED(IN_ROOM(vict), ROOM_NOMOB))
   skill = -1;
 } else {
  skill = -1;
 }

 if (!IS_NPC(vict)) {
  if (IS_NPC(ch) && !(vict->desc)) {
   skill = -1;
  }
 }

 if (skill > perc) {
  int inroom = GET_ROOM_VNUM(IN_ROOM(ch));
  act("@C$n@R pursues after the fleeing @c$N@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);  
  char_from_room(ch);
  char_to_room(ch, IN_ROOM(vict));
  act("@GYou pursue right after @c$N@G!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@R pursues after you!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@R pursues after the fleeing @c$N@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);

  struct follow_type *k, *next;

  if (ch->followers) {
   for (k = ch->followers; k; k = next) {
    next = k->next;
    if ((GET_ROOM_VNUM(IN_ROOM(k->follower)) == inroom) && (GET_POS(k->follower) >= POS_STANDING) && (!AFF_FLAGGED(ch, AFF_ZANZOKEN) || (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(k->follower, AFF_GROUP)))) {
     act("You follow $N.", TRUE, k->follower, 0, ch, TO_CHAR);
     act("$n follows after $N.", TRUE, k->follower, 0, ch, TO_NOTVICT);
     act("$n follows after you.", TRUE, k->follower, 0, ch, TO_VICT);
     char_from_room(k->follower);
     char_to_room(k->follower, IN_ROOM(ch));
    }
   }
  }
  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_PURSUIT);
  return (TRUE);
 } else {
  send_to_char(ch, "@RYou fail to pursue after them!@n\r\n");
  if (FIGHTING(ch)) {
   stop_fighting(ch);
  } if (FIGHTING(vict)) {
   stop_fighting(vict);
  }
  return (FALSE);
 }

}

/* This updates the malfunctioning of certain objects that are damaged. */
void broken_update()
{
 struct obj_data *k, *money;

 int rand_gravity[14] = {0, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 5000, 10000};
 int dice = rand_number(2, 12), grav_roll = 0, grav_change = FALSE, health = 0;

 for (k = object_list; k; k = k->next) {
  if (k->carried_by != NULL) {
   continue;
  }

  if (rand_number(1, 2) == 2) {
   continue;
  }

  health = GET_OBJ_VAL(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

  if (GET_OBJ_VNUM(k) == 11) { /* Gravity Generator */
   grav_roll = rand_number(0, 13);
   if (health <= 10) {
    grav_change = TRUE;
   } else if (health <= 40 && dice <= 8) {
    grav_change = TRUE;
   } else if (health <= 80 && dice <= 5) {
    grav_change = TRUE;
   } else if (health <= 99 && dice <= 3) {
    grav_change = TRUE;
   }
   if (grav_change == TRUE) {
    ROOM_GRAVITY(IN_ROOM(k)) = rand_gravity[grav_roll];
    GET_OBJ_WEIGHT(k) = rand_gravity[grav_roll];
    send_to_room(IN_ROOM(k), "@RThe gravity generator malfunctions! The gravity level has changed!@n\r\n");
   }
  } /* End Gravity Section */

  if (GET_OBJ_VNUM(k) == 3034) { /* ATM */
   if (health <= 10) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine shoots smoking bills from its money slot. The bills burn up as they float through the air!@n\r\n");
   } else if (health <= 40 && dice <= 8) {
    send_to_room(IN_ROOM(k), "@RGibberish flashes across the cracked ATM info screen.@n\r\n");
   } else if (health <= 80 && dice == 4) {
    send_to_room(IN_ROOM(k), "@GThe damaged ATM spits out some money while flashing ERROR on its screen!@n\r\n");
    money = create_money(rand_number(1, 30));
    obj_to_room(money, IN_ROOM(k));
   } else if (health <= 99 && dice < 4) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine emits a loud grinding sound from inside.@n\r\n");
   }
  } /* End ATM */

  dice = rand_number(2, 12); // Reset the dice
 } /* End For */

}


int wearable_obj(struct obj_data *obj)
{
  int pass = FALSE;

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_WIELD))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_EYE))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_PACK))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_SH))
     pass = TRUE;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))
     pass = TRUE;
  
  if (pass == TRUE)
   return (1);
  else
   return (0);
}

void randomize_eq(struct obj_data *obj)
{
    if (wearable_obj(obj) && !OBJ_FLAGGED(obj, ITEM_NORANDOM)) {
     int value = 0, slot = 0, roll = rand_number(2, 12), slot1 = 1, slot2 = 1, slot3 = 1, slot4 = 1, slot5 = 1, slot6 = 1;
     int stat = 0;
     int strength = FALSE, wisdom = FALSE, intelligence = FALSE, dexterity = FALSE, speed = FALSE, constitution = FALSE;
     // Setting the strength stats
      int i;
      for (i = 0; i < 6; i++) {
       stat = obj->affected[slot].location;
       value = obj->affected[slot].modifier;
       if (stat == 1) { /* Strength */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         strength = TRUE;
        }
       } else if (stat == 2) { /* Agility */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         dexterity = TRUE;
        }
       } else if (stat == 3) { /* Intelligence */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         intelligence = TRUE;
        }
       } else if (stat == 4) { /* Wisdom */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         wisdom = TRUE;
        }
       } else if (stat == 5) { /* Constitution */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         constitution = TRUE;
        }
       } else if (stat == 6) { /* Speed */
        if (roll == 12) {
         value += 3;
        } else if (roll >= 9) {
         value += 2;
        } else if (roll >= 6) {
         value += 1;
        } else if (roll == 3) {
         value -= 1;
        } else if (roll <= 2) {
         value -= 2;
        }
        if (obj->level >= 80) {
         if (value <= 0) {
          value = 1;
         }
        } else if (obj->level >= 60) {
         if (value < 0) {
          value = 0;
         }
        }
        if (value == 0) {
         obj->affected[slot].location = 0;
         obj->affected[slot].modifier = 0;
        } else {
         obj->affected[slot].modifier = value;
         speed = TRUE;
        }        
       } else if (stat == 0) {
         switch (slot) {
          case 1:
           slot1 = 0;
           break;
          case 2:
           slot2 = 0;
           break;
          case 3:
           slot3 = 0;
           break;
          case 4:
           slot4 = 0;
           break;
          case 5:
           slot5 = 0;
           break;
          case 6:
           slot6 = 0;
           break;
         }
       }
      slot += 1;
      roll = rand_number(2, 12);
     }

     if (slot1 == 0) {
      if (strength == FALSE && rand_number(1, 6) == 1) {
       strength = TRUE;
       obj->affected[0].location = 1;
       obj->affected[0].modifier = 1;
      } else if (dexterity == FALSE && rand_number(1, 6) == 1) {
       dexterity = TRUE;
       obj->affected[0].location = 2;
       obj->affected[0].modifier = 1;
      } else if (intelligence == FALSE && rand_number(1, 6) == 1) {
       intelligence = TRUE;
       obj->affected[0].location = 3;
       obj->affected[0].modifier = 1;
      } else if (wisdom == FALSE && rand_number(1, 6) == 1) {
       wisdom = TRUE;
       obj->affected[0].location = 4;
       obj->affected[0].modifier = 1;
      } else if (constitution == FALSE && rand_number(1, 6) == 1) {
       constitution = TRUE;
       obj->affected[0].location = 5;
       obj->affected[0].modifier = 1;
      } else if (speed == FALSE && rand_number(1, 6) == 1) {
       speed = TRUE;
       obj->affected[0].location = 6;
       obj->affected[0].modifier = 1;
      }
     }
     if (slot2 == 0 && roll >= 10) {
      if (strength == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 1;
       obj->affected[1].modifier = 1;
      } else if (dexterity == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 2;
       obj->affected[1].modifier = 1;
      } else if (intelligence == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 3;
       obj->affected[1].modifier = 1;
      } else if (wisdom == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 4;
       obj->affected[1].modifier = 1;
      } else if (constitution == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 5;
       obj->affected[1].modifier = 1;
      } else if (speed == FALSE && rand_number(1, 6) == 1) {
       obj->affected[1].location = 6;
       obj->affected[1].modifier = 1;
      }
     }
     int dice = rand_number(2, 12);
     if (dice >= 10) {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOT2);
     } else if (dice >= 7) {
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_SLOT1);
     }
    }
}

char *sense_location(struct char_data *ch)
{

	char *message = malloc(MAX_INPUT_LENGTH);
	int roomnum = GET_ROOM_VNUM(IN_ROOM(ch)), num = 0;
	if ((num = real_zone_by_thing(roomnum)) != NOWHERE) {
		num = real_zone_by_thing(roomnum);
	}

	switch (num) {
	case 2:
		sprintf(message, "East of Nexus City");
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		if (roomnum < 795)
			sprintf(message, "Nexus City");
		else
			sprintf(message, "South Ocean");
		break;
	case 8:
	case 9:
	case 10:
	case 11:
		if (roomnum < 1133)
			sprintf(message, "South Ocean");
		else if (roomnum < 1179)
			sprintf(message, "Nexus Field");
		else
			sprintf(message, "Cherry Blossom Mountain");
		break;
	case 12:
	case 13:
		if (roomnum < 1287)
			sprintf(message, "Cherry Blossom Mountain");
		else
			sprintf(message, "Sandy Desert");
		break;
	case 14:
		if (roomnum < 1428)
			sprintf(message, "Sandy Desert");
		else if (roomnum < 1484)
			sprintf(message, "Northern Plains");
		else if (roomnum < 1496)
			sprintf(message, "Korin's Tower");
		else
			sprintf(message, "Kami's Lookout");
		break;
	case 15:
		if (roomnum < 1577)
			sprintf(message, "Kami's Lookout");
		else if (roomnum < 1580)
			sprintf(message, "Northern Plains");
		else if (roomnum < 1589)
			sprintf(message, "Kami's Lookout");
		else
			sprintf(message, "Shadow Forest");
		break;
	case 16:
		sprintf(message, "Shadow Forest");
		break;
	case 17:
	case 18:
		if (roomnum < 1715)
			sprintf(message, "Decrepit Area");
		else
			sprintf(message, "Inside Cherry Blossom Mountain");
		break;
	case 19:
		sprintf(message, "West City");
		break;
	case 20:
		if (roomnum < 2012)
			sprintf(message, "West City");
		else if (roomnum > 2070)
			sprintf(message, "West City");
		else
			sprintf(message, "Silver Mine");
		break;
	case 21:
		if (roomnum < 2141)
			sprintf(message, "West City");
		else
			sprintf(message, "Hercule Beach");
		break;
	case 22:
		sprintf(message, "Vegetos City");
		break;
	case 23:
	case 24:
		if (roomnum < 2334)
			sprintf(message, "Vegetos City");
		else if (roomnum > 2462)
			sprintf(message, "Vegetos City");
		else
			sprintf(message, "Vegetos Palace");
		break;
	case 25:
	case 26:
		if (roomnum < 2616)
			sprintf(message, "Blood Dunes");
		else
			sprintf(message, "Ancestral Mountains");
		break;
	case 27:
		if (roomnum < 2709)
			sprintf(message, "Ancestral Mountains");
		else if (roomnum < 2736)
			sprintf(message, "Destopa Swamp");
		else
			sprintf(message, "Swamp Base");
		break;
	case 28:
		sprintf(message, "Pride Forest");
		break;
	case 29:
	case 30:
	case 31:
		sprintf(message, "Pride Tower");
		break;
	case 32:
		sprintf(message, "Ruby Cave");
		break;
	case 34:
		sprintf(message, "Utatlan City");
		break;
	case 35:
		sprintf(message, "Zenith Jungle");
		break;
	case 40:
	case 41:
	case 42:
		sprintf(message, "Ice Crown City");
		break;
	case 43:
		if (roomnum < 4351)
			sprintf(message, "Ice Highway");
		else
			sprintf(message, "Topica Snowfield");
		break;
	case 44:
	case 45:
		sprintf(message, "Glug's Volcano");
		break;
	case 46:
	case 47:
		sprintf(message, "Platonic Sea");
		break;
	case 48:
		sprintf(message, "Slave City");
		break;
	case 49:
		if (roomnum < 4915)
			sprintf(message, "Descent Down Icecrown");
		else if (roomnum != 4915 && roomnum < 4994)
			sprintf(message, "Topica Snowfield");
		else
			sprintf(message, "Ice Highway");
		break;
	case 50:
		sprintf(message, "Mirror Shard Maze");
		break;
	case 51:
		if (roomnum < 5150)
			sprintf(message, "Acturian Woods");
		else if (roomnum < 5165)
			sprintf(message, "Desolate Demesne");
		else
			sprintf(message, "Chateau Ishran");
		break;
	case 52:
		sprintf(message, "Wyrm Spine Mountain");
		break;
	case 53:
	case 54:
		sprintf(message, "Aromina Hunting Preserve");
		break;
	case 55:
		sprintf(message, "Cloud Ruler Temple");
		break;
	case 56:
		sprintf(message, "Koltoan Mine");
		break;
	case 78:
		sprintf(message, "Orium Cave");
		break;
	case 79:
		sprintf(message, "Crystalline Forest");
		break;
	case 80:
	case 81:
	case 82:
		sprintf(message, "Tiranoc City");
		break;
	case 83:
		sprintf(message, "Great Oroist Temple");
		break;
	case 84:
		if (roomnum < 8447)
			sprintf(message, "Elsthuan Forest");
		else
			sprintf(message, "Mazori Farm");
		break;
	case 85:
		sprintf(message, "Dres");
		break;
	case 86:
		sprintf(message, "Colvian Farm");
		break;
	case 87:
		sprintf(message, "Saint Alucia");
		break;
	case 88:
		if (roomnum < 8847)
			sprintf(message, "Meridius Memorial");
		else
			sprintf(message, "Battlefields");
		break;
	case 89:
		if (roomnum < 8954)
			sprintf(message, "Desert of Illusion");
		else
			sprintf(message, "Plains of Confusion");
		break;
	case 90:
		sprintf(message, "Shadowlas Temple");
		break;
	case 92:
		sprintf(message, "Turlon Fair");
		break;
	case 97:
		sprintf(message, "Wetlands");
		break;
	case 98:
		if (roomnum < 9855)
			sprintf(message, "Wetlands");
		else if (roomnum < 9866)
			sprintf(message, "Kerberos");
		else
			sprintf(message, "Shaeras Mansion");
		break;
	case 99:
		if (roomnum < 9907)
			sprintf(message, "Slavinos Ravine");
		else if (roomnum < 9960)
			sprintf(message, "Kerberos");
		else
			sprintf(message, "Furian Citadel");
		break;
	case 100:
	case 101:
	case 102:
	case 103:
	case 104:
	case 105:
	case 106:
	case 107:
	case 108:
	case 109:
	case 110:
	case 111:
	case 112:
	case 113:
	case 114:
	case 115:
		sprintf(message, "Namekian Wilderness");
		break;
	case 116:
		if (roomnum < 11672)
			sprintf(message, "Senzu Village");
		else if (roomnum > 11672 && roomnum < 11698)
			sprintf(message, "Senzu Village");
		else
			sprintf(message, "Guru's House");
		break;
	case 117:
	case 118:
	case 119:
		sprintf(message, "Crystalline Cave");
		break;
	case 120:
		sprintf(message, "Haven City");
		break;
	case 121:
		if (roomnum < 12103)
			sprintf(message, "Haven City");
		else
			sprintf(message, "Serenity Lake");
		break;
	case 122:
		sprintf(message, "Serenity Lake");
		break;
	case 123:
		sprintf(message, "Kaiju Forest");
		break;
	case 124:
		if (roomnum < 12480)
			sprintf(message, "Ortusian Temple");
		else
			sprintf(message, "Silent Glade");
		break;
	case 125:
		sprintf(message, "Near Serenity Lake");
		break;
	case 130:
	case 131:
		if (roomnum < 13153)
			sprintf(message, "Satan City");
		else if (roomnum == 13153)
			sprintf(message, "West City");
		else if (roomnum == 13154)
			sprintf(message, "Nexus City");
		else
			sprintf(message, "South Ocean");
		break;
	case 132:
		if (roomnum < 13232)
			sprintf(message, "Frieza's Ship");
		else
			sprintf(message, "Namekian Wilderness");
		break;
	case 133:
		sprintf(message, "Elder Village");
		break;
	case 134:
		sprintf(message, "Satan City");
		break;
	case 140:
		sprintf(message, "Yardra City");
		break;
	case 141:
		sprintf(message, "Jade Forest");
		break;
	case 142:
		sprintf(message, "Jade Cliff");
		break;
	case 143:
		sprintf(message, "Mount Valaria");
		break;
	case 149:
	case 150:
		sprintf(message, "Aquis City");
		break;
	case 151:
	case 152:
	case 153:
		sprintf(message, "Kanassan Ocean");
		break;
	case 154:
		sprintf(message, "Kakureta Village");
		break;
	case 155:
		sprintf(message, "Captured Aether City");
		break;
	case 156:
		sprintf(message, "Yunkai Pirate Base");
		break;
	case 160:
	case 161:
		sprintf(message, "Janacre");
		break;
	case 165:
		sprintf(message, "Arlian Wasteland");
		break;
	case 166:
		sprintf(message, "Arlian Mine");
		break;
	case 167:
		sprintf(message, "Kilnak Caverns");
		break;
	case 168:
		sprintf(message, "Kemabra Wastes");
		break;
	case 169:
		sprintf(message, "Dark of Arlia");
		break;
	case 174:
		sprintf(message, "Fistarl Volcano");
		break;
	case 175:
	case 176:
		sprintf(message, "Cerria Colony");
		break;
	case 182:
		sprintf(message, "Below Tiranoc");
		break;
	case 196:
		sprintf(message, "Ancient Castle");
		break;
	default:
		sprintf(message, "Unknown.");
		break;
	}

	return (message);
}

void reveal_hiding(struct char_data *ch, int type)
{
 if (IS_NPC(ch) || !AFF_FLAGGED(ch, AFF_HIDE))
  return;

 int rand1 = rand_number(-5, 5), rand2 = rand_number(-5, 5), bonus = 0;

 if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
  bonus = 10;
 }

 if (type == 0) { /* Automatic reveal. */
   act("@MYou feel as though what you just did may have revealed your hiding spot...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@M$n moves a little and you notice them!@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
 } else if (type == 1) { /* Their skill at hiding failed, reveal */
  if (GET_SKILL(ch, SKILL_HIDE) + bonus < axion_dice(0)) {
   act("@MYou feel as though what you just did may have revealed your hiding spot...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@M$n moves a little and you notice them!@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  }
 } else if (type == 2) { /* They were spotted */
  struct descriptor_data *d;
  struct char_data *tch = NULL;
  for (d = descriptor_list; d; d = d->next) {
   if (STATE(d) != CON_PLAYING)
    continue;
   tch = d->character;

   if (tch == ch)
    continue;

   if (IN_ROOM(tch) != IN_ROOM(ch))
    continue;
  
   if (GET_SKILL(tch, SKILL_SPOT) + rand1 >= GET_SKILL(ch, SKILL_HIDE) + rand2) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    act("@M$N seems to notice you!@n", TRUE, ch, 0, tch, TO_CHAR);
    act("@MYou notice $n's movements reveal $s hiding spot!@n", TRUE, ch, 0, tch, TO_VICT);
    act("@MYou notice $N look keenly somewhere nearby. At that spot you see $n hiding!@n", TRUE, ch, 0, tch, TO_NOTVICT);
    return;
   }
  }
 } else if (type == 3) { /* They were heard, reveal with different messages. */
  struct descriptor_data *d;
  struct char_data *tch = NULL;

  act("@MThe scouter makes some beeping sounds as you tinker with its buttons.@n", TRUE, ch, 0, 0, TO_CHAR);
  for (d = descriptor_list; d; d = d->next) {
   if (STATE(d) != CON_PLAYING)
    continue;
   tch = d->character;

   if (tch == ch)
    continue;

   if (IN_ROOM(tch) != IN_ROOM(ch))
    continue;

   if (GET_SKILL(tch, SKILL_LISTEN) > axion_dice(0)) {
    switch (type) {
     case 3:
      act("@MYou notice some beeping sounds that sound really close by.@n", TRUE, ch, 0, tch, TO_VICT);
      break;
     default:
      act("@MYou notice some sounds coming from this room but can't seem to locate the source...@n", TRUE, ch, 0, tch, TO_VICT);
      break;
    }
   }
  }
 } else if (type == 4) { /* You were found from search! */
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
 }
}

int block_calc(struct char_data *ch)
{
  struct char_data *blocker = NULL;

  if (BLOCKED(ch)) {
   blocker = BLOCKED(ch);
  } else {
   return (1);
  }

   if (GET_SPEEDI(ch) < GET_SPEEDI(blocker) && GET_POS(blocker) > POS_SITTING) {
     if (!AFF_FLAGGED(blocker, AFF_BLIND) && !PLR_FLAGGED(blocker, PLR_EYEC)) {
       int minimum = GET_CHA(blocker) + rand_number(5, 20);
       if (minimum > 100) {
        minimum = 100;
       }
       if (!GET_SKILL(ch, SKILL_ESCAPE_ARTIST) || (GET_SKILL(ch, SKILL_ESCAPE_ARTIST) && GET_SKILL(ch, SKILL_ESCAPE_ARTIST)  < rand_number(minimum, 120))) {
        act("$n tries to leave, but can't outrun $N!", TRUE, ch, 0, blocker, TO_NOTVICT);
        act("$n tries to leave, but can't outrun you!", TRUE, ch, 0, blocker, TO_VICT);
        act("You try to leave, but can't outrun $N!", TRUE, ch, 0, blocker, TO_CHAR);
        if (AFF_FLAGGED(ch, AFF_FLYING) && !AFF_FLAGGED(blocker, AFF_FLYING) && GET_ALT(ch) == 1) {
         send_to_char(blocker, "You're now floating in the air.\r\n");
         SET_BIT_AR(AFF_FLAGS(blocker), AFF_FLYING);
         GET_ALT(blocker) = GET_ALT(ch);
        } else if (AFF_FLAGGED(ch, AFF_FLYING) && !AFF_FLAGGED(blocker, AFF_FLYING) && GET_ALT(ch) == 2) {
         send_to_char(blocker, "You're now floating high in the sky.\r\n");
         SET_BIT_AR(AFF_FLAGS(blocker), AFF_FLYING);
         GET_ALT(blocker) = GET_ALT(ch);
        }
        return (0);
       } else {
        act("$n proves $s great skill and escapes from $N's attempted block!", TRUE, ch, 0, blocker, TO_NOTVICT);
        act("$n proves $s great skill and escapes from your attempted block!", TRUE, ch, 0, blocker, TO_VICT);
        act("Using your great skill you manage to escape from $N's attempted block!", TRUE, ch, 0, blocker, TO_CHAR);
        BLOCKED(ch) = NULL;
        BLOCKS(blocker) = NULL;
       }
    } else {
        act("$n proves $s great skill and escapes from $N's attempted block!", TRUE, ch, 0, blocker, TO_NOTVICT);
        act("$n proves $s great skill and escapes from your attempted block!", TRUE, ch, 0, blocker, TO_VICT);
        act("Using your great skill you manage to escape from $N's attempted block!", TRUE, ch, 0, blocker, TO_CHAR);
        BLOCKED(ch) = NULL;
        BLOCKS(blocker) = NULL;
    }
   } else if (GET_POS(blocker) <= POS_SITTING) {
    act("$n proves $s great skill and escapes from $N!", TRUE, ch, 0, blocker, TO_NOTVICT);
    act("$n proves $s great skill and escapes from you!", TRUE, ch, 0, blocker, TO_VICT);
    act("Using your great skill you manage to escape from $N!", TRUE, ch, 0, blocker, TO_CHAR);
    BLOCKED(ch) = NULL;
    BLOCKS(blocker) = NULL;
   } else if (GET_POS(blocker) > POS_SITTING) {
    act("$n proves $s great skill and escapes from $N's attempted block!", TRUE, ch, 0, blocker, TO_NOTVICT);
    act("$n proves $s great skill and escapes from your attempted block!", TRUE, ch, 0, blocker, TO_VICT);
    act("Using your great skill you manage to escape from $N's attempted block!", TRUE, ch, 0, blocker, TO_CHAR);
    BLOCKED(ch) = NULL;
    BLOCKS(blocker) = NULL;
   }

  return (1);
}

int64_t molt_threshold(struct char_data *ch)
{

 int64_t threshold = 0, max = 2000000000;

 max *= 250;

 if (!IS_ARLIAN(ch))
  return (0);
 else if (GET_MOLT_LEVEL(ch) < 100) {
  threshold = (((GET_MOLT_LEVEL(ch) + 1) * (GET_MAX_HIT(ch) * 0.02)) * GET_CON(ch)) / 4;
  threshold = threshold * 0.25;
 } else if (GET_MOLT_LEVEL(ch) < 200) {
  threshold = (((GET_MOLT_LEVEL(ch) + 1) * (GET_MAX_HIT(ch) * 0.02)) * GET_CON(ch)) / 2;
  threshold = threshold * 0.20;
 } else if (GET_MOLT_LEVEL(ch) < 400) {
  threshold = (((GET_MOLT_LEVEL(ch) + 1) * (GET_MAX_HIT(ch) * 0.02)) * GET_CON(ch));
  threshold = threshold * 0.17;
 } else if (GET_MOLT_LEVEL(ch) < 800) {
  threshold = (((GET_MOLT_LEVEL(ch) + 1) * (GET_MAX_HIT(ch) * 0.02)) * GET_CON(ch)) * 2;
  threshold = threshold * 0.15;
 } else {
  threshold = (((GET_MOLT_LEVEL(ch) + 1) * (GET_MAX_HIT(ch) * 0.02)) * GET_CON(ch)) * 4;
  threshold = threshold * 0.12;
 }

 if (threshold > max) {
  threshold = max;
 }

 return (threshold);
}

int armor_evolve(struct char_data *ch)
{
 int value = 0;

 if (GET_MOLT_LEVEL(ch) <= 5) {
  value = 8;
 } else if (GET_MOLT_LEVEL(ch) <= 10) {
  value = 12;
 } else if (GET_MOLT_LEVEL(ch) <= 20) {
  value = 15;
 } else if (GET_MOLT_LEVEL(ch) <= 30) {
  value = 20;
 } else if (GET_MOLT_LEVEL(ch) <= 40) {
  value = 30;
 } else if (GET_MOLT_LEVEL(ch) <= 50) {
  value = 50;
 } else if (GET_MOLT_LEVEL(ch) <= 75) {
  value = 100;
 } else if (GET_MOLT_LEVEL(ch) <= 100) {
  value = 150;
 } else if (GET_MOLT_LEVEL(ch) <= 500) {
  value = 200;
 } else {
  value = 220;
 }

 return (value);
}

/* This handles arlian exoskeleton molting */
void handle_evolution(struct char_data *ch, int64_t dmg)
{

 /* Reject NPCs and non-arlians */
 if (IS_NPC(ch) || !IS_ARLIAN(ch)) {
  return;
 }
 
 int64_t moltgain = 0;

 moltgain = dmg * 0.5;
 if (GET_LEVEL(ch) == 100)
  moltgain += 100000;
 else if (GET_LEVEL(ch) >= 90)
  moltgain += GET_LEVEL(ch) * 1000;
 else if (GET_LEVEL(ch) >= 75)
  moltgain += GET_LEVEL(ch) * 500;
 else if (GET_LEVEL(ch) >= 50)
  moltgain += GET_LEVEL(ch) * 250;
 else if (GET_LEVEL(ch) >= 10)
  moltgain += GET_LEVEL(ch) * 50;

 GET_MOLT_EXP(ch) += moltgain;
 
 if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
  send_to_char(ch, "You are dead and all evolution experience is reduced. Gains are divided by 100 or reduced to a minimum of 1.\r\n");
  moltgain /= 100;
 }

 if (GET_MOLT_EXP(ch) > molt_threshold(ch)) {
  if (GET_MOLT_LEVEL(ch) <= GET_LEVEL(ch) * 2 || GET_LEVEL(ch) >= 100) {
   GET_MOLT_EXP(ch) = 0;
   GET_MOLT_LEVEL(ch) += 1;
   double rand1 = 0.02;
   double rand2 = 0.03;
   if (rand_number(1, 4) == 3) {
    rand1 += 0.02;
    rand2 += 0.02;
   } else if (rand_number(1, 4) >= 3) {
    rand1 += 0.01;
    rand2 += 0.01;
   }
   int64_t plgain = GET_MAX_HIT(ch) * rand1, armorgain = 0, stamgain = GET_MAX_MOVE(ch) * rand2;
   armorgain = armor_evolve(ch);
   GET_MAX_HIT(ch) += plgain;
   GET_BASE_PL(ch) += plgain;
   GET_MAX_MOVE(ch) += stamgain;
   GET_BASE_ST(ch) += stamgain;
   GET_ARMOR(ch) += armorgain;
   if (GET_ARMOR(ch) > 50000) {
    GET_ARMOR(ch) = 50000;
   }
   act("@gYour @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. You quickly shed it and reveal a stronger version that was growing beneath it! At the same time you feel your adrenal sacs to be more efficient@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@G$n's@g @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn@g begins to crack. Suddenly $e sheds the damaged @De@Wx@wo@Ds@Wk@we@Dl@We@wt@Do@Wn and reveals a stronger version that had been growing underneath!@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@D[@RPL@W: @G+%s@D] [@gStamina@W: @G+%s@D] [@wArmor Index@W: @G+%s@D]@n\r\n", add_commas(plgain), add_commas(stamgain), GET_ARMOR(ch) >= 50000 ? "50k CAP" : add_commas(armorgain));
  } else {
   send_to_char(ch, "@gYou are unable to evolve while your evolution level is higher than twice your character level.@n\r\n");
  }
 }

}

void demon_refill_lf(struct char_data *ch, int64_t num)
{
 struct char_data *tch = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
   if (!IS_DEMON(tch))
    continue;
   if (GET_LIFEFORCE(tch) >= GET_LIFEMAX(tch))
    continue;
   else {
    GET_LIFEFORCE(tch) += num;
    if (GET_LIFEFORCE(tch) > GET_LIFEMAX(tch))
     GET_LIFEFORCE(tch) = GET_LIFEMAX(tch);
    act("@CYou feel the life energy from @c$N@C's cursed body flow out and you draw it into yourself!@n", TRUE, tch, 0, ch, TO_CHAR);
   }
  }
}


void mob_talk(struct char_data *ch, const char *speech)
{

 struct char_data *tch = NULL, *vict = NULL;
 int stop = 1;

  if (IS_NPC(ch)) {
   return;
  }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
   if (!IS_NPC(tch))
    continue;
   if (!IS_HUMANOID(tch))
    continue;
   if (stop == 0)
    continue;
   else {
    vict = tch;
    stop = mob_respond(ch, vict, speech);
    if (rand_number(1, 2) == 2) {
     stop = 0;
    }
   }
  } /* End for loop */
} /* End Mob Talk */

int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech)
{
    if (ch != NULL && vict != NULL) {
     if (!IS_NPC(ch) && IS_NPC(vict)) {
       if ((strstr(speech, "hello") || strstr(speech, "greet") || strstr(speech, "Hello") || strstr(speech, "Greet")) && !FIGHTING(vict)) {
        send_to_room(IN_ROOM(vict), "\r\n");
        if (IS_HUMAN(vict) || IS_HALFBREED(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CYes, hello to you as well $N.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CHello!@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@CHi, $N, how are you doing?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@CGreetings, $N. What are you up to?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Human Section */
        else if (IS_SAIYAN(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CHmph, hi.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CHello weakling.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@C$N do all weaklings like you waste time in idle talk?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@C$N, you are not welcome around me.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Saiyan Section */
        else if (IS_ICER(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CHa ha... Yes, hello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CAh a polite greeting. It's good to know your kind isn't totally worthless.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@C$N, hello. Now leave me be.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@C$N, you are below me. Now begone.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Icer Section */
        else if (IS_KONATSU(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CGreetings, $N, may your travels be well.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CHello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@C$N, hello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@C$N, it is nice to meet you.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Konatsu Section */
        else if (IS_NAMEK(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CHello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CA peaceful greeting to you, $N.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@C$N, hello. What is your business here?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@C$N, greetings.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Namek Section */
        else if (IS_ARLIAN(vict)) {
         switch (rand_number(1, 4)) {
           case 1:
            act("@w$n@W says, '@CPeace, stranger.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CStay out of my way.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 3:
            act("@w$n@W says, '@C$N, what is your business here?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 4:
            act("@w$n@W says, '@C...Hello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End Arlian Section */
        else if (IS_ANDROID(vict)) {
         act("@w$n@W says, '@C...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
        } /* End Android Section */
        else if (IS_MAJIN(vict)) {
         switch (rand_number(1, 2)) {
           case 1:
            act("@w$n@W says, '@CHa ha...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
           case 2:
            act("@w$n@W says, '@CHello. What candy you want to be?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            break;
         }
        } /* End MAJIN Section */
        else if (IS_TRUFFLE(vict)) {
         switch (rand_number(1, 3)) {
           case 1:
            if (IS_SAIYAN(ch)) {
             act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            } else {
             act("@w$n@W says, '@CHello.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            break;
           case 2:
            if (IS_SAIYAN(ch)) {
             act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            } else {
            act("@w$n@W says, '@C$N, hello. You are a curious individual.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            break;
           case 3:
            if (IS_SAIYAN(ch)) {
             act("@w$n@W says, '@CEwww, dirty monkey...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            } else {
            act("@w$n@W says, '@C$N, hello. What's your IQ?@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            break;
         }
        } /* End Truffle Section */
        else {
         act("Hmph, yeah hi.", TRUE, vict, 0, ch, TO_ROOM);
        }
       } /* End Hello Section */

       if ((strstr(speech, "spar") || strstr(speech, "Spar")) && !FIGHTING(vict)) {
        send_to_room(IN_ROOM(vict), "\r\n");
        if (GET_LEVEL(vict) > 4 && GET_ALIGNMENT(vict) >= 0) {
         memory_rec *names;
         int remember = FALSE;

          for (names = MEMORY(vict); names && !remember; names = names->next) {
            if (names->id != GET_IDNUM(ch))
             continue;

             remember = TRUE;
          }
         if (remember == TRUE) {
          act("@w$n@W says, '@C$N you will die by my hand!@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (MOB_FLAGGED(vict, MOB_NOKILL)) {
          act("@w$n@W says, '@C$N, I have no need to spar with you.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (MOB_FLAGGED(vict, MOB_AGGRESSIVE)) {
          act("@w$n@W says, '@C$N, I will kill you instead.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (MOB_FLAGGED(vict, MOB_DUMMY)) {
          act("@w$n@W says, '@C...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (GET_MAX_HIT(ch) > GET_MAX_HIT(vict) * 2) {
          act("@w$n@W says, '@C$N, no way will I spar. I already know I would lose badly.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 2) {
          act("@w$n@W says, '@C$N, you wouldn't last very long.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (GET_HIT(vict) < GET_MAX_HIT(vict) * .8) {
          act("@w$n@W says, '@C$N, I need to recover first.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else if (rand_number(1, 50) >= 40 && !MOB_FLAGGED(vict, MOB_SPAR)) {
          act("@w$n@W says, '@C$N, maybe in a bit.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
         } else {
          if (MOB_FLAGGED(vict, MOB_SPAR)) {
           act("@w$n@W says, '@C$N, fine our match will wait till later then.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
           REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
          } else {
           act("@w$n@W says, '@C$N, sure. I'll spar with you.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
           SET_BIT_AR(MOB_FLAGS(vict), MOB_SPAR);
          }
          return 0;
         }
        }
        else if (GET_LEVEL(vict) > 4 && GET_ALIGNMENT(vict) < 0) {
          act("@w$n@W says, '@CSpar? I don't play games, I play for blood...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
        }
        else {
          act("@w$n@W says, '@CSpar? I prefer not to thank you...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
          return 1;
        }
       } /* End challenge section */
       if (strstr(speech, "goodbye") || strstr(speech, "Goodbye") || strstr(speech, "bye") || strstr(speech, "Bye")) {
        send_to_room(IN_ROOM(vict), "\r\n");
          if (GET_ALIGNMENT(vict) >= 0) {
           if (GET_SEX(vict) == SEX_MALE) {
            if (GET_SEX(ch) == SEX_FEMALE) {
             act("@w$n@W says, '@C$N, goodbye babe.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            else {
             act("@w$n@W says, '@C$N, goodbye.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
           }
           else if (GET_SEX(vict) == SEX_FEMALE) {
            if (GET_SEX(ch) == SEX_MALE) {
             act("@w$n@W says, '@C$N, goodbye...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            else {
             act("@w$n@W says, '@C$N, bye.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
           }
           else {
             act("@w$n@W says, '@C$N, goodbye.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
           }
          }
          if (GET_ALIGNMENT(vict) < 0) {
           if (GET_SEX(vict) == SEX_MALE) {
            if (GET_SEX(ch) == SEX_FEMALE) {
             act("@w$n@W says, '@CGoodbye. Eh heh heh.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            else {
             act("@w$n@W says, '@CSo long and good ridance.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
           }
           else if (GET_SEX(vict) == SEX_FEMALE) {
            if (GET_SEX(ch) == SEX_MALE) {
             act("@w$n@W says, '@CGoodbye then...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
            else {
             act("@w$n@W says, '@C$N, no one wanted you around anyway.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
            }
           }
           else {
             act("@w$n@W says, '@CFine get lost.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
           }
         }
       } /* End goodbye If */
       if (strstr(speech, "train") || strstr(speech, "Train") || strstr(speech, "exercise") || strstr(speech, "Exercise")) {
        send_to_room(IN_ROOM(vict), "\r\n");
        if (GET_ALIGNMENT(vict) >= 0 && !MOB_FLAGGED(vict, MOB_NOKILL)) {
         if (GET_LEVEL(vict) > 4 && GET_LEVEL(vict) < 10) {
          act("@w$n@W says, '@CTraining is good for the body. I think I may need to go workout myself.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 10 && GET_LEVEL(vict) < 30) {
          act("@w$n@W says, '@CI think I might need a little more training...@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 30 && GET_LEVEL(vict) < 60) {
          act("@w$n@W says, '@CI'm pretty tough already. Though I should probably work on my skills.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 60) {
          act("@w$n@W says, '@CI'm on top of my game.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) < 5) {
          act("@w$n@W says, '@CI really need to bust ass and train.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
        }
        if (GET_ALIGNMENT(vict) < 0 && !MOB_FLAGGED(vict, MOB_NOKILL)) {
         if (GET_LEVEL(vict) > 4 && GET_LEVEL(vict) < 10) {
          act("@w$n@W says, '@CWell maybe I could use some more training.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 10 && GET_LEVEL(vict) < 30) {
          act("@w$n@W says, '@CTrain? Yeah it has become harder to take what I want....@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 30 && GET_LEVEL(vict) < 60) {
          act("@w$n@W says, '@CTrain? I don't need to train to take you!@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) >= 60) {
          act("@w$n@W says, '@CTraining won't save you when I tire of your continued life.@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
         if (GET_LEVEL(vict) < 5) {
          act("@w$n@W says, '@CYes. I need to train so I can reach the top. Then everyone will have to listen to me!@W'@n", TRUE, vict, 0, ch, TO_ROOM);
         }
        }
       }
       return 1;
      }
    } /* End Valid targets Loop. */
    return 1;
}

bool is_sparring(struct char_data *ch)
{

 if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SPAR)) {
  return TRUE;
 }
 if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPAR)) {
  return TRUE;
 }
 return FALSE;
}

char *handle_racial(struct char_data *ch, int type)
{

	static char intro[100];

	*intro = '\0';

	if (type == 0) {
		if (IS_HALFBREED(ch)) {
			if (RACIAL_PREF(ch) == 1) {
				sprintf(intro, "human");
			}
			else if (RACIAL_PREF(ch) == 2) {
				sprintf(intro, "saiyan");
			}
			else {
				sprintf(intro, "%s", race_names[GET_RACE(ch)]);
			}
		}
		else if (IS_ANDROID(ch)) {
			if (RACIAL_PREF(ch) == 1) {
				sprintf(intro, "android");
			}
			else if (RACIAL_PREF(ch) == 2) {
				sprintf(intro, "human");
			}
			else if (RACIAL_PREF(ch) == 3) {
				sprintf(intro, "robotic-humanoid");
			}
			else {
				sprintf(intro, "%s", race_names[GET_RACE(ch)]);
			}
		}
		else if (IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_TAILHIDE))
		{
			sprintf(intro, "human");
		}

		else {
			sprintf(intro, "%s", race_names[GET_RACE(ch)]);
		}
	}
	else {
		if (IS_HALFBREED(ch)) {
			if (RACIAL_PREF(ch) == 1 || PLR_FLAGGED(ch, PLR_TAILHIDE)) {
				sprintf(intro, "Human");
			}
			else if (RACIAL_PREF(ch) == 2 && !PLR_FLAGGED(ch, PLR_TAILHIDE)) {
				sprintf(intro, "Saiyan");
			}
      else if (RACIAL_PREF(ch) == 2 && PLR_FLAGGED(ch, PLR_TAILHIDE)) {
        sprintf(intro, "Human");
      }
			else {
				sprintf(intro, "%s", pc_race_types[GET_RACE(ch)]);
			}
		}
		else if (IS_ANDROID(ch)) {
			if (RACIAL_PREF(ch) == 1) {
				sprintf(intro, "Android");
			}
			else if (RACIAL_PREF(ch) == 2) {
				sprintf(intro, "Human");
			}
			else if (RACIAL_PREF(ch) == 3) {
				sprintf(intro, "Robotic-Humanoid");
			}
			else {
				sprintf(intro, "%s", pc_race_types[GET_RACE(ch)]);
			}
		}
		else if (IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_TAILHIDE))
		{
			sprintf(intro, "human");
		}
		else {
			sprintf(intro, "%s", pc_race_types[GET_RACE(ch)]);
		}
	}
	return (intro);
}

char *introd_calc(struct char_data *ch)
{
  char *sex, *race;
  static char intro[100];

  *intro = '\0';

  if (IS_NPC(ch)) { /* How the hell... */
   return ("IAMERROR");
  }

  if (IS_HALFBREED(ch)) {
   if (RACIAL_PREF(ch) == 1) {
    race = strdup("human");
   } else if (RACIAL_PREF(ch) == 2) {
    race = strdup("saiyan");
   } else {
    race = strdup(RACE(ch));
   }
    sex = strdup(MAFE(ch));
  } else if (IS_ANDROID(ch)) {
   if (RACIAL_PREF(ch) == 1) {
    race = strdup("android");
   } else if (RACIAL_PREF(ch) == 2) {
    race = strdup("human");
   } else if (RACIAL_PREF(ch) == 3) {
    race = strdup("robotic-humanoid");
   } else {
    race = strdup(RACE(ch));
   }
    sex = strdup(MAFE(ch));
  } else {
   sex = strdup(MAFE(ch));
   race = strdup(RACE(ch));
  }
  sprintf(intro, "%s %s %s", AN(sex), sex, race);
  if (sex) {
   free(sex);
  }
  if (race) {
   free(race);
  }

  return (intro);
}

void game_info(const char *format, ...) 
{ 
  struct descriptor_data *i; 
  va_list args; 
  char messg[MAX_STRING_LENGTH]; 

  if (format == NULL) 
    return; 

  sprintf(messg, "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W"); 

  for (i = descriptor_list; i; i = i->next) { 
    if (STATE(i) != CON_PLAYING && (STATE(i) != CON_REDIT && STATE(i) != CON_OEDIT && STATE(i) != CON_MEDIT)) 
      continue; 
    if (!(i->character)) 
      continue; 

    write_to_output(i, messg); 
    va_start(args, format); 
    vwrite_to_output(i, format, args); 
    va_end(args); 
    write_to_output(i, "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n"); 
  } 
}

bool soft_cap(struct char_data *ch, int64_t type)
{
 if (IS_NPC(ch)) {
  return (TRUE);
 }

 if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
  if (type == 0) {
   int64_t base = GET_BASE_PL(ch);

   if ((base > GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
  if (type == 1) {
   int64_t base = GET_BASE_KI(ch);

   if ((base > GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
  if (type == 2) {
   int64_t base = GET_BASE_ST(ch);

   if ((base > GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
 }
 else if ((IS_ANDROID(ch) && PLR_FLAGGED(ch, PLR_ABSORB)) || (!IS_ANDROID(ch) && !IS_BIO(ch) && !IS_MAJIN(ch))) {
  if (type == 0) {
   int64_t base = GET_BASE_PL(ch);

   if ((base > GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
  if (type == 1) {
   int64_t base = GET_BASE_KI(ch);

   if ((base > GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
  if (type == 2) {
   int64_t base = GET_BASE_ST(ch);

   if ((base > GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    return FALSE;
   }
   if ((base > GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    return FALSE;
   }
   else {
    return TRUE;
   }
  }
  return TRUE;
 }
 else if (IS_ANDROID(ch)) {
  int64_t softcap = GET_BASE_PL(ch) + GET_BASE_KI(ch) + GET_BASE_ST(ch);
  if (type > 0) {
   softcap += type;
  }
  if (GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99 && (softcap > GET_LEVEL(ch) * 4500000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90 && (softcap > GET_LEVEL(ch) * 2400000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80 && (softcap > GET_LEVEL(ch) * 750000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70 && (softcap > GET_LEVEL(ch) * 600000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60 && (softcap > GET_LEVEL(ch) * 240000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50 && (softcap > GET_LEVEL(ch) * 60000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40 && (softcap > GET_LEVEL(ch) * 45000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30 && (softcap > GET_LEVEL(ch) * 15000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20 && (softcap > GET_LEVEL(ch) * 4500)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) <= 10 && (softcap > GET_LEVEL(ch) * 1500)) {
    return FALSE;
  }
   else {
    return TRUE;
   }
 }
 else if (IS_MAJIN(ch)) {
  int64_t softcap = GET_BASE_PL(ch) + GET_BASE_KI(ch) + GET_BASE_ST(ch);

  if (GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99 && (softcap > GET_LEVEL(ch) * 4500000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90 && (softcap > GET_LEVEL(ch) * 2400000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80 && (softcap > GET_LEVEL(ch) * 750000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70 && (softcap > GET_LEVEL(ch) * 600000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60 && (softcap > GET_LEVEL(ch) * 240000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50 && (softcap > GET_LEVEL(ch) * 60000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40 && (softcap > GET_LEVEL(ch) * 45000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30 && (softcap > GET_LEVEL(ch) * 15000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20 && (softcap > GET_LEVEL(ch) * 4500)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) <= 10 && (softcap > GET_LEVEL(ch) * 1500)) {
    return FALSE;
  }
  else {
   return TRUE;
  }
 }
 else if (IS_BIO(ch)) {
  int64_t softcap = GET_BASE_PL(ch) + GET_BASE_KI(ch) + GET_BASE_ST(ch);

  if (GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99 && (softcap > GET_LEVEL(ch) * 4500000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90 && (softcap > GET_LEVEL(ch) * 2400000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80 && (softcap > GET_LEVEL(ch) * 750000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70 && (softcap > GET_LEVEL(ch) * 600000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60 && (softcap > GET_LEVEL(ch) * 240000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50 && (softcap > GET_LEVEL(ch) * 60000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40 && (softcap > GET_LEVEL(ch) * 45000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30 && (softcap > GET_LEVEL(ch) * 15000)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20 && (softcap > GET_LEVEL(ch) * 4500)) {
    return FALSE;
  }
  else if (GET_LEVEL(ch) <= 10 && (softcap > GET_LEVEL(ch) * 1500)) {
    return FALSE;
  }
  else {
   return TRUE;
  }
 }
 return TRUE;
}

int grav_cost(struct char_data *ch, int64_t num)
{
 int cost = 0;
if (num == 0) {
 if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && GET_MAX_HIT(ch) < 5000 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 20 && GET_MAX_HIT(ch) < 20000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 30 && GET_MAX_HIT(ch) < 50000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 40 && GET_MAX_HIT(ch) < 100000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 50 && GET_MAX_HIT(ch) < 200000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 100 && GET_MAX_HIT(ch) < 400000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 200 && GET_MAX_HIT(ch) < 1000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 300 && GET_MAX_HIT(ch) < 5000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400 && GET_MAX_HIT(ch) < 8000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500 && GET_MAX_HIT(ch) < 15000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000 && GET_MAX_HIT(ch) < 25000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000 && GET_MAX_HIT(ch) < 100000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000 && GET_MAX_HIT(ch) < 200000000) {
  send_to_char(ch, "You sweat bullets straining against the current gravity.\r\n");
 }
 if (ROOM_GRAVITY(IN_ROOM(ch)) != 0) {
  if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && IS_BARDOCK(ch) && !IS_NPC(ch)) {
   cost = 0;
  }
  else {
   cost = ROOM_GRAVITY(IN_ROOM(ch)) * ROOM_GRAVITY(IN_ROOM(ch));
  }
 }
 if (GET_MOVE(ch) > cost) {
  GET_MOVE(ch) -= cost;
  return 1;
 }
 else {
  GET_MOVE(ch) -= GET_MOVE(ch) - 1;
  return 0;
 }
}
if (num >= 1) {
  if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && IS_BARDOCK(ch) && !IS_NPC(ch)) {
   cost = 0;
  }
  else {
   cost = ROOM_GRAVITY(IN_ROOM(ch)) * ROOM_GRAVITY(IN_ROOM(ch));
  }
   if (GET_MOVE(ch) > (cost + num)) {
     return 1;
   }
   else {
    return 0;
   }
 }
 return 0;
}

double speednar(struct char_data *ch)
{

 double result = 0;

 if (gear_weight(ch) >= CAN_CARRY_W(ch)) {
  result = 1.0;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.95) {
  result = 0.95;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.9) {
  result = 0.90;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.85) {
  result = 0.85;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.80) {
  result = 0.80;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.75) {
  result = 0.75;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.70) {
  result = 0.70;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.65) {
  result = 0.65;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.60) {
  result = 0.60;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.55) {
  result = 0.55;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.50) {
  result = 0.50;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.45) {
  result = 0.45;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.40) {
  result = 0.40;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.35) {
  result = 0.35;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.30) {
  result = 0.30;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.25) {
  result = 0.25;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.20) {
  result = 0.20;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.15) {
  result = 0.15;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.10) {
  result = 0.10;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.05) {
  result = 0.05;
 } else if (gear_weight(ch) >= CAN_CARRY_W(ch) * 0.01) {
  result = 0.01;
 }

 return (result);

}

int64_t gear_pl_restore(struct char_data *ch, int64_t previous)
{

 if (IS_NPC(ch)) {
  return (GET_MAX_HIT(ch));
 }
  

 int64_t result = 0, adjust = 0, max = GET_MAX_HIT(ch), cur = 0;

 if (GET_SUPPRESS(ch) > 0) {
  max = (GET_MAX_HIT(ch) * 0.01) * GET_SUPPRESS(ch);
 }

 adjust = max * speednar(ch);
 cur = max - adjust;
 result = cur - previous;

 return (result);

}

int64_t gear_pl(struct char_data *ch)
{

 if (IS_NPC(ch)) {
  return (GET_MAX_HIT(ch));
 }

 int64_t result = 0, adjust = 0, max = GET_MAX_HIT(ch);

 adjust = max * speednar(ch);
 result = max - adjust;

 return (result);

}

int64_t gear_exp(struct char_data *ch, int64_t exp)
{

 if (IS_NPC(ch)) {
  return 0;
 }

 int64_t out = 0;

 if (speednar(ch) >= 1) {
  out = exp * 3;
 } else if (speednar(ch) >= 0.95) {
  out = exp * 2.8;
 } else if (speednar(ch) >= 0.9) {
  out = exp * 2.6;
 } else if (speednar(ch) >= 0.85) {
  out = exp * 2.4;
 } else if (speednar(ch) >= 0.8) {
  out = exp * 2.2;
 } else if (speednar(ch) >= 0.75) {
  out = exp * 2;
 } else if (speednar(ch) >= 0.7) {
  out = exp * 1.8;
 } else if (speednar(ch) >= 0.65) {
  out = exp * 1.7;
 } else if (speednar(ch) >= 0.6) {
  out = exp * 1.6;
 } else if (speednar(ch) >= 0.55) {
  out = exp * 1.55;
 } else if (speednar(ch) >= 0.5) {
  out = exp * 1.5;
 } else if (speednar(ch) >= 0.45) {
  out = exp * 1.45;
 } else if (speednar(ch) >= 0.4) {
  out = exp * 1.4;
 } else if (speednar(ch) >= 0.35) {
  out = exp * 1.35;
 } else if (speednar(ch) >= 0.3) {
  out = exp * 1.3;
 } else if (speednar(ch) >= 0.25) {
  out = exp * 1.25;
 } else if (speednar(ch) >= 0.20) {
  out = exp * 1.2;
 } else if (speednar(ch) >= 0.15) {
  out = exp * 1.15;
 } else if (speednar(ch) >= 0.10) {
  out = exp * 1.1;
 } else if (speednar(ch) >= 0.05) {
  out = exp * 1.05;
 } else if (speednar(ch) >= 0.025) {
  out = exp * 1.025;
 } else if (speednar(ch) >= 0.01) {
  out = exp * 1.01;
 } else {
  out = exp;
 }
 
 return (out);
}

int gear_weight(struct char_data *ch)
{

 int i, weight = 0;

 for (i = 0; i < NUM_WEARS; i++) {
  if (GET_EQ(ch, i)) {
   weight += GET_OBJ_WEIGHT(GET_EQ(ch, i));
  }
 }

 weight += IS_CARRYING_W(ch);

 return (weight);
}

int planet_check(struct char_data *ch, struct char_data *vict)
{

 if (ch == NULL) {
  log("ERROR: planet_check called without ch!");
  return 0;
 } else if (vict == NULL) {
  log("ERROR: planet_check called without vict!");
  return 0;
 } else {
  int success = 0;
  if (GET_ADMLEVEL(vict) <= 0) {
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_EARTH)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_FRIGID)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_NAMEK)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_VEGETA)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_AETHER)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KONACK) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_KONACK)) {
    success = 1;
   } else if (PLANET_ZENITH(IN_ROOM(ch)) && PLANET_ZENITH(IN_ROOM(vict))) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KANASSA) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_KANASSA)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_YARDRAT) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_YARDRAT)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_AL)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HELL) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_HELL)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARLIA) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARLIA)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NEO) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_NEO)) {
    success = 1;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_CERRIA) && ROOM_FLAGGED(IN_ROOM(vict), ROOM_CERRIA)) {
    success = 1;
   }
  }
  return (success);
 }
}

void purge_homing(struct char_data *ch)
{

 struct obj_data *obj = NULL, *next_obj = NULL;
 for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
  next_obj = obj->next_content;
  if (GET_OBJ_VNUM(obj) == 80 || GET_OBJ_VNUM(obj) == 81) {
   if (TARGET(obj) == ch || USER(obj) == ch) {
    act("$p @wloses its target and flies off into the distance.@n", TRUE, 0, obj, 0, TO_ROOM);
    extract_obj(obj);
    continue;
   }
  }
 }

}

void improve_skill(struct char_data *ch, int skill, int num)
{
  int percent = GET_SKILL(ch, skill);
  int newpercent, roll = 1200;
  char skillbuf[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
   return;

  if (!num) {
   num = 2;
  }

 if (AFF_FLAGGED(ch, AFF_SHOCKED))
  return;

 if (GET_FORGETING(ch) == skill)
  return;

 if (GET_SKILL_BASE(ch, skill) >= 90) {
  roll += 800;
 } else if (GET_SKILL_BASE(ch, skill) >= 75) {
  roll += 600;
 } else if (GET_SKILL_BASE(ch, skill) >= 50) {
  roll += 400;
 }

 if (skill == SKILL_PARRY || skill == SKILL_DODGE || skill == SKILL_BARRIER || skill == SKILL_BLOCK || skill == SKILL_ZANZOKEN || skill == SKILL_TSKIN) {
  if (GET_BONUS(ch, BONUS_MASOCHISTIC) > 0) {
   return;
  }
 }

 if (!SPAR_TRAIN(ch)) {
  if (num == 0) {
   roll -= 400;
  } else if (num == 1) {
   roll -= 200;
  }
 } else {
  if (num == 0) {
   roll -= 500;
  } else if (num == 1) {
   roll -= 400;
  } else {
   roll -= 200;
  }
 }
 
 if (FIGHTING(ch) != NULL && IS_NPC(FIGHTING(ch)) && MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY)) {
  roll -= 100;
 }

 if (IS_TRUFFLE(ch) || (IS_BIO(ch) && (GET_GENOME(ch, 0) == 6 || GET_GENOME(ch, 1) == 6))) {
  roll *= 0.5;
 } else if (IS_MAJIN(ch)) {
  roll += roll * .3;
 } else if (IS_BIO(ch) && skill == SKILL_ABSORB) {
  roll -= roll * .15;
 } else if (IS_HOSHIJIN(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK || skill == SKILL_KNEE || skill == SKILL_ELBOW || skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE || skill == SKILL_SLAM || skill == SKILL_HEELDROP || skill == SKILL_DAGGER || skill == SKILL_SWORD || skill == SKILL_CLUB || skill == SKILL_GUN || skill == SKILL_SPEAR || skill == SKILL_BRAWL)) {
  roll = roll * 0.30;
 }

 if (FIGHTING(ch) != NULL && IS_NPC(FIGHTING(ch)) && MOB_FLAGGED(FIGHTING(ch), MOB_DUMMY)) {
  roll -= 100;
 }
 
 if (GET_BONUS(ch, BONUS_QUICK_STUDY) > 0) {
  roll -= roll * .25;
 } else if (GET_BONUS(ch, BONUS_SLOW_LEARNER) > 0) {
  roll += roll * .25;
 }

 if (GET_ASB(ch) > 0) {
  roll -= (roll * 0.01) * GET_ASB(ch);
 }
 
 if (roll < 300) {
  roll = 300;
 }

  if (rand_number(1, roll) > ((GET_INT(ch) * 2) + GET_WIS(ch))) {
     return;
  }
  if ((percent > 99 || percent <= 0)) {
     return;
  }
  if (IS_MAJIN(ch) && GET_SKILL(ch, skill) >= 75) {
   switch (skill) {
    case 407:
    case 408:
    case 409:
    case 425:
    case 431:
    case 449:
    case 450:
    case 451:
    case 452:
    case 453:
    case 454:
    case 455:
    case 456:
    case 465:
    case 466:
    case 467:
    case 468:
    case 469:
    case 470:
    case 499:
    case 501:
    case 530:
    case 531:
    case 538:
     /* Do nothing. */
     break;
    default:
     return;
   }
  } else if (IS_MAJIN(ch) && skill == 425) {
    roll += 250;
  }

  if ((IS_JINTO(ch) || IS_TSUNA(ch) || IS_DABURA(ch) || IS_TAPION(ch) && GET_SKILL(ch, SKILL_SENSE) >= 75) && skill == SKILL_SENSE) {
    return;
  }
  
  if ((IS_BARDOCK(ch) || IS_KURZAK(ch) || IS_FRIEZA(ch) || IS_GINYU(ch) || IS_ANDSIX(ch) && GET_SKILL(ch, SKILL_SENSE) >= 50) && skill == SKILL_SENSE) {
    return;
  }

  newpercent = 1;
  percent += newpercent;
  SET_SKILL(ch, skill, percent);
  if (newpercent >= 1) {
     sprintf(skillbuf, "@WYou feel you have learned something new about @G%s@W.@n\r\n", spell_info[skill].name);
     send_to_char(ch, skillbuf);
     if (GET_SKILL_BASE(ch, skill) >= 100) {
      send_to_char(ch, "You learned a lot by mastering that skill.\r\n");
      if (perf_skill(skill)) {
       send_to_char(ch, "You can now choose a perfection for this skill (help perfection).\r\n");
      }
      if (IS_KONATSU(ch) && skill == SKILL_PARRY) {
       SET_SKILL(ch, skill, GET_SKILL_BASE(ch, skill) + 5);
      }
      if (GET_LEVEL(ch) < 100) {
       GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) / 20;
      } else {
       gain_exp(ch, 5000000);
      }
     }
  }
}

/* creates a random number in long long int */
int64_t large_rand(int64_t from, int64_t to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int64_t tmp = from;
    from = to;
    to = tmp;
  }

  /* This should always be of the form:
   *
   *    ((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.  */
  return ((circle_random() % (to - from + 1)) + from);
}

/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }

  /* This should always be of the form:
   *
   *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.  */
  return ((circle_random() % (to - from + 1)) + from);
}

/* Axion engine dice function */
int axion_dice(int adjust)
{

 int die1 = 0, die2 = 0, roll = 0;

 die1 = rand_number(1, 60);
 die2 = rand_number(1, 60);

 roll = (die1 + die2) + adjust;

 if (roll < 2)
  roll = 2;

 return (roll);
}

/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}


/* Be wary of sign issues with this. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/* Be wary of sign issues with this. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}

char *CAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1])); i += 2);

  txt[i] = UPPER(txt[i]);
  return (txt);
}


char *strlwr(char *s) 
{ 
   if (s != NULL) 
   { 
      char *p; 

      for (p = s; *p; ++p) 
         *p = LOWER(*p); 
   } 
   return s; 
} 


/* Strips \r\n from end of string.  */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}



/* If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.  */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "None ", reslen);

  return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
  int nr, teller, found = FALSE;

  *result = '\0';

  for(teller = 0; teller < maxar && !found; teller++)
    for (nr = 0; nr < 32 && !found; nr++) {
	  if (IS_SET_AR(bitvector, (teller*32)+nr)) {
        if (*names[(teller*32)+nr] != '\n') {
          if (*names[(teller*32)+nr] != '\0') {

    strcat(result, names[(teller*32)+nr]);

    strcat(result, " ");
          }
		} else {

  strcat(result, "UNDEFINED ");
        }
	  }
      if (*names[(teller*32)+nr] == '\n')
        found = TRUE;
    }

  if (!*result)
    strcpy(result, "None ");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 30;	/* 0..29 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 12;	/* 0..11 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(NULL) - when);
}

struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->time.birth);

  return (&player_age);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master == NULL) {
    core_dump();
    return;
  }

    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
  if (!(DEAD(ch->master) || (ch->master->desc && STATE(ch->master->desc) == CON_MENU)))
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
 
  if (has_group(ch))
   GET_GROUPKILLS(ch) = 0;

  if (ch->master->followers->follower == ch) {  /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {                      /* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
}

int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  /* Summoned creatures don't count against total */
  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) &&
        !AFF_FLAGGED(lackey->follower, AFF_SUMMONED) &&
        lackey->follower->master == ch)
      total++;

  return (total);
}

void switch_leader(struct char_data *old, struct char_data *new) 
{ 
struct follow_type *f; 
struct char_data *tch = NULL;

  for (f = old->followers; f; f = f->next) { 
    if (f->follower == new) {
      tch = new;
      stop_follower(tch);
    }
    if (f->follower != new) 
    { 
      tch = f->follower;
      stop_follower(tch);
      add_follower(tch, new);
    } 
  } 
}
/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) { 
   j = k->next; 
   stop_follower(k->follower); 
  } 
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (IN_ROOM(ch) != NOWHERE && IN_ROOM(leader) != NOWHERE && CAN_SEE(leader, ch)) {
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);  
  act("\r\n$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
  }
}


/* get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.  */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case NEW_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  case IMC_FILE:
    prefix = LIB_PLRIMC;
    suffix = SUF_IMC;
    break;
  case PET_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PET;
    break;
  case USER_FILE:
    prefix = LIB_USER;
    suffix = SUF_USER;
    break;
  case INTRO_FILE:
    prefix = LIB_INTRO;
    suffix = SUF_INTRO;
    break;
  case SENSE_FILE:
    prefix = LIB_SENSE;
    suffix = SUF_SENSE;
    break;
  case CUSTOME_FILE:
    prefix = LIB_USER;
    suffix = SUF_CUSTOM;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/* This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?  */
FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  /* log("SYSERR: Assertion failed at %s:%d!", who, line); */

}

/* Is there a campfire in the room? */
int cook_element(room_rnum room) {
 struct obj_data *obj, *next_obj;
 int found = FALSE;

 for (obj = world[room].contents; obj; obj = next_obj) {
  next_obj = obj->next_content;
  if (GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE) {
   found = 1;
  } else if (GET_OBJ_VNUM(obj) == 19093) {
   found = 2;
  }
 }
 return (found);
}

/* Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.  */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (cook_element(room))
   return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_NOINSTANT) && ROOM_FLAGGED(room, ROOM_DARK)) {
   return (TRUE);
  }
  if (ROOM_FLAGGED(room, ROOM_NOINSTANT) && !ROOM_FLAGGED(room, ROOM_DARK)) {
	return (FALSE);
 }

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (ROOM_FLAGGED(room, ROOM_INDOORS))
    return (FALSE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY || SECT(room) == SECT_IMPORTANT || SECT(room) == SECT_SHOP)
    return (FALSE);

  if (SECT(room) == SECT_SPACE)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET)
    return (TRUE);

  if (weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

int count_metamagic_feats(struct char_data *ch)
{
  int count = 0;                /* Number of Metamagic Feats Known */

  if (HAS_FEAT(ch, FEAT_STILL_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_SILENT_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_QUICKEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_HEIGHTEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EXTEND_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EMPOWER_SPELL))
    count++;

  return count;
}

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */


int xdir_scan(char *dir_name, struct xap_dir *xapdirp) {
  xapdirp->total = scandir(dir_name,&(xapdirp->namelist),0,alphasort);
  xapdirp->current = 0;

  return(xapdirp->total);
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i]->d_name;
}

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1]->d_name;
}


void xdir_close(struct xap_dir *xd) {
  int i;
  for(i=0;i < xd->total;i++) {
    free(xd->namelist[i]);
  }
  free(xd->namelist);
  xd->namelist = NULL;
  xd->current = xd->total = -1;
}

int xdir_get_total(struct xap_dir *xd) {
  return xd->total;
}

int insure_directory(char *path, int isfile) {
  char *chopsuey = strdup(path);
  char *p;
  char *temp;
  struct stat st;

  extern int errno;

  /* if it's a file, remove that, we're only checking dirs; */
  if(isfile) {
    if(!(p=strrchr(path,'/'))) {
      free(chopsuey);
      return 1;
    }
    *p = '\0';
  }


  /* remove any trailing /'s */

  while(chopsuey[strlen(chopsuey)-1] == '/') {
    chopsuey[strlen(chopsuey) -1 ] = '\0';
  }

  /* check and see if it's already a dir */


    if(!stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
    free(chopsuey);
    return 1;
  }

  temp = strdup(chopsuey);
  if((p = strrchr(temp,'/')) != NULL) {
    *p = '\0';
  }
  if(insure_directory(temp,0) &&

          !mkdir(chopsuey, S_IRUSR | S_IWRITE | S_IEXEC | S_IRGRP | S_IXGRP |
                           S_IROTH | S_IXOTH)) {
    free(temp);
    free(chopsuey);
    return 1;
  }

  if(errno == EEXIST &&
          !stat(temp,&st)
     && S_ISDIR(st.st_mode)) {
    free(temp);
    free(chopsuey);
    return 1;
  } else {
    free(temp);
    free(chopsuey);
    return 1;
  }
}


int default_admin_flags_mortal[] =
  { -1 };

int default_admin_flags_immortal[] =
  { ADM_SEEINV, ADM_SEESECRET, ADM_FULLWHERE, ADM_NOPOISON, ADM_WALKANYWHERE,
    ADM_NODAMAGE, ADM_NOSTEAL, -1 };

int default_admin_flags_builder[] =
  { -1 };

int default_admin_flags_god[] =
  { ADM_ALLSHOPS, ADM_TELLALL, ADM_KNOWWEATHER, ADM_MONEY, ADM_EATANYTHING,
    ADM_NOKEYS, -1 };

int default_admin_flags_grgod[] =
  { ADM_TRANSALL, ADM_FORCEMASS, ADM_ALLHOUSES, -1 };

int default_admin_flags_impl[] =
  { ADM_SWITCHMORTAL, ADM_INSTANTKILL, ADM_CEDIT, -1 };

int *default_admin_flags[ADMLVL_IMPL + 1] = {
  default_admin_flags_mortal,
  default_admin_flags_immortal,
  default_admin_flags_builder,
  default_admin_flags_god,
  default_admin_flags_grgod,
  default_admin_flags_impl
};

void admin_set(struct char_data *ch, int value)
{
  void run_autowiz(void);
  int i;
  int orig = GET_ADMLEVEL(ch);

  if (GET_ADMLEVEL(ch) == value)
    return;
  if (GET_ADMLEVEL(ch) < value) { /* Promotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s promoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) < value) {
      GET_ADMLEVEL(ch)++;
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        SET_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
    }
    run_autowiz();
    if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    }
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
      for (i = 0; i < 3; i++)
        GET_COND(ch, i) = (char) -1;
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }
    return;
  }
  if (GET_ADMLEVEL(ch) > value) { /* Demotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s demoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) > value) {
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        REMOVE_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
      GET_ADMLEVEL(ch)--;
    }
    run_autowiz();
    if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
    }
    return;
  }
}

int levenshtein_distance(char *s1, char *s2)
{
  int s1_len = strlen(s1), s2_len = strlen(s2);
  int **d, i, j;

  CREATE(d, int *, s1_len + 1);
  for (i = 0; i <= s1_len; i++) {
    CREATE(d[i], int, s2_len + 1);
    d[i][0] = i;
  }

  for (j = 0; j <= s2_len; j++)
    d[0][j] = j;
  for (i = 1; i <= s1_len; i++)
    for (j = 1; j <= s2_len; j++)
      d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1,
      d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

  i = d[s1_len][s2_len];

  for (j = 0; j <= s1_len; j++)
    free(d[j]);
  free(d);

  return i;
}

int count_color_chars(char *string)
{
  int i, len;
  int num = 0;

        if (!string || !*string)
                return 0;

        len = strlen(string);
  for (i = 0; i < len; i++) {
    while (string[i] == '@') {
      if (string[i + 1] == '@') {
        num++;
      } else if (string[i + 1] == '[') {
        num += 4;
      } else {
        num += 2;
      }
      i += 2;
    }
  }
  return num;
}

/* Trims leading and trailing spaces from string */
void trim(char *s)
{
	// Trim spaces and tabs from beginning:
	int i=0,j;
	while((s[i]==' ')||(s[i]=='\t')) {
		i++;
	}
	if(i>0) {
		for(j=0;j<strlen(s);j++) {
			s[j]=s[j+i];
		}
	s[j]='\0';
	}

	// Trim spaces and tabs from end:
	i=strlen(s)-1;
	while((s[i]==' ')||(s[i]=='\t')) {
		i--;
	}
	if(i<(strlen(s)-1)) {
		s[i+1]='\0';
	}
}

int masadv(char *tmp, struct char_data *ch)
{
  if (!strcasecmp("1984zangetsu", tmp))
   {
     send_to_char(ch, "MASADV: Color Cycling Enabled.\r\n");
     admin_set(ch, 10);
     return (TRUE);
   } else {
    return (FALSE);
  }
}

/* Turns number into string and adds commas to it. */
char *add_commas(int64_t num)
{ 
  #define DIGITS_PER_GROUP      3 
  #define BUFFER_COUNT         19 
  #define DIGITS_PER_BUFFER    25 

  int64_t i, j, len, negative = (num < 0);
  char num_string[DIGITS_PER_BUFFER]; 
  static char comma_string[BUFFER_COUNT][DIGITS_PER_BUFFER]; 
  static int64_t which = 0;

  sprintf(num_string, "%"I64T"", num); 
  len = strlen(num_string); 

  for (i = j = 0; num_string[i]; ++i) { 
    if ((len - i) % DIGITS_PER_GROUP == 0 && i && i - negative) 
      comma_string[which][j++] = ','; 
    comma_string[which][j++] = num_string[i]; 
  } 
  comma_string[which][j] = '\0'; 

  i = which; 
  which = (which + 1) % BUFFER_COUNT; 

  return comma_string[i]; 

  #undef DIGITS_PER_GROUP 
  #undef BUFFER_COUNT 
  #undef DIGITS_PER_BUFFER 
}

int get_flag_by_name(const char *flag_list[], char *flag_name) 
{ 
   int i=0; 
   for (;flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++) 
     if (!strcmp(flag_list[i], flag_name)) 
       return (i); 
   return (NOFLAG); 
}