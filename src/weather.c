/* ************************************************************************
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "weather.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "dg_comm.h"
#include "act.other.h"
#include "config.h"

static void another_hour(int mode);
static void weather_change(void);
static void phase_powerup(struct char_data *ch, int type, int phase);
static void grow_plants(void);


static void grow_plants(void)
{
  struct obj_data *k;

 for (k = object_list; k; k = k->next) {
  if (IN_ROOM(k) == NOWHERE) {
   continue;
  } else if (k->carried_by || k->in_obj) {
   continue;
  } else if (ROOM_FLAGGED(IN_ROOM(k), ROOM_GARDEN1) || ROOM_FLAGGED(IN_ROOM(k), ROOM_GARDEN2)) {
   if (GET_OBJ_VAL(k, VAL_WATERLEVEL) < 0 && GET_OBJ_VAL(k, VAL_WATERLEVEL) > -10) {
    GET_OBJ_VAL(k, VAL_WATERLEVEL) -= 1;
    if (GET_OBJ_VAL(k, VAL_WATERLEVEL) > -10) {
     send_to_room(IN_ROOM(k), "%s@y withers a bit.\r\n", k->short_description);
    } else {
     send_to_room(IN_ROOM(k), "%s@y has withered to a dried up dead husk.\r\n", k->short_description);
    }    
   } else if (GET_OBJ_VAL(k, VAL_WATERLEVEL) >= 0) {
    GET_OBJ_VAL(k, VAL_WATERLEVEL) -= 1;
    if (GET_OBJ_VAL(k, VAL_GROWTH) < GET_OBJ_VAL(k, VAL_MATGOAL) && GET_OBJ_VAL(k, VAL_MATURITY) < GET_OBJ_VAL(k, VAL_MAXMATURE)) {
      GET_OBJ_VAL(k, VAL_GROWTH) += 1;
     if (GET_OBJ_VAL(k, VAL_GROWTH) >= GET_OBJ_VAL(k, VAL_MATGOAL)) {
      GET_OBJ_VAL(k, VAL_GROWTH) = 0;
      GET_OBJ_VAL(k, VAL_MATURITY) += 1;
     }
     if (GET_OBJ_VAL(k, VAL_MATURITY) >= GET_OBJ_VAL(k, VAL_MAXMATURE)) {
      send_to_room(IN_ROOM(k), "%s@G is now fully grown!@n\r\n", k->short_description);
     }
    }
   }
  }
 }

}

void weather_and_time(int mode)
{
  another_hour(mode);
  grow_plants();
  if (mode)
    weather_change();
}


static void another_hour(int mode)
{
  time_info.hours++;

  if (mode) {
    switch (time_info.hours) {
    case 4:
      if (MOON_DATE) {
       send_to_moon("The full moon disappears.\r\n");
       MOON_UP = FALSE;
       oozaru_drop(NULL);
      }
      else if (time_info.day == 22) {
        send_to_moon("The full moon disappears.\r\n");
        MOON_UP = FALSE;
        oozaru_drop(NULL);
      }
      break;
    case 5:
      weather_info.sunlight = SUN_RISE;
      send_to_outdoor("The sun rises in the east.\r\n");
       if (time_info.day <= 14) {
        star_phase(NULL, 1);
       } else if (time_info.day <= 21) {
        star_phase(NULL, 2);
       } else {
        star_phase(NULL, 0);
       }
      break;
    case 6:
      weather_info.sunlight = SUN_LIGHT;
      send_to_outdoor("The day has begun.\r\n");
      break;
    case 19:
      weather_info.sunlight = SUN_SET;
      send_to_outdoor("The sun slowly disappears in the west.\r\n");
      break;
    case 20:
      weather_info.sunlight = SUN_DARK;
      send_to_outdoor("The night has begun.\r\n");
      break;
    case 21:
      if (MOON_DATE) {
       send_to_moon("The full moon has risen.\r\n");
       MOON_UP = TRUE;
       oozaru_add(NULL);
      }
      break;
    default:
      break;
    }
  }
  if (time_info.hours > 23) {	/* Changed by HHS due to bug ??? */
    time_info.hours -= 24;
    time_info.day++;

    if (time_info.day > 29) {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month > 11) {
	time_info.month = 0;
	time_info.year++;
      }
    }
  }
}


