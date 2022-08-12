/* ************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __UTILS_H__
#define __UTILS_H__

#include "structs.h"
#include "db.h"
#include "races.h"

#define log			basic_mud_log

#define READ_SIZE	256

/* global variables */
extern FILE *player_fl;

/* public functions in utils.c */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);

int masadv(char *tmp, struct char_data *ch);
void demon_refill_lf(struct char_data *ch, int64_t num);
void dispel_ash(struct char_data *ch);
int grav_cost(struct char_data *ch, int64_t num);
void prune_crlf(char *txt);
int count_metamagic_feats(struct char_data *ch);
int mob_respond(struct char_data *ch, struct char_data *vict, const char *speech);
int armor_evolve(struct char_data *ch);
int has_group(struct char_data *ch);
const char *report_party_health(struct char_data *ch);
int know_skill(struct char_data *ch, int skill);
int roll_aff_duration(int num, int add);
void null_affect(struct char_data *ch, int aff_flag);
void assign_affect(struct char_data *ch, int aff_flag, int skill, int dur, int str, int con, int intel, int agl, int wis, int spd);
int sec_roll_check(struct char_data *ch);
int get_measure(struct char_data *ch, int height, int weight);
int64_t physical_cost(struct char_data *ch, int skill);
int android_can(struct char_data *ch);
int trans_cost(struct char_data *ch, int trans);
int trans_req(struct char_data *ch, int trans);
void customRead(struct descriptor_data *d, int type, char *name);
void customWrite(struct char_data *ch, struct obj_data *obj);
void customCreate(struct descriptor_data *d);
int axion_dice(int adjust);
int64_t show_softcap(struct char_data *ch);
const char *disp_align(struct char_data *ch);
void senseCreate(struct char_data *ch);
void sense_memory_write(struct char_data *ch, struct char_data *vict);
int read_sense_memory(struct char_data *ch, struct char_data *vict);
int roll_pursue(struct char_data *ch, struct char_data *vict);
void broken_update(void);
int wearable_obj(struct obj_data *obj);
void randomize_eq(struct obj_data *obj);
char *sense_location(struct char_data *ch);
void handle_evolution(struct char_data *ch, int64_t dmg);
int64_t molt_threshold(struct char_data *ch);
int cook_element(room_rnum room);
void purge_homing(struct char_data *ch);
bool soft_cap(struct char_data *ch, int64_t type);
int64_t gear_pl_restore(struct char_data *ch, int64_t previous);
int64_t gear_pl(struct char_data *ch);
int planet_check(struct char_data *ch, struct char_data *vict);
void improve_skill(struct char_data *ch, int skill, int num);
double speednar(struct char_data *ch);
int gear_weight(struct char_data *ch);
int64_t gear_exp(struct char_data *ch, int64_t exp);
int get_flag_by_name(const char *flag_list[], char *flag_name);
char* add_commas(int64_t X);
void trim(char *s);
char *handle_racial(struct char_data *ch, int type);
char *introd_calc(struct char_data *ch);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void	basic_mud_vlog(const char *format, va_list args);
int	touch(const char *path);
void	mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));
void	log_death_trap(struct char_data *ch);
int	rand_number(int from, int to);
int64_t large_rand(int64_t from, int64_t to);
int	dice(int number, int size);
size_t	sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t	sprinttype(int type, const char *names[], char *result, size_t reslen);
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
time_t	mud_time_to_secs(struct time_info_data *now);
struct time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
void    core_dump_real(const char *who, int line);
int	room_is_dark(room_rnum room);
int     count_color_chars(char *string);
void    game_info(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
bool is_sparring(struct char_data *ch);
void mob_talk(struct char_data *ch, const char *speech);
int block_calc(struct char_data *ch);
void reveal_hiding(struct char_data *ch, int type);

#define core_dump()		core_dump_real(__FILE__, __LINE__)

/*
 * Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists.
 */



/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

/* Followers */
int	num_followers_charmed(struct char_data *ch);
void	die_follower(struct char_data *ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
void	stop_follower(struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
extern void	look_at_room(room_rnum target_room, struct char_data *ch, int mode);
extern void	add_history(struct char_data *ch, char *msg, int type);

/* in act.movement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in act.item.c */
int64_t	max_carry_weight(struct char_data *ch);

/* in limits.c */
void	advance_level(struct char_data *ch, int whichclass);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int64_t gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	point_update(void);
void	update_pos(struct char_data *victim);

/* in class.c */
char *  class_desc_str(struct char_data *ch, int howlong, int wantthe);
int     total_skill_levels(struct char_data *ch, int skill);
int8_t  ability_mod_value(int abil);
int8_t  dex_mod_capped(const struct char_data *ch);
int	highest_skill_value(int level, int type);
int     calc_penalty_exp(struct char_data *ch, int gain);
int	raise_class_only(struct char_data *ch, int cl, int v);

/* in races.c */
int	get_size(struct char_data *ch);
int get_size_bonus(int sz);
int wield_type(int chsize, const struct obj_data *weap);


/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define SCRIPT_VARS_FILE 3
#define NEW_OBJ_FILES   4
#define PLR_FILE        5
#define PET_FILE        6
#define IMC_FILE        7 /**< The IMC2 Data for players */
#define USER_FILE       8 /* User Account System */
#define INTRO_FILE      9
#define SENSE_FILE      10
#define CUSTOME_FILE    11
#define MAX_FILES       12

/* breadth-first searching */
#define BFS_ERROR		(-1)
#define BFS_ALREADY_THERE	(-2)
#define BFS_TO_FAR              (-3)
#define BFS_NO_PATH		(-4)

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR	900
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(30*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(12*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 

/* See also: ANA, SANA */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'cmtemp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next, cmtemp)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      cmtemp = head;			\
      while (cmtemp && (cmtemp->next != (item))) \
	 cmtemp = cmtemp->next;		\
      if (cmtemp)				\
         cmtemp->next = (item)->next;	\
   }					\

#define REMOVE_FROM_DOUBLE_LIST(item, head, next, prev)\
      if((item) == (head))			\
      {						\
            head = (item)->next;  		\
            if(head) head->prev = NULL;		\
      }						\
      else					\
      {						\
        temp = head;				\
          while(temp && (temp->next != (item)))	\
            temp = temp->next;			\
             if(temp)				\
            {					\
               temp->next = item->next;		\
               if(item->next)			\
                item->next->prev = temp;	\
            }					\
      }						\

/* basic bitvector utils *************************************************/


#define Q_FIELD(x)  ((int) (x) / 32)
#define Q_BIT(x)    (1 << ((x) % 32))
 
#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = \
                                   (var)[Q_FIELD(bit)] ^ Q_BIT(bit))
#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("OHNO: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)	((ch)->act)
#define PLR_FLAGS(ch)	((ch)->act)
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
#define AFF_FLAGS(ch)	((ch)->affected_by)
#define ADM_FLAGS(ch)	((ch)->admflags)
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
#define SPELL_ROUTINES(spl)	(spell_info[spl].routines)
#define ZONE_FLAGS(rnum)       (zone_table[(rnum)].zone_flags)
#define ZONE_MINLVL(rnum)      (zone_table[(rnum)].min_level)
#define ZONE_MAXLVL(rnum)      (zone_table[(rnum)].max_level)
/* Return the gauntlet highest room for ch */ 
#define GET_GAUNTLET(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->gauntlet))

/*
 * See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC.
 * IS_MOB() acts as a VALID_MOB_RNUM()-like function.
 */
#define IS_NPC(ch)	(IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))
#define ADM_FLAGGED(ch, flag) (IS_SET_AR(ADM_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJAFF_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_PERM(obj), (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), VAL_CONTAINER_FLAGS), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_WEAR(obj), (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))
#define BODY_FLAGGED(ch, flag) (IS_SET_AR(BODY_PARTS(ch), (flag)))
#define ZONE_FLAGGED(rnum, flag)   (IS_SET_AR(zone_table[(rnum)].zone_flags, flag))
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))
#define ADM_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(ADM_FLAGS(ch), (flag))) & Q_BIT(flag))
#define AFF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(AFF_FLAGS(ch), (flag))) & Q_BIT(flag))

/* new define for quick check */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

/* room utils ************************************************************/


#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)
#define ROOM_DAMAGE(room)   (world[(room)].dmg)
#define ROOM_EFFECT(room)   (world[(room)].geffect)
#define ROOM_GRAVITY(room)  (world[(room)].gravity)
#define SUNKEN(room)    (ROOM_EFFECT(room) < 0 || SECT(room) == SECT_UNDERWATER)

#define IS_DARK(room)	room_is_dark((room))
#define IS_LIGHT(room)  (!IS_DARK(room))

#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)

/* Minor Planet Defines */
#define PLANET_ZENITH(room) ((GET_ROOM_VNUM(room) >= 3400 && GET_ROOM_VNUM(room) <= 3599) || (GET_ROOM_VNUM(room) >= 62900 && GET_ROOM_VNUM(room) <= 62999) || \
				(GET_ROOM_VNUM(room) == 19600))

/* char utils ************************************************************/


#define IN_ROOM(ch)	((ch)->in_room)
#define IN_ZONE(ch)   (zone_table[(world[(IN_ROOM(ch))].zone)].number)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_AGE(ch)     (age(ch)->year)

#define GET_PC_NAME(ch)	((ch)->name)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)   ((ch)->desc ? ((ch)->desc->title ? (ch)->desc->title : "[Unset Title]") : "@D[@GNew User@D]")
#define GET_USER_TITLE(d) ((d)->title)
#define GET_PHASE(ch)   ((ch)->starphase)
#define GET_MIMIC(ch)   ((ch)->mimic)
#define GET_VOICE(ch)   ((ch)->voice)
#define GET_CLAN(ch)    ((ch)->clan)
#define GET_TRANSCLASS(ch) ((ch)->transclass)
#define GET_FEATURE(ch) ((ch)->feature)
#define GET_USER(ch)    ((ch)->desc ? ((ch)->desc->user ? (ch)->desc->user : "NOUSER") : "NOUSER")
#define GET_LOG_USER(ch) ((ch)->loguser)
#define GET_CRANK(ch)   ((ch)->crank)
#define GET_ADMLEVEL(ch)	((ch)->admlevel)
#define GET_CLASS_LEVEL(ch)	((ch)->level)
#define GET_LEVEL_ADJ(ch)	((ch)->level_adj)
#define GET_HITDICE(ch)		((ch)->race_level)
#define GET_LEVEL(ch)	(GET_CLASS_LEVEL(ch) + GET_LEVEL_ADJ(ch) + GET_HITDICE(ch))
#define GET_PFILEPOS(ch)((ch)->pfilepos)

#define GET_CLASS(ch)   ((ch)->chclass)
#define GET_CLASS_NONEPIC(ch, whichclass) ((ch)->chclasses[whichclass])
#define GET_CLASS_EPIC(ch, whichclass) ((ch)->epicclasses[whichclass])
#define GET_CLASS_RANKS(ch, whichclass) (GET_CLASS_NONEPIC(ch, whichclass) + \
                                         GET_CLASS_EPIC(ch, whichclass))
