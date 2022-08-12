/* ************************************************************************
*  File: dg_comm.c                               Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle mud to player communication         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */
#include "dg_comm.h"
#include "act.informative.h"
#include "utils.h"
#include "races.h"
#include "comm.h"
#include "dg_scripts.h"
#include "graph.h"
#include "spells.h"
#include "handler.h"

/* local functions */
void sub_write_to_char(struct char_data *ch, char *tokens[], void *otokens[], char type[]);


/* same as any_one_arg except that it stops at punctuation */
char *any_one_name(char *argument, char *first_arg)
{
    char* arg;

    /* Find first non blank */
    while(isspace(*argument))
        argument++;

    /* Find length of first word */
    for(arg = first_arg ;
        *argument && !isspace(*argument) &&
          (!ispunct(*argument) || *argument == '#' || *argument == '-') ;
        arg++, argument++)
        *arg = LOWER(*argument);
    *arg = '\0';

    return argument;
}


void sub_write_to_char(struct char_data *ch, char *tokens[],
		       void *otokens[], char type[])
{
    char sb[MAX_STRING_LENGTH];
    int i;

    strcpy(sb,"");

    for (i = 0; tokens[i + 1]; i++)
    {
	strcat(sb,tokens[i]);

	switch (type[i])
	{
	case '~':
	    if (!otokens[i])
		strcat(sb,"someone");
	    else if ((struct char_data *)otokens[i] == ch)
		strcat(sb,"you");
	    else
		strcat(sb,PERS((struct char_data *)otokens[i], ch));
	    break;

	case '|':
	    if (!otokens[i])
		strcat(sb,"someone's");
	    else if ((struct char_data *)otokens[i] == ch)
		strcat(sb,"your");
	    else
	    {
		strcat(sb,PERS((struct char_data *) otokens[i], ch));
		strcat(sb,"'s");
	    }
	    break;

	case '^':
	    if (!otokens[i] || !CAN_SEE(ch, (struct char_data *) otokens[i]))
		strcat(sb,"its");
	    else if (otokens[i] == ch)
		strcat(sb,"your");
	    else
		strcat(sb,HSHR((struct char_data *) otokens[i]));
	    break;

	case '&':
	    if (!otokens[i] || !CAN_SEE(ch, (struct char_data *) otokens[i]))
		strcat(sb,"it");
	    else if (otokens[i] == ch)
		strcat(sb,"you");
	    else
		strcat(sb,HSSH((struct char_data *) otokens[i]));
	    break;

	case '*':
	    if (!otokens[i] || !CAN_SEE(ch, (struct char_data *) otokens[i]))
		strcat(sb,"it");
	    else if (otokens[i] == ch)
		strcat(sb,"you");
	    else
		strcat(sb,HMHR((struct char_data *) otokens[i]));
	    break;

	case '¨':
	    if (!otokens[i])
		strcat(sb,"something");
	    else
		strcat(sb,OBJS(((struct obj_data *) otokens[i]), ch));
	    break;
	}
    }

    strcat(sb,tokens[i]);
    strcat(sb,"\n\r");
    sb[0] = toupper(sb[0]);
    send_to_char(ch, "%s", sb);
}


