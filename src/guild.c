/* ************************************************************************
 *   File: Guild.c                                                         *
 *  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
 *                        practicing.                                      *
 *                                                                         *
 * Based on shop.c.  As such, the CircleMud License applies                *
 * Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
 ************************************************************************ */

#include "guild.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "gengld.h"
#include "local_limits.h"
#include "feats.h"
#include "act.comm.h"
#include "handler.h"
#include "shop.h"
#include "class.h"
#include "constants.h"

/* Local variables */
int spell_sort_info[SKILL_TABLE_SIZE + 1];
int top_guild = -1;
struct guild_data *guild_index;

char *guild_customer_string(int guild_nr, int detailed);
int calculate_skill_cost(struct char_data *ch, int skill);

int calculate_skill_cost(struct char_data *ch, int skill)
{
 int cost = 0;

 if (IS_SET(spell_info[skill].flags, SKFLAG_TIER2)) {
  cost = 8;
 } else if (IS_SET(spell_info[skill].flags, SKFLAG_TIER3)) {
  cost = 15;
 } else if (IS_SET(spell_info[skill].flags, SKFLAG_TIER4)) {
  if (GET_SKILL_BASE(ch, skill) == 0) {
   cost = 200;
  } else {
   cost = 25;
  }
 } else if (IS_SET(spell_info[skill].flags, SKFLAG_TIER5)) {
  if (GET_SKILL_BASE(ch, skill) == 0) {
   cost = 300;
  } else {
   cost = 40;
  }
 } else {
  cost = 4;
 }

 if (GET_SKILL_BASE(ch, skill) > 90)
  cost += 12;
 else if (GET_SKILL_BASE(ch, skill) > 80)
  cost += 10;
 else if (GET_SKILL_BASE(ch, skill) > 70)
  cost += 8;
 else if (GET_SKILL_BASE(ch, skill) > 50)
  cost += 6;
 else if (GET_SKILL_BASE(ch, skill) > 40)
  cost += 2;
 else if (GET_SKILL_BASE(ch, skill) > 30)
  cost += 1;

 if (GET_FORGETING(ch) != 0)
  cost += 6;

 if (skill == SKILL_RUNIC)
  cost += 6;
 if (skill == SKILL_EXTRACT)
  cost += 3;

 if (IS_HOSHIJIN(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK || skill == SKILL_KNEE || skill == SKILL_ELBOW || skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE || skill == SKILL_SLAM || skill == SKILL_HEELDROP || skill == SKILL_DAGGER || skill == SKILL_SWORD || skill == SKILL_CLUB || skill == SKILL_GUN || skill == SKILL_SPEAR || skill == SKILL_BRAWL)) {
  cost += 5;
 }

 if (skill == SKILL_INSTANTT) {
   if (GET_SKILL_BASE(ch, skill) == 0) {
    cost = 2000;
   }  else {
    cost = 50;
   }
 }
 if (skill == SKILL_MYSTICMUSIC) {
  cost = cost * 1.5;
 }

 return (cost);
}

void handle_ingest_learn(struct char_data *ch, struct char_data *vict)
{
	
	int i = 1;
	send_to_char(ch, "@YAll your current skills improve somewhat!@n\r\n");

	for (i = 1; i <= SKILL_TABLE_SIZE; i++) 
	{
		
		if (GET_SKILL_BASE(ch, i) > 0 && GET_SKILL_BASE(vict, i) > 0 && i != 141)
		{
			send_to_char(ch, "@YYou gained a lot of new knowledge about @y%s@Y!@n\r\n", spell_info[i].name);

			if (GET_SKILL_BASE(ch, i) + 10 < 100)
			{
				GET_SKILL_BASE(ch, i) += 10;
			}
			else if (GET_SKILL_BASE(ch, i) > 0 && GET_SKILL_BASE(ch, i) < 100)
			{
				GET_SKILL_BASE(ch, i) += 1;
			}
			else
			{
				GET_SKILL_BASE(ch, i) = 100;
			}
			
		}
		if (((i >= 481 && i <= 489) || i == 517 || i == 535) && ((GET_SKILL_BASE(ch, i) <= 0) && GET_SKILL_BASE(vict, i) > 0))
		{
			SET_SKILL(ch, i, GET_SKILL_BASE(ch, i) + rand_number(10, 25));
			send_to_char(ch, "@YYou learned @y%s@Y from ingesting your target!@n\r\n", spell_info[i].name);
			GET_SLOTS(ch) += 1;
			GET_INGESTLEARNED(ch) = 1;

		}
		

	}
}