#define GET_RACE(ch)    ((ch)->race)
#define GET_HAIRL(ch)   ((ch)->hairl)
#define GET_HAIRC(ch)   ((ch)->hairc)
#define GET_HAIRS(ch)   ((ch)->hairs)
#define GET_SKIN(ch)    ((ch)->skin)
#define GET_EYE(ch)     ((ch)->eye)
#define GET_DISTFEA(ch) ((ch)->distfea)
#define GET_HOME(ch)	((ch)->hometown)
#define GET_WEIGHT(ch)  ((ch)->weight)
#define GET_HEIGHT(ch)  ((ch)->height)
#define GET_PC_HEIGHT(ch)	(!IS_NPC(ch) ? age(ch)->year <= 10 ? (int)((ch)->height * 0.68) : age(ch)->year <= 12 ? (int)((ch)->height * 0.72) : age(ch)->year <= 14 ? (int)((ch)->height * 0.85) : age(ch)->year <= 16 ? (int)((ch)->height * 0.92) : (ch)->height : (ch)->height)
#define GET_PC_WEIGHT(ch)	(!IS_NPC(ch) ? age(ch)->year <= 10 ? (int)((ch)->weight * 0.48) : age(ch)->year <= 12 ? (int)((ch)->weight * 0.55) : age(ch)->year <= 14 ? (int)((ch)->weight * 0.7) : age(ch)->year <= 16 ? (int)((ch)->weight * 0.85) : (ch)->weight : (ch)->weight)
#define GET_SEX(ch)	((ch)->sex)
#define GET_TLEVEL(ch)	((ch)->player_specials->tlevel)
#define CARRYING(ch)    ((ch)->player_specials->carrying)
#define CARRIED_BY(ch)  ((ch)->player_specials->carried_by)
#define RACIAL_PREF(ch) ((ch)->player_specials->racial_pref)
#define GET_RP(ch)      ((ch)->rp)
#define GET_RBANK(ch)   ((ch)->rbank)
#define GET_TRP(ch)     ((ch)->trp)
#define GET_SUPPRESS(ch) ((ch)->suppression)
#define GET_SUPP(ch)    ((ch)->suppressed)
#define GET_RDISPLAY(ch) ((ch)->rdisplay)

#define GET_STR(ch)     ((ch)->aff_abils.str)
/*
 * We could define GET_ADD to be ((GET_STR(ch) > 18) ?
 *                                ((GET_STR(ch) - 18) * 10) : 0)
 * but it's better to leave it undefined and fix the places that call
 * GET_ADD to use the new semantics for abilities.
 *                               - Elie Rosenblum 13/Dec/2003
 */