void sub_write(char *arg, struct char_data *ch, int8_t find_invis, int targets)
{
    char str[MAX_INPUT_LENGTH * 2];
    char type[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
    char *tokens[MAX_INPUT_LENGTH], *s, *p;
    void *otokens[MAX_INPUT_LENGTH];
    struct char_data *to;
    struct obj_data *obj;
    int i, tmp;
    int to_sleeping = 1; /* mainly for windows compiles */

    if (!arg)
	return;

    tokens[0] = str;

    for (i = 0, p = arg, s = str; *p;)
    {
	switch (*p) {
	case '~':
	case '|':
	case '^':
	case '&':
	case '*':
	    /* get char_data, move to next token */
	    type[i] = *p;
	    *s = '\0';
	    p = any_one_name(++p, name);
	    otokens[i] =
		find_invis ? (void *)get_char_in_room(&world[IN_ROOM(ch)], name) : (void *)get_char_room_vis(ch, name, NULL);
	    tokens[++i] = ++s;
	    break;

	case '¨':
	    /* get obj_data, move to next token */
	    type[i] = *p;
	    *s = '\0';
	    p = any_one_name(++p, name);

            if (find_invis) obj = get_obj_in_room(&world[IN_ROOM(ch)], name);
            else if (!(obj = get_obj_in_list_vis(ch, name, NULL, world[IN_ROOM(ch)].contents))) ;
            else if (!(obj = get_obj_in_equip_vis(ch, name, &tmp, ch->equipment))) ;
            else obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying);

	    otokens[i] = (void *)obj;
	    tokens[++i] = ++s;
	    break;

	case '\\':
	    p++;
	    *s++ = *p++;
	    break;

	default:
	    *s++ = *p++;
	}
    }

    *s = '\0';
    tokens[++i] = NULL;

    if (IS_SET(targets, TO_CHAR) && SENDOK(ch))
	sub_write_to_char(ch, tokens, otokens, type);

    if (IS_SET(targets, TO_ROOM))
	for (to = world[IN_ROOM(ch)].people;
	     to; to = to->next_in_room)
	    if (to != ch && SENDOK(to))
		sub_write_to_char(to, tokens, otokens, type);
}



void send_to_zone(char *messg, zone_rnum zone)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) &&
        (IN_ROOM(i->character) != NOWHERE) &&
        (world[IN_ROOM(i->character)].zone == zone))
      write_to_output(i, "%s", messg);
}

void fly_zone(zone_rnum zone, char *messg, struct char_data *ch)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i->character && AWAKE(i->character) && OUTSIDE(i->character) && (IN_ROOM(i->character) != NOWHERE) && (world[IN_ROOM(i->character)].zone == zone) && i->character != ch) {
       if (PLR_FLAGGED(i->character, PLR_DISGUISED)) {
         write_to_output(i, "A disguised figure %s", messg);
       } else {
         write_to_output(i, "%s%s %s", readIntro(i->character, ch) == 1 ? "" : "A ", get_i_name(i->character, ch), messg);
       }
    }
  }
}

