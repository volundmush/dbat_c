/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

#include "class.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "handler.h"
#include "feats.h"
#include "oasis.h"
#include "act.wizard.h"
#include "dg_comm.h"
#include "config.h"
#include "act.other.h"

/* Names first */

const char *class_abbrevs[NUM_CLASSES+1] = {
  "Ro",
  "Pi",
  "Kr",
  "Na",
  "Ba",
  "Gi",
  "Fr",
  "Ta",
  "An",
  "Da",
  "Ki",
  "Ji",
  "Ts",
  "Ku",
  "As",
  "Bl",
  "Dd",
  "Du",
  "Dw",
  "Ek",
  "Ht",
  "Hw",
  "Lo",
  "Mt",
  "Sh",
  "Th",
  "Ex",
  "Ad",
  "Co",
  "Ar",
  "Wa",
  "\n"
};


/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *pc_class_types[NUM_CLASSES+1] = {
  "Roshi",
  "Piccolo",
  "Krane",
  "Nail",
  "Bardock",
  "Ginyu",
  "Frieza",
  "Tapion",
  "Android 16",
  "Dabura",
  "Kibito",
  "Jinto",
  "Tsuna",
  "Kurzak",
  "Assassin",
  "Blackguard",
  "Dragon Disciple",
  "Duelist",
  "Dwarven Defender",
  "Eldritch Knight",
  "Hierophant",
  "Horizon Walker",
  "Loremaster",
  "Mystic Theurge",
  "Shinobi",
  "Thaumaturgist",
  "Expert",
  "Adept",
  "Commoner",
  "Aristrocrat",
  "Warrior",
  "\n"
};