/* The old define: */
/* #define GET_ADD(ch)     ((ch)->aff_abils.str_add) */
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)
#define GET_MUTBOOST(ch) (IS_MUTANT(ch) ? ((GET_GENOME(ch, 0) == 1 || GET_GENOME(ch, 1) == 1) ? (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch)) * 0.3 : 0) : 0)
#define GET_SPEEDI(ch)  (GET_SPEEDCALC(ch) + GET_SPEEDBONUS(ch) + GET_SPEEDBOOST(ch) + GET_MUTBOOST(ch))
#define GET_SPEEDCALC(ch) (IS_GRAP(ch) ? GET_CHA(ch) : (IS_INFERIOR(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 1.25) : GET_SPEEDVAR(ch)) : GET_SPEEDVAR(ch)))
#define GET_SPEEDBONUS(ch) (IS_ARLIAN(ch) ? AFF_FLAGGED(ch, AFF_SHELL) ? GET_SPEEDVAR(ch) * -0.5 : (IS_MALE(ch) ? (AFF_FLAGGED(ch, AFF_FLYING) ? (GET_SPEEDVAR(ch) * 0.5) : 0) : 0) : 0)
#define GET_SPEEDVAR(ch) (GET_SPEEDVEM(ch) > GET_CHA(ch) ? GET_SPEEDVEM(ch) : GET_CHA(ch))
#define GET_SPEEDVEM(ch) (GET_SPEEDINT(ch) - (GET_SPEEDINT(ch) * speednar(ch)))
#define IS_GRAP(ch)     (GRAPPLING(ch) || GRAPPLED(ch))
#define GET_SPEEDINT(ch) (IS_BIO(ch) ? ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1200) / 1200) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)) : ((GET_CHA(ch) * GET_DEX(ch)) * (GET_MAX_HIT(ch) / 1000) / 1000) + (GET_CHA(ch) * (GET_KAIOKEN(ch) * 100)))
#define IS_INFERIOR(ch) (IS_KONATSU(ch) || IS_DEMON(ch))
#define IS_WEIGHTED(ch) (gear_pl(ch) < GET_MAX_HIT(ch))


#define GET_EXP(ch)	  ((ch)->exp)
/*
 * Changed GET_AC to GET_ARMOR so that code with GET_AC will need to be
 * looked at to see if it needs to change before being converted to use
 * GET_ARMOR
 */