void send_to_sense(int type, char *messg, struct char_data *ch)
{
  struct descriptor_data *i;
  struct char_data *tch;
  struct obj_data *obj;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING) {
     continue;
    }
    tch = i->character;
    obj = GET_EQ(tch, WEAR_EYE);
    if (tch == ch) {
      continue;
    }
    if (!GET_SKILL(tch, SKILL_SENSE)) {
      continue;
    }
    if (((world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone && type == 0) || !AWAKE(tch))) {
      continue;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SHIP)) {
      continue;
    }
    if (obj && type == 0) {
     continue;
    }
    if (IS_ANDROID(ch)) {
     continue;
    }
    if (IN_ROOM(ch) == IN_ROOM(tch)) {
       continue;
    } else if (GET_HIT(ch) < (GET_HIT(tch) * 0.001) + 1) {
       continue;
    } else if (type == 0) {
       if (GET_MAX_HIT(ch) > GET_MAX_HIT(tch)) {
        write_to_output(i, "%s who is stronger than you. They are nearby.\r\n", messg);
       }
       else if (GET_MAX_HIT(ch) >= GET_MAX_HIT(tch) * .9) {
        write_to_output(i, "%s who is near your strength. They are nearby.\r\n", messg);
       }
       else if (GET_MAX_HIT(ch) >= GET_MAX_HIT(tch) * .6) {
        write_to_output(i, "%s who is a good bit weaker than you. They are nearby.\r\n", messg);
       }
       else if (GET_MAX_HIT(ch) >= GET_MAX_HIT(tch) * .4) {
        write_to_output(i, "%s who is a lot weaker than you. They are nearby.\r\n", messg);
       }
       else {
        continue;
       }
       if (readIntro(tch, ch) == 1) {
        write_to_output(i, "@YYou recognise this signal as @y%s@Y!@n\r\n", get_i_name(tch, ch));
       } else if (read_sense_memory(ch, tch)) {
        write_to_output(i, "@YYou recognise this signal, but don't seem to know their name.@n\r\n");
       }
      } else if (planet_check(ch, tch)) {
        char *blah = sense_location(ch);
        char power[MAX_INPUT_LENGTH];
        char align[MAX_INPUT_LENGTH];
        if (GET_HIT(ch) > GET_HIT(tch) * 10) {
         sprintf(power, ", who is @Runbelievably stronger@Y than you");
        } else if (GET_HIT(ch) > GET_HIT(tch) * 5) {
         sprintf(power, ", who is much @Rstronger@Y than you");
        } else if (GET_HIT(ch) > GET_HIT(tch) * 2) {
         sprintf(power, ", who is more than twice as @Rstrong@Y as you");
        } else if (GET_HIT(ch) > GET_HIT(tch)) {
         sprintf(power, ", who is somewhat @mstronger@Y than you");
        } else if (GET_HIT(ch) * 10 < GET_HIT(tch)) {
         sprintf(power, ", who is @Munbelievably weaker@Y than you");
        } else if (GET_HIT(ch) * 5 < GET_HIT(tch)) {
         sprintf(power, ", who is much @Mweaker@Y than you");
        } else if (GET_HIT(ch) * 2 < GET_HIT(tch)) {
         sprintf(power, ", who is more than twice as @Mweak@Y as you");
        } else if (GET_HIT(ch) < GET_HIT(tch)) {
         sprintf(power, ", who is somewhat @Wweaker@Y than you");
        } else {
         sprintf(power, ", who is close to @Cequal@Y with you");
        }
        if (GET_ALIGNMENT(ch) >= 1000) {
         sprintf(align, ", with a @wsaintly@Y aura,");
        } else if (GET_ALIGNMENT(ch) >= 500) {
         sprintf(align, ", with a very @Cgood@Y aura,");
        } else if (GET_ALIGNMENT(ch) >= 200) {
         sprintf(align, ", with a @cgood@Y aura,");
        } else if (GET_ALIGNMENT(ch) > -100) {
         sprintf(align, ", with a near @Wneutral@Y aura,");
        } else if (GET_ALIGNMENT(ch) > -200) {
         sprintf(align, ", with a sorta @revil@Y aura,");
        } else if (GET_ALIGNMENT(ch) > -500) {
         sprintf(align, ", with an @revil@Y aura,");
        } else if (GET_ALIGNMENT(ch) > -900) {
         sprintf(align, ", with a @rvery evil@Y aura,");
        } else {
         sprintf(align, ", with a @rd@De@Wv@wil@Wi@Ds@rh@Y aura,");
        }
        if (strstr(messg, "land"))
         write_to_output(i, "@YYou sense %s%s%s %s! They appear to have landed at...@G%s@n\r\n", readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg, blah);
        else
         write_to_output(i, "@YYou sense %s%s%s %s!@n\r\n", readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg);
      }
   }
}