ACMD(do_teach)
{

 if (IS_NPC(ch))
  return;

 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 int skill = 100;
 struct char_data *vict;

 two_arguments(argument, arg, arg2);

 if (!*arg) {
  send_to_char(ch, "What skill are you wanting to teach?\r\n");
  return;
 }

 skill = find_skill_num(arg, SKTYPE_SKILL);

 if (GET_SKILL_BASE(ch, skill) < 101) {
  send_to_char(ch, "You are not a Grand Master in that skill!\r\n");
  send_to_char(ch, "@wSyntax: teach (skill) (target)@n\r\n");
  return;
 }

 if (!*arg2) {
  send_to_char(ch, "@wWho are you wanting to teach @C%s@w to?@n\r\n", spell_info[skill].name);
  send_to_char(ch, "@wSyntax: teach (skill) (target)@n\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "@wTeach who?@n\r\n");
  send_to_char(ch, "@wSyntax: teach (skill) (target)@n\r\n");
  return;
 }

 int cost = calculate_skill_cost(vict, skill), free = FALSE;
 
 if (GET_SKILL_BASE(ch, skill) >= 103) {
  cost = cost * 0.5;
  if (rand_number(1, 4) == 4) {
   free = TRUE;
  }
 } else if (GET_SKILL_BASE(ch, skill) == 102) {
  cost = cost * 0.5;
 } else {
  cost = cost * 0.75;
 }

 if (cost == 0) /* Just to be sure */
  cost = 1;

 if (!vict->master) {
  send_to_char(ch, "They must be following you in order for you to teach them.\r\n");
  return;
 } else if (vict->master != ch) {
  send_to_char(ch, "They must be following you in order for you to teach them.\r\n");
  return;
 } else if (GET_FORGETING(vict) == skill) {
  send_to_char(ch, "They are trying to forget that skill!\r\n");
  return;
 } else if (GET_PRACTICES(vict, GET_CLASS(vict)) < cost) {
  send_to_char(ch, "They do not have enough practice sessions for you to teach them.\r\n");
  return;
 } else if (GET_SKILL_BASE(vict, skill) >= 80) {
  send_to_char(ch, "You can not teach them anymore.\r\n");
  return;
 } else if (GET_SKILL_BASE(vict, skill) > 0) {
  char tochar[MAX_STRING_LENGTH], tovict[MAX_STRING_LENGTH], toother[MAX_STRING_LENGTH];
  sprintf(tochar, "@YYou instruct @y$N@Y in the finer points of @C%s@Y.@n\r\n", spell_info[skill].name);
  sprintf(tovict, "@y$n@Y instructs you in the finer points of @C%s@Y.@n\r\n", spell_info[skill].name);
  sprintf(toother, "@y$n@Y instructs @y$N@Y in the finer points of @C%s@Y.@n\r\n", spell_info[skill].name);
  act(tochar, TRUE, ch, 0, vict, TO_CHAR);
  act(tovict, TRUE, ch, 0, vict, TO_VICT);
  act(toother, TRUE, ch, 0, vict, TO_NOTVICT);
  SET_SKILL(vict, skill, GET_SKILL_BASE(vict, skill) + 1);
  if (free == FALSE) {
   GET_PRACTICES(vict, GET_CLASS(vict)) -= cost;
  } else {
   send_to_char(ch, "@GYou teach your lesson so well that it cost them nothing to learn from you!@n\r\n");
   send_to_char(vict, "@GYour teacher taught you the lesson so well that it cost you nothing!@n\r\n");
  }
 } else {
  send_to_char(ch, "They do not even know the basics. It's a waste of your teaching skills.\r\n");
  return;
 }
}

const char *how_good(int percent)
{
  if (percent < 0)
    return " error)";
  if (percent == 0)
    return "(@Mnot@n)";
  if (percent <= 10)
    return "(@rawful@n)";
  if (percent <= 20)
    return "(@Rbad@n)";
  if (percent <= 40)
    return "(@ypoor@n)";
  if (percent <= 55)
    return "(@Yaverage@n)";
  if (percent <= 70)
    return "(@gfair@n)";
  if (percent <= 80)
    return "(@Ggood@n)";
  if (percent <= 85)
    return "(@bgreat@n)";
  if (percent <= 100)
    return "(@Bsuperb@n)";

  return "(@rinate@n)";
}

const char *prac_types[] = {
  "spell",
  "skill"
};


int compare_spells(const void *x, const void *y)
{
  int	a = *(const int *)x,
	b = *(const int *)y;

  return strcmp(spell_info[a].name, spell_info[b].name);
}


int print_skills_by_type(struct char_data *ch, char *buf, int maxsz, int sktype, char *argument)
{
  char arg[1000];
  size_t len = 0;
  int i, t, known, nlen = 0, count = 0, canknow = 0;
  char buf2[READ_SIZE];

  one_argument(argument, arg);

  for (i = 1; i <= SKILL_TABLE_SIZE; i++) { 
    t = spell_info[i].skilltype;

    if (t != sktype)
      continue;

    if ((t & SKTYPE_SKILL) || (t & SKTYPE_SPELL)) {
      for (nlen = 0, known = 0; nlen <= NUM_CLASSES; nlen++)
        if (GET_CLASS_RANKS(ch, nlen) > 0 && (spell_info[i].can_learn_skill[nlen] > SKLEARN_CANT)) {
          known = spell_info[i].can_learn_skill[nlen];
        }
    } else {
      known = 0;
    }
    if (GET_SKILL(ch, i) <= 0) {
      known = 0;
    }
    if (*arg) {
     if (atoi(arg) <= 0 && strstr(spell_info[i].name, arg) == FALSE) {
      known = 0;
     } else if (atoi(arg) > GET_SKILL(ch, i)) {
      known = 0;
     }
    }
    if (known) {
      if (t & SKTYPE_LANG) {
	nlen = snprintf(buf + len, maxsz - len, "%-20s  (%s)\r\n",
                        spell_info[i].name, GET_SKILL_BASE(ch, i) ? "known" : "unknown");
      } else if (t & SKTYPE_SKILL) {
        if (GET_SKILL_BONUS(ch, i))
          snprintf(buf2, sizeof(buf2), " (base %d + bonus %d)", GET_SKILL_BASE(ch, i), GET_SKILL_BONUS(ch, i));
        else
          buf2[0] = 0;
        if (known == SKLEARN_CROSSCLASS) {
         count++;
         canknow = highest_skill_value(GET_LEVEL(ch), GET_SKILL(ch, i));
         nlen = snprintf(buf + len, maxsz - len, "@y(@Y%2d@y) @W%-30s  @y(@Y%2d@y) @C%3d@D/@c%3d   %s@n%s%s\r\n", count,
                          spell_info[i].name, count, GET_SKILL(ch, i), canknow, GET_SKILL_PERF(ch, i) > 0 ? (GET_SKILL_PERF(ch, i) == 1 ? "@ROver Charge" : (GET_SKILL_PERF(ch, i) == 2 ? "@BAccurate" : "@GEfficient")) : "", GET_SKILL_BASE(ch, i) > 100 ? " @D(@YGrand Master@D)@n" : "", buf2);
        } else {
         count++;
         canknow = highest_skill_value(GET_LEVEL(ch), GET_SKILL(ch, i));
         nlen = snprintf(buf + len, maxsz - len, "@y(@Y%2d@y) @W%-30s  @y(@Y%2d@y) @C%3d@D/@c%d3   %s@n%s%s\r\n", count,
                          spell_info[i].name, count, GET_SKILL(ch, i), canknow, GET_SKILL_PERF(ch, i) > 0 ? (GET_SKILL_PERF(ch, i) == 1 ? "@ROver Charge" : (GET_SKILL_PERF(ch, i) == 2 ? "@BAccurate" : "@GEfficient")) : "", GET_SKILL_BASE(ch, i) > 100 ? " @D(@YGrand Master@D)@n" : "", buf2);
       }
      }
      if (len + nlen >= maxsz || nlen < 0)
        break;
      len += nlen;
    }
  }

  return len;
}

int slot_count(struct char_data *ch)
{
   int i, skills = -1, fail = FALSE;
   int punch = FALSE, kick = FALSE, knee = FALSE, elbow = FALSE, kiball = FALSE, kiblast = FALSE, beam = FALSE, renzo = FALSE, shogekiha = FALSE;

   for (i = 1; i <= SKILL_TABLE_SIZE; i++) {
    if (GET_SKILL(ch, i) > 0) {
     switch (i) {
      case SKILL_PUNCH:
       fail = TRUE;
       punch = TRUE;
      break;
      case SKILL_KICK:
       fail = TRUE;
       kick = TRUE;
      break;
      case SKILL_ELBOW:
       fail = TRUE;
       elbow = TRUE;
      break;
      case SKILL_KNEE:
       fail = TRUE;
       knee = TRUE;
      break;
      case SKILL_KIBALL:
       fail = TRUE;
       kiball = TRUE;
      break;
      case SKILL_KIBLAST:
       fail = TRUE;
       kiblast = TRUE;
      break;
      case SKILL_BEAM:
       fail = TRUE;
       beam = TRUE;
      break;
      case SKILL_SHOGEKIHA:
       fail = TRUE;
       shogekiha = TRUE;
      break;
      case SKILL_RENZO:
       fail = TRUE;
       renzo = TRUE;
      break;
      case SKILL_TELEPATHY:
       if (IS_KANASSAN(ch) || IS_KAI(ch)) {
        fail = TRUE;
       }
       break;
      case SKILL_ABSORB:
       if (IS_BIO(ch) || IS_ANDROID(ch)) {
        fail = TRUE;
       }
       break;
      case SKILL_TAILWHIP:
       if (IS_ICER(ch)) {
        fail = TRUE;
       }
       break;
      case SKILL_SEISHOU:
       if (IS_ARLIAN(ch)) {
        fail = TRUE;
       }
       break;
      case SKILL_REGENERATE:
       if (IS_MAJIN(ch) || IS_NAMEK(ch) || IS_BIO(ch)) {
        fail = TRUE;
       }
       break;
     }
     if (fail == FALSE) {
      skills += 1;
     }
     fail = FALSE;
    }
   }

 if (punch == TRUE && kick == TRUE && elbow == TRUE && knee == TRUE) {
  skills += 1;
 }
 if (kiball == TRUE && kiblast == TRUE && beam == TRUE && shogekiha == TRUE && renzo == TRUE) {
  skills += 1;
 }

 return (skills);
}

void list_skills(struct char_data *ch, char *arg)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  size_t len = 0;
  int slots = FALSE;
  char buf2[MAX_STRING_LENGTH];

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch, GET_CLASS(ch)), GET_PRACTICES(ch, GET_CLASS(ch)) == 1 ? "" : "s");

  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nYou know the following skills:     @CKnown@D/@cPrac. Max@n\r\n@w-------------------------------------------------------@n\r\n");

  len += print_skills_by_type(ch, buf2 + len, sizeof(buf2) - len, SKTYPE_SKILL, arg);

  if (slots == FALSE) {
   len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\n@DSkill Slots@W: @M%d@W/@m%d", slot_count(ch), GET_SLOTS(ch));
  }

  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}


int is_guild_open(struct char_data *keeper, int guild_nr, int msg)
{
  char buf[200];
  *buf = 0;

  if (GM_OPEN(guild_nr) > time_info.hours &&
    GM_CLOSE(guild_nr) < time_info.hours)
  strlcpy(buf, MSG_TRAINER_NOT_OPEN, sizeof(buf));

  if (!*buf)
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);

  return (FALSE);
}