#define SPOILED(ch)       ((ch)->time.played > 86400)
#define GET_DEATH_TYPE(ch) ((ch)->death_type)
#define GET_SLEEPT(ch)    ((ch)->sleeptime)
#define GET_FOODR(ch)     ((ch)->foodr)
#define GET_ALT(ch)       ((ch)->altitude)
#define GET_CHARGE(ch)    ((ch)->charge)
#define GET_CHARGETO(ch)  ((ch)->chargeto)
#define GET_ARMOR(ch)     ((ch)->armor)
#define GET_ARMOR_LAST(ch) ((ch)->armor_last)
#define GET_HIT(ch)	  ((ch)->hit)
#define GET_MAX_HIT(ch)	  ((ch)->max_hit)
#define GET_MOVE(ch)	  ((ch)->move)
#define GET_MAX_MOVE(ch)  ((ch)->max_move)
#define GET_MANA(ch)	  ((ch)->mana)
#define GET_MAX_MANA(ch)  ((ch)->max_mana)
#define GET_KI(ch)	  ((ch)->ki)
#define GET_MAX_KI(ch)    ((ch)->max_ki)
#define GET_DROOM(ch)     ((ch)->droom)
#define GET_OVERFLOW(ch)  ((ch)->overf)
#define GET_SPAM(ch)      ((ch)->spam)
#define GET_SHIP(ch)      ((ch)->ship)
#define GET_SHIPROOM(ch)  ((ch)->shipr)
#define GET_LPLAY(ch)     ((ch)->lastpl)
#define GET_DTIME(ch)     ((ch)->deathtime)
#define GET_RTIME(ch)     ((ch)->rewtime)
#define GET_DCOUNT(ch)    ((ch)->dcount)
#define GET_BOARD(ch, i)  ((ch)->lboard[i])
#define GET_LIMBS(ch, i)  ((ch)->limbs[i])
#define GET_LIMBCOND(ch, i) ((ch)->limb_condition[i])
#define GET_SONG(ch)      ((ch)->powerattack)
#define GET_BONUS(ch, i)  ((ch)->bonuses[i])
#define GET_TRANSCOST(ch, i) ((ch)->transcost[i])
#define GET_CCPOINTS(ch)  ((ch)->ccpoints)
#define GET_NEGCOUNT(ch)  ((ch)->negcount)
#define GET_GENOME(ch, i)    ((ch)->genome[i])
#define COMBO(ch)         ((ch)->combo)
#define LASTATK(ch)       ((ch)->lastattack)
#define COMBHITS(ch)      ((ch)->combhits)
#define GET_AURA(ch)      ((ch)->aura)
#define GET_RADAR1(ch)    ((ch)->radar1)
#define GET_RADAR2(ch)    ((ch)->radar2)
#define GET_RADAR3(ch)    ((ch)->radar3)
#define GET_PING(ch)      ((ch)->ping)
#define GET_SLOTS(ch)     ((ch)->skill_slots)
#define GET_TGROWTH(ch)   ((ch)->tail_growth)
#define GET_RMETER(ch)    ((ch)->rage_meter)
#define GET_PERSONALITY(ch) ((ch)->personality)
#define GET_COMBINE(ch)   ((ch)->combine)
#define GET_PREFERENCE(ch) ((ch)->preference)
#define GET_BASE_PL(ch)   ((ch)->basepl)
#define GET_RELAXCOUNT(ch) ((ch)->relax_count)
#define GET_BLESSLVL(ch)  ((ch)->blesslvl)
#define GET_ASB(ch)       ((ch)->asb)
#define GET_REGEN(ch)     ((ch)->regen)
#define GET_BLESSBONUS(ch) (AFF_FLAGGED(ch, AFF_BLESS) ? (GET_BLESSLVL(ch) >= 100 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.1 : GET_BLESSLVL(ch) >= 60 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.05 : GET_BLESSLVL(ch) >= 40 ? ((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.02 : 0) : 0) 
#define GET_POSELF(ch)    (!IS_NPC(ch) ? PLR_FLAGGED(ch, PLR_POSE) ? GET_SKILL(ch, SKILL_POSE) >= 100 ? 0.15 : GET_SKILL(ch, SKILL_POSE) >= 60 ? 0.1 : GET_SKILL(ch, SKILL_POSE) >= 40 ? 0.05 : 0 : 0 : 0)
#define GET_POSEBONUS(ch) (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * GET_POSELF(ch))
#define GET_LIFEBONUS(ch) (IS_ARLIAN(ch) ? ((GET_MAX_MANA(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) + ((GET_MAX_MOVE(ch) * 0.01) * (GET_MOLT_LEVEL(ch) / 100)) : 0)
#define GET_LIFEBONUSES(ch) ((ch)->lifebonus > 0 ? (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)) * (((ch)->lifebonus + 100) * 0.01) : (GET_LIFEBONUS(ch) + GET_BLESSBONUS(ch) + GET_POSEBONUS(ch)))
#define GET_LIFEMAX(ch)   (IS_DEMON(ch) ? (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.75) + GET_LIFEBONUSES(ch) : (IS_KONATSU(ch) ? (((GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5)) * 0.85) + GET_LIFEBONUSES(ch) : (GET_MAX_MANA(ch) * 0.5) + (GET_MAX_MOVE(ch) * 0.5) + GET_LIFEBONUSES(ch)))
#define GET_LIFEFORCE(ch) ((ch)->lifeforce)
#define GET_LIFEPERC(ch)  ((ch)->lifeperc)
#define GET_STUPIDKISS(ch) ((ch)->stupidkiss)
#define GET_SPEEDBOOST(ch) ((ch)->speedboost)
#define GET_BACKSTAB_COOL(ch) ((ch)->backstabcool)
#define GET_COOLDOWN(ch)  ((ch)->con_cooldown)
#define GET_BASE_KI(ch)   ((ch)->baseki)
#define GET_BASE_ST(ch)   ((ch)->basest)
#define GET_BARRIER(ch)   ((ch)->barrier)
#define GET_GOLD(ch)	  ((ch)->gold)
#define GET_KAIOKEN(ch)   ((ch)->kaioken)
#define GET_BOOSTS(ch)    ((ch)->boosts)
#define MAJINIZED(ch)     ((ch)->majinize)
#define GET_MAJINIZED(ch) ((ch)->majinizer)
#define GET_FURY(ch)      ((ch)->fury)
#define GET_BTIME(ch)     ((ch)->btime)
#define GET_UP(ch)        ((ch)->upgrade)
#define GET_FORGETING(ch) ((ch)->forgeting)
#define GET_FORGET_COUNT(ch) ((ch)->forgetcount)
#define GET_BANK_GOLD(ch) ((ch)->bank_gold)
#define GET_POLE_BONUS(ch) ((ch)->accuracy)
#define GET_FISHSTATE(ch)  ((ch)->fishstate)
#define GET_FISHD(ch)     ((ch)->accuracy_mod)
#define GET_DAMAGE_MOD(ch) ((ch)->damage_mod)
#define GET_SPELLFAIL(ch) ((ch)->spellfail)
#define GET_ARMORCHECK(ch) ((ch)->armorcheck)
#define GET_ARMORCHECKALL(ch) ((ch)->armorcheckall)
#define GET_MOLT_EXP(ch)  ((ch)->moltexp)
#define GET_MOLT_LEVEL(ch) ((ch)->moltlevel)
#define GET_SDCOOLDOWN(ch) ((ch)->con_sdcooldown)
#define GET_INGESTLEARNED(ch) ((ch)->ingestLearned)
#define GET_POS(ch)		((ch)->position)
#define GET_IDNUM(ch)		((ch)->idnum)
#define GET_ID(x)		((x)->id)
#define IS_CARRYING_W(ch)	((ch)->carry_weight)
#define IS_CARRYING_N(ch)	((ch)->carry_items)
#define FIGHTING(ch)		((ch)->fighting)
#define GET_POWERATTACK(ch)	((ch)->powerattack)
#define GET_GROUPKILLS(ch)	((ch)->combatexpertise)
#define GET_SAVE_BASE(ch, i)	((ch)->saving_throw[i])
#define GET_SAVE_MOD(ch, i)	((ch)->apply_saving_throw[i])
#define GET_SAVE(ch, i)		(GET_SAVE_BASE(ch, i) + GET_SAVE_MOD(ch, i))
#define GET_ALIGNMENT(ch)	((ch)->alignment)
#define GET_ETHIC_ALIGNMENT(ch)	((ch)->alignment_ethic)
#define SITS(ch)                ((ch)->sits)
#define MINDLINK(ch)            ((ch)->mindlink)
#define LINKER(ch)              ((ch)->linker)
#define LASTHIT(ch)             ((ch)->lasthit)
#define DRAGGING(ch)            ((ch)->drag)
#define DRAGGED(ch)             ((ch)->dragged)
#define GRAPPLING(ch)           ((ch)->grappling)
#define GRAPPLED(ch)            ((ch)->grappled)
#define GRAPTYPE(ch)            ((ch)->grap)
#define GET_ORIGINAL(ch)        ((ch)->original)
#define GET_CLONES(ch)          ((ch)->clones)
#define GET_DEFENDER(ch)        ((ch)->defender)
#define GET_DEFENDING(ch)       ((ch)->defending)
#define BLOCKS(ch)              ((ch)->blocks)
#define BLOCKED(ch)             ((ch)->blocked)
#define ABSORBING(ch)           ((ch)->absorbing)
#define ABSORBBY(ch)            ((ch)->absorbby)
#define GET_EAVESDROP(ch)       ((ch)->listenroom)
#define GET_EAVESDIR(ch)        ((ch)->eavesdir)
#define GET_ABSORBS(ch)         ((ch)->absorbs)
#define GET_LINTEREST(ch)       ((ch)->lastint)

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_PRACTICES(ch,cl)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->class_skill_points[cl]))
#define GET_RACE_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->skill_points))
#define GET_TRAINS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->ability_trains))
#define GET_TRAINSTR(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainstr))
#define GET_TRAININT(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainint))
#define GET_TRAINCON(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->traincon))
#define GET_TRAINWIS(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainwis))
#define GET_TRAINAGL(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainagl))
#define GET_TRAINSPD(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->trainspd))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_OLC_ZONE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->olc_zone))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_HOST(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_HISTORY(ch, i)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->comm_hist[i]))

#define GET_SKILL_BONUS(ch, i)		(ch->skillmods[i])
#define GET_SKILL_PERF(ch, i)           (ch->skillperfs[i])
#define SET_SKILL_BONUS(ch, i, value)	do { (ch)->skillmods[i] = value; } while (0)
#define SET_SKILL_PERF(ch, i, value)    do { (ch)->skillperfs[i] = value; } while (0)
#define GET_SKILL_BASE(ch, i)		(ch->skills[i])
#define GET_SKILL(ch, i)		((ch)->skills[i] + GET_SKILL_BONUS(ch, i))
#define SET_SKILL(ch, i, val)		do { (ch)->skills[i] = val; } while(0)
#define BODY_PARTS(ch)  ((ch)->bodyparts)

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)
#define MOB_COOLDOWN(ch)        ((ch)->cooldown)

/* STRENGTH_APPLY_INDEX is no longer needed with the death of GET_ADD */
/* #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        ) */