/* Copied from the SRD under OGL, see ../doc/srd.txt for information */
const char *class_names[NUM_CLASSES+1] = {
  "roshi",
  "piccolo",
  "krane",
  "nail",
  "bardock",
  "ginyu",
  "frieza",
  "tapion",
  "android 16",
  "dabura",
  "kibito",
  "jinto",
  "tsuna",
  "kurzak",
  "assassin",
  "blackguard",
  "dragon disciple",
  "duelist",
  "dwarven defender",
  "eldritch knight",
  "hierophant",
  "horizon walker",
  "loremaster",
  "mystic theurge",
  "shadowdancer",
  "thaumaturgist",
  "artisan",
  "magi",
  "normal",
  "noble",
  "soldier",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_display[NUM_CLASSES] = {
  "@B1@W) @MRoshi\r\n",
  "@B2@W) @WPiccolo\r\n",
  "@B3@W) @YKrane\r\n",
  "@B4@W) @BNail\r\n",
  "@B5@W) @BBardock\r\n",
  "@B6@W) @BGinyu\r\n",
  "@B7@W) @WFrieza\r\n",
  "@B8@W) @YTapion\r\n",
  "@B9@W) @BAndroid 16\r\n",
  "@B10@W) @BDabura\r\n",
  "@B11@W) @BKibito\r\n",
  "@B12@W) @BJinto\r\n",
  "@B13@W) @BTsuna\r\n",
  "@B14@W) @BKurzak\r\n",
  "assassin (P)\r\n",
  "blackguard (P)\r\n",
  "dragon disciple (P)\r\n",
  "duelist (P)\r\n",
  "dwarven defender (P)\r\n",
  "eldritch knight (P)\r\n",
  "hierophant (P)\r\n",
  "horizon walker (P)\r\n",
  "loremaster (P)\r\n",
  "mystic theurge (P)\r\n",
  "shadowdancer (P)\r\n",
  "thaumaturgist (P)\r\n",
  "Artisan NPC\r\n",
  "Magi NPC\r\n",
  "Normal NPC\r\n",
  "Noble NPC\r\n",
  "Soldier NPC\r\n",
};

#define Y   TRUE
#define N   FALSE

/* Some races copied from the SRD under OGL, see ../doc/srd.txt for information */
const int class_ok_race[NUM_RACES][NUM_CLASSES] = {
  /*                Wi,Cl,Ro,Fi,Mo,Pa,So,Dr,Ba,Ra,Bn,Aa,At,Am,As,Bg,Dd,Du,Dw,Ek,Ht,Hw,Lr,Mt,Sd,Th,Ad,Ma,No,Nl,So,   */
  /* Human      */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Saiyan     */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Icer       */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Konatsu    */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Namek      */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Mutant     */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Kanassan   */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Halfbreed  */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Bio        */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Android    */ { N, N, N, N, N, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Demon      */ { Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Majin      */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Kai        */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Truffle    */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Hoshijin   */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Animal     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Orc        */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Snake      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Troll      */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Minotaur   */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Arlian     */ { Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Mindflayer */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Warhost    */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N },
  /* Faerie     */ { N, N, N, N, N, N, Y, Y, Y, Y, Y, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N }
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int class_ok_align[NUM_ALIGNS][NUM_CLASSES] = {
/*         Wi,Cl,Ro,Fi,Mo,Pa,So,Dr,Ba,Ra,Bn,Aa,At,Am,As,Bg,Dd,Du,Dw,Ek,He,Hw,Lm,Mt,Sd,Th*/
/* LG */ { Y, Y, Y, Y, Y, Y, Y, N, N, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NG */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CG */ { Y, Y, Y, Y, N, N, Y, N, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* LN */ { Y, Y, Y, Y, Y, N, Y, Y, N, Y, N, Y, N, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NN */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CN */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, N, N, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* LE */ { Y, Y, Y, Y, Y, N, Y, N, N, Y, N, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* NE */ { Y, Y, Y, Y, N, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y },
/* CE */ { Y, Y, Y, Y, N, N, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, N, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y }
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int favored_class[NUM_RACES] = {
/* -1 means highest class is considered favored */
/* Human      */ -1,
/* Saiyan     */ -1,
/* Icer       */ -1,
/* Konatsu    */ -1,
/* Namek      */ -1,
/* Mutant     */ -1,
/* Kanassan   */ -1,
/* Halfbreed  */ -1,
/* Bio-android*/ -1,
/* Android    */ -1,
/* Demon      */ -1,
/* Majin      */ -1,
/* Kai        */ -1,
/* TRuffle    */ -1,
/* Goblin     */ -1,
/* Insect     */ -1,
/* Orc        */ -1,
/* Snake      */ -1,
/* Troll      */ -1,
/* Minotaur   */ -1,
/* Arlian     */ -1,
/* Lizardfolk */ -1,
/* Warhost    */ -1,
/* Faerie     */ -1
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
int prestige_classes[NUM_CLASSES] = {
/* WIZARD	*/ N,
/* CLERIC	*/ N,
/* ROGUE	*/ N,
/* FIGHTER	*/ N,
/* MONK		*/ N,
/* PALADIN	*/ N,
/* SORCERER	*/ N,
/* DRUID	*/ N,
/* BARD 	*/ N,
/* RANGER	*/ N,
/* BARBARIAN	*/ N,
/* Arcane ARCHER    */ N,
/* ARCANE TRICKSTER */ N,
/* ARCHMAGE         */ N,
/* ASSASSIN         */ Y,
/* BLACKGUARD       */ Y,
/* DRAGON DISCIPLE  */ Y,
/* DUELIST          */ Y,
/* DWARVEN DEFENDER */ Y,
/* ELDRITCH KNIGHT  */ Y,
/* HIEROPHANT       */ Y,
/* HORIZON WALKER   */ Y,
/* LOREMASTER       */ Y,
/* MYSTIC THEURGE   */ Y,
/* SHADOWDANCER     */ Y,
/* THAUMATURGIST    */ Y,
/* ARTISAN	*/ N,
/* MAGI		*/ N,
/* NORMAL	*/ N,
/* NOBLE	*/ N,
/* SOLDIER	*/ N
};

/* Adapted from the SRD under OGL, see ../doc/srd.txt for information */
/* -1 indicates no limit to the number of levels in this class under
 * epic rules */
int class_max_ranks[NUM_CLASSES] = {
/* WIZARD	*/ -1,
/* CLERIC	*/ -1,
/* ROGUE	*/ -1,
/* FIGHTER	*/ -1,
/* MONK		*/ -1,
/* PALADIN	*/ -1,
/* SORCERER	*/ -1,
/* DRUID	*/ -1,
/* BARD 	*/ -1,
/* RANGER	*/ -1,
/* BARBARIAN	*/ -1,
/* ARCANE ARCHER    */ -1,
/* ARCANE TRICKSTER */ -1,
/* ARCHMAGE         */ -1,
/* ASSASSIN         */ 10,
/* BLACKGUARD       */ 10,
/* DRAGON DISCIPLE  */ 10,
/* DUELIST          */ 10,
/* DWARVEN DEFENDER */ 10,
/* ELDRITCH KNIGHT  */ 10,
/* HIEROPHANT       */ 5,
/* HORIZON WALKER   */ 10,
/* LOREMASTER       */ 10,
/* MYSTIC THEURGE   */ 10,
/* SHADOWDANCER     */ 10,
/* THAUMATURGIST    */ 5,
/* ARTISAN	*/ -1,
/* MAGI		*/ -1,
/* NORMAL	*/ -1,
/* NOBLE	*/ -1,
/* SOLDIER	*/ -1
};


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(struct char_data *ch, int arg)
{
  int chclass = CLASS_UNDEFINED;

  switch (arg) {
  case 1 : chclass = CLASS_ROSHI    ; break;
  case 2 : chclass = CLASS_PICCOLO  ; break;
  case 3 : chclass = CLASS_KRANE    ; break;
  case 4 : chclass = CLASS_NAIL     ; break;
  case 5 : chclass = CLASS_BARDOCK  ; break;
  case 6 : chclass = CLASS_GINYU    ; break;
  case 7 : chclass = CLASS_FRIEZA   ; break;
  case 8 : chclass = CLASS_TAPION   ; break;
  case 9 : chclass = CLASS_ANDSIX   ; break;
  case 10 : chclass = CLASS_DABURA   ; break;
  case 11 : chclass = CLASS_KABITO   ; break;
  case 12 : chclass = CLASS_JINTO    ; break;
  case 13 : chclass = CLASS_TSUNA    ; break;
  case 14 : chclass = CLASS_KURZAK   ; break;
  default:  chclass = CLASS_UNDEFINED; break;
  }
  if (chclass >= 0 && chclass < NUM_BASIC_CLASSES)
    if (!class_ok_race[(int)GET_RACE(ch)][chclass])
      chclass = CLASS_UNDEFINED;

  return (chclass);
}

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only WIZARDS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
const struct guild_info_type guild_info[6] = {

/* Kortaal */
  { CLASS_ROSHI,	3017,	SCMD_EAST	},
  { CLASS_PICCOLO,	3004,	SCMD_NORTH	},
  { CLASS_KRANE, 	3027,	SCMD_EAST	},
  { CLASS_NAIL, 	3021,	SCMD_EAST	},

/* Brass Dragon */
  { -999 /* all */ ,	5065,	SCMD_WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};
/* 
 * These tables hold the various level configuration setting;
 * experience points, base hit values, character saving throws.
 * They are read from a configuration file (normally etc/levels)
 * as part of the boot process.  The function load_levels() at
 * the end of this file reads in the actual values.
 */

const char *config_sect[NUM_CONFIG_SECTIONS+1] = {
  "version",
  "experience",
  "vernum",
  "fortitude",
  "reflex",
  "will",
  "basehit",
  "\n"
};

#define CONFIG_LEVEL_VERSION	0
#define CONFIG_LEVEL_EXPERIENCE	1
#define CONFIG_LEVEL_VERNUM	2
#define CONFIG_LEVEL_FORTITUDE	3
#define CONFIG_LEVEL_REFLEX	4
#define CONFIG_LEVEL_WILL	5
#define CONFIG_LEVEL_BASEHIT	6

static char level_version[READ_SIZE];
static int level_vernum = 0;
static int save_classes[SAVING_WILL+1][NUM_CLASSES];
static int basehit_classes[NUM_CLASSES];

#define SAVE_MANUAL 0
#define SAVE_LOW 1
#define SAVE_HIGH 2

static const char *save_type_names[] = {
  "manual",
  "low",
  "high"
};

#define BASEHIT_MANUAL 0
#define BASEHIT_LOW     1
#define BASEHIT_MEDIUM  2
#define BASEHIT_HIGH    3

static const char *basehit_type_names[] = {
  "manual",
  "low",
  "medium",
  "high"
};

/* Class Template Attribute values were created so all default PC classes would
   total 80 before racial modifiers. Non defaults add up to 60. */
static const int class_template[NUM_BASIC_CLASSES][6] = {
/* 		      S,  D,  C,  I,  W,  C */
/* Wizard 	*/ { 10, 13, 13, 18, 16, 10 },
/* Cleric 	*/ { 13, 10, 13, 10, 18, 16 },
/* Rogue 	*/ { 13, 18, 13, 16, 10, 10 },
/* Fighter 	*/ { 18, 13, 16, 10, 13, 10 },
/* Monk 	*/ { 13, 16, 13, 10, 18, 10 },
/* Paladin 	*/ { 18, 10, 13, 10, 16, 13 },
/* Sorcerer 	*/ { 10, 13, 13, 18, 16, 10 },
/* Druid 	*/ { 13, 10, 13, 10, 18, 16 },
/* Bard 	*/ { 13, 18, 13, 16, 10, 10 },
/* Ranger 	*/ { 13, 13, 13, 10, 16, 10 },
/* Barbarian 	*/ { 13, 12, 18, 10, 12, 10 }
};

/* Race Template Attribute values were created so all default PC races would
   total 80 before racial modifiers. Non defaults add up to 60. */
static const int race_template[NUM_RACES][6] = {
/* 		      S,  D,  C,  I,  W,  C */
/* Human 	*/ { 13, 13, 13, 13, 13, 13 },
/* Saiyan  	*/ { 16, 12, 14, 10, 12, 12 },
/* Icer 	*/ { 14, 14, 12, 12, 12, 12 },
/* Konatsu 	*/ { 10, 16, 10, 13, 14, 14 },
/* Namek 	*/ { 14, 12, 13, 12, 14, 12 },
/* Mutant 	*/ { 12, 12, 15, 13, 13, 13 },
/* Kanassan 	*/ { 10, 14, 10, 15, 13, 10 },
/* Halfbreed 	*/ { 14, 13, 14, 12, 13, 12 },
/* Bio  	*/ { 15, 10, 15, 12, 12, 10 },
/* Android 	*/ { 14, 14, 14, 12, 10, 12 },
/* Demon 	*/ { 14, 13, 14, 10, 12, 10 },
/* Majin 	*/ { 15, 10, 15, 10, 12, 14 },
/* Kai  	*/ { 11, 14, 10, 14, 14, 11 },
/* Truffle 	*/ { 10, 14, 10, 16, 16, 12 },
/* Goblin 	*/ { 13, 13, 13, 13, 13, 13 },
/* Insect 	*/ { 10, 10, 10, 10, 10, 10 },
/* Orc  	*/ { 10, 10, 10, 10, 10, 10 },
/* Snake  	*/ { 10, 10, 10, 10, 10, 10 },
/* Troll  	*/ { 10, 10, 10, 10, 10, 10 },
/* Minotaur  	*/ { 10, 10, 10, 10, 10, 10 },
/* Arlian  	*/ { 13, 13, 13, 12, 12, 14 },
/* Lizardfolk  	*/ { 10, 10, 10, 10, 10, 10 },
/* Warhost  	*/ { 10, 10, 10, 10, 10, 10 },
/* Faerie 	*/ { 10, 10, 10, 10, 10, 10 }
};

void cedit_creation(struct char_data *ch)
{
  switch (CONFIG_CREATION_METHOD) {
    case CEDIT_CREATION_METHOD_3: /* Points Pool */
      break;
    case CEDIT_CREATION_METHOD_4: /* Racial based template */
      break;
    case CEDIT_CREATION_METHOD_5: /* Class based template */
      break;
    case CEDIT_CREATION_METHOD_2: /* Random rolls, player can adjust */
    case CEDIT_CREATION_METHOD_1: /* Standard random roll, system assigned */
    default:
      break;
  }
  racial_ability_modifiers(ch);
  racial_body_parts(ch);
  ch->aff_abils = ch->real_abils;
}

/*
 * Roll the 6 stats for a character... each stat is made of some combination
 * of 6-sided dice with various rolls discarded.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
/* This character creation method is original */

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
const int class_hit_die_size[NUM_CLASSES] = {
/* Wi */ 4,
/* Cl */ 8,
/* Ro */ 6,
/* Fi */ 10,
/* Mo */ 8,
/* Pa */ 10,
/* So */ 4,
/* Dr */ 8,
/* Ba */ 6,
/* Ra */ 8,
/* Ba */ 12,
/* AA */ 8,
/* AT */ 4,
/* AM */ 4,
/* AS */ 6,
/* BG */ 10,
/* DD */ 12,
/* Du */ 10,
/* Dw */ 12,
/* EK */ 6,
/* HE */ 8,
/* HW */ 8,
/* LM */ 4,
/* MT */ 4,
/* SD */ 8,
/* TH */ 4,
/* Ex */ 6,
/* Ad */ 6,
/* Co */ 4,
/* Ar */ 8,
/* Wa */ 8
};

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  int punch;
  struct obj_data *obj;

  GET_CLASS_LEVEL(ch) = 1;
  GET_HITDICE(ch) = 0;
  GET_LEVEL_ADJ(ch) = 0;
  GET_CLASS_NONEPIC(ch, GET_CLASS(ch)) = 1;
  GET_EXP(ch) = 1;
 
  if (IS_ANDROID(ch)) {
   GET_COND(ch, HUNGER) = -1;
   GET_COND(ch, THIRST) = -1;
   GET_COND(ch, DRUNK) = -1;
  } else if (IS_BIO(ch) && (GET_GENOME(ch, 0) == 3 || GET_GENOME(ch, 1) == 3)) {
   GET_COND(ch, HUNGER) = -1;
   GET_COND(ch, DRUNK) = 0;
   GET_COND(ch, THIRST) = 48;
  }else if (IS_NAMEK(ch)) {
   GET_COND(ch, HUNGER) = -1;
   GET_COND(ch, DRUNK) = 0;
   GET_COND(ch, THIRST) = 48;
  } else {
   GET_COND(ch, THIRST) = 48;
   GET_COND(ch, HUNGER) = 48;
   GET_COND(ch, DRUNK) = 0;
  }

  SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HINTS);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_NOMUSIC);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
  GET_LIMBCOND(ch, 1) = 100;
  GET_LIMBCOND(ch, 2) = 100;
  GET_LIMBCOND(ch, 3) = 100;
  GET_LIMBCOND(ch, 4) = 100;
  SET_BIT_AR(PLR_FLAGS(ch), PLR_HEAD);

  GET_SLOTS(ch) = 30;

  if (GET_RACE(ch) == RACE_HUMAN) {
   GET_SLOTS(ch) += 1;
  } else if (GET_RACE(ch) == RACE_SAIYAN) {
   GET_SLOTS(ch) -= 1;
  } else if (GET_RACE(ch) == RACE_TRUFFLE) {
   GET_SLOTS(ch) += 2;
  } else if (GET_RACE(ch) == RACE_HALFBREED) {
   GET_SLOTS(ch) += 1;
  } else if (GET_RACE(ch) == RACE_MAJIN) {
   GET_SLOTS(ch) -= 1;
  } else if (GET_RACE(ch) == RACE_KAI) {
   GET_SLOTS(ch) += 4;
  }
  

  if (IS_TSUNA(ch) || IS_KABITO(ch) || IS_NAIL(ch))
   GET_SLOTS(ch) += 5;

  if (GET_BONUS(ch, BONUS_GMEMORY))
   GET_SLOTS(ch) += 2;
  if (GET_BONUS(ch, BONUS_BMEMORY))
   GET_SLOTS(ch) -= 5;

 
  if (GET_RACE(ch) == RACE_SAIYAN || GET_RACE(ch) == RACE_HALFBREED) {
    if (GET_RACE(ch) != RACE_HALFBREED || (GET_RACE(ch) == RACE_HALFBREED && RACIAL_PREF(ch) != 1)) {
     SET_BIT_AR(PLR_FLAGS(ch), PLR_STAIL);
    }
  }
  if (GET_RACE(ch) == RACE_ICER || GET_RACE(ch) == RACE_BIO) {
     SET_BIT_AR(PLR_FLAGS(ch), PLR_TAIL);
  }
  if (GET_RACE(ch) == RACE_MAJIN) {
		GET_ABSORBS(ch) = 0;
		GET_INGESTLEARNED(ch) = 0;
		
	}
  if (GET_RACE(ch) == RACE_BIO) {
   GET_ABSORBS(ch) = 3;
  }
  SET_BIT_AR(PRF_FLAGS(ch), PRF_VIEWORDER);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPEXP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);

  if (!PLR_FLAGGED(ch, PLR_FORGET)) {
   if (ch->choice == 1) {
    punch = rand_number(30, 40);
    SET_SKILL(ch, SKILL_BLOCK, punch);
   }
   if (ch->choice == 2) {
    punch = rand_number(10, 20);
    SET_SKILL(ch, SKILL_PUNCH, punch);
   }
   if (ch->choice == 3) {
    punch = rand_number(30, 40);
    SET_SKILL(ch, SKILL_KICK, punch);
   }
   if (ch->choice == 4) {
    punch = rand_number(20, 30);
    SET_SKILL(ch, SKILL_SLAM, punch);
   }
   if (ch->choice == 5) {
    punch = rand_number(20, 30);
    SET_SKILL(ch, SKILL_FOCUS, punch);
   }
   if (IS_HUMAN(ch)) {
    punch = rand_number(5, 15);
    SET_SKILL(ch, SKILL_BUILD, punch);
   }
   if (IS_TRUFFLE(ch)) {
    punch = rand_number(15, 25);
    SET_SKILL(ch, SKILL_BUILD, punch);
   }
   if (IS_KONATSU(ch)) {
    punch = rand_number(50, 60);
    SET_SKILL(ch, SKILL_SWORD, punch);
    punch = rand_number(10, 30);
    SET_SKILL(ch, SKILL_MOVE_SILENTLY, punch);
    punch = rand_number(10, 30);
    SET_SKILL(ch, SKILL_HIDE, punch);
   }
   if (IS_TAPION(ch) || IS_GINYU(ch) || IS_DABURA(ch) || IS_KURZAK(ch)) {
    punch = rand_number(30, 40);
    if (IS_KURZAK(ch) || IS_TAPION(ch)) {
     punch += rand_number(5, 10);
    }
    SET_SKILL(ch, SKILL_THROW, punch);
   }
   if (IS_KAI(ch) || IS_KANASSAN(ch)) {
    punch = rand_number(40, 60);
    SET_SKILL(ch, SKILL_FOCUS, punch);
   }
   if (IS_KANASSAN(ch)) {
    punch = rand_number(40, 60);
    SET_SKILL(ch, SKILL_CONCENTRATION, punch);
   }
   if (IS_KAI(ch)) {
    punch = rand_number(30, 50);
    SET_SKILL(ch, SKILL_HEAL, punch);
   }
   if (IS_DEMON(ch)) {
    punch = rand_number(50, 60);
    SET_SKILL(ch, SKILL_SPEAR, punch);
   }

   punch = 0;
   punch = rand_number(50, 70);
   SET_SKILL(ch, SKILL_PUNCH, GET_SKILL_BASE(ch, SKILL_PUNCH) + punch);
  } /* End CC skills */
   else {
   REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_FORGET);
  }

   if (IS_KAI(ch) || IS_KANASSAN(ch)) {
    punch = rand_number(15, 30);
    SET_SKILL(ch, SKILL_TELEPATHY, punch);
   }
   if (IS_MAJIN(ch) || IS_NAMEK(ch) || IS_BIO(ch)) {
    punch = rand_number(10, 16);
    SET_SKILL(ch, SKILL_REGENERATE, punch);
   }
   if (IS_ANDROID(ch) && PLR_FLAGGED(ch, PLR_ABSORB)) {
    punch = rand_number(25, 35);
    SET_SKILL(ch, SKILL_ABSORB, punch);
   }
   if (IS_BIO(ch)) {
    punch = rand_number(15, 25);
    SET_SKILL(ch, SKILL_ABSORB, punch);
   }
   if (IS_ARLIAN(ch)) {
    punch = rand_number(30, 50);
    SET_SKILL(ch, SKILL_SEISHOU, punch);
   }
   if (IS_ICER(ch)) {
    punch = rand_number(20, 30);
    SET_SKILL(ch, SKILL_TAILWHIP, punch);
   }

  if (GET_CLASS(ch) < 0 || GET_CLASS(ch) > NUM_CLASSES) {
    log("Unknown character class %d in do_start, resetting.", GET_CLASS(ch));
    GET_CLASS(ch) = 0;
  }
  if (GET_ALIGNMENT(ch) < 51 && GET_ALIGNMENT(ch) > -51) {
   set_title(ch, "the Warrior");
  }
  if (GET_ALIGNMENT(ch) >= 51) {
   set_title(ch, "the Hero");
  }
  if (GET_ALIGNMENT(ch) <= -51) {
   set_title(ch, "The Villain");
  }
  /* roll_real_abils(ch); */
  if (GET_GOLD(ch) <= 0) {
  GET_GOLD(ch) = dice(3, 6) * 10;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (GET_RACE(ch)) {
  case RACE_HUMAN:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_SAIYAN:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_HALFBREED:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_ICER:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_KONATSU:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_NAMEK:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_MUTANT:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  case RACE_ANDROID:
    SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  default:
    SET_SKILL(ch, SKILL_LANG_COMMON, 1);
    break;
  }

  SPEAKING(ch) = SKILL_LANG_COMMON;

  GET_LIFEPERC(ch) = 75;

  /* assign starting items etc...*/
  obj = read_object(17, VIRTUAL);
  obj_to_char(obj, ch);
  if (IS_HOSHIJIN(ch)) {
   obj = read_object(3428, VIRTUAL);
   obj_to_char(obj, ch);
  }
  struct obj_data *obj2;
  obj2 = read_object(17998, VIRTUAL);
  obj_to_char(obj2, ch);

      if (IS_TAPION(ch) || IS_GINYU(ch)) {
       struct obj_data *throw;
       throw = read_object(19050, VIRTUAL);
       obj_to_char(throw, ch);
       if (rand_number(1, 2) == 2) {
        throw = NULL;
        throw = read_object(19050, VIRTUAL);
        obj_to_char(throw, ch);
       }
       if (rand_number(1, 2) == 2) {
        throw = NULL;
        throw = read_object(19050, VIRTUAL);
        obj_to_char(throw, ch);
       }

      } else if (IS_DABURA(ch)) {
       struct obj_data *throw;
       throw = read_object(19055, VIRTUAL);
       obj_to_char(throw, ch);
       throw = NULL;
       throw = read_object(19055, VIRTUAL);
       obj_to_char(throw, ch);
      }

  send_to_imm("New character created, %s, by user, %s.", GET_NAME(ch), GET_USER(ch));
  advance_level(ch, GET_CLASS(ch));
  /*mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));*/

  if (GET_MAX_HIT(ch) < 100) {
   GET_MAX_HIT(ch) = 100;
  }
  if (GET_MAX_MANA(ch) < 100) {
   GET_MAX_MANA(ch) = 100;
  }
  if (GET_MAX_MOVE(ch) < 100) {
   GET_MAX_MOVE(ch) = 100;
  }

  if (IS_ANDROID(ch) && PLR_FLAGGED(ch, PLR_SENSEM)) {
   SET_SKILL(ch, SKILL_SENSE, 100);
   GET_MAX_HIT(ch) += rand_number(400, 500);
   GET_MAX_MANA(ch) += rand_number(400, 500);
   GET_MAX_MOVE(ch) += rand_number(400, 500);
  }

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  
  GET_BASE_PL(ch) = GET_MAX_HIT(ch);
  GET_BASE_KI(ch) = GET_MAX_MANA(ch);
  GET_BASE_ST(ch) = GET_MAX_MOVE(ch);
     if (ch->real_abils.str > 20) {
      ch->real_abils.str = 20;
     }
     if (ch->real_abils.str < 8) {
      ch->real_abils.str = 8;
     }
     if (ch->real_abils.con > 20) {
      ch->real_abils.con = 20;
     }
     if (ch->real_abils.con < 8) {
      ch->real_abils.con = 8;
     }
     if (ch->real_abils.intel > 20) {
      ch->real_abils.intel = 20;
     }
     if (ch->real_abils.intel < 8) {
      ch->real_abils.intel = 8;
     }
     if (ch->real_abils.cha > 20) {
      ch->real_abils.cha = 20;
     }
     if (ch->real_abils.cha < 8) {
      ch->real_abils.cha = 8;
     }
     if (ch->real_abils.dex > 20) {
      ch->real_abils.dex = 20;
     }
     if (ch->real_abils.dex < 8) {
      ch->real_abils.dex = 8;
     }
     if (ch->real_abils.wis > 20) {
      ch->real_abils.wis = 20;
     }
     if (ch->real_abils.wis < 8) {
      ch->real_abils.wis = 8;
     }

  GET_TRANSCLASS(ch) = rand_number(1, 3);

  if (CONFIG_SITEOK_ALL)
    SET_BIT_AR(PLR_FLAGS(ch), PLR_SITEOK);

  if (GET_RACE(ch) == RACE_SAIYAN && rand_number(1, 100) >= 95) {
     SET_BIT_AR(PLR_FLAGS(ch), PLR_LSSJ);
     write_to_output(ch->desc, "@GYou were one of the few born a Legendary Super Saiyan!@n\r\n");
  }
  ch->player_specials->olc_zone = NOWHERE;
  save_char(ch);
}


static const int free_start_feats_wizard[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_SCRIBE_SCROLL,
  0
};

static const int free_start_feats_sorcerer[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  0
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static const int free_start_feats_cleric[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static const int free_start_feats_rogue[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  0
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static const int free_start_feats_fighter[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

static const int free_start_feats_paladin[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_HEAVY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};



static const int free_start_feats_barbarian[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

static const int free_start_feats_bard[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

static const int free_start_feats_ranger[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static const int free_start_feats_monk[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_MARTIAL_WEAPON_PROFICIENCY,
  FEAT_IMPROVED_GRAPPLE,
  0
};

static const int free_start_feats_druid[] = {
  FEAT_SIMPLE_WEAPON_PROFICIENCY,
  FEAT_ARMOR_PROFICIENCY_LIGHT,
  FEAT_ARMOR_PROFICIENCY_MEDIUM,
  FEAT_ARMOR_PROFICIENCY_SHIELD,
  0
};

static const int no_free_start_feats[] = {
 0
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
static const int *free_start_feats[] = {
 /* CLASS_ROSHI	*/ free_start_feats_wizard,
 /* CLASS_PICCOLO	*/ free_start_feats_cleric,
 /* CLASS_KRANE		*/ free_start_feats_rogue,
 /* CLASS_NAIL	*/ free_start_feats_fighter,
 /* CLASS_ANDSIX		*/ free_start_feats_monk,
 /* CLASS_GINYU	*/ free_start_feats_paladin,
 /* CLASS_FRIEZA      */ free_start_feats_sorcerer,
 /* CLASS_TAPION         */ free_start_feats_druid,
 /* CLASS_ANDSIX          */ free_start_feats_bard,
 /* CLASS_DABURA        */ free_start_feats_ranger,
 /* CLASS_KABITO     */ free_start_feats_barbarian,
 /* CLASS_ARCANE_AR	*/ no_free_start_feats,
 /* CLASS_ARCANE_TR	*/ no_free_start_feats,
 /* CLASS_ARCHMAGE	*/ free_start_feats_wizard,
 /* CLASS_ASSASSIN	*/ no_free_start_feats,
 /* CLASS_BLACKGUARD	*/ no_free_start_feats,
 /* CLASS_DRAGON_D 	*/ no_free_start_feats,
 /* CLASS_DUELIST       */ no_free_start_feats,
 /* CLASS_DWARVEN_DEF   */ no_free_start_feats,
 /* CLASS_ELDRITCH_KN   */ no_free_start_feats,
 /* CLASS_HIEROPHANT    */ no_free_start_feats,
 /* CLASS_HORIZON_WALK  */ no_free_start_feats,
 /* CLASS_LOREMASTER    */ no_free_start_feats,
 /* CLASS_MYSTIC_THEU   */ no_free_start_feats,
 /* CLASS_SHADOWDANCER  */ no_free_start_feats,
 /* CLASS_THAUMATURGIST */ no_free_start_feats
};

/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
 /* Rillao: transloc, add new transes here */
void advance_level(struct char_data *ch, int whichclass)
{
  struct levelup_data *llog;
  int64_t add_hp = 0, add_move = 0, add_mana = 0, add_ki = 0;
  int add_prac = 1, add_train, i, j = 0, ranks;
  int add_gen_feats = 0, add_class_feats = 0;
  char buf[MAX_STRING_LENGTH];

  if (whichclass < 0 || whichclass >= NUM_CLASSES) {
    log("Invalid class %d passed to advance_level, resetting.", whichclass);
    whichclass = 0;
  }

  if (!CONFIG_ALLOW_MULTICLASS && whichclass != GET_CLASS(ch)) {
    log("Attempt to gain a second class without multiclass enabled for %s", GET_NAME(ch));
    whichclass = GET_CLASS(ch);
  }
  ranks = GET_CLASS_RANKS(ch, whichclass);

  CREATE(llog, struct levelup_data, 1);
  llog->next = ch->level_info;
  llog->prev = NULL;
  if (llog->next)
    llog->next->prev = llog;
  ch->level_info = llog;

  llog->skills = llog->feats = NULL;
  llog->type = LEVELTYPE_CLASS;
  llog->spec = whichclass;
  llog->level = GET_LEVEL(ch);

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  switch (ranks) {
  case 1:
    for (i = 0; (j = free_start_feats[whichclass][i]); i++) {
      HAS_FEAT(ch, j) = 1;
    }
    break;
  case 2:
    switch (whichclass) {
    case CLASS_ROSHI:
      break;
    case CLASS_PICCOLO:
      break;
    }
    break;
  case 6:
    switch (whichclass) {
    case CLASS_ROSHI:
      break;
    }
    break;
  case 9:
    switch (whichclass) {
    case CLASS_ROSHI:
      break;
    }
    break;
  }

  if (GET_CLASS_LEVEL(ch) == 1 && GET_HITDICE(ch) < 2) { /* Filled in below */
      GET_HITDICE(ch) = 0;
      GET_SAVE_BASE(ch, SAVING_FORTITUDE) = 0;
      GET_SAVE_BASE(ch, SAVING_REFLEX) = 0;
      GET_SAVE_BASE(ch, SAVING_WILL) = 0;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (ranks >= LVL_EPICSTART * 20) { /* Epic class */
    j = ranks - 20;
    switch (whichclass) {
    case CLASS_ROSHI:
    case CLASS_JINTO:
    case CLASS_DRAGON_DISCIPLE:
    case CLASS_ELDRITCH_KNIGHT:
    case CLASS_HORIZON_WALKER:
      if (!(j % 4))
        add_class_feats++;
      break;
    case CLASS_PICCOLO:
      if (!(j % 2))
        add_class_feats++;
      break;
    case CLASS_KRANE:
      if (!(j % 5))
        add_class_feats++;
      break;
    case CLASS_NPC_EXPERT:
    case CLASS_NPC_ADEPT:
    case CLASS_NPC_COMMONER:
    case CLASS_NPC_ARISTOCRAT:
    case CLASS_NPC_WARRIOR:
      break;
    case CLASS_DWARVEN_DEFENDER:
    case CLASS_MYSTIC_THEURGE:
     if (!(j % 6))
       add_class_feats++;
      break;
    default:
      if (!(j % 3))
        add_class_feats++;
      break;
    }
  } else {
    switch (whichclass) {
    case CLASS_ROSHI:
      if (ranks == 1 || !(ranks % 2))
        add_class_feats++;
      break;
    case CLASS_PICCOLO:
      if (ranks > 9 && !(ranks % 3))
        add_class_feats++;
      break;
    case CLASS_KRANE:
      if (!(ranks % 5))
        add_class_feats++;
      break;
    default:
      break;
    }
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_LEVEL(ch) >= 1) {
  double pl_percent = 10, ki_percent = 10, st_percent = 10, prac_reward = GET_WIS(ch);
  
  if (prac_reward < 10) {
   prac_reward = 10;
  }

  if (GET_LEVEL(ch) >= 91) {
   pl_percent -= 7.2;
   ki_percent -= 7.2;
   st_percent -= 7.2;
  } else if (GET_LEVEL(ch) >= 81) {
   pl_percent -= 5.8;
   ki_percent -= 5.8;
   st_percent -= 5.8; 
  } else if (GET_LEVEL(ch) >= 71) {
   pl_percent -= 5;
   ki_percent -= 5;
   st_percent -= 5; 
  } else if (GET_LEVEL(ch) >= 61) {
   pl_percent -= 4;
   ki_percent -= 4;
   st_percent -= 4; 
  } else if (GET_LEVEL(ch) >= 51) {
   pl_percent -= 3;
   ki_percent -= 3;
   st_percent -= 3; 
  } else if (GET_LEVEL(ch) >= 41) {
   pl_percent -= 1.7;
   ki_percent -= 1.7;
   st_percent -= 1.7; 
  } else if (GET_LEVEL(ch) >= 31) {
   pl_percent -= 0.8;
   ki_percent -= 0.8;
   st_percent -= 0.8; 
  } else if (GET_LEVEL(ch) >= 21) {
   pl_percent -= 0.35;
   ki_percent -= 0.35;
   st_percent -= 0.35; 
  } else if (GET_LEVEL(ch) >= 11) {
   pl_percent -= 0.15;
   ki_percent -= 0.15;
   st_percent -= 0.15;
  }

  switch (GET_RACE(ch)) {
    case RACE_HUMAN:
     ki_percent += 3;
     break;
    case RACE_SAIYAN:
    case RACE_MAJIN:
     pl_percent += 3;
     break;
    case RACE_MUTANT:
     st_percent += 3;
     break;
    case RACE_HALFBREED:
     pl_percent += 1.5;
     ki_percent += 1.5;
     prac_reward -= prac_reward * 0.4;
     break;
    case RACE_TRUFFLE:
     prac_reward += prac_reward * 0.5;
     break;
   }

   if (!IS_HUMAN(ch)) {
    add_hp = (GET_BASE_PL(ch) * 0.01) * pl_percent;
   } else if (IS_HUMAN(ch)) {
    add_hp = ((GET_BASE_PL(ch) * 0.01) * pl_percent) * 0.8;
   }
   add_mana = (GET_BASE_KI(ch) * 0.01) * ki_percent;
   add_move = (GET_BASE_ST(ch) * 0.01) * st_percent;
   add_prac = prac_reward + GET_INT(ch);
  }
  if (add_hp >= 300000 && add_hp < 600000) {
   add_hp *= .75;
   if (add_hp < 300000) {
    add_hp = rand_number(300000, 330000);
   }
  }
  else if (add_hp >= 600000 && add_hp < 1000000) {
   add_hp *= .70;
   if (add_hp < 600000) {
    add_hp = rand_number(600000, 650000);
   }
  }
  else if (add_hp >= 1000000 && add_hp < 2000000) {
   add_hp *= .65;
   if (add_hp < 1000000) {
    add_hp = rand_number(1000000, 1250000);
   }
  }
  else if (add_hp >= 2000000) {
   add_hp *= .45;
   if (add_hp < 2000000) {
    add_hp = rand_number(2000000, 2250000);
   }
  }
  if (add_hp >= 15000000) {
   add_hp = 15000000;
  }
  if (add_move >= 300000 && add_move < 600000) {
   add_move *= .75;
   if (add_move < 300000) {
    add_move = rand_number(300000, 330000);
   }
  }
  else if (add_move >= 600000 && add_move < 1000000) {
   add_move *= .70;
   if (add_move < 600000) {
    add_move = rand_number(600000, 650000);
   }
  }
  else if (add_move >= 1000000 && add_move < 2000000) {
   add_move *= .65;
   if (add_move < 1000000) {
    add_move = rand_number(1000000, 1250000);
   }
  }
  else if (add_move >= 2000000) {
   add_move *= .45;
   if (add_move < 2000000) {
    add_move = rand_number(2000000, 2250000);
   }
  }
  if (add_move >= 15000000) {
   add_move = 15000000;
  }
  if (add_mana >= 300000 && add_mana < 600000) {
   add_mana *= .75;
   if (add_mana < 300000) {
    add_mana = rand_number(300000, 330000);
   }
  }
  else if (add_mana >= 600000 && add_mana < 1000000) {
   add_mana *= .70;
   if (add_mana < 600000) {
    add_mana = rand_number(600000, 650000);
   }
  }
  else if (add_mana >= 1000000 && add_mana < 2000000) {
   add_mana *= .65;
   if (add_mana < 1000000) {
    add_mana = rand_number(1000000, 1250000);
   }
  }
  else if (add_mana >= 2000000) {
   add_mana *= .45;
   if (add_mana < 2000000) {
    add_mana = rand_number(2000000, 2250000);
   }
  }
  if (add_mana >= 15000000) {
   add_mana = 15000000;
  }
  switch (GET_LEVEL(ch)) {
   case 5:
    add_hp += rand_number(600, 1000);
    add_move += rand_number(600, 1000);
    add_mana += rand_number(600, 1000);
    break;
   case 10:
    add_hp += rand_number(5000, 8000);
    add_move += rand_number(5000, 8000);
    add_mana += rand_number(5000, 8000);
    break;
   case 20:
    add_hp += rand_number(15000, 18000);
    add_move += rand_number(15000, 18000);
    add_mana += rand_number(15000, 18000);
    break;
   case 30:
    add_hp += rand_number(20000, 30000);
    add_move += rand_number(20000, 30000);
    add_mana += rand_number(20000, 30000);
    break;
   case 40:
    add_hp += rand_number(50000, 60000);
    add_move += rand_number(50000, 60000);
    add_mana += rand_number(50000, 60000);
    break;
   case 50:
    add_hp += rand_number(60000, 70000);
    add_move += rand_number(60000, 70000);
    add_mana += rand_number(60000, 70000);
    break;
   case 60:
    add_hp += rand_number(80000, 100000);
    add_move += rand_number(80000, 100000);
    add_mana += rand_number(80000, 100000);
    break;
   case 70:
    add_hp += rand_number(100000, 150000);
    add_move += rand_number(100000, 150000);
    add_mana += rand_number(100000, 150000);
    break;
   case 80:
    add_hp += rand_number(150000, 200000);
    add_move += rand_number(150000, 200000);
    add_mana += rand_number(150000, 200000);
    break;
   case 90:
    add_hp += rand_number(150000, 200000);
    add_move += rand_number(150000, 200000);
    add_mana += rand_number(150000, 200000);
    break;
   case 99:
    add_hp += rand_number(1500000, 2000000);
    add_move += rand_number(1500000, 2000000);
    add_mana += rand_number(1500000, 2000000);
    break;
   case 100:
    add_hp += rand_number(5000000, 6000000);
    add_move += rand_number(5000000, 6000000);
    add_mana += rand_number(5000000, 6000000);
    break;
  }
  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_LEVEL(ch) == 1 || !(GET_LEVEL(ch) % 3)) {
    add_gen_feats += 1;
  }

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (GET_RACE(ch) == RACE_HUMAN) /* Humans get 4 extra at level 1 and  */
    add_prac += 2;                /* 1 extra per level for adaptability */

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  i = ability_mod_value(GET_CON(ch));
  if (GET_LEVEL(ch) > 1) {
    /* blah */
  } else {
    GET_MAX_HIT(ch) += rand_number(1, 20);
    if (GET_MAX_HIT(ch) > 250) {
      GET_MAX_HIT(ch) = 250;
    }
    if (GET_MAX_MANA(ch) > 250) {
      GET_MAX_MANA(ch) = 250;
    }
    if (GET_MAX_MOVE(ch) > 250) {
      GET_MAX_MOVE(ch) = 250;
    }
    GET_BASE_PL(ch) = GET_MAX_HIT(ch);
    GET_BASE_KI(ch) = GET_MAX_MANA(ch);
    GET_BASE_ST(ch) = GET_MAX_HIT(ch);
    add_prac = 5;
    if (PLR_FLAGGED(ch, PLR_SKILLP)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_SKILLP);
    add_prac *= 5;
    }
    else {
    add_prac *= 2;
    }
  }
  llog->hp_roll = j;

  /* Derived from the SRD under OGL, see ../doc/srd.txt for information */
  if (rand_number(1, 8) == 2) {
  add_train = 1;
   if (add_train) {
    GET_TRAINS(ch) += add_train;
   }
  }
  if (rand_number(1, 4) == 4) {
  send_to_char(ch, "@D[@mPractice Session Bonus!@D]@n\r\n");
  add_prac += rand_number(4, 12);
  }

  if ((IS_DEMON(ch) || IS_KANASSAN(ch)) && GET_LEVEL(ch) > 80) {
   add_hp *= 2;
   add_mana *= 2;
   add_move *= 2;
  }
  else if ((IS_DEMON(ch) || IS_KANASSAN(ch)) && GET_LEVEL(ch) > 60) {
   add_hp *= 1.75;
   add_mana *= 1.75;
   add_move *= 1.75;
  }
  else if ((IS_DEMON(ch) || IS_KANASSAN(ch)) && GET_LEVEL(ch) > 50) {
   add_hp *= 1.5;
   add_mana *= 1.5;
   add_move *= 1.5;
  }
  /* Rillao: transloc, add new transes here */
  else if ((IS_DEMON(ch) || IS_KANASSAN(ch)) && GET_LEVEL(ch) > 40) {
   add_hp *= 1.25;
   add_mana *= 1.25;
   add_move *= 1.25;
  }
  llog->mana_roll = add_mana;
  llog->move_roll = add_move;
  llog->ki_roll = add_ki;
  llog->add_skill = add_prac;
  GET_PRACTICES(ch, whichclass) += add_prac;
  GET_BASE_PL(ch) += add_hp;
  GET_BASE_KI(ch) += add_mana;
  GET_BASE_ST(ch) += add_move;
  int nhp = add_hp;
  int nma = add_mana;
  int nmo = add_move;
 if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
  add_hp *= 3;
  add_move *= 3;
  add_mana *= 3;
 }
 else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
  add_hp *= 4;
  add_move *= 4;
  add_mana *= 4;
 }
 else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
  add_hp *= 5;
  add_move *= 5;
  add_mana *= 5;
 }
 else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 1) {
  add_hp *= 2;
  add_move *= 2;
  add_mana *= 2;
 }
 else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 2) {
  add_hp *= 3;
  add_move *= 3;
  add_mana *= 3;
 }
 else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
  add_hp *= 2;
  add_move *= 2;
  add_mana *= 2;
 }
 else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
  add_hp *= 3;
  add_move *= 3;
  add_mana *= 3;
 }
 else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
  add_hp *= 3.5;
  add_move *= 3.5;
  add_mana *= 3.5;
 }
 else if (IS_BIO(ch) && PLR_FLAGGED(ch, PLR_TRANS4)) {
  add_hp *= 4;
  add_move *= 4;
  add_mana *= 4;
 }
 else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
  add_hp *= 2;
  add_move *= 2;
  add_mana *= 2;
 }
 else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
  add_hp *= 3;
  add_move *= 3;
  add_mana *= 3;
 }
 else if (IS_MAJIN(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
  add_hp *= 4.5;
  add_move *= 4.5;
  add_mana *= 4.5;
 }
 GET_MAX_HIT(ch) += add_hp;
 GET_MAX_MOVE(ch) += add_move;
 GET_MAX_MANA(ch) += add_mana;
 add_hp = nhp;
 add_mana = nma;
 add_move = nmo;
  if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART) { /* Epic character */
    GET_EPIC_FEAT_POINTS(ch) += add_gen_feats;
    llog->add_epic_feats = add_gen_feats;
    GET_EPIC_CLASS_FEATS(ch, whichclass) += add_class_feats;
    llog->add_class_epic_feats = add_class_feats;
  } else {
    GET_FEAT_POINTS(ch) += add_gen_feats;
    llog->add_gen_feats = add_gen_feats;
    GET_CLASS_FEATS(ch, whichclass) += add_class_feats;
    llog->add_class_feats = add_class_feats;
  }

  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  sprintf(buf, "@D[@YGain@D: @RPl@D(@G%s@D) @gSt@D(@G%s@D) @CKi@D(@G%s@D) @bPS@D(@G%s@D)]", add_commas(add_hp), add_commas(add_move), add_commas(add_mana), add_commas(add_prac));
  if (GET_BONUS(ch, BONUS_GMEMORY) && (GET_LEVEL(ch) == 20 || GET_LEVEL(ch) == 40 || GET_LEVEL(ch) == 60 || GET_LEVEL(ch) == 80 || GET_LEVEL(ch) == 100)) {
   GET_SLOTS(ch) += 1;
   send_to_char(ch, "@CYou feel like you could remember a new skill!@n\r\n");
  }
  if (IS_NAMEK(ch) && rand_number(1, 100) <= 5) {
   GET_SLOTS(ch) += 1;
   send_to_char(ch, "@CYou feel as though you could learn another skill.@n\r\n");
  }
  if (IS_ICER(ch) && rand_number(1, 100) <= 25) {
   bring_to_cap(ch);
   send_to_char(ch, "@GYou feel your body obtain its current optimal strength!@n\r\n");
  }

  int gain_stat = FALSE;

  switch (GET_LEVEL(ch)) {
   case 10:
   case 20:
   case 30:
   case 40:
   case 50:
   case 60:
   case 70:
   case 80:
   case 90:
   case 100:
    gain_stat = TRUE;
    break;
  }

  if (gain_stat == TRUE) {
   int raise = FALSE, stat_fail = 0;
   if (IS_KONATSU(ch)) {
    while (raise == FALSE) {
     if (ch->real_abils.dex < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
      if (ch->real_abils.dex < 45 || GET_BONUS(ch, BONUS_CLUMSY) <= 0) { 
       ch->real_abils.dex += 1;
       send_to_char(ch, "@GYou feel your agility increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 1;
      }
     } else if (ch->real_abils.cha < 100 && raise == FALSE && stat_fail < 2) {
      if (ch->real_abils.cha < 45 || GET_BONUS(ch, BONUS_SLOW) > 0) {
       ch->real_abils.cha += 1;
       send_to_char(ch, "@GYou feel your speed increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 2;
      }
     } else if (stat_fail == 3) {
      send_to_char(ch, "@RBoth agility and speed are capped!@n");
      raise = TRUE;
     }
    } // End while
   } // End Konatsu

   else if (IS_MUTANT(ch)) {
    while (raise == FALSE) {
     if (ch->real_abils.con < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
      if (ch->real_abils.con < 45 || GET_BONUS(ch, BONUS_FRAIL) <= 0) { 
       ch->real_abils.con += 1;
       send_to_char(ch, "@GYou feel your constitution increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 1;
      }
     } else if (ch->real_abils.cha < 100 && raise == FALSE && stat_fail < 2) {
      if (ch->real_abils.cha < 45 || GET_BONUS(ch, BONUS_SLOW) > 0) {
       ch->real_abils.cha += 1;
       send_to_char(ch, "@GYou feel your speed increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 2;
      }
     } else if (stat_fail == 3) {
      send_to_char(ch, "@RBoth constitution and speed are capped!@n");
      raise = TRUE;
     }
    } // End while
   } // End Mutant

   else if (IS_HOSHIJIN(ch)) {
    while (raise == FALSE) {
     if (ch->real_abils.str < 100 && rand_number(1, 2) == 2 && stat_fail != 1) {
      if (ch->real_abils.str < 45 || GET_BONUS(ch, BONUS_WIMP) <= 0) { 
       ch->real_abils.str += 1;
       send_to_char(ch, "@GYou feel your strength increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 1;
      }
     } else if (ch->real_abils.dex < 100 && raise == FALSE && stat_fail < 2) {
      if (ch->real_abils.dex < 45 || GET_BONUS(ch, BONUS_SLOW) > 0) {
       ch->real_abils.dex += 1;
       send_to_char(ch, "@GYou feel your agility increase!@n\r\n");
       raise = TRUE;
      } else {
       stat_fail += 2;
      }
     } else if (stat_fail == 3) {
      send_to_char(ch, "@RBoth strength and agility are capped!@n");
      raise = TRUE;
     }
    } // End while
   } // End Mutant

  } // End stat bonus on level gain

  strcat(buf, ".\r\n");
  send_to_char(ch, "%s", buf);

  if (GET_SKILL(ch, SKILL_POTENTIAL) && rand_number(1, 4) == 4) {
   send_to_char(ch, "You can now perform another Potential Release.\r\n");
   GET_BOOSTS(ch) += 1;   
  }
  if (IS_MAJIN(ch) && GET_LEVEL(ch) == 25) {
   send_to_char(ch, "You can now perform another Majinization.\r\n");
   GET_BOOSTS(ch) += 1;
  }
  if (IS_MAJIN(ch) && GET_LEVEL(ch) == 50) {
   send_to_char(ch, "You can now perform another Majinization.\r\n");
   GET_BOOSTS(ch) += 1;
  }
  if (IS_MAJIN(ch) && GET_LEVEL(ch) == 75) {
   send_to_char(ch, "You can now perform another Majinization.\r\n");
   GET_BOOSTS(ch) += 1;
  }
  if (IS_MAJIN(ch) && GET_LEVEL(ch) == 100) {
   send_to_char(ch, "You can now perform another Majinization.\r\n");
   GET_BOOSTS(ch) += 1;
  }

   switch (GET_LEVEL(ch)) {
    case 10:
    case 20:
    case 30:
    case 40:
    case 50:
    case 60:
    case 70:
    case 80:
    case 90:
    case 100:
     if (GET_BONUS(ch, BONUS_BRAWNY) > 0) {
      ch->real_abils.str += 2;
      send_to_char(ch, "@GYour muscles have grown stronger!@n\r\n");
     }
     if (GET_BONUS(ch, BONUS_SCHOLARLY) > 0) {
      ch->real_abils.intel += 2;
      send_to_char(ch, "@GYour mind has grown sharper!@n\r\n");
     }
     if (GET_BONUS(ch, BONUS_SAGE) > 0) {
      ch->real_abils.wis += 2;
      send_to_char(ch, "@GYour understanding about life has improved!@n\r\n");
     }
     if (GET_BONUS(ch, BONUS_AGILE) > 0) {
      ch->real_abils.dex += 2;
      send_to_char(ch, "@GYour body has grown more agile!@n\r\n");
     }
     if (GET_BONUS(ch, BONUS_QUICK) > 0) {
      ch->real_abils.cha += 2;
      send_to_char(ch, "@GYou feel like your speed has improved!@n\r\n");
     }
     if (GET_BONUS(ch, BONUS_STURDY) > 0) {
      ch->real_abils.con += 2;
      send_to_char(ch, "@GYour body feels tougher now!@n\r\n");
     }
     break;
   }

  if (GET_LEVEL(ch) == 1) {
   GET_ARMOR(ch) = 0;
  }
  if (GET_LEVEL(ch) == 2) {
   ERAPLAYERS += 1;
  }
  snoop_check(ch);
  save_char(ch);
}

/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_ROSHI(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_PICCOLO(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_FIGHTER) && IS_NAIL(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ROGUE) && IS_KRANE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MONK) && IS_BARDOCK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_MONK) && !IS_BARDOCK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_WIZARD) && !IS_ROSHI(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_JINTO) && !IS_JINTO(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_CLERIC) && !IS_PICCOLO(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_ROGUE) && !IS_KRANE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ASSASSIN) && !IS_KURZAK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_FIGHTER) && !IS_NAIL(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_PALADIN) && !IS_GINYU(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WIZARD) && IS_ROSHI(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_PALADIN) && IS_GINYU(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARBARIAN) && IS_KABITO(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BARD) && IS_ANDSIX(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ONLY_BARD) && !IS_ANDSIX(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_DABURA(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRUID) && IS_TAPION(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_ARCHER) && IS_JINTO(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANE_TRICKSTER) && IS_TSUNA(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ARCHMAGE) && IS_KURZAK(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_BLACKGUARD) && IS_BLACKGUARD(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DRAGON_DISCIPLE) && IS_DRAGON_DISCIPLE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DUELIST) && IS_DUELIST(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARVEN_DEFENDER) && IS_DWARVEN_DEFENDER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ELDRITCH_KNIGHT) && IS_ELDRITCH_KNIGHT(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HIEROPHANT) && IS_HIEROPHANT(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HORIZON_WALKER) && IS_HORIZON_WALKER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_LOREMASTER) && IS_LOREMASTER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_MYSTIC_THEURGE) && IS_MYSTIC_THEURGE(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_SHADOWDANCER) && IS_SHADOWDANCER(ch))
    return TRUE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_THAUMATURGIST) && IS_THAUMATURGIST(ch))
    return TRUE;


  return FALSE;
}