int is_guild_ok_char(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	char buf[200];

	if (!(CAN_SEE(keeper, ch))) {
		do_say(keeper, MSG_TRAINER_NO_SEE_CH, cmd_say, 0);
		return (FALSE);
	}

	
	if (GET_LEVEL(ch) < GM_MINLVL(guild_nr)) {
				snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_MINLVL);
		do_tell(keeper, buf, cmd_tell, 0); 
		return (FALSE);
	}


	if ((IS_GOOD(ch) && NOTRAIN_GOOD(guild_nr)) ||
		 (IS_EVIL(ch) && NOTRAIN_EVIL(guild_nr)) ||
		 (IS_NEUTRAL(ch) && NOTRAIN_NEUTRAL(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if (IS_NPC(ch))
		return (FALSE);

	if ((IS_ROSHI(ch) && NOTRAIN_WIZARD(guild_nr)) ||
		(IS_PICCOLO(ch) && NOTRAIN_CLERIC(guild_nr)) ||
		(IS_KRANE(ch) && NOTRAIN_ROGUE(guild_nr)) ||
		(IS_NAIL(ch) && NOTRAIN_FIGHTER(guild_nr)) ||
		(IS_GINYU(ch) && NOTRAIN_PALADIN(guild_nr)) ||
		(IS_FRIEZA(ch) && NOTRAIN_SORCERER(guild_nr)) ||
		(IS_TAPION(ch) && NOTRAIN_DRUID(guild_nr)) ||
		(IS_ANDSIX(ch) && NOTRAIN_BARD(guild_nr)) ||
		(IS_DABURA(ch) && NOTRAIN_RANGER(guild_nr)) ||
		(IS_BARDOCK(ch) && NOTRAIN_MONK(guild_nr)) ||
		(IS_KABITO(ch) && NOTRAIN_BARBARIAN(guild_nr)) ||
        	(IS_JINTO(ch) && NOTRAIN_ARCANE_ARCHER(guild_nr)) ||
		(IS_TSUNA(ch) && NOTRAIN_ARCANE_TRICKSTER(guild_nr)) ||
		(IS_KURZAK(ch) && NOTRAIN_ARCHMAGE(guild_nr)) ||
		(IS_ASSASSIN(ch) && NOTRAIN_ASSASSIN(guild_nr)) ||
		(IS_BLACKGUARD(ch) && NOTRAIN_BLACKGUARD(guild_nr)) ||
		(IS_DRAGON_DISCIPLE(ch) && NOTRAIN_DRAGON_DISCIPLE(guild_nr)) ||
		(IS_DUELIST(ch) && NOTRAIN_DUELIST(guild_nr)) ||
		(IS_DWARVEN_DEFENDER(ch) && NOTRAIN_DWARVEN_DEFENDER(guild_nr)) ||
		(IS_ELDRITCH_KNIGHT(ch) && NOTRAIN_ELDRITCH_KNIGHT(guild_nr)) ||
		(IS_HIEROPHANT(ch) && NOTRAIN_HIEROPHANT(guild_nr)) ||
        	(IS_HORIZON_WALKER(ch) && NOTRAIN_HORIZON_WALKER(guild_nr)) ||
        	(IS_LOREMASTER(ch) && NOTRAIN_LOREMASTER(guild_nr)) ||
		(IS_MYSTIC_THEURGE(ch) && NOTRAIN_MYSTIC_THEURGE(guild_nr)) ||
		(IS_SHADOWDANCER(ch) && NOTRAIN_SHADOWDANCER(guild_nr)) ||
        	(IS_THAUMATURGIST(ch) && NOTRAIN_THAUMATURGIST(guild_nr))) {

		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((!IS_ROSHI(ch) && TRAIN_WIZARD(guild_nr)) ||
	    (!IS_PICCOLO(ch) && TRAIN_CLERIC(guild_nr)) ||
	    (!IS_KRANE(ch) && TRAIN_ROGUE(guild_nr)) ||
	    (!IS_BARDOCK(ch) && TRAIN_MONK(guild_nr)) ||
	    (!IS_GINYU(ch) && TRAIN_PALADIN(guild_nr)) ||
	    (!IS_NAIL(ch) && TRAIN_FIGHTER(guild_nr)) ||
	    (!IS_FRIEZA(ch) && TRAIN_SORCERER(guild_nr)) ||
	    (!IS_TAPION(ch) && TRAIN_DRUID(guild_nr)) ||
	    (!IS_ANDSIX(ch) && TRAIN_BARD(guild_nr)) ||
	    (!IS_DABURA(ch) && TRAIN_RANGER(guild_nr)) ||
	    (!IS_KABITO(ch) && TRAIN_BARBARIAN(guild_nr)) ||
            (!IS_JINTO(ch) && TRAIN_ARCANE_ARCHER(guild_nr)) ||
	    (!IS_TSUNA(ch) && TRAIN_ARCANE_TRICKSTER(guild_nr)) ||
	    (!IS_KURZAK(ch) && TRAIN_ARCHMAGE(guild_nr)) ||
            (!IS_ASSASSIN(ch) && TRAIN_ASSASSIN(guild_nr)) ||
            (!IS_BLACKGUARD(ch) && TRAIN_BLACKGUARD(guild_nr)) ||
            (!IS_DRAGON_DISCIPLE(ch) && TRAIN_DRAGON_DISCIPLE(guild_nr)) ||
            (!IS_DUELIST(ch) && TRAIN_DUELIST(guild_nr)) ||
            (!IS_DWARVEN_DEFENDER(ch) && TRAIN_DWARVEN_DEFENDER(guild_nr)) ||
            (!IS_ELDRITCH_KNIGHT(ch) && TRAIN_ELDRITCH_KNIGHT(guild_nr)) ||
            (!IS_HIEROPHANT(ch) && TRAIN_HIEROPHANT(guild_nr)) ||
            (!IS_HORIZON_WALKER(ch) && TRAIN_HORIZON_WALKER(guild_nr)) ||
            (!IS_LOREMASTER(ch) && TRAIN_LOREMASTER(guild_nr)) ||
            (!IS_MYSTIC_THEURGE(ch) && TRAIN_MYSTIC_THEURGE(guild_nr)) ||
            (!IS_SHADOWDANCER(ch) && TRAIN_SHADOWDANCER(guild_nr)) ||
            (!IS_THAUMATURGIST(ch) && TRAIN_THAUMATURGIST(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((IS_HUMAN(ch) && NOTRAIN_HUMAN(guild_nr)) ||
		 (IS_SAIYAN(ch)   && NOTRAIN_SAIYAN(guild_nr)) ||
		 (IS_ICER(ch) && NOTRAIN_ICER(guild_nr)) ||
		 (IS_KONATSU(ch) && NOTRAIN_KONATSU(guild_nr)) ||
		 (IS_NAMEK(ch) && NOTRAIN_NAMEK(guild_nr)) ||
		 (IS_MUTANT(ch) && NOTRAIN_MUTANT(guild_nr)) ||
		 (IS_KANASSAN(ch) && NOTRAIN_KANASSAN(guild_nr)) ||
 		 (IS_ANDROID(ch) && NOTRAIN_ANDROID(guild_nr)) ||
		 (IS_BIO(ch) && NOTRAIN_BIO(guild_nr)) ||
		 (IS_DEMON(ch) && NOTRAIN_DEMON(guild_nr)) ||
 		 (IS_MAJIN(ch) && NOTRAIN_MAJIN(guild_nr)) ||
 		 (IS_KAI(ch) && NOTRAIN_KAI(guild_nr)) ||
 		 (IS_TRUFFLE(ch) && NOTRAIN_TRUFFLE(guild_nr)) ||
 		 (IS_GOBLIN(ch) && NOTRAIN_GOBLIN(guild_nr)) ||
 		 (IS_ANIMAL(ch) && NOTRAIN_ANIMAL(guild_nr)) ||
 		 (IS_ORC(ch) && NOTRAIN_ORC(guild_nr)) ||
 		 (IS_SNAKE(ch) && NOTRAIN_SNAKE(guild_nr)) ||
		 (IS_TROLL(ch) && NOTRAIN_TROLL(guild_nr)) ||
 		 (IS_HALFBREED(ch) && NOTRAIN_HALFBREED(guild_nr)) ||
 		 (IS_MINOTAUR(ch) && NOTRAIN_MINOTAUR(guild_nr)) ||
 		 (IS_ARLIAN(ch) && NOTRAIN_KOBOLD(guild_nr))) {
		snprintf(buf, sizeof(buf), "%s %s", 
					GET_NAME(ch), MSG_TRAINER_DISLIKE_RACE);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_guild_ok(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
	if (is_guild_open(keeper, guild_nr, TRUE))
		return (is_guild_ok_char(keeper, ch, guild_nr));

	return (FALSE);
}


int does_guild_know(int guild_nr, int i)
{
  return ((int)(guild_index[guild_nr].skills[i]));
}

int does_guild_know_feat(int guild_nr, int i)
{
  return ((int)(guild_index[guild_nr].feats[i]));
}


void sort_spells(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < SKILL_TABLE_SIZE; a++)
    spell_sort_info[a] = a;

  qsort(&spell_sort_info[1], SKILL_TABLE_SIZE, sizeof(int), compare_spells);
}


/* this and list skills should probally be combined.  perhaps in the
 * next release?  */
void what_does_guild_know(int guild_nr, struct char_data * ch)
{
  const char *overflow = "\r\n**OVERFLOW**\r\n";
  char buf2[MAX_STRING_LENGTH];
  int i, sortpos, canknow, j, k, count = 0, cost = 0;
  size_t nlen = 0, len = 0;

  len = snprintf(buf2, sizeof(buf2), "You have %d practice session%s remaining.\r\n", GET_PRACTICES(ch, GET_CLASS(ch)), GET_PRACTICES(ch, GET_CLASS(ch)) == 1 ? "" : "s"); // ???

  nlen = snprintf(buf2 + len, sizeof(buf2) - len, "You can practice these skills:     @CKnown@D/@cPrac. Max    @GPS Cost@n\r\n@w-------------------------------------------------------------@n\r\n");
  len += nlen;
  
  /* Need to check if trainer can train doesnt do it now ??? */
  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, i) && skill_type(i) == SKTYPE_SKILL) {
      for (canknow = 0, k = 0, j = 0; j <= NUM_CLASSES; j++)
        if (GET_CLASS_RANKS(ch, j) > 0 && (spell_info[i].can_learn_skill[j] > SKLEARN_CANT)) {
          k = spell_info[i].can_learn_skill[j];
        }
        canknow = highest_skill_value(GET_LEVEL(ch), k);
        count++;
        cost = calculate_skill_cost(ch, i);
        if (k == SKLEARN_CLASS) {
          if (GET_SKILL_BASE(ch, i) < canknow) {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@y(@Y%2d@y) @W%-30s @y(@Y%2d@y) @C%d@D/@c%3d        @g%d%s@n\r\n", count, spell_info[i].name, count, GET_SKILL_BASE(ch, i), canknow, cost, GET_SKILL_BASE(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
            if (len + nlen >= sizeof(buf2) || nlen < 0)
              break;
            len += nlen;
          } else {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@y(@Y%2d@y) @W%-30s @y(@Y%2d@y) @C%d@D/@c%3d        @g%d%s@n\r\n", count, spell_info[i].name, count, GET_SKILL_BASE(ch, i), canknow, cost, GET_SKILL_BASE(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
          }
        } else {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@y(@Y%2d@y) @W%-30s @y(@Y%2d@y) @C%d@D/@c%3d        @g%d%s@n\r\n", count, spell_info[i].name, count, GET_SKILL_BASE(ch, i), canknow, cost, GET_SKILL_BASE(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
    }
  }

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) {
    i = feat_sort_info[sortpos];
    if (does_guild_know_feat(guild_nr, i) && feat_is_available(ch, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn) {
      nlen = snprintf(buf2 + len, sizeof(buf2) - len, "@b%-20s@n\r\n", feat_list[i].name);
      if (len + nlen >= sizeof(buf2) || nlen < 0)
        break;
      len += nlen;
    }
  }

  if (CONFIG_ENABLE_LANGUAGES) {
  len += snprintf(buf2 + len, sizeof(buf2) - len, "\r\nand the following languages:\r\n");

  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(guild_nr, i) && IS_SET(skill_type(i), SKTYPE_LANG)) {
      //if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
        for (canknow = 0, j = 0; j < NUM_CLASSES; j++)
	  if (spell_info[i].can_learn_skill[j] > canknow)
	    canknow = spell_info[i].can_learn_skill[j];
        canknow = highest_skill_value(GET_LEVEL(ch), canknow);
        if (GET_SKILL_BASE(ch, i) < canknow) {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %s\r\n", spell_info[i].name, GET_SKILL_BASE(ch, i) ? "known" : "unknown");
          if (len + nlen >= sizeof(buf2) || nlen < 0)
            break;
          len += nlen;
        }
      //}
    }
  }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  page_string(ch->desc, buf2, TRUE);
}

int prereq_pass(struct char_data *ch, int snum)
{
 if (snum == SKILL_KOUSENGAN || snum == SKILL_TSUIHIDAN || snum == SKILL_RENZO || snum == SKILL_SHOGEKIHA) {
  if (GET_SKILL_BASE(ch, SKILL_KIBALL) < 40 || GET_SKILL_BASE(ch, SKILL_KIBLAST) < 40 || GET_SKILL_BASE(ch, SKILL_BEAM) < 40) {
   send_to_char(ch, "You can not train that skill until you at least have trained Kiball, Kiblast, and Beam to Skill LVL 40.");
   return 0;
  }
 } else if (snum == SKILL_INSTANTT) {
  if (GET_SKILL_BASE(ch, SKILL_FOCUS) < 90 || GET_SKILL_BASE(ch, SKILL_CONCENTRATION) < 90 || GET_SKILL_BASE(ch, SKILL_ZANZOKEN) < 90) {
   send_to_char(ch, "You can not train instant transmission until you have Focus, Concentration, and Zanzoken up to Skill LVL 90.");
   return 0;
  }
 } else if (snum == SKILL_SLAM) {
  if (GET_SKILL_BASE(ch, SKILL_UPPERCUT) < 50) {
   send_to_char(ch, "You can not train that skill until you at least have trained uppercut to Skill LVL 50.");
   return 0;
  }
 } else if (snum == SKILL_UPPERCUT) {
  if (GET_SKILL_BASE(ch, SKILL_ELBOW) < 40) {
   send_to_char(ch, "You can not train that skill until you at least have trained elbow to Skill LVL 40.");
   return 0;
  }
 } else if (snum == SKILL_HEELDROP) {
  if (GET_SKILL_BASE(ch, SKILL_ROUNDHOUSE) < 50) {
   send_to_char(ch, "You can not train that skill until you at least have trained roundhouse to Skill LVL 50.");
   return 0;
  }
 } else if (snum == SKILL_ROUNDHOUSE) {
  if (GET_SKILL_BASE(ch, SKILL_KNEE) < 40) {
   send_to_char(ch, "You can not train that skill until you at least have trained knee to Skill LVL 40.");
   return 0;
  }
 } else if (snum == SKILL_KIBALL || snum == SKILL_KIBLAST || snum == SKILL_BEAM) {
  if (GET_SKILL_BASE(ch, SKILL_FOCUS) < 30) {
   send_to_char(ch, "You can not train that skill until you at least have trained focus to Skill LVL 30.");
   return 0;
  }
 } else if (IS_SET(spell_info[snum].flags, SKFLAG_TIER2) || IS_SET(spell_info[snum].flags, SKFLAG_TIER3)) {
  if (snum != 530 && snum != 531) {
   if (GET_SKILL_BASE(ch, SKILL_TSUIHIDAN) < 40 || GET_SKILL_BASE(ch, SKILL_RENZO) < 40 || GET_SKILL_BASE(ch, SKILL_SHOGEKIHA) < 40) {
    send_to_char(ch, "You can not train that skill until you at least have trained Tsuihidan, Renzokou Energy Dan, and Shogekiha to Skill LVL 40.");
    return 0;
   }
  }
 } else if (IS_SET(spell_info[snum].flags, SKFLAG_TIER4)) {
  if (IS_ROSHI(ch) && (GET_SKILL_BASE(ch, SKILL_KAMEHAMEHA) < 40 || GET_SKILL_BASE(ch, SKILL_KIENZAN) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Kamehameha and Kienzan to Skill LVL 40.");
   return 0;
  }
  if (IS_TSUNA(ch) && (GET_SKILL_BASE(ch, SKILL_WRAZOR) < 40 || GET_SKILL_BASE(ch, SKILL_WSPIKE) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Water Razor and Water Spikes to Skill LVL 40.");
   return 0;
  }
  if (IS_PICCOLO(ch) && (GET_SKILL_BASE(ch, SKILL_MASENKO) < 40 || GET_SKILL_BASE(ch, SKILL_SBC) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Masenko and Special Beam Cannon to Skill LVL 40.");
   return 0;
  }
  if (IS_FRIEZA(ch) && (GET_SKILL_BASE(ch, SKILL_DEATHBEAM) < 40 || GET_SKILL_BASE(ch, SKILL_KIENZAN) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Deathbeam and Kienzan to Skill LVL 40.");
   return 0;
  }
  if (IS_GINYU(ch) && (GET_SKILL_BASE(ch, SKILL_CRUSHER) < 40 || GET_SKILL_BASE(ch, SKILL_ERASER) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Crusher Ball and Eraser Cannon to Skill LVL 40.");
   return 0;
  }
  if (IS_BARDOCK(ch) && (GET_SKILL_BASE(ch, SKILL_GALIKGUN) < 40 || GET_SKILL_BASE(ch, SKILL_FINALFLASH) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Galik Gun and Final Flash to Skill LVL 40.");
   return 0;
  }
  if (IS_TAPION(ch) && (GET_SKILL_BASE(ch, SKILL_TSLASH) < 40 || GET_SKILL_BASE(ch, SKILL_DDSLASH) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Twin Slash and Darkness Dragon Slash to Skill LVL 40.");
   return 0;
  }
  if (IS_NAIL(ch) && (GET_SKILL_BASE(ch, SKILL_MASENKO) < 40 || GET_SKILL_BASE(ch, SKILL_KOUSENGAN) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Masenko and Kousengan to Skill LVL 40.");
   return 0;
  }
  if (IS_ANDROID(ch) && (GET_SKILL_BASE(ch, SKILL_DUALBEAM) < 40 || GET_SKILL_BASE(ch, SKILL_HELLFLASH) < 40)) {
   send_to_char(ch, "You can not train that skill until you at least have trained Dual Beam and Hell Flash to Skill LVL 40.");
   return 0;
  }
  if (IS_JINTO(ch) && GET_SKILL_BASE(ch, SKILL_BREAKER) < 40) {
   send_to_char(ch, "You can not train that skill until you at least have trained Star Breaker to Skill LVL 40.");
   return 0;
  }
 } else if (IS_SET(spell_info[snum].flags, SKFLAG_TIER5)) {
  if (GET_SKILL_BASE(ch, SKILL_FOCUS) < 60 || GET_SKILL_BASE(ch, SKILL_CONCENTRATION) < 80 ) {
   send_to_char(ch, "You can not train that skill until you at least have trained focus to Skill LVL 60 and concentration to Skill LVL 80.");
   return 0;
  }  
 }
  return 1;
}


void handle_forget(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{

 int skill_num;
  
 skip_spaces(&argument);

 if (!*argument) {
  send_to_char(ch, "What skill do you want to start to forget?\r\n");
  return;
 }

 skill_num = find_skill_num(argument, SKTYPE_SKILL);

 if (GET_SKILL_BASE(ch, skill_num) > 30) {
  send_to_char(ch, "@MYou can not forget that skill, you know too much about it.@n\r\n");
  return;
 } else if (skill_num == SKILL_MIMIC && GET_MIMIC(ch) > 0) {
  send_to_char(ch, "@MYou can not forget mimic while you are using it!\r\n");
 } else if (skill_num == SKILL_FOCUS) {
  send_to_char(ch, "@MYou can not forget such a fundamental skill!@n\r\n");
 } else if (GET_SKILL_BASE(ch, skill_num) <= 0) {
  send_to_char(ch, "@MYou can not forget a skill you don't know!@n\r\n");
 } else if (GET_FORGETING(ch) == skill_num) {
  send_to_char(ch, "@MYou stop forgetting %s@n\r\n", spell_info[skill_num].name);
  GET_FORGET_COUNT(ch) = 0;
  GET_FORGETING(ch) = 0;
 } else if (GET_FORGETING(ch) != 0) {
  send_to_char(ch, "@MYou stop forgetting %s, and start trying to forget %s.@n\r\n", spell_info[GET_FORGETING(ch)].name, spell_info[skill_num].name); 
  GET_FORGET_COUNT(ch) = 0;
  GET_FORGETING(ch) = skill_num;
 } else {
  send_to_char(ch, "@MYou start trying to forget %s.@n\r\n", spell_info[skill_num].name);
  GET_FORGET_COUNT(ch) = 0;
  GET_FORGETING(ch) = skill_num;
 }

}

void handle_grand(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{

 int skill_num;

 skip_spaces(&argument);

 if (!CAN_GRAND_MASTER(ch)) {
  send_to_char(ch, "Your race can not become a Grand Master in a skill through this process.\r\n");
  return;
 }

 if (!*argument) {
  send_to_char(ch, "What skill do you want to become a Grand Master in?");
  return;
 }

 skill_num = find_skill_num(argument, SKTYPE_SKILL);
 char buf[MAX_STRING_LENGTH];

 if (!(does_guild_know(guild_nr, skill_num))) {
   snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
   do_tell(keeper, buf, cmd_tell, 0);
   return;
 }


 if (GET_SKILL_BASE(ch, skill_num) <= 0) {
  send_to_char(ch, "You do not know that skill!\r\n");
  return;
 } else if (GET_SKILL_BASE(ch, skill_num) < 100) {
  send_to_char(ch, "You haven't even mastered that skill. How can you become a Grand Master in it?\r\n");
  return;
 } else if (GET_SKILL_BASE(ch, skill_num) >= 103) {
  send_to_char(ch, "You have already become a Grand Master in that skill and have progessed as far as possible in it.\r\n");
  return;
 } else if (GET_PRACTICES(ch, GET_CLASS(ch)) < 1000) {
  send_to_char(ch, "You need at least 1,000 practice sessions to rank up beyond 100 in a skill.\r\n");
  return;
 } else {
  if (GET_SKILL_BASE(ch, skill_num) == 100) {
   send_to_char(ch, "@YYou have ascended to Grand Master in the skill, @C%s@Y.\r\n", spell_info[skill_num].name);
  } else {
   send_to_char(ch, "@YYou have ranked up in your Grand Mastery of the skill, @C%s@Y.\r\n", spell_info[skill_num].name);
  }
  SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + 1);
  GET_PRACTICES(ch, GET_CLASS(ch)) -= 1000;
 }

}

void handle_practice(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument, int skill)
{
  int percent = GET_SKILL(ch, skill);
  int skill_num, learntype, pointcost, highest, i;
  char buf[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (!*argument) {
    what_does_guild_know(guild_nr, ch);
    return;
  }

  if (GET_PRACTICES(ch, GET_CLASS(ch)) <= 0) {
    send_to_char(ch, "You do not seem to be able to practice now.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SHOCKED)) {
   send_to_char(ch, "You can not practice while your mind is shocked!\r\n");
   return;
  }

  skill_num = find_skill_num(argument, SKTYPE_SKILL);

  if (strstr(sensei_style[GET_CLASS(ch)], argument)) {
   skill_num = 539;
  }

  if (skill_num == GET_FORGETING(ch)) {
   send_to_char(ch, "You can't practice that! You are trying to forget it!@n\r\n");
   return;
  }

  /****  Does the GM know the skill the player wants to learn?  ****/
  if (!(does_guild_know(guild_nr, skill_num))) {
    snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }

  /**** Can the player learn the skill if the GM knows it?  ****/ 
  if (IS_SET(spell_info[skill_num].skilltype, SKTYPE_SKILL)) {
    for (learntype = 0, i = 0; i < NUM_CLASSES; i++)
      if (spell_info[skill_num].can_learn_skill[i] > learntype)
        learntype = spell_info[skill_num].can_learn_skill[i];
    switch (learntype) {
    case SKLEARN_CANT:
      snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
      return;
    case SKLEARN_CROSSCLASS:
      highest = highest_skill_value(GET_LEVEL(ch), learntype);
      break;
    case SKLEARN_CLASS:
      highest = highest_skill_value(GET_LEVEL(ch), learntype);
      break;
    default:
      log("Unknown SKLEARN type for skill %d in practice", skill_num);
      send_to_char(ch, "You can't learn that.\r\n");
      return;
    }
    pointcost = calculate_skill_cost(ch, skill_num);
    if (GET_PRACTICES(ch, GET_CLASS(ch)) >= pointcost) {
      if (!prereq_pass(ch, skill_num)) {
        return;
      } if (GET_SKILL_BASE(ch, skill_num) >= highest) {
        send_to_char(ch, "You cannot increase that skill again until you progress further.\r\n");
        return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 75 && GET_BONUS(ch, BONUS_MASOCHISTIC) > 0) {
       if (skill_num == SKILL_PARRY || skill_num == SKILL_ZANZOKEN || skill_num == SKILL_DODGE || skill_num == SKILL_BARRIER || skill_num == SKILL_BLOCK || skill_num == SKILL_TSKIN) {
         send_to_char(ch, "You cannot increase that skill again because it would deny you the pain you enjoy.\r\n");
         return;
       }
      } if (GET_SKILL_BASE(ch, skill_num) >= 75 && IS_TAPION(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 75 && IS_DABURA(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 75 && IS_JINTO(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 75 && IS_TSUNA(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 50 && IS_FRIEZA(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 50 && IS_ANDSIX(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 50 && IS_KURZAK(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 50 && IS_GINYU(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 50 && IS_BARDOCK(ch) && skill_num == SKILL_SENSE) {
          send_to_char(ch, "You cannot practice that anymore.\r\n");
            return;
      } if (GET_SKILL_BASE(ch, skill_num) >= 100) {
        send_to_char(ch, "You know everything about that skill.\r\n");
        return;
      } else {
        if (GET_SKILL_BASE(ch, skill_num) == 0) {
         if (slot_count(ch) < GET_SLOTS(ch)) {
          if (skill_num != 539)
           send_to_char(ch, "You practice and master the basics!\r\n");
          else
           send_to_char(ch, "You practice the basics of %s\r\n", sensei_style[GET_CLASS(ch)]);
          SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + rand_number(10, 25));
          GET_PRACTICES(ch, GET_CLASS(ch)) -= pointcost;
          if (GET_FORGETING(ch) != 0 && GET_SKILL_BASE(ch, GET_FORGETING(ch)) < 30) {
           GET_FORGET_COUNT(ch) += 1;
           if (GET_FORGET_COUNT(ch) >= 5) {
            SET_SKILL(ch, GET_FORGETING(ch), 0);
            send_to_char(ch, "@MYou have finally forgotten what little you knew of %s@n\r\n", spell_info[GET_FORGETING(ch)].name);
            GET_FORGETING(ch) = 0;
            GET_FORGET_COUNT(ch) = 0;
            save_char(ch);
           }
          } else if (GET_SKILL_BASE(ch, GET_FORGETING(ch)) < 30) {
           GET_FORGETING(ch) = 0;
          }
         } else {
          send_to_char(ch, "You already know the maximum number of skills you can for the time being!\r\n");
          return;
         }
        }
        else {
          if (skill_num != 539)
           send_to_char(ch, "You practice for a while and manage to advance your technique.\r\n");
          else
           send_to_char(ch, "You practice the basics of %s\r\n", sensei_style[GET_CLASS(ch)]);
         SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + 1);
         GET_PRACTICES(ch, GET_CLASS(ch)) -= pointcost;
         if (GET_SKILL_BASE(ch, skill_num) >= 100) {
          send_to_char(ch, "You learned a lot by mastering that skill.\r\n");
          if (IS_KONATSU(ch) && skill_num == SKILL_PARRY) {
           SET_SKILL(ch, skill_num, GET_SKILL_BASE(ch, skill_num) + 5);
          }
          gain_exp(ch, level_exp(ch, GET_LEVEL(ch) + 1) / 20);
         }
          if (GET_FORGETING(ch) != 0) {
           GET_FORGET_COUNT(ch) += 1;
           if (GET_FORGET_COUNT(ch) >= 5) {
            SET_SKILL(ch, GET_FORGETING(ch), 0);
            send_to_char(ch, "@MYou have finally forgotten what little you knew of %s@n\r\n", spell_info[GET_FORGETING(ch)].name);
            GET_FORGETING(ch) = 0;
            GET_FORGET_COUNT(ch) = 0;
            save_char(ch);
           }
          }
        }
      }
    } else {
      send_to_char(ch, "You need %d practice session%s to increase that skill.\r\n",
             pointcost, (pointcost == 1) ? "" : "s");
    }
  } else {
    snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
  }
}


void handle_train(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  skip_spaces(&argument);
  if (!argument || !*argument)
    send_to_char(ch, "Training sessions remaining: %d\r\n"
                 "Stats: strength constitution agility intelligence wisdom speed\r\n",
                 GET_TRAINS(ch));
  else if (!GET_TRAINS(ch))
    send_to_char(ch, "You have no ability training sessions.\r\n");
  else if (!strncasecmp("strength", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.str += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("constitution", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.con += 1;
    /* Give them retroactive hit points for constitution */
    if (! (ch->real_abils.con % 2))
      GET_MAX_HIT(ch) += GET_LEVEL(ch);
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("agility", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.dex += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("intelligence", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.intel += 1;
    /* Give extra skill practice, but only for this level */
    if (! (ch->real_abils.intel % 2))
      GET_PRACTICES(ch, GET_CLASS(ch)) += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("wisdom", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.wis += 1;
    GET_TRAINS(ch) -= 1;
  } else if (!strncasecmp("speed", argument, strlen(argument))) {
    send_to_char(ch, CONFIG_OK);
    ch->real_abils.cha += 1;
    GET_TRAINS(ch) -= 1;
  } else
    send_to_char(ch, "Stats: strength constitution agility intelligence wisdom speed\r\n");
  affect_total(ch);
  return;
}


void handle_gain(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int whichclass = GET_CLASS(ch);

  skip_spaces(&argument);


  if (GET_LEVEL(ch) < 100 && GET_EXP(ch) >= level_exp(ch, GET_LEVEL(ch) + 1)) {
    if (GET_RP(ch) < rpp_to_level(ch)) {
     send_to_char(ch, "You need at least %d RPP to gain the next level.\r\n", rpp_to_level(ch));
    }
    else if (rpp_to_level(ch) <= GET_RP(ch)) {
     GET_RP(ch) -= rpp_to_level(ch);
     ch->desc->rpp = GET_RP(ch);
     userWrite(ch->desc, 0, 0, 0, "index");
     send_to_char(ch, "@D(@cRPP@W: @w-%d@D)@n\n\n", rpp_to_level(ch));
     gain_level(ch, whichclass);
    }
    else {
     gain_level(ch, whichclass);
    }
  } else {
    send_to_char(ch, "You are not yet ready for further advancement.\r\n");
  }
  return;
}

int rpp_to_level(struct char_data *ch)
{

 if (GET_LEVEL(ch) == 2) {
   if IS_SAIYAN(ch)
    return 60;
   else if IS_BIO(ch)
    return 35;
   else if IS_MAJIN(ch)
    return 55;
   else if IS_GOBLIN(ch)
    return 30;
  } else if (GET_LEVEL(ch) >= 90) {
    if (GET_LEVEL(ch) == 91)
     return 3;
    else if (GET_LEVEL(ch) == 92)
     return 3;
    else if (GET_LEVEL(ch) == 93)
     return 3;
    else if (GET_LEVEL(ch) == 94)
     return 3;
    else if (GET_LEVEL(ch) == 95)
     return 3;
    else if (GET_LEVEL(ch) == 96)
     return 4;
    else if (GET_LEVEL(ch) == 97)
     return 4;
    else if (GET_LEVEL(ch) == 98)
     return 4;
    else if (GET_LEVEL(ch) == 99)
     return 5;
    } else {
     return 0;
     }
/*
 if (!IS_TRUFFLE(ch) && !IS_MUTANT(ch) && !IS_KONATSU(ch)) {
  if (GET_LEVEL(ch) >= 60) {
   if (GET_LEVEL(ch) == 61)
    return 0;
   else if (GET_LEVEL(ch) == 71)
    return 0;
   else if (GET_LEVEL(ch) == 81)
    return 0;
   else if (GET_LEVEL(ch) == 91)
    return 0;
   else if (GET_LEVEL(ch) % 2 == 0)
    return 2;
   else
    return 3;
  } else if (GET_LEVEL(ch) >= 30) {
   if (GET_LEVEL(ch) == 31)
    return 0;
   else if (GET_LEVEL(ch) == 41)
    return 0;
   else if (GET_LEVEL(ch) == 51)
    return 0;
   else if (GET_LEVEL(ch) % 2 == 0)
    return 1;
   else
    return 2;
  } else if (GET_LEVEL(ch) >= 15) {
   if (GET_LEVEL(ch) == 21)
    return 0;
   else if (GET_LEVEL(ch) % 2 == 0)
    return 0;
   else
    return 1;
  } else {
   return 0;
  }
 } else {
  if (GET_LEVEL(ch) >= 60) {
   if (GET_LEVEL(ch) == 61)
    return 0;
   else if (GET_LEVEL(ch) == 71)
    return 0;
   else if (GET_LEVEL(ch) == 81)
    return 0;
   else if (GET_LEVEL(ch) == 91)
    return 0;
   else if (GET_LEVEL(ch) % 2 == 0)
    return 2;
   else
    return 3;
  } else if (GET_LEVEL(ch) >= 30) {
   if (GET_LEVEL(ch) == 31)
    return 0;
   else if (GET_LEVEL(ch) == 41)
    return 0;
   else if (GET_LEVEL(ch) == 51)
    return 0;
   else if (GET_LEVEL(ch) % 2 == 0)
    return 1;
   else
    return 2;
  } else {
   return 0;
  }
 }*/
}

void handle_exp(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
 if (GET_PRACTICES(ch, GET_CLASS(ch)) < 25) {
    send_to_char(ch, "You need at least 25 practice sessions to learn.\r\n");
    return;
 }
 if (GET_EXP(ch) > level_exp(ch, GET_LEVEL(ch) + 1) && GET_LEVEL(ch) != 100) {
    send_to_char(ch, "You can't learn with negative TNL.\r\n");
    return;
 }
 else {
  int64_t amt = level_exp(ch, GET_LEVEL(ch) + 1) / 100;
  if (GET_LEVEL(ch) == 100) {
   amt = 400000;
  }
  act("@c$n@W spends time training you in $s fighting style.@n", TRUE, keeper, 0, ch, TO_VICT);
  act("@c$n@W spends time training @C$N@W in $s fighting style.@n", TRUE, keeper, 0, ch, TO_NOTVICT);
  send_to_char(ch, "@wExperience Gained: @C%s@n\r\n", add_commas(amt));
  GET_PRACTICES(ch, GET_CLASS(ch)) -= 25;
  if (IS_SAIYAN(ch) || IS_HALFBREED(ch)) {
   amt = amt + (amt * .30);
  }
  if (IS_ICER(ch)) {
   amt = amt - (amt * .10);
  }
  gain_exp(ch, amt); 
  return;
 }
}

void handle_study(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{

 int expcost = 25000, goldcost = 750, fail = FALSE, reward = 25, goldadjust = 0, expadjust = 0;

 if (GET_LEVEL(ch) >= 100) {
  goldadjust = 500;
  expadjust = 15000;
 } else if (GET_LEVEL(ch) >= 91) {
  goldadjust = 450;
  expadjust = 12500;
 } else if (GET_LEVEL(ch) >= 81) {
  goldadjust = 400;
  expadjust = 10000;
 } else if (GET_LEVEL(ch) >= 71) {
  goldadjust = 350;
  expadjust = 7500;
 } else if (GET_LEVEL(ch) >= 61) {
  goldadjust = 300;
  expadjust = 5000;
 } else if (GET_LEVEL(ch) >= 51) {
  goldadjust = 250;
  expadjust = 2500;
 } else if (GET_LEVEL(ch) >= 41) {
  goldadjust = 200;
 } else if (GET_LEVEL(ch) >= 31) {
  goldadjust = 150;
 } else if (GET_LEVEL(ch) >= 21) {
  goldadjust = 100;
 } else if (GET_LEVEL(ch) >= 11) {
  goldadjust = 50;
 }

 goldcost += goldadjust;
 expcost += expadjust;

 if (GET_EXP(ch) < expcost) {
  send_to_char(ch, "You do not have enough experience to study. @D[@wCost@W: @G%s@D]@n\r\n", add_commas(expcost));
  fail = TRUE;
 }

 if (GET_GOLD(ch) < goldcost) {
  send_to_char(ch, "You do not have enough zenni to study. @D[@wCost@W: @Y%s@D]@n\r\n", add_commas(goldcost));
  fail = TRUE;
 }

 if (fail == TRUE)
  return;

 GET_EXP(ch) -= expcost;
 GET_GOLD(ch) -= goldcost;
 GET_PRACTICES(ch, GET_CLASS(ch)) += 25;
 
 act("@c$N@W spends time lecturing you on various subjects.@n", TRUE, ch, 0, keeper, TO_CHAR);
 act("@c$N@W spends time lecturing @C$n@W on various subjects.@n", TRUE, ch, 0, keeper, TO_ROOM);
 send_to_char(ch, "@wYou have gained %d practice sessions in exchange for %s EXP and %s zenni.\r\n", reward, add_commas(expcost), add_commas(goldcost));

}

void handle_learn(struct char_data *keeper, int guild_nr, struct char_data *ch, char *argument)
{
  int feat_num, subval, sftype, subfeat;
  char *ptr;
  const char *cptr;
  char buf[MAX_STRING_LENGTH];

  if (!*argument) {
    send_to_char(ch, "Which feat would you like to learn?\r\n");
    return;
  }

  if (GET_FEAT_POINTS(ch) < 1) {
    send_to_char(ch, "You can't learn any new feats right now.\r\n");
    return;
  }

  ptr = strchr(argument, ':');
  if (ptr)
    *ptr = 0;
  feat_num = find_feat_num(argument);
  if (ptr)
    *ptr = ':';

  if (!(does_guild_know_feat(guild_nr, feat_num))) {
    snprintf(buf, sizeof(buf), guild_index[guild_nr].no_such_skill, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }

  if (HAS_FEAT(ch, feat_num) && !feat_list[feat_num].can_stack) {
    send_to_char(ch, "You already know the %s feat.\r\n", feat_list[feat_num].name);
    return;
  }

  if (!feat_is_available(ch, feat_num, 0, NULL) || !feat_list[feat_num].in_game || !feat_list[feat_num].can_learn) {
    send_to_char(ch, "The %s feat is not available to you at this time.\r\n", argument);
    return;
  }

  sftype = 2;
  switch (feat_num) {
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
  case FEAT_GREATER_WEAPON_FOCUS:
  case FEAT_WEAPON_SPECIALIZATION:
  case FEAT_WEAPON_FOCUS:
  case FEAT_WEAPON_FINESSE:
  case FEAT_IMPROVED_CRITICAL:
    sftype = 1;
  case FEAT_SPELL_FOCUS:
  case FEAT_GREATER_SPELL_FOCUS:
    subfeat = feat_to_subfeat(feat_num);
    if (subfeat == -1) {
      log("guild: Unconfigured subfeat '%s', check feat_to_subfeat()", feat_list[feat_num].name);
      send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return;
    }
    if (ptr == NULL || !*ptr) {
      if (sftype == 2)
        cptr = "spell school";
      else
        cptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "No ':' found. You must specify a %s to improve. Example:\r\n"
                         " learn %s: %s\r\nAvailable %s:\r\n", cptr,
                         feat_list[feat_num].name,
                         (sftype == 2) ? spell_schools[0] : weapon_type[0], cptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          cptr = spell_schools[subval];
        else
          cptr = weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", cptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    if (*ptr == ':') ptr++;
    skip_spaces(&ptr);
    if (!ptr || !*ptr) {
      if (sftype == 2)
        cptr = "spell school";
      else
        cptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "No %s found. You must specify a %s to improve.\r\n\r\nExample:\r\n"
                         " learn %s: %s\r\n\r\nAvailable %s:\r\n", cptr, cptr,
                         feat_list[feat_num].name,
                         (sftype == 2) ? spell_schools[0] : weapon_type[0], cptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          cptr = spell_schools[subval];
        else
          cptr = weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", cptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    subval = search_block(ptr, (sftype == 2) ? spell_schools : weapon_type, FALSE);
    if (subval == -1) {
      log("bad subval: %s", ptr);
      if (sftype == 2)
        ptr = "spell school";
      else
        ptr = "weapon type";
      subfeat = snprintf(buf, sizeof(buf),
                         "That is not a known %s. Available %s:\r\n",
                         ptr, ptr);
      for (subval = 1; subval <= (sftype == 2 ? NUM_SCHOOLS : MAX_WEAPON_TYPES); subval++) {
        if (sftype == 2)
          cptr = spell_schools[subval];
        else
          cptr = weapon_type[subval];
        subfeat += snprintf(buf + subfeat, sizeof(buf) - subfeat, "  %s\r\n", cptr);
      }
      page_string(ch->desc, buf, TRUE);
      return;
    }
    if (!feat_is_available(ch, feat_num, subval, NULL)) {
      send_to_char(ch, "You do not satisfy the prerequisites for that feat.\r\n");
      return;
    }
    if (sftype == 1) {
      if (HAS_COMBAT_FEAT(ch, subfeat, subval)) {
        send_to_char(ch, "You already have that weapon feat.\r\n");
        return;
      }
      SET_COMBAT_FEAT(ch, subfeat, subval);
    } else if (sftype == 2) {
      if (HAS_SCHOOL_FEAT(ch, subfeat, subval)) {
        send_to_char(ch, "You already have that spell school feat.\r\n");
        return;
      }
      SET_SCHOOL_FEAT(ch, subfeat, subval);
    } else {
      log("unknown feat subtype %d in subfeat code", sftype);
      send_to_char(ch, "That feat is not yet ready for use.\r\n");
      return;
    }
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  case FEAT_GREAT_FORTITUDE:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_FORTITUDE) += 2;
    break;
  case FEAT_IRON_WILL:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_WILL) += 2;
    break;
  case FEAT_LIGHTNING_REFLEXES:
    SET_FEAT(ch, feat_num, 1);
    GET_SAVE_MOD(ch, SAVING_REFLEX) += 2;
    break;
  case FEAT_TOUGHNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_MAX_HIT(ch) += 3;
    break;
  case FEAT_SKILL_FOCUS:
    if (!ptr || !*ptr) {
      send_to_char(ch, "You must specify a skill to improve. Syntax:\r\n  learn skill focus: skill\r\n");
      return;
    }
    if (*ptr == ':') ptr++;
    skip_spaces(&ptr);
    if (!ptr || !*ptr) {
      send_to_char(ch, "You must specify a skill to improve. Syntax:\r\n  learn skill focus: skill\r\n");
      return;
    }
    if (GET_LEVEL(ch) <= 49) {
      send_to_char(ch, "You must be at least level 50 to gain this feat on a skill.\r\n");
      return;
    }
    subval = find_skill_num(ptr, SKTYPE_SKILL);
    if (subval < 0) {
      send_to_char(ch, "I don't recognize that skill.\r\n");
      return;
    }
    int snum = GET_SKILL(ch, subval);
    if (snum > 100) {
      send_to_char(ch, "You have already focused that skill as high as possible.\r\n");
      return;
    }
    SET_SKILL_BONUS(ch, subval, GET_SKILL_BONUS(ch, subval) + 5);
    SET_FEAT(ch, feat_num, HAS_FEAT(ch, feat_num) + 1);
    break;
  case FEAT_SPELL_MASTERY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    GET_SPELL_MASTERY_POINTS(ch) += MAX(1, ability_mod_value(GET_INT(ch)));
    break;
  case FEAT_ACROBATIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_AGILE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_BALANCE, GET_SKILL_BONUS(ch, SKILL_BALANCE) + 2);
    SET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST, GET_SKILL_BONUS(ch, SKILL_ESCAPE_ARTIST) + 2);
    break;
  case FEAT_ALERTNESS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_LISTEN, GET_SKILL_BONUS(ch, SKILL_LISTEN) + 2);
    SET_SKILL_BONUS(ch, SKILL_SPOT, GET_SKILL_BONUS(ch, SKILL_SPOT) + 2);
    break;
  case FEAT_ANIMAL_AFFINITY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_ATHLETIC:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_DECEITFUL:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_DISGUISE, GET_SKILL_BONUS(ch, SKILL_DISGUISE) + 2);
    SET_SKILL_BONUS(ch, SKILL_FORGERY, GET_SKILL_BONUS(ch, SKILL_FORGERY) + 2);
    break;
  case FEAT_DEFT_HANDS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND, GET_SKILL_BONUS(ch, SKILL_SLEIGHT_OF_HAND) + 2);
    break;
  case FEAT_DILIGENT:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_APPRAISE, GET_SKILL_BONUS(ch, SKILL_APPRAISE) + 2);
    break;
  case FEAT_INVESTIGATOR:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_EAVESDROP, GET_SKILL_BONUS(ch, SKILL_EAVESDROP) + 2);
    SET_SKILL_BONUS(ch, SKILL_SEARCH, GET_SKILL_BONUS(ch, SKILL_SEARCH) + 2);
    break;
  case FEAT_MAGICAL_APTITUDE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_NEGOTIATOR:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_NIMBLE_FINGERS:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_OPEN_LOCK, GET_SKILL_BONUS(ch, SKILL_OPEN_LOCK) + 2);
    break;
  case FEAT_PERSUASIVE:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    break;
  case FEAT_SELF_SUFFICIENT:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HEAL, GET_SKILL_BONUS(ch, SKILL_HEAL) + 2);
    SET_SKILL_BONUS(ch, SKILL_SURVIVAL, GET_SKILL_BONUS(ch, SKILL_SURVIVAL) + 2);
    break;
  case FEAT_STEALTHY:
    subval = HAS_FEAT(ch, feat_num) + 1;
    SET_FEAT(ch, feat_num, subval);
    SET_SKILL_BONUS(ch, SKILL_HIDE, GET_SKILL_BONUS(ch, SKILL_HIDE) + 2);
    SET_SKILL_BONUS(ch, SKILL_MOVE_SILENTLY, GET_SKILL_BONUS(ch, SKILL_MOVE_SILENTLY) + 2);
    break;
  default:
    SET_FEAT(ch, feat_num, TRUE);
    break;
  }
  save_char(ch);

  GET_FEAT_POINTS(ch)--;
  send_to_char(ch, "Your training has given you the %s feat!\r\n", feat_list[feat_num].name);

  return;
}


SPECIAL(guild)
{
  char arg[MAX_INPUT_LENGTH];
  int guild_nr, i;
  struct char_data *keeper = (struct char_data *) me;
  struct {
    const char *cmd;
    void (*func)(struct char_data *, int, struct char_data *, char *);
  } guild_cmd_tab[] = {
    { "practice",	handle_practice },
    { "gain",		handle_gain },
    { "forget",		handle_forget },
    { "study",		handle_study },
    { "grand",          handle_grand },
    { NULL,		NULL }
  };

  for (guild_nr = 0; guild_nr <= top_guild; guild_nr++)
    if (GM_TRAINER(guild_nr) == keeper->nr)
      break;

  if (guild_nr > top_guild)
    return (FALSE);

  if (GM_FUNC(guild_nr))
    if ((GM_FUNC(guild_nr)) (ch, me, cmd, arg))
      return (TRUE);

  /*** Is the GM able to train?    ****/
  if (!AWAKE(keeper))
    return (FALSE);

  for (i = 0; guild_cmd_tab[i].cmd; i++)
    if (CMD_IS(guild_cmd_tab[i].cmd))
      break;

  if (!guild_cmd_tab[i].cmd)
    return (FALSE);

  if (!(is_guild_ok(keeper, ch, guild_nr)))
    return (TRUE);

  (guild_cmd_tab[i].func)(keeper, guild_nr, ch, argument);

  return (TRUE);
}


/**** This function is here just because I'm extremely paranoid.  Take
      it out if you aren't ;)  ****/
void clear_skills(int gdindex)
{
  int i;

  for  (i = 0; i < SKILL_TABLE_SIZE; i++) 
    guild_index[gdindex].skills[i] = 0;
}


/****  This is ripped off of read_line from shop.c.  They could be
 *  combined. But why? ****/

void read_guild_line(FILE * gm_f, char *string, void *data, char *type)
{
	char buf[MAX_STRING_LENGTH];
	
	if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
		fprintf(stderr, "Error in guild #%d, Could not get %s\n", GM_NUM(top_guild), type);
		exit(1);
	}
}


void boot_the_guilds(FILE * gm_f, char *filename, int rec_count)
{
  char *buf, buf2[256], *p, buf3[READ_SIZE];
  int temp, val, t1, t2, rv;
  int done = FALSE;

  snprintf(buf2, sizeof(buf2), "beginning of GM file %s", filename);

  buf = fread_string(gm_f, buf2);
  while (!done) {
    if (*buf == '#') {		/* New Trainer */
      sscanf(buf, "#%d\n", &temp);
      snprintf(buf2, sizeof(buf2), "GM #%d in GM file %s", temp, filename);
      free(buf);		/* Plug memory leak! */
      top_guild++;
      if (!top_guild)
	CREATE(guild_index, struct guild_data, rec_count);
      GM_NUM(top_guild) = temp;

      clear_skills(top_guild);    
      get_line(gm_f, buf3);
      rv = sscanf(buf3, "%d %d", &t1, &t2);
      while( t1 > -1) {
        if (rv == 1) { /* old style guilds, only skills */
	  guild_index[top_guild].skills[(int)t1] = 1;
        } else if (rv == 2) { /* new style guilds, skills and feats */
          if (t2 == 1) {
	    guild_index[top_guild].skills[(int)t1] = 1;
          } else if (t2 == 2) {
	    guild_index[top_guild].feats[(int)t1] = 1;
          } else {
            log("SYSERR: Invalid 2nd arg in guild file!");
            exit(1);
          }
        } else {
          log("SYSERR: Invalid format in guild file. Expecting 2 args but got %d!", rv);
          exit(1);
        }
        get_line(gm_f, buf3);
        rv = sscanf(buf3, "%d %d", &t1, &t2);
      }
      read_guild_line(gm_f, "%f", &GM_CHARGE(top_guild), "GM_CHARGE");
      guild_index[top_guild].no_such_skill = fread_string(gm_f, buf2);
      guild_index[top_guild].not_enough_gold = fread_string(gm_f, buf2);

      read_guild_line(gm_f, "%d", &GM_MINLVL(top_guild), "GM_MINLVL");
      read_guild_line(gm_f, "%d", &GM_TRAINER(top_guild), "GM_TRAINER");

      GM_TRAINER(top_guild) = real_mobile(GM_TRAINER(top_guild));
      read_guild_line(gm_f, "%d", &GM_WITH_WHO(top_guild)[0], "GM_WITH_WHO");

      read_guild_line(gm_f, "%d", &GM_OPEN(top_guild), "GM_OPEN");
      read_guild_line(gm_f, "%d", &GM_CLOSE(top_guild), "GM_CLOSE");

      GM_FUNC(top_guild) = NULL;
      CREATE(buf, char, READ_SIZE);
      get_line(gm_f, buf);
      if (buf && *buf != '#' && *buf != '$') {
	p = buf;
	for (temp = 1; temp < GW_ARRAY_MAX; temp++) {
	  if (!p || !*p)
	    break;
	  if (sscanf(p, "%d", &val) != 1) {
	    log("SYSERR: Can't parse GM_WITH_WHO line in %s: '%s'", buf2, buf);
	    break;
	  }
	  GM_WITH_WHO(top_guild)[temp] = val;
	  while (isdigit(*p) || *p == '-') {
	    p++;
	  }
	  while (*p && !(isdigit(*p) || *p == '-')) {
	    p++;
	  }
	}
	while (temp < GW_ARRAY_MAX)
	  GM_WITH_WHO(top_guild)[temp++] = 0;
	free(buf);
	buf = fread_string(gm_f, buf2);
      }
    } else {
      if (*buf == '$')		/* EOF */
	done = TRUE;
      free(buf);		/* Plug memory leak! */
    }
  }
}


void assign_the_guilds(void)
{
	int gdindex;

	cmd_say = find_command("say");
	cmd_tell = find_command("tell");

	for (gdindex = 0; gdindex <= top_guild; gdindex++) {
		if (GM_TRAINER(gdindex) == NOBODY)
			continue;

          if (mob_index[GM_TRAINER(gdindex)].func && mob_index[GM_TRAINER(gdindex)].func != guild)
			GM_FUNC(gdindex) = mob_index[GM_TRAINER(gdindex)].func;

		mob_index[GM_TRAINER(gdindex)].func = guild;
	}
}

char *guild_customer_string(int guild_nr, int detailed)
{
  int gindex = 0, flag = 0, nlen;
  size_t len = 0;
  static char buf[MAX_STRING_LENGTH];

  while (*trade_letters[gindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!IS_SET_AR(GM_WITH_WHO(guild_nr), flag)) {
	nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[gindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (IS_SET_AR(GM_WITH_WHO(guild_nr), flag) ? '_' : *trade_letters[gindex]);
      buf[len] = '\0';

      if (len >= sizeof(buf))
        break;
    }

    gindex++;
    flag += 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}

void list_all_guilds(struct char_data *ch)
{
  const char *list_all_guilds_header =
  "Virtual   G.Master	Charge   Members\r\n"
  "----------------------------------------------------------------------\r\n";
  int gm_nr, headerlen = strlen(list_all_guilds_header);
  size_t len = 0;
  char buf[MAX_STRING_LENGTH], buf1[16];

  *buf = '\0';
  for (gm_nr = 0; gm_nr <= top_guild && len < sizeof(buf); gm_nr++) {
  /* New page in page_string() mechanism, print the header again. */
    if (!(gm_nr % (PAGE_LENGTH - 2))) {
    /*
     * If we don't have enough room for the header, or all we have room left
     * for is the header, then don't add it and just quit now.
     */
    if (len + headerlen + 1 >= sizeof(buf))
      break;
    strcpy(buf + len, list_all_guilds_header);	/* strcpy: OK (length checked above) */
    len += headerlen;
    }

    if (GM_TRAINER(gm_nr) == NOBODY)
      strcpy(buf1, "<NONE>");  /* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", mob_index[GM_TRAINER(gm_nr)].vnum);  /* sprintf: OK (for 'buf1 >= 11', 32-bit int) */	

    len += snprintf(buf + len, sizeof(buf) - len, "%6d	%s		%5.2f	%s\r\n",
      GM_NUM(gm_nr), buf1, GM_CHARGE(gm_nr), guild_customer_string(gm_nr, FALSE));
  }

  page_string(ch->desc, buf, TRUE);
}


void list_detailed_guild(struct char_data * ch, int gm_nr)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (GM_TRAINER(gm_nr) < NOBODY)
    strcpy(buf1, "<NONE>");
  else
    sprintf(buf1, "%6d   ", mob_index[GM_TRAINER(gm_nr)].vnum);

  sprintf(buf, " Guild Master: %s\r\n", buf1);
  sprintf(buf, "%s Hours: %4d to %4d,  Surcharge: %5.2f\r\n", buf,
			  GM_OPEN(gm_nr), GM_CLOSE(gm_nr), GM_CHARGE(gm_nr));
  sprintf(buf, "%s Min Level will train: %d\r\n", buf, GM_MINLVL(gm_nr));
  sprintf(buf, "%s Whom will train: %s\r\n", buf, guild_customer_string(gm_nr, TRUE));

   /* now for the REAL reason why someone would want to see a Guild :) */

  sprintf(buf, "%s The GM can teach the following:\r\n", buf);

  *buf2 = '\0';
  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (does_guild_know(gm_nr, i))
      sprintf(buf2, "%s %s \r\n", buf2, spell_info[i].name);
  }
 
  strcat(buf, buf2);

  page_string(ch->desc, buf, 1);
}
  

void show_guild(struct char_data * ch, char *arg)
{
  int gm_nr, gm_num;

  if (!*arg)
    list_all_guilds(ch);
  else {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

    if (gm_num > 0) {
      for (gm_nr = 0; gm_nr <= top_guild; gm_nr++) {
      if (gm_num == GM_NUM(gm_nr))
        break; 
      }

      if (gm_num < 0 || gm_nr > top_guild) {
        send_to_char(ch, "Illegal guild master number.\n\r");
        return;
      }
      list_detailed_guild(ch, gm_nr);
    }
  }
}

/*
 * List all guilds in a zone.                              
 */                                                                           
void list_guilds(struct char_data *ch, zone_rnum rnum, guild_vnum vmin, guild_vnum vmax)
{
  int i, bottom, top, counter = 0;
  
  if (rnum != NOWHERE) {
    bottom = zone_table[rnum].bot;
    top    = zone_table[rnum].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  
  /****************************************************************************/
  /** Store the header for the guild listing.                                **/
  /****************************************************************************/
  send_to_char (ch,
  "Index VNum    Guild Master\r\n"
  "----- ------- ---------------------------------------------\r\n");
  
  if (!top_guild)
    return;

  for (i = 0; i <= top_guild; i++) {
    if (GM_NUM(i) >= bottom && GM_NUM(i) <= top) {
      counter++;
      
      send_to_char(ch, "@g%4d@n) [@c%-5d@n]", counter, GM_NUM(i));

      /************************************************************************/
      /** Retrieve the list of rooms for this guild.                         **/
      /************************************************************************/

		send_to_char(ch, " @c[@y%d@c]@y %s@n", 
			(GM_TRAINER(i) == NOBODY) ?
			  -1 : mob_index[GM_TRAINER(i)].vnum,
			(GM_TRAINER(i) == NOBODY) ?
			  "" : mob_proto[GM_TRAINER(i)].short_descr); 
      
      send_to_char(ch, "\r\n");
    }
  }
  
  if (counter == 0)
    send_to_char(ch, "None found.\r\n");
}

void destroy_guilds(void)
{
  ssize_t cnt/*, itr*/;

  if (!guild_index)
    return;

  for (cnt = 0; cnt <= top_guild; cnt++) {
    if (guild_index[cnt].no_such_skill)
      free(guild_index[cnt].no_such_skill);
    if (guild_index[cnt].not_enough_gold)
      free(guild_index[cnt].not_enough_gold);
  }

  free(guild_index);
  guild_index = NULL;
  top_guild = -1;
}


int count_guilds(guild_vnum low, guild_vnum high)
{
  int i, j;
  
  for (i = j = 0; (GM_NUM(i) <= high && i <= top_guild); i++) {
    if (GM_NUM(i) >= low) {
      j++;
    }
  }
 
  return j;
}


void levelup_parse(struct descriptor_data *d, char *arg)
{
}