#define CAN_CARRY_W(ch) (max_carry_weight(ch))
#define CAN_CARRY_N(ch) (50)
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) || (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 4 || GET_GENOME(ch, 1) == 4)) || PLR_FLAGGED(ch, PLR_AURALIGHT))

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 50)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -50)
#define IS_LAWFUL(ch)   (GET_ETHIC_ALIGNMENT(ch) >= 350)
#define IS_CHAOTIC(ch)  (GET_ETHIC_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_ENEUTRAL(ch) (!IS_LAWFUL(ch) && !IS_CHAOTIC(ch))
#define ALIGN_TYPE(ch)	((IS_GOOD(ch) ? 0 : (IS_EVIL(ch) ? 6 : 3)) + \
                         (IS_LAWFUL(ch) ? 0 : (IS_CHAOTIC(ch) ? 2 : 1)))

#define IN_ARENA(ch)   (GET_ROOM_VNUM(IN_ROOM(ch)) >= 17800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 17874)
#define ARENA_IDNUM(ch) ((ch)->arenawatch)

/* These three deprecated. */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

#define IS_PLAYING(d)   (STATE(d) == CON_TEDIT || STATE(d) == CON_REDIT ||      \
                        STATE(d) == CON_MEDIT || STATE(d) == CON_OEDIT ||       \
                        STATE(d) == CON_ZEDIT || STATE(d) == CON_SEDIT ||       \
                        STATE(d) == CON_CEDIT || STATE(d) == CON_PLAYING ||     \
                        STATE(d) == CON_TRIGEDIT || STATE(d) == CON_AEDIT ||    \
                        STATE(d) == CON_GEDIT || STATE(d) == CON_IEDIT ||       \
                        STATE(d) == CON_HEDIT || STATE(d) == CON_NEWSEDIT ||    \
                        STATE(d) == CON_POBJ)
#define IS_INMENU(d)    (STATE(d) == CON_MENU || STATE(d) == CON_EXDESC || STATE(d) == CON_UMENU || STATE(d) == CON_GET_USER || STATE(d) == CON_GET_EMAIL || STATE(d) == CON_CHPWD_GETOLD || STATE(d) == CON_CHPWD_GETNEW || STATE(d) == CON_CHPWD_VRFY || STATE(d) == CON_DELCNF1 || STATE(d) == CON_DELCNF2 || STATE(d) == CON_QRACE || STATE(d) == CON_QCLASS || STATE(d) == CON_CLASS_HELP || STATE(d) == CON_RACE_HELP || STATE(d) == CON_BONUS || STATE(d) == CON_NEGATIVE || STATE(d) == CON_DISTFEA || STATE(d) == CON_HW || STATE(d) == CON_AURA)

#define SENDOK(ch)    (((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
                      (to_sleeping || AWAKE(ch)) && \
                      !PLR_FLAGGED((ch), PLR_WRITING))
/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)	((d)->connected)

/* object utils **********************************************************/

/*
 * Check for NOWHERE or the top array index?
 * If using unsigned types, the top array index will catch everything.
 * If using signed types, NOTHING will catch the majority of bad accesses.
 */
#define VALID_OBJ_RNUM(obj)	(GET_OBJ_RNUM(obj) <= top_of_objt && \
				 GET_OBJ_RNUM(obj) != NOTHING)

#define GET_OBJ_LEVEL(obj)      ((obj)->level)
#define GET_OBJ_PERM(obj)       ((obj)->bitvector)
#define GET_OBJ_TYPE(obj)	((obj)->type_flag)
#define GET_OBJ_COST(obj)	((obj)->cost)
#define GET_OBJ_RENT(obj)	((obj)->cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->extra_flags)
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->extra_flags[(i)])
#define GET_OBJ_WEAR(obj)	((obj)->wear_flags)
#define GET_OBJ_VAL(obj, val)	((obj)->value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->weight)
#define GET_OBJ_TIMER(obj)	((obj)->timer)
#define SITTING(obj)            ((obj)->sitting)
#define GET_OBJ_POSTTYPE(obj)   ((obj)->posttype)
#define GET_OBJ_POSTED(obj)     ((obj)->posted_to)
#define GET_FELLOW_WALL(obj)    ((obj)->fellow_wall)
#define GET_AUCTER(obj)         ((obj)->aucter)
#define GET_CURBID(obj)         ((obj)->curBidder)
#define GET_AUCTERN(obj)        ((obj)->auctname)
#define GET_AUCTIME(obj)        ((obj)->aucTime)
#define GET_BID(obj)            ((obj)->bid)
#define GET_STARTBID(obj)       ((obj)->startbid)
#define FOOB(obj)               ((obj)->foob)
/* Below is used for "homing" ki attacks */
#define TARGET(obj)             ((obj)->target)
#define KICHARGE(obj)           ((obj)->kicharge)
#define KITYPE(obj)             ((obj)->kitype)
#define USER(obj)               ((obj)->user)
#define KIDIST(obj)             ((obj)->distance)
/* Above is used for "homing ki attacks */
#define SFREQ(obj)              ((obj)->scoutfreq)
#define HCHARGE(obj)            ((obj)->healcharge)
#define GET_LAST_LOAD(obj)      ((obj)->lload)
#define GET_OBJ_SIZE(obj)	((obj)->size)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_SPEC(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].func : NULL)
#define GET_FUEL(obj)           (GET_OBJ_VAL((obj), 2))
#define GET_FUELCOUNT(obj)      (GET_OBJ_VAL((obj), 3))

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), VAL_CONTAINER_CORPSE) == 1)

#define CAN_WEAR(obj, part)	OBJWEAR_FLAGGED((obj), (part))
#define GET_OBJ_MATERIAL(obj)   ((obj)->value[7])
#define GET_OBJ_SHORT(obj)	((obj)->short_description)

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")
#define MAFE(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "male":"female") : "questionably gendered")

#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && !PLR_FLAGGED(sub, PLR_EYEC) && \
   (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION) || (IS_MUTANT(sub) && (GET_GENOME(sub, 0) == 4 || GET_GENOME(sub, 1) == 4)) || PLR_FLAGGED(sub, PLR_AURALIGHT)) )

#define INVIS_OK(sub, obj) \
 (!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_ADMLEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj) && (NOT_HIDDEN(obj) || GET_ADMLEVEL(sub) > 0)))