void send_to_scouter(char *messg, struct char_data *ch, int num, int type)
{
  struct descriptor_data *i;
  struct char_data *tch;
  struct obj_data *obj;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING) {
     continue;
    }
    tch = i->character;
    obj = GET_EQ(tch, WEAR_EYE);
    if (tch == ch) {
      continue;
    }
    else {
    if ((((world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone) && type == 0) || !AWAKE(tch))) {
      continue;
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SHIP)) {
      continue;
    }
    if (GET_INVIS_LEV(ch) > GET_ADMLEVEL(tch)) {
     continue;
    }
    if (IS_ANDROID(ch)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_EARTH)) {
     continue;
    } else if (PLANET_ZENITH(IN_ROOM(ch)) && !PLANET_ZENITH(IN_ROOM(tch))) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_FRIGID)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_NAMEK)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_AL)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_VEGETA)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KONACK) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_KONACK)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NEO) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_NEO)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_YARDRAT) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_YARDRAT)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KANASSA) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_KANASSA)) {
     continue;
    } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARLIA) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_ARLIA)) {
     continue;
    }  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER) && !ROOM_FLAGGED(IN_ROOM(tch), ROOM_AETHER)) {
     continue;
    }
    if (!obj) {
     continue;
    } else if (IN_ROOM(ch) == IN_ROOM(tch)) {
     continue;
    } else if (type == 0) {
        if (num == 1) {
         struct obj_data *obj = GET_EQ(tch, WEAR_EYE);        
          if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(ch) >= 150000) {
          write_to_output(i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(ch) >= 5000000) {
          write_to_output(i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(ch) >= 15000000) {
          write_to_output(i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else {
          write_to_output(i, "%s@n", messg);
         }
        } else {
          if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(ch) >= 150000) {
           write_to_output(i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(ch) >= 5000000) {
           write_to_output(i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(ch) >= 15000000) {
           write_to_output(i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
          }
          else {
           write_to_output(i, "%s\r\n", messg);
          }
        }
       } else if (type == 1 && GET_SKILL(tch, SKILL_SENSE) < 20) {
          if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(ch) >= 150000) {
           write_to_output(i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
          } else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(ch) >= 5000000) {
           write_to_output(i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
          } else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(ch) >= 15000000) {
           write_to_output(i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
          } else {
           write_to_output(i, "@D[Blip@D]@w %s. @RPL@D:@Y %s@n\r\n\r\n", messg, add_commas(GET_HIT(ch)));
          }
       }  else if (type == 2 && GET_SKILL(tch, SKILL_SENSE) < 20) {
          char *blah = sense_location(ch);
          if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(ch) >= 150000) {
           write_to_output(i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
          } else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(ch) >= 5000000) {
           write_to_output(i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
          } else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(ch) >= 15000000) {
           write_to_output(i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
          } else {
           write_to_output(i, "@D[Blip@D]@w %s at... @G%s. @RPL@D:@Y %s@n\r\n\r\n", messg, blah, add_commas(GET_HIT(ch)));
          }
       }
    }
  }
}

void send_to_worlds(struct char_data *ch)
{
  struct descriptor_data *i;
  char message[MAX_INPUT_LENGTH];

  if (GET_MAX_HIT(ch) > 2000000000) {
   sprintf(message, "@RThe whole planet begins to quake violently as if the world is ending!@n\r\n");
  } else if (GET_MAX_HIT(ch) > 1000000000) {
   sprintf(message, "@RThe whole planet begins to quake violently with a thunderous roar!@n\r\n");
  } else if (GET_MAX_HIT(ch) > 500000000) {
   sprintf(message, "@RThe whole planet begins to quake violently!@n\r\n");
  } else if (GET_MAX_HIT(ch) > 100000000) {
   sprintf(message, "@RThe whole planet rumbles and shakes!@n\r\n");
  } else if (GET_MAX_HIT(ch) > 50000000) {
   sprintf(message, "@RThe whole planet rumbles faintly!@n\r\n");
  } else {
   return;
  }

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING) {
     continue;
    }
    if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_EARTH) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH)) {   
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_VEGETA) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA)) {
     send_to_char(i->character, "%s", message);
    } else if (PLANET_ZENITH(IN_ROOM(i->character)) && PLANET_ZENITH(IN_ROOM(ch))) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_NAMEK) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_KONACK) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_KONACK)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_YARDRAT) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_YARDRAT)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_FRIGID) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_KANASSA) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_KANASSA)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_ARLIA) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARLIA)) {
     send_to_char(i->character, "%s", message);
    } else if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_AETHER) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER)) {
     send_to_char(i->character, "%s", message);
    }
  }
}

void send_to_imm(char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING) {
     continue;
    }
    else if (GET_ADMLEVEL(i->character) == 0) {
    continue;
    }
    else if (!PRF_FLAGGED(i->character, PRF_LOG2)) {
    continue;
    }
    else if (PLR_FLAGGED(i->character, PLR_WRITING)) {
     continue;
    }
    else {
    write_to_output(i, "@g[ Log: ");
    va_list args;
    va_start(args, messg);

    vwrite_to_output(i, messg, args);
    write_to_output(i, " ]@n\n");
    va_end(args);
    }
  }
    va_list args;
    va_start(args, messg);
    basic_mud_vlog(messg, args);
    va_end(args);
}