static void weather_change(void)
{
  int diff, change;
  if ((time_info.month >= 9) && (time_info.month <= 16))
    diff = (weather_info.pressure > 985 ? -2 : 2);
  else
    diff = (weather_info.pressure > 1015 ? -2 : 2);

  weather_info.change += (dice(1, 4) * diff + dice(2, 6) - dice(2, 6));

  weather_info.change = MIN(weather_info.change, 12);
  weather_info.change = MAX(weather_info.change, -12);

  weather_info.pressure += weather_info.change;

  weather_info.pressure = MIN(weather_info.pressure, 1040);
  weather_info.pressure = MAX(weather_info.pressure, 960);

  change = 0;

  switch (weather_info.sky) {
  case SKY_CLOUDLESS:
    if (weather_info.pressure < 990)
      change = 1;
    else if (weather_info.pressure < 1010)
      if (dice(1, 4) == 1)
	change = 1;
    break;
  case SKY_CLOUDY:
    if (weather_info.pressure < 970)
      change = 2;
    else if (weather_info.pressure < 990) {
      if (dice(1, 4) == 1)
	change = 2;
      else
	change = 0;
    } else if (weather_info.pressure > 1030)
      if (dice(1, 4) == 1)
	change = 3;

    break;
  case SKY_RAINING:
    if (weather_info.pressure < 970) {
      if (dice(1, 4) == 1)
	change = 4;
      else
	change = 0;
    } else if (weather_info.pressure > 1030)
      change = 5;
    else if (weather_info.pressure > 1010)
      if (dice(1, 4) == 1)
	change = 5;

    break;
  case SKY_LIGHTNING:
    if (weather_info.pressure > 1010)
      change = 6;
    else if (weather_info.pressure > 990)
      if (dice(1, 4) == 1)
	change = 6;

    break;
  default:
    change = 0;
    weather_info.sky = SKY_CLOUDLESS;
    break;
  }

  switch (change) {
  case 0:
    break;
  case 1:
    send_to_outdoor("The sky starts to get cloudy.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 2:
    send_to_outdoor("It starts to rain.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  case 3:
    send_to_outdoor("The clouds disappear.\r\n");
    weather_info.sky = SKY_CLOUDLESS;
    break;
  case 4:
    send_to_outdoor("Lightning starts to show in the sky.\r\n");
    weather_info.sky = SKY_LIGHTNING;
    break;
  case 5:
    send_to_outdoor("The rain stops.\r\n");
    weather_info.sky = SKY_CLOUDY;
    break;
  case 6:
    send_to_outdoor("The lightning stops.\r\n");
    weather_info.sky = SKY_RAINING;
    break;
  default:
    break;
  }
}

void oozaru_add(struct char_data *tch)
{
 struct descriptor_data *d;
if (tch == NULL) {
 for (d = descriptor_list; d; d = d->next) {
  if (!IS_PLAYING(d)) {
   continue;
  }
  if (MOON_OK(d->character) && !PLR_FLAGGED(d->character, PLR_OOZARU)) {
   act("@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Hair sprouts  all over your body and your teeth become sharp as your body takes on the Oozaru form!@n", TRUE, d->character, 0, 0, TO_CHAR);
   act("@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant ape!@n", TRUE, d->character, 0, 0, TO_ROOM);
   if (GET_KAIOKEN(d->character) > 0) {
    do_kaioken(d->character, "0", 0, 0);
   }
   SET_BIT_AR(PLR_FLAGS(d->character), PLR_OOZARU);
        int add = 10000;
        int mult = 2;

        /* handle pl */
        GET_MAX_HIT(d->character) = (GET_BASE_PL(d->character) + add) * mult;
        if ((GET_HIT(d->character) + add) * mult <= GET_MAX_HIT(d->character)) {
         GET_HIT(d->character) = (GET_HIT(d->character) + add) * mult;
        }
        else if ((GET_HIT(d->character) + add) * mult > GET_MAX_HIT(d->character)) {
         GET_HIT(d->character) = GET_MAX_HIT(d->character);
        }

        /* handle ki */
        GET_MAX_MANA(d->character) = (GET_BASE_KI(d->character) + add) * mult;
        if ((GET_MANA(d->character) + add) * mult <= GET_MAX_MANA(d->character)) {
         GET_MANA(d->character) = (GET_MANA(d->character) + add) * mult;
        }
        else if ((GET_MANA(d->character) + add) * mult > GET_MAX_MANA(d->character)) {
         GET_MANA(d->character) = GET_MAX_MANA(d->character);
        }

        /* handle st */
        GET_MAX_MOVE(d->character) = (GET_BASE_ST(d->character) + add) * mult;
        if ((GET_MOVE(d->character) + add) * mult <= GET_MAX_MOVE(d->character)) {
         GET_MOVE(d->character) = (GET_MOVE(d->character) + add) * mult;
        }
        else if ((GET_MOVE(d->character) + add) * mult > GET_MAX_MOVE(d->character)) {
         GET_MOVE(d->character) = GET_MAX_MOVE(d->character);
        }
   continue;
  }
 }
 }
 else {
  if (MOON_OK(tch) && !PLR_FLAGGED(tch, PLR_OOZARU)) {
   act("@rLooking up at the moon your heart begins to beat loudly. Sudden rage begins to fill your mind while your body begins to grow. Hair sprouts  all over your body and your teeth become sharp as your body takes on the Oozaru form!@n", TRUE, tch, 0, 0, TO_CHAR);
   act("@R$n@r looks up at the moon as $s eyes turn red and $s heart starts to beat loudly. Hair starts to grow all over $s body as $e starts screaming. The scream turns into a roar as $s body begins to grow into a giant ape!@n", TRUE, tch, 0, 0, TO_ROOM);
   SET_BIT_AR(PLR_FLAGS(tch), PLR_OOZARU);
        int add = 10000;
        int mult = 2;

        /* handle pl */
        GET_MAX_HIT(tch) = (GET_BASE_PL(tch) + add) * mult;
        if ((GET_HIT(tch) + add) * mult <= GET_MAX_HIT(tch)) {
         GET_HIT(tch) = (GET_HIT(tch) + add) * mult;
        }
        else if ((GET_HIT(tch) + add) * mult > GET_MAX_HIT(tch)) {
         GET_HIT(tch) = GET_MAX_HIT(tch);
        }

        /* handle ki */
        GET_MAX_MANA(tch) = (GET_BASE_KI(tch) + add) * mult;
        if ((GET_MANA(tch) + add) * mult <= GET_MAX_MANA(tch)) {
         GET_MANA(tch) = (GET_MANA(tch) + add) * mult;
        }
        else if ((GET_MANA(tch) + add) * mult > GET_MAX_MANA(tch)) {
         GET_MANA(tch) = GET_MAX_MANA(tch);
        }

        /* handle st */
        GET_MAX_MOVE(tch) = (GET_BASE_ST(tch) + add) * mult;
        if ((GET_MOVE(tch) + add) * mult <= GET_MAX_MOVE(tch)) {
         GET_MOVE(tch) = (GET_MOVE(tch) + add) * mult;
        }
        else if ((GET_MOVE(tch) + add) * mult > GET_MAX_MOVE(tch)) {
         GET_MOVE(tch) = GET_MAX_MOVE(tch);
        }
  }
 }
}

void oozaru_drop(struct char_data *tch)
{
 struct descriptor_data *d;
if (tch == NULL) {
 for (d = descriptor_list; d; d = d->next) {
  if (!IS_PLAYING(d)) {
   continue;
  }
  if (PLR_FLAGGED(d->character, PLR_OOZARU)) {
   act("@CYour body begins to shrink back to its normal form as the power of the Oozaru leaves you. You fall asleep shortly after returning to normal!@n", TRUE, d->character, 0, 0, TO_CHAR);
   act("@c$n@C's body begins to shrink and return to normal. Their giant ape features fading back into humanoid features until $e is left normal and asleep.@n", TRUE, d->character, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_OOZARU);
   GET_POS(d->character) = POS_SLEEPING;
        GET_HIT(d->character) = (GET_HIT(d->character) / 2) - 10000;
        GET_MANA(d->character) = (GET_MANA(d->character) / 2) - 10000;
        GET_MOVE(d->character) = (GET_MOVE(d->character) / 2) - 10000;

        GET_MAX_HIT(d->character) = GET_BASE_PL(d->character);
        GET_MAX_MANA(d->character) = GET_BASE_KI(d->character);
        GET_MAX_MOVE(d->character) = GET_BASE_ST(d->character);

        if (GET_MOVE(d->character) < 1) {
         GET_MOVE(d->character) = 1;
        }
        if (GET_MANA(d->character) < 1) {
         GET_MANA(d->character) = 1;
        }
        if (GET_HIT(d->character) < 1) {
         GET_HIT(d->character) = 1;
        }
   }
  }
 }
 else {
   if (PLR_FLAGGED(tch, PLR_OOZARU)) {
   act("@CYour body begins to shrink back to its normal form as the power of the Oozaru leaves you. You fall asleep shortly after returning to normal!@n", TRUE, tch, 0, 0, TO_CHAR);
   act("@c$n@C's body begins to shrink and return to normal. Their giant ape features fading back into humanoid features until $e is left normal and asleep.@n", TRUE, tch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(PLR_FLAGS(tch), PLR_OOZARU);
   GET_POS(tch) = POS_SLEEPING;
        GET_HIT(tch) = (GET_HIT(tch) / 2) - 10000;
        GET_MANA(tch) = (GET_MANA(tch) / 2) - 10000;
        GET_MOVE(tch) = (GET_MOVE(tch) / 2) - 10000;

        GET_MAX_HIT(tch) = GET_BASE_PL(tch);
        GET_MAX_MANA(tch) = GET_BASE_KI(tch);
        GET_MAX_MOVE(tch) = GET_BASE_ST(tch);

        if (GET_MOVE(tch) < 1) {
         GET_MOVE(tch) = 1;
        }
        if (GET_MANA(tch) < 1) {
         GET_MANA(tch) = 1;
        }
        if (GET_HIT(tch) < 1) {
         GET_HIT(tch) = 1;
        }
   }
 }
}

/* This controls the powering up of Hoshi-jin from their Eldritch Star */
void star_phase(struct char_data *ch, int type)
{
 struct descriptor_data *d;

 if (ch == NULL) {
  for (d = descriptor_list; d; d = d->next) {
   if (!IS_PLAYING(d)) {
    continue;
   }
   if (IS_NPC(d->character)) {
    continue;
   }
   if (GET_LEVEL(d->character) < 2) {
    continue;
   }
   if (IS_HOSHIJIN(d->character)) {
     ch = d->character;
     switch (type) {
	  case 0:
	     if (GET_PHASE(ch) > 0) {
		  act("@WYour eyes and the glyphs on your skin slowly start to lose their glow. You feel the power received from the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W drain away from your body. It has apparently entered the @rDeath Phase@W of its cycle...@n", TRUE, ch, 0, 0, TO_CHAR);
		  act("@c$n@W's eyes and the glyphs on $s skin slowly start to lose their glow. You notice that $e seems weaker now for some reason.@n", TRUE, ch, 0, 0, TO_ROOM);
		  phase_powerup(ch, 0, GET_PHASE(ch));
         	 }
	   break; // Drop Powerup
	  case 1:
	     if (GET_PHASE(ch) != 1) {
		  act("@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @CBirth Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an electric @bb@Bl@Cue@W!@n", TRUE, ch, 0, 0, TO_CHAR);
		  act("@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow an electric @bb@Bl@Cue@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", TRUE, ch, 0, 0, TO_ROOM);
		  phase_powerup(ch, 0, GET_PHASE(ch));
		  phase_powerup(ch, 1, 1);
		 }
	   break; // Powerup Phase 1
	  case 2:
	     if (GET_PHASE(ch) != 2) {
		  act("@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @GLife Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an fiery @Rr@re@Rd@W!@n", TRUE, ch, 0, 0, TO_CHAR);
		  act("@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow a fiery @rR@Re@rd@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", TRUE, ch, 0, 0, TO_ROOM);
		  phase_powerup(ch, 0, GET_PHASE(ch));
		  phase_powerup(ch, 1, 2);
		 }
	   break; // Powerup Phase 2
	  default:
	    send_to_imm("Strange Error in star_phase by: %s", GET_NAME(ch));
	   break; // Error
	 }
	 
   } // End of is HOSHIJIN
  } // End of descriptor_list for
  return;
 } else if (ch != NULL && !IS_NPC(ch) && GET_LEVEL(ch) > 1) {
   if (IS_HOSHIJIN(ch)) {
     switch (type) {
	  case 0:
	    if (GET_PHASE(ch) > 0) {
		 act("@WYour eyes and the glyphs on your skin slowly start to lose their glow. You feel the power received from the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W drain away from your body. It has apparently entered the @rDeath Phase@W of its cycle...@n", TRUE, ch, 0, 0, TO_CHAR);
		 act("@c$n@W's eyes and the glyphs on $s skin slowly start to lose their glow. You notice that $e seems weaker now for some reason.@n", TRUE, ch, 0, 0, TO_ROOM);
		 phase_powerup(ch, 0, GET_PHASE(ch));
		}
	   break; // Drop Powerup
	  case 1:
	    if (GET_PHASE(ch) != 1) {
		 act("@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @CBirth Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an electric @bb@Bl@Cue@W!@n", TRUE, ch, 0, 0, TO_CHAR);
		 act("@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow an electric @bb@Bl@Cue@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", TRUE, ch, 0, 0, TO_ROOM);
                 phase_powerup(ch, 0, GET_PHASE(ch));		 
		 phase_powerup(ch, 1, 1);
		}
	   break; // Powerup Phase 1
	  case 2:
	    if (GET_PHASE(ch) != 2) {
		 act("@WYou suddenly feel a @RSURGE@W of power through your body. You feel the @GE@gl@Dd@wri@Dt@gc@Gh @YS@yta@Yr@W come into its @GLife Phase@W and its power is flowing into your body! Finally your eyes and the glyphs on your skin begin to glow an fiery @Rr@re@Rd@W!@n", TRUE, ch, 0, 0, TO_CHAR);
		 act("@c$n@W suddenly seems to grow stronger for some reason. You notice $s eyes begin to glow a fiery @rR@Re@rd@W. Suddenly glyphs start to appear all over $s skin and glow with the same light!@n", TRUE, ch, 0, 0, TO_ROOM);
         	 phase_powerup(ch, 0, GET_PHASE(ch));		 
		 phase_powerup(ch, 1, 2);
		}
	   break; // Powerup Phase 2
	  default:
	    send_to_imm("Strange Error in star_phase by: %s", GET_NAME(ch));
	   break; // Error
	 }
	 return;
   } // End of is HOSHIJIN
   else {
     return;
   }
 } // End of ch is/isn't NULL

 return;
}

/* This handles powering up a Hoshijin or powering them down */
static void phase_powerup(struct char_data *ch, int type, int phase)
{
 if (ch == NULL) {
  return;
 }
 if (IS_NPC(ch)) {
  return;
 }
 
	 int change = 0, bonus = 0;
         double mult = 0.0;
	 
 	 switch (phase) {
	  case 0:
	   return;
	   break;
	  case 1:
	   change = 2;
	   mult = 4.0;
	   bonus = 5;
	   break;
	  case 2:
	   change = 3;
	   mult = 8.0;
	   bonus = 8;
	   break;
	  default:
	   send_to_imm("Error: phase_powerup called with GET_PHASE equal to zero by: %s", GET_NAME(ch));
	   return;
	   break;
	 }

           if (ETHER_STREAM(ch)) {
            mult += 0.5;
           }


 if (type == 0) { // Drop their stats
	 GET_HIT(ch) = ((GET_HIT(ch) - ((GET_BASE_PL(ch) * .1) * mult)) / change);
	 GET_MANA(ch) = ((GET_MANA(ch) - ((GET_BASE_KI(ch) * .1) * mult)) / change);
	 GET_MOVE(ch) = ((GET_MOVE(ch) - ((GET_BASE_ST(ch) * .1) * mult)) / change);

         if (GET_HIT(ch) < 0) {
          GET_HIT(ch) = 1;
         }
         if (GET_MANA(ch) < 0) {
          GET_MANA(ch) = 1;
         }
         if (GET_MOVE(ch) < 0) {
          GET_MOVE(ch) = 1;
         }
	 
	 GET_MAX_HIT(ch) = GET_BASE_PL(ch);
	 GET_MAX_MANA(ch) = GET_BASE_KI(ch);
	 GET_MAX_MOVE(ch) = GET_BASE_ST(ch);
         if (GET_BONUS(ch, BONUS_WIMP) > 0 && GET_STR(ch) < 25) {	 
   	  ch->real_abils.str -= bonus;
         }
         if (GET_BONUS(ch, BONUS_SLOW) > 0 && GET_CHA(ch) < 25) {
          ch->real_abils.cha -= bonus;
         }
	 
	 GET_PHASE(ch) = 0;
 } else { // Raise their stats
 
	 GET_HIT(ch) = ((GET_HIT(ch) + ((GET_BASE_PL(ch) * .1) * mult)) * change);
	 GET_MANA(ch) = ((GET_MANA(ch) + ((GET_BASE_KI(ch) * .1) * mult)) * change);
	 GET_MOVE(ch) = ((GET_MOVE(ch) + ((GET_BASE_ST(ch) * .1) * mult)) * change);

	 GET_MAX_HIT(ch) = ((GET_BASE_PL(ch) + ((GET_BASE_PL(ch) * .1) * mult)) * change);
	 GET_MAX_MANA(ch) = ((GET_BASE_KI(ch) + ((GET_BASE_KI(ch) * .1) * mult)) * change);
	 GET_MAX_MOVE(ch) = ((GET_BASE_ST(ch) + ((GET_BASE_ST(ch) * .1) * mult)) * change);

         if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
          GET_HIT(ch) = GET_MAX_HIT(ch);
         }
         if (GET_MANA(ch) > GET_MAX_MANA(ch)) {
          GET_MANA(ch) = GET_MAX_MANA(ch);
         }
         if (GET_MOVE(ch) > GET_MAX_MOVE(ch)) {
          GET_MOVE(ch) = GET_MAX_MOVE(ch);
         }

        if (GET_BONUS(ch, BONUS_WIMP) > 0 && GET_STR(ch) + bonus <= 25) {
         ch->real_abils.str += bonus;
        } if (GET_BONUS(ch, BONUS_SLOW) > 0 && GET_CHA(ch) + bonus <= 25) {
         ch->real_abils.cha += bonus;
        }
	 
	 GET_PHASE(ch) = phase;
 }

  save_char(ch);   
  return;
}