#define NOT_HIDDEN(ch) (!AFF_FLAGGED(ch, AFF_HIDE))
/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  ((LIGHT_OK(sub) || obj->carried_by == sub || obj->worn_by) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && !SITTING(obj) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define DISG(ch, vict) ((!PLR_FLAGGED(ch, PLR_DISGUISED)) || \
   (PLR_FLAGGED(ch, PLR_DISGUISED) && (GET_ADMLEVEL(vict) > 0 || IS_NPC(vict))))

#define INTROD(ch, vict) (ch == vict || readIntro(ch, vict) == 1 || (IS_NPC(vict) || IS_NPC(ch) || (GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0)))

#define ISWIZ(ch, vict) (ch == vict || GET_ADMLEVEL(ch) > 0 || GET_ADMLEVEL(vict) > 0 || IS_NPC(vict) || IS_NPC(ch))

#define PERS(ch, vict) ((DISG(ch, vict) ? (CAN_SEE(vict, ch) ? (INTROD(vict, ch) ? (ISWIZ(ch, vict) ? GET_NAME(ch) :\
                        get_i_name(vict, ch)) : introd_calc(ch)) : "Someone") :\
                        d_race_types[(int)GET_RACE(ch)]))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door]) 
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])
#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define RACE(ch)      (JUGGLERACE(ch))
#define JUGGLERACE(ch)	(IS_HOSHIJIN(ch) ? (GET_MIMIC(ch) > 0 ? pc_race_types[GET_MIMIC(ch) - 1] : pc_race_types[GET_RACE(ch)]) : handle_racial(ch, 1))
#define LRACE(ch)     (JUGGLERACELOWER(ch))
#define JUGGLERACELOWER(ch)  (IS_HOSHIJIN(ch) ? (GET_MIMIC(ch) > 0 ? race_names[GET_MIMIC(ch) - 1] : race_names[GET_RACE(ch)]) : handle_racial(ch, 0))
#define CLASS_ABBR(ch) (class_abbrevs[(int)GET_CLASS(ch)])
#define RACE_ABBR(ch) (race_abbrevs[(int)GET_RACE(ch)])

#define IS_ROSHI(ch)            (GET_CLASS(ch) == CLASS_ROSHI)
#define IS_PICCOLO(ch)          (GET_CLASS(ch) == CLASS_PICCOLO)
#define IS_KRANE(ch)            (GET_CLASS(ch) == CLASS_KRANE)
#define IS_NAIL(ch)             (GET_CLASS(ch) == CLASS_NAIL)
#define IS_BARDOCK(ch)          (GET_CLASS(ch) == CLASS_BARDOCK)
#define IS_GINYU(ch)            (GET_CLASS(ch) == CLASS_GINYU)
#define IS_FRIEZA(ch)           (GET_CLASS(ch) == CLASS_FRIEZA)
#define IS_TAPION(ch)           (GET_CLASS(ch) == CLASS_TAPION)
#define IS_ANDSIX(ch)           (GET_CLASS(ch) == CLASS_ANDSIX)
#define IS_DABURA(ch)           (GET_CLASS(ch) == CLASS_DABURA)
#define IS_KABITO(ch)           (GET_CLASS(ch) == CLASS_KABITO)
#define IS_JINTO(ch)            (GET_CLASS(ch) == CLASS_JINTO)
#define IS_TSUNA(ch)            (GET_CLASS(ch) == CLASS_TSUNA)
#define IS_KURZAK(ch)           (GET_CLASS(ch) == CLASS_KURZAK)
#define IS_ASSASSIN(ch)         (GET_CLASS_RANKS(ch, CLASS_ASSASSIN) > 0)
#define IS_BLACKGUARD(ch)       (GET_CLASS_RANKS(ch, CLASS_BLACKGUARD) > 0)
#define IS_DRAGON_DISCIPLE(ch)  (GET_CLASS_RANKS(ch, CLASS_DRAGON_DISCIPLE) > 0)
#define IS_DUELIST(ch)          (GET_CLASS_RANKS(ch, CLASS_DUELIST) > 0)
#define IS_DWARVEN_DEFENDER(ch) (GET_CLASS_RANKS(ch, CLASS_DWARVEN_DEFENDER) > 0)
#define IS_ELDRITCH_KNIGHT(ch)  (GET_CLASS_RANKS(ch, CLASS_ELDRITCH_KNIGHT) > 0)
#define IS_HIEROPHANT(ch)       (GET_CLASS_RANKS(ch, CLASS_HIEROPHANT) > 0)
#define IS_HORIZON_WALKER(ch)   (GET_CLASS_RANKS(ch, CLASS_HORIZON_WALKER) > 0)
#define IS_LOREMASTER(ch)       (GET_CLASS_RANKS(ch, CLASS_LOREMASTER) > 0)
#define IS_MYSTIC_THEURGE(ch)   (GET_CLASS_RANKS(ch, CLASS_MYSTIC_THEURGE) > 0)
#define IS_SHADOWDANCER(ch)     (GET_CLASS_RANKS(ch, CLASS_SHADOWDANCER) > 0)
#define IS_THAUMATURGIST(ch)    (GET_CLASS_RANKS(ch, CLASS_THAUMATURGIST) > 0)


#define GOLD_CARRY(ch)		(GET_LEVEL(ch) < 100 ? (GET_LEVEL(ch) < 50 ? GET_LEVEL(ch) * 10000 : 500000) : 50000000)
#define IS_SHADOW_DRAGON1(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON1_VNUM)
#define IS_SHADOW_DRAGON2(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON2_VNUM)
#define IS_SHADOW_DRAGON3(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON3_VNUM)
#define IS_SHADOW_DRAGON4(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON4_VNUM)
#define IS_SHADOW_DRAGON5(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON5_VNUM)
#define IS_SHADOW_DRAGON6(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON6_VNUM)
#define IS_SHADOW_DRAGON7(ch)   (IS_NPC(ch) && GET_MOB_VNUM(ch) == SHADOW_DRAGON7_VNUM)
#define CAN_GRAND_MASTER(ch)    (IS_HUMAN(ch))
#define IS_HUMANOID(ch)         (!IS_SNAKE(ch) && !IS_ANIMAL(ch))
#define RESTRICTED_RACE(ch)     (IS_MAJIN(ch) || IS_SAIYAN(ch) || IS_BIO(ch) || IS_HOSHIJIN(ch))
#define CHEAP_RACE(ch)          (IS_TRUFFLE(ch) || IS_MUTANT(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define SPAR_TRAIN(ch)          (FIGHTING(ch) && !IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPAR) &&\
                                 !IS_NPC(FIGHTING(ch)) && PLR_FLAGGED(FIGHTING(ch), PLR_SPAR))
#define IS_NONPTRANS(ch)        (IS_HUMAN(ch) || ((IS_SAIYAN(ch) || IS_HALFBREED(ch)) && !IS_FULLPSSJ(ch) && !PLR_FLAGGED(ch, PLR_LSSJ) && !PLR_FLAGGED(ch, PLR_OOZARU)) ||\
                                 IS_NAMEK(ch) || IS_MUTANT(ch) || IS_ICER(ch) ||\
                                 IS_KAI(ch) || IS_KONATSU(ch) || IS_DEMON(ch) || IS_KANASSAN(ch))