/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */

/* Function to return the exp required for each class/level */
int level_exp(struct char_data *ch, int level)
{
  int req = 1;
 
  switch (level) {
  case 0:
    req = 0;
    break;
  case 1:
    req = 1;
    break;
  case 2:
    req = 999;
    break;
  case 3:
    req = 2000;
    break;
  case 4:
    req = 4500;
    break;
  case 5:
    req = 6000;
    break;
  case 6:
    req = 10000;
    break;
  case 7:
    req = 15000;
    break;
  case 8:
    req = 25000;
    break;
  case 9:
    req = 35000;
    break;
  case 10:
    req = 50000;
    break;
  case 11:
    req = 75000;
    break;
  case 12:
    req = 100000;
    break;
  case 13:
    req = 125000;
    break;
  case 14:
    req = 150000;
    break;
  case 15:
    req = 175000;
    break;
  case 16:
    req = 225000;
    break;
  case 17:
    req = 300000;
    break;
  case 18:
    req = 350000;
    break;
  case 19:
    req = 400000;
    break;
  case 20:
    req = 500000;
    break;
  case 21:
    req = 650000;
    break;
  case 22:
    req = 800000;
    break;
  case 23:
    req = 1000000;
    break;
  case 24:
    req = 1250000;
    break;
  case 25:
    req = 1500000;
    break;
  case 26:
    req = 1750000;
    break;
  case 27:
    req = 2000000;
    break;
  case 28:
    req = 2250000;
    break;
  case 29:
    req = 2500000;
    break;
  case 30:
    req = 2750000;
    break;
  case 31:
    req = 3000000;
    break;
  case 32:
    req = 3250000;
    break;
  case 33:
    req = 3500000;
    break;
  case 34:
    req = 3750000;
    break;
  case 35:
    req = 4000000;
    break;
  case 36:
    req = 4250000;
    break;
  case 37:
    req = 4500000;
    break;
  case 38:
    req = 4750000;
    break;
  case 39:
    req = 5000000;
    break;
  case 40:
    req = 5500000;
    break;
  case 41:
    req = 6000000;
    break;
  case 42:
    req = 6500000;
    break;
  case 43:
    req = 7000000;
    break;
  case 44:
    req = 7500000;
    break;
  case 45:
    req = 8000000;
    break;
  case 46:
    req = 8500000;
    break;
  case 47:
    req = 9000000;
    break;
  case 48:
    req = 9500000;
    break;
  case 49:
    req = 10000000;
    break;
  case 50:
    req = 10500000;
    break;
  case 51:
    req = 11000000;
    break;
  case 52:
    req = 11500000;
    break;
  case 53:
    req = 12000000;
    break;
  case 54:
    req = 12500000;
    break;
  case 55:
    req = 13000000;
    break;
  case 56:
    req = 13500000;
    break;
  case 57:
    req = 14000000;
    break;
  case 58:
    req = 14500000;
    break;
  case 59:
    req = 15000000;
    break;
  case 60:
    req = 18000000;
    break;
  case 61:
    req = 25000000;
    break;
  case 62:
    req = 28000000;
    break;
  case 63:
    req = 31000000;
    break;
  case 64:
    req = 34000000;
    break;
  case 65:
    req = 37000000;
    break;
  case 66:
    req = 40000000;
    break;
  case 67:
    req = 43000000;
    break;
  case 68:
    req = 46000000;
    break;
  case 69:
    req = 49000000;
    break;
  case 70:
    req = 52000000;
    break;
  case 71:
    req = 55000000;
    break;
  case 72:
    req = 58000000;
    break;
  case 73:
    req = 61000000;
    break;
  case 74:
    req = 64000000;
    break;
  case 75:
    req = 67000000;
    break;
  case 76:
    req = 70000000;
    break;
  case 77:
    req = 73000000;
    break;
  case 78:
    req = 76000000;
    break;
  case 79:
    req = 79000000;
    break;
  case 80:
    req = 82000000;
    break;
  case 81:
    req = 88000000;
    break;
  case 82:
    req = 94000000;
    break;
  case 83:
    req = 100000000;
    break;
  case 84:
    req = 106000000;
    break;
  case 85:
    req = 112000000;
    break;
  case 86:
    req = 118000000;
    break;
  case 87:
    req = 124000000;
    break;
  case 88:
    req = 130000000;
    break;
  case 89:
    req = 136000000;
    break;
  case 90:
    req = 142000000;
    break;
  case 91:
    req = 150000000;
    break;
  case 92:
    req = 175000000;
    break;
  case 93:
    req = 200000000;
    break;
  case 94:
    req = 225000000;
    break;
  case 95:
    req = 250000000;
    break;
  case 96:
    req = 300000000;
    break;
  case 97:
    req = 400000000;
    break;
  case 98:
    req = 500000000;
    break;
  case 99:
    req = 600000000;
    break;
  case 100:
    req = 800000000;
    break;
 }

 if (IS_KAI(ch)) {
  req += req * 0.15;
 }

 return (req);
}


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int8_t ability_mod_value(int abil)
{
  return ((int)(abil / 2)) - 5;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int8_t dex_mod_capped(const struct char_data *ch)
{
  int8_t mod;
  struct obj_data *armor;
  mod = ability_mod_value(GET_DEX(ch));
  armor = GET_EQ(ch, WEAR_BODY);
  if (armor && GET_OBJ_TYPE(armor) == ITEM_ARMOR) {
    mod = MIN(mod, GET_OBJ_VAL(armor, VAL_ARMOR_MAXDEXMOD));
  }
  return mod;
}

static int cabbr_ranktable[NUM_CLASSES];

static int comp_rank(const void *a, const void *b)
{
  int first, second;
  first = *(const int *)a;
  second = *(const int *)b;
  return cabbr_ranktable[second] - cabbr_ranktable[first];
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
char *class_desc_str(struct char_data *ch, int howlong, int wantthe)
{
  static char str[MAX_STRING_LENGTH];
  char *ptr = str;
  int i, rank, j;
  int rankorder[NUM_CLASSES];
  char *buf, *buf2, *buf3;

  if (IS_NPC(ch)) {
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d", class_abbrevs[GET_CLASS(ch)], GET_LEVEL(ch) );
    return str;
  }

  if (wantthe)
    ptr += sprintf(str, "the ");

  if (howlong) {
    buf2 = buf = buf3 = "";
    if (howlong == 2) {
      buf3 = " ";
      if (GET_CLASS_LEVEL(ch) >= LVL_EPICSTART)
        ptr += sprintf(ptr, "Epic ");
    }
    for (i = 0; i < NUM_CLASSES; i++) {
      cabbr_ranktable[i] = GET_CLASS_RANKS(ch, i);
      rankorder[i] = i;
    }
    rankorder[0] = GET_CLASS(ch); /* we always want primary class first */
    rankorder[GET_CLASS(ch)] = 0;
    qsort((void *)rankorder, NUM_CLASSES, sizeof(int), comp_rank);
    for (i = 0; i < NUM_CLASSES; i++) {
      rank = rankorder[i];
      if (cabbr_ranktable[rank] == 0)
        continue;
      ptr += snprintf(ptr, sizeof(str) - (ptr - str), "%s%s%s%s%s%d", buf, buf2, buf,
                      (howlong == 2 ? pc_class_types : class_abbrevs)[rank], buf3,
                      cabbr_ranktable[rank]);
      buf2 = "/";
      if (howlong == 2)
        buf = " ";
    }
    return str;
  } else {
    rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
    j = GET_CLASS(ch);
    for (i = 0; i < NUM_CLASSES; i++)
      if (GET_CLASS_RANKS(ch, i) > rank) {
        j = i;
        rank = GET_CLASS_RANKS(ch, j);
      }
    rank = GET_CLASS_RANKS(ch, GET_CLASS(ch));
    snprintf(ptr, sizeof(str) - (ptr - str), "%s%d%s", class_names[GET_CLASS(ch)],
             rank, GET_LEVEL(ch) == rank ? "" : "+");
    return str;
  }
}

int total_skill_levels(struct char_data *ch, int skill)
{
  int i = 0, j, total = 0;
  for (i = 0; i < NUM_CLASSES; i++) {
    j = 1 + GET_CLASS_RANKS(ch, i) - spell_info[skill].min_level[i];
    if (j > 0)
     total += j;
  }
  return total;
}

int load_levels()
{
  FILE *fp;
  char line[READ_SIZE], sect_name[READ_SIZE] = { '\0' }, *ptr;
  int  linenum = 0, tp, cls, sect_type = -1;

  if (!(fp = fopen(LEVEL_CONFIG, "r"))) {
    log("SYSERR: Could not open level configuration file, error: %s!", 
         strerror(errno));
    return -1;
  }

  for (cls = 0; cls < NUM_CLASSES; cls++) {
    for (tp = 0; tp <= SAVING_WILL; tp++) {
      save_classes[tp][cls] = 0;
    }
    basehit_classes[cls] = 0;
  }

  for (;;) {
    linenum++;
    if (!fgets(line, READ_SIZE, fp)) {  /* eof check */
      log("SYSERR: Unexpected EOF in file %s.", LEVEL_CONFIG);
      return -1;
    } else if (*line == '$') { /* end of file */
      break;
    } else if (*line == '*') { /* comment line */
      continue;
    } else if (*line == '#') { /* start of a section */
      if ((tp = sscanf(line, "#%s", sect_name)) != 1) {
        log("SYSERR: Format error in file %s, line number %d - text: %s.", 
             LEVEL_CONFIG, linenum, line);
        return -1;
      } else if ((sect_type = search_block(sect_name, config_sect, FALSE)) == -1) {
          log("SYSERR: Invalid section in file %s, line number %d: %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
      }
    } else {
      if (sect_type == CONFIG_LEVEL_VERSION) {
        if (!strncmp(line, "Suntzu", 6)) {
          log("SYSERR: Suntzu %s config files are not compatible with rasputin", LEVEL_CONFIG);
          return -1;
        } else {
          strcpy(level_version, line); /* OK - both are READ_SIZE */
        }
      } else if (sect_type == CONFIG_LEVEL_VERNUM) {
	level_vernum = atoi(line);
      } else if (sect_type == CONFIG_LEVEL_EXPERIENCE) {
        tp = atoi(line);
        exp_multiplier = tp;
      } else if ((sect_type >= CONFIG_LEVEL_FORTITUDE && sect_type <= CONFIG_LEVEL_WILL) ||
                 sect_type == CONFIG_LEVEL_BASEHIT) {
        for (ptr = line; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (!ptr || !*ptr || !isdigit(*ptr)) {
          log("SYSERR: Cannot find class number in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        cls = atoi(ptr);
        for (; ptr && *ptr && isdigit(*ptr); ptr++);
        for (; ptr && *ptr && !isdigit(*ptr); ptr++);
        if (ptr && *ptr && !isdigit(*ptr)) {
          log("SYSERR: Non-numeric entry in file %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (ptr && *ptr) /* There's a value */
          tp = atoi(ptr);
        else {
          log("SYSERR: Need 1 value in %s, line number %d, section %s.", 
              LEVEL_CONFIG, linenum, sect_name);
          return -1;
        }
        if (cls < 0 || cls >= NUM_CLASSES) {
          log("SYSERR: Invalid class number %d in file %s, line number %d.", 
              cls, LEVEL_CONFIG, linenum);
          return -1;
        } else {
          if (sect_type == CONFIG_LEVEL_BASEHIT) {
            basehit_classes[cls] = tp;
          } else {
            save_classes[SAVING_FORTITUDE + sect_type - CONFIG_LEVEL_FORTITUDE][cls] = tp;
          }
        }
      } else {
        log("Unsupported level config option");
      }
    }
  }
  fclose(fp);

  for (cls = 0; cls < NUM_CLASSES; cls++) 
    log("Base hit for class %s: %s", class_names[cls], basehit_type_names[basehit_classes[cls]]);

  for (cls = 0; cls < NUM_CLASSES; cls++)
    log("Saves for class %s: fort=%s, reflex=%s, will=%s", class_names[cls],
        save_type_names[save_classes[SAVING_FORTITUDE][cls]],
        save_type_names[save_classes[SAVING_REFLEX][cls]],
        save_type_names[save_classes[SAVING_WILL][cls]]);

  return 0;
}


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int highest_skill_value(int level, int type)
{
  if (level >= 60)
   return 100;
  else if (level >= 20)
   return level + 40;
  else if (level >= 10)
   return level + 30;
  else if (level >= 1)
   return level + 25;
  else
   return 0;
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
/* Not anymore because for DBZ it was crap and unnecessary. */
int calc_penalty_exp(struct char_data *ch, int gain)
{
  return gain;
}

static const int size_scaling_table[NUM_SIZES][4] = {
/*                   str       dex     con  nat arm */
/* Fine		*/ { -10,	-2,	-2,	0 },
/* Diminutive	*/ { -10,	-2,	-2,	0 },
/* Tiny		*/ { -8,	-2,	-2,	0 },
/* Small	*/ { -4,	-2,	-2,	0 },
/* Medium	*/ { 0,		0,	0,	0 },
/* Large	*/ { 8,		-2,	4,	2 },
/* Huge		*/ { 16,	-4,	8,	5 },
/* Gargantuan	*/ { 24,	-4,	12,	9 },
/* Colossal	*/ { 32,	-4,	16,	14 }
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
time_t birth_age(struct char_data *ch)
{
  int tmp;

  tmp = rand_number(16, 18);

  return tmp;
}

time_t max_age(struct char_data *ch)
{
  struct aging_data *aging;
  size_t tmp;

  if (ch->time.maxage)
    return ch->time.maxage - ch->time.birth;

  aging = racial_aging_data + GET_RACE(ch);

  tmp = 120;

  return tmp;
}

static const int class_feats_wizard[] = {
  FEAT_UNDEFINED
};

/*
 * Rogues follow opposite logic - they can take any feat in place of these,
 * all of these are abilities that are not normally able to be taken as
 * feats. Most classes can ONLY take from these lists for their class
 * feats.
 */
static const int class_feats_rogue[] = {
  FEAT_UNDEFINED
};

static const int class_feats_fighter[] = {
  FEAT_UNDEFINED
};

static const int no_class_feats[] = {
  FEAT_UNDEFINED
};

static const int *class_bonus_feats[NUM_CLASSES] = {
/* WIZARD		*/ class_feats_wizard,
/* CLERIC		*/ no_class_feats,
/* ROGUE		*/ class_feats_rogue,
/* FIGHTER		*/ class_feats_fighter,
/* MONK			*/ no_class_feats,
/* PALADIN		*/ no_class_feats,
/* NPC_EXPERT		*/ no_class_feats,
/* NPC_ADEPT		*/ no_class_feats,
/* NPC_COMMONER		*/ no_class_feats,
/* NPC_ARISTOCRAT	*/ no_class_feats,
/* NPC_WARRIOR		*/ no_class_feats
};