#define IS_FULLPSSJ(ch)             ((IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) && PLR_FLAGGED(ch, PLR_TRANS1)) ||\
                                 (IS_HALFBREED(ch) && PLR_FLAGGED(ch, PLR_FPSSJ) && PLR_FLAGGED(ch, PLR_TRANS1)))
#define IS_TRANSFORMED(ch)      (PLR_FLAGGED(ch, PLR_TRANS1) || PLR_FLAGGED(ch, PLR_TRANS2) ||\
                                 PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4) ||\
                                 PLR_FLAGGED(ch, PLR_TRANS5) || PLR_FLAGGED(ch, PLR_TRANS6) ||\
                                 PLR_FLAGGED(ch, PLR_OOZARU))
#define BIRTH_PHASE             (time_info.day <= 15)
#define LIFE_PHASE              (!BIRTH_PHASE && time_info.day <= 22)
#define DEATH_PHASE             (!BIRTH_PHASE && !LIFE_PHASE)
#define MOON_OK(ch)             (HAS_MOON(ch) && MOON_DATE && OOZARU_OK(ch))
#define OOZARU_OK(ch)           (OOZARU_RACE(ch) && PLR_FLAGGED(ch, PLR_STAIL) && !IS_TRANSFORMED(ch))
#define OOZARU_RACE(ch)         (IS_SAIYAN(ch) || IS_HALFBREED(ch))
#define MOON_DATE               (time_info.day == 19 || time_info.day == 20 || time_info.day == 21)
#define ETHER_STREAM(ch)        (ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK) || PLANET_ZENITH(IN_ROOM(ch)))
#define HAS_MOON(ch)            (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH) ||\
                                 ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER))
#define HAS_ARMS(ch)            (((IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_LARM) || \
                                 MOB_FLAGGED(ch, MOB_RARM))) || GET_LIMBCOND(ch, 1) > 0 || \
                                 GET_LIMBCOND(ch, 2) > 0 || \
                                 PLR_FLAGGED(ch, PLR_CRARM) || \
                                 PLR_FLAGGED(ch, PLR_CLARM)) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1 && GRAPTYPE(ch) != 4)))
#define HAS_LEGS(ch)            (((IS_NPC(ch) && (MOB_FLAGGED(ch, MOB_LLEG) || \
                                 MOB_FLAGGED(ch, MOB_RLEG))) || GET_LIMBCOND(ch, 3) > 0 || \
                                 GET_LIMBCOND(ch, 4) > 0 || \
                                 PLR_FLAGGED(ch, PLR_CRLEG) || \
                                 PLR_FLAGGED(ch, PLR_CLLEG)) && \
                                 ((!GRAPPLING(ch) && !GRAPPLED(ch)) || \
                                 (GRAPPLING(ch) && GRAPTYPE(ch) == 3) || \
                                 (GRAPPLED(ch) && GRAPTYPE(ch) != 1)))

#define IS_HUMAN(ch)            (GET_RACE(ch) == RACE_HUMAN)
#define IS_SAIYAN(ch)           (GET_RACE(ch) == RACE_SAIYAN)
#define IS_ICER(ch)             (GET_RACE(ch) == RACE_ICER)
#define IS_KONATSU(ch)          (GET_RACE(ch) == RACE_KONATSU)
#define IS_NAMEK(ch)            (GET_RACE(ch) == RACE_NAMEK)
#define IS_MUTANT(ch)           (GET_RACE(ch) == RACE_MUTANT)
#define IS_KANASSAN(ch)         (GET_RACE(ch) == RACE_KANASSAN)
#define IS_HALFBREED(ch)        (GET_RACE(ch) == RACE_HALFBREED)
#define IS_BIO(ch)              (GET_RACE(ch) == RACE_BIO)
#define IS_ANDROID(ch)          (GET_RACE(ch) == RACE_ANDROID)
#define IS_DEMON(ch)            (GET_RACE(ch) == RACE_DEMON)
#define IS_MAJIN(ch)            (GET_RACE(ch) == RACE_MAJIN)
#define IS_KAI(ch)              (GET_RACE(ch) == RACE_KAI)
#define IS_TRUFFLE(ch)          (GET_RACE(ch) == RACE_TRUFFLE)
#define IS_GOBLIN(ch)           (GET_RACE(ch) == RACE_GOBLIN)
#define IS_HOSHIJIN(ch)         (GET_RACE(ch) == RACE_GOBLIN)
#define IS_ANIMAL(ch)           (GET_RACE(ch) == RACE_ANIMAL)
#define IS_ORC(ch)              (GET_RACE(ch) == RACE_ORC)
#define IS_SNAKE(ch)            (GET_RACE(ch) == RACE_SNAKE)
#define IS_TROLL(ch)            (GET_RACE(ch) == RACE_TROLL)
#define IS_MINOTAUR(ch)         (GET_RACE(ch) == RACE_MINOTAUR)
#define IS_ARLIAN(ch)           (GET_RACE(ch) == RACE_KOBOLD)
#define IS_DRAGON(ch)           (GET_RACE(ch) == RACE_LIZARDFOLK)
#define IS_WARHOST(ch)          (GET_RACE(ch) == RACE_WARHOST)
#define IS_FAERIE(ch)           (GET_RACE(ch) == RACE_FAERIE)
#define IS_UNDEAD(ch)           (IS_AFFECTED(ch, AFF_UNDEAD))

/* Define Gender More Easily */
#define IS_MALE(ch)             (GET_SEX(ch) == SEX_MALE)
#define IS_FEMALE(ch)           (GET_SEX(ch) == SEX_FEMALE)
#define IS_NEUTER(ch)           (!IS_MALE(ch) && !IS_FEMALE(ch))

#define OUTSIDE(ch)	(OUTSIDE_ROOMFLAG(ch) && OUTSIDE_SECTTYPE(ch))

#define OUTSIDE_ROOMFLAG(ch)	(!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS) && \
			 !ROOM_FLAGGED(IN_ROOM(ch), ROOM_UNDERGROUND) && \
                          !ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE))

#define OUTSIDE_SECTTYPE(ch)	((SECT(IN_ROOM(ch)) != SECT_INSIDE) && \
                         (SECT(IN_ROOM(ch)) != SECT_UNDERWATER) && \
                          (SECT(IN_ROOM(ch)) != SECT_IMPORTANT) && \
                           (SECT(IN_ROOM(ch)) != SECT_SHOP) && \
                            (SECT(IN_ROOM(ch)) != SECT_SPACE))

#define DIRT_ROOM(ch) (OUTSIDE_SECTTYPE(ch) && ((SECT(IN_ROOM(ch)) != SECT_WATER_NOSWIM) && \
                       (SECT(IN_ROOM(ch)) != SECT_WATER_SWIM)))

#define SPEAKING(ch)     ((ch)->player_specials->speaking)

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

#if !defined(YES)
#define YES 1
#endif

#if !defined(NO)
#define NO 0
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#define CRYPT(a,b) ((char *) crypt((a),(b)))

/*******************  Config macros *********************/

#define CONFIG_CONFFILE         config_info.CONFFILE

#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
#define CONFIG_MAX_PC_CORPSE_TIME config_info.play.max_pc_corpse_time
#define CONFIG_IDLE_VOID        config_info.play.idle_void
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
#define CONFIG_LEVEL_CAP	config_info.play.level_cap
#define CONFIG_STACK_MOBS	config_info.play.stack_mobs
#define CONFIG_STACK_OBJS	config_info.play.stack_objs
#define CONFIG_MOB_FIGHTING	config_info.play.mob_fighting
#define CONFIG_OK               config_info.play.OK
#define CONFIG_NOPERSON         config_info.play.NOPERSON
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
#define CONFIG_REROLL_PLAYER_CREATION	config_info.play.reroll_player
#define CONFIG_INITIAL_POINTS_POOL	config_info.play.initial_points
#define CONFIG_ENABLE_COMPRESSION	config_info.play.enable_compression
#define CONFIG_ENABLE_LANGUAGES	config_info.play.enable_languages
#define CONFIG_ALL_ITEMS_UNIQUE	config_info.play.all_items_unique
#define CONFIG_EXP_MULTIPLIER	config_info.play.exp_multiplier

  /** Crash Saves **/
#define CONFIG_FREE_RENT        config_info.csd.free_rent
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

  /** Room Numbers **/
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3
#define CONFIG_DEATH_START      config_info.room_nums.death_start_room

  /** Game Operation **/
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
#define CONFIG_MENU             config_info.operation.MENU
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
#define CONFIG_START_MESSG      config_info.operation.START_MESSG
/** Can players communicate on the IMC channel ? */
#define CONFIG_IMC_ENABLED      config_info.operation.imc_enabled

  /** Autowiz **/
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

  /** Character Advancement **/
#define CONFIG_ALLOW_MULTICLASS	config_info.advance.allow_multiclass
#define CONFIG_ALLOW_PRESTIGE	config_info.advance.allow_prestige

  /** For tick system **/
#define CONFIG_PULSE_VIOLENCE	config_info.ticks.pulse_violence
#define CONFIG_PULSE_MOBILE	config_info.ticks.pulse_mobile
#define CONFIG_PULSE_ZONE	config_info.ticks.pulse_zone
#define CONFIG_PULSE_AUTOSAVE	config_info.ticks.pulse_autosave
#define CONFIG_PULSE_IDLEPWD	config_info.ticks.pulse_idlepwd
#define CONFIG_PULSE_SANITY	config_info.ticks.pulse_sanity
#define CONFIG_PULSE_USAGE	config_info.ticks.pulse_usage
#define CONFIG_PULSE_TIMESAVE	config_info.ticks.pulse_timesave
#define CONFIG_PULSE_CURRENT	config_info.ticks.pulse_current

  /** Character Creation Method **/
#define CONFIG_CREATION_METHOD	config_info.creation.method

#define GET_SPELLMEM(ch, i)	((ch->player_specials->spellmem[i]))
#define GET_MEMCURSOR(ch)	((ch->player_specials->memcursor))
/* returns the number of spells per slot */
#define GET_SPELL_LEVEL(ch, i)	((ch)->player_specials->spell_level[i])
#define IS_ARCANE(ch)		(IS_WIZARD(ch))
#define IS_DIVINE(ch)		(IS_CLERIC(ch))
#define HAS_FEAT(ch, i)		((ch)->feats[i])
#define HAS_COMBAT_FEAT(ch,i,j)	IS_SET_AR((ch)->combat_feats[(i)], (j))
#define SET_COMBAT_FEAT(ch,i,j)	SET_BIT_AR((ch)->combat_feats[(i)], (j))
#define HAS_SCHOOL_FEAT(ch,i,j)	IS_SET((ch)->school_feats[(i)], (j))
#define SET_SCHOOL_FEAT(ch,i,j)	SET_BIT((ch)->school_feats[(i)], (j))
#define GET_BAB(ch)		GET_POLE_BONUS(ch)
#define SET_FEAT(ch, i, value)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->feats[i]) = value; } while(0)
#define GET_SPELL_MASTERY_POINTS(ch) \
				(ch->player_specials->spell_mastery_points)
#define GET_FEAT_POINTS(ch)	(ch->player_specials->feat_points)
#define GET_EPIC_FEAT_POINTS(ch) \
				(ch->player_specials->epic_feat_points)
#define GET_CLASS_FEATS(ch,cl)	(ch->player_specials->class_feat_points[cl])
#define GET_EPIC_CLASS_FEATS(ch,cl) \
				(ch->player_specials->epic_class_feat_points[cl])
#define IS_EPIC_LEVEL(ch)	(GET_CLASS_LEVEL(ch) >= 20)
#define HAS_CRAFT_SKILL(ch,i,j)	IS_SET_AR((ch)->craft_skill[(i)], (j))
#define SET_CRAFT_SKILL(ch,i,j)	SET_BIT_AR((ch)->craft_skill[(i)], (j))
#define HAS_KNOWLEDGE_SKILL(ch,i,j)	IS_SET_AR((ch)->knowledge_skill[(i)], (j))
#define SET_KNOWLEDGE_SKILL(ch,i,j)	SET_BIT_AR((ch)->knowledge_skill[(i)], (j))
#define HAS_PROFESSION_SKILL(ch,i,j)	IS_SET_AR((ch)->profession_skill[(i)], (j))
#define SET_PROFESSION_SKILL(ch,i,j)	SET_BIT_AR((ch)->profession_skill[(i)], (j))

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */

struct xap_dir {
    int total, current;
    struct dirent **namelist;
};

int xdir_scan(char *dir_name, struct xap_dir *xapdirp);
int xdir_get_total(struct xap_dir *xd);
char *xdir_get_name(struct xap_dir *xd, int num);
char *xdir_next(struct xap_dir *xd);
void xdir_close(struct xap_dir *xd);
int insure_directory(char *path, int isfile);
void admin_set(struct char_data *ch, int value);
#define GET_PAGE_LENGTH(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->page_length))
#define IS_COLOR_CHAR(c)  (c == 'n' || c == 'b' || c == 'B' || c == 'c' || \
   c == 'C' || c == 'g' || c == 'G' || c == 'm' || c == 'M' || c == 'r' || \
   c == 'R' || c == 'y' || c == 'Y' || c == 'w' || c == 'W' || c == 'k' || \
   c == 'K' || c == '0' || c == '2' || c == '3' || c == '4' || c == '5' || \
   c == '6' || c == '7' || c == 'o' || c == 'e' || c == 'u' || c == 'l') 
#define MOB_LOADROOM(ch)      ((ch)->hometown)  /*hometown not used for mobs*/
#define OBJ_LOADROOM(obj)     ((obj)->room_loaded)

int     levenshtein_distance(char *s1, char *s2);
#define GET_MURDER(ch)          CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->murder))

#endif