/*************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "db.h"
#include "utils.h"
#include "feats.h"
#include "config.h"
#include "players.h"
#include "spec_assign.h"
#include "act.informative.h"
#include "act.other.h"
#include "act.social.h"
#include "random.h"
#include "assemblies.h"
#include "house.h"
#include "dg_event.h"
#include "reset.h"
#include "class.h"
#include "comm.h"
#include "dg_scripts.h"
#include "interpreter.h"
#include "htree.h"
#include "genolc.h"
#include "shop.h"
#include "handler.h"
#include "mail.h"
#include "clan.h"
#include "boards.h"
#include "objsave.h"
#include "constants.h"
#include "genmob.h"
#include "spells.h"
#include "races.h"
#include "imc.h"
#include "spell_parser.h"
#include "genobj.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

struct config_data config_info; /* Game configuration list.    */

struct room_data *world = NULL;	/* array of rooms		 */
room_rnum top_of_world = 0;	/* ref to top element of world	 */
struct htree_node *room_htree = NULL;	/* hash tree for fast room lookup */

struct char_data *character_list = NULL; /* global linked list of chars	 */
struct char_data *affect_list = NULL; /* global linked list of chars with affects */
struct char_data *affectv_list = NULL; /* global linked list of chars with round-based affects */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
mob_rnum top_of_mobt = 0;	/* top of mobile index table	 */
struct htree_node *mob_htree = NULL;	/* hash tree for fast mob lookup */

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
obj_rnum top_of_objt = 0;	/* top of object index table	 */
struct htree_node *obj_htree = NULL;	/* hash tree for fast obj lookup */

struct zone_data *zone_table;	/* zone table			 */
zone_rnum top_of_zone_table = 0;/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct index_data **trig_index; /* index table for triggers      */
struct trig_data *trigger_list = NULL;  /* all attached triggers */
int top_of_trigt = 0;           /* top of trigger index table    */
long max_mob_id = MOB_ID_BASE;  /* for unique mob id's       */
long max_obj_id = OBJ_ID_BASE;  /* for unique obj id's       */
int dg_owner_purged;            /* For control of scripts */

int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
int dballtime = 0;              /* used by dragonball load system*/
int SHENRON = FALSE;            /* Shenron has been summoned     */
int DRAGONR = 0;                /* Room Shenron has been summoned to */
int DRAGONZ = 0;                /* Zone Shenron has been summoned to */
int WISH[2] = {0, 0};           /* Keeps track of wishes granted */
int DRAGONC = 0;                /* Keeps count of Shenron's remaining time */
struct char_data *EDRAGON = NULL;      /* This is Shenron when he is loaded */
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */
int xap_objs = 0;               /* Xap objs                      */
int converting = FALSE;

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals  */
char *imotd = NULL;		/* message of the day - immorts  */
char *GREETINGS = NULL;		/* opening credits screen	 */
char *GREETANSI = NULL;		/* ansi opening credits screen	 */
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *ihelp = NULL;		/* help screen (immortals)	 */

struct help_index_element *help_table = NULL;	/* the help table	 */
int top_of_helpt = 0;

struct social_messg *soc_mess_list = NULL;      /* list of socials */
int top_of_socialt = -1;                        /* number of socials */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

extern struct board_info *boards; /* our boards */
extern int imc_is_enabled;

/* local functions */
static void mob_stats(struct char_data *mob);
static void dragon_level(struct char_data *ch);
static int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
static int check_object_spell_number(struct obj_data *obj, int val);
static int check_object_level(struct obj_data *obj, int val);
static void setup_dir(FILE *fl, int room, int dir);
static void discrete_load(FILE *fl, int mode, char *filename);
static int check_object(struct obj_data *);
static void parse_room(FILE *fl, int virtual_nr);
static void parse_mobile(FILE *mob_f, int nr);
static char *parse_object(FILE *obj_f, int nr);
static void load_zones(FILE *fl, char *zonename);
static int file_to_string(const char *name, char *buf);
static int file_to_string_alloc(const char *name, char **buf);
static int count_alias_records(FILE *fl);
static int count_hash_records(FILE *fl);
static bitvector_t asciiflag_conv_aff(char *flag);
static int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr);
static void interpret_espec(const char *keyword, const char *value, struct char_data *ch, int nr);
static void parse_espec(char *buf, struct char_data *ch, int nr);
static int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr);
static void get_one_line(FILE *fl, char *buf);
static void check_start_rooms(void);
static void renum_world(void);
static void renum_zone_table(void);
static void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
static void reset_time(void);
static int suntzu_armor_convert(struct obj_data *obj);
static int suntzu_weapon_convert(int wp_type);
static void free_obj_unique_hash();
static void mob_autobalance(struct char_data *ch);

/* external functions */

void paginate_string(char *str, struct descriptor_data *d);

void free_alias(struct alias_data *a);
void mag_assign_spells(void);
void create_command_list(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
int hsort(const void *a, const void *b);
void prune_crlf(char *txt);
void build_player_index(void);
void clean_pfiles(void);
void boot_the_guilds(FILE *gm_f, char *filename, int rec_count);
void destroy_guilds(void);
void assign_the_guilds(void);
void memorize_add(struct char_data * ch, int spellnum, int timer);
void assign_feats(void);
void free_feats(void);
void sort_feats(void);
void free_assemblies(void);

/* external vars */
extern int no_specials;
extern int scheck;

extern long top_idnum;

static void dragon_level(struct char_data *ch)
{
 struct descriptor_data *d;
 int level = 0, count = 0;
  
 for (d = descriptor_list; d; d = d->next) {
  if (IS_PLAYING(d) && GET_ADMLEVEL(d->character) < 1) {
   level += GET_LEVEL(d->character);
   count += 1;
  }
 }

 if (level > 0 && count > 0) {
  level = level / count;
 } else {
  level = rand_number(60, 110);
 }

 if (level < 50) {
  level = rand_number(40, 60);
 }

 ch->race_level = 0;
 ch->race_level = level + rand_number(5, 20);
}

static void mob_stats(struct char_data *mob)
{
  int start = GET_LEVEL(mob) * 0.5, finish = GET_LEVEL(mob);
  
  if (finish < 20)
   finish = 20;

  if (!IS_HUMANOID(mob)) {
   mob->real_abils.str = rand_number(start, finish);
   mob->real_abils.intel = rand_number(start, finish) - 30;
   mob->real_abils.wis = rand_number(start, finish) - 30;
   mob->real_abils.dex = rand_number(start + 5, finish);
   mob->real_abils.con = rand_number(start + 5, finish);
   mob->real_abils.cha = rand_number(start, finish);
  } else {
   if (IS_SAIYAN(mob)) {
    mob->real_abils.str = rand_number(start + 10, finish);
    mob->real_abils.intel = rand_number(start, finish - 10);
    mob->real_abils.wis = rand_number(start, finish - 5);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start + 5, finish);
    mob->real_abils.cha = rand_number(start + 5, finish);
   } else if (IS_KONATSU(mob)) {
    mob->real_abils.str = rand_number(start, finish - 10);
    mob->real_abils.intel = rand_number(start, finish);
    mob->real_abils.wis = rand_number(start, finish);
    mob->real_abils.dex = rand_number(start + 10, finish);
    mob->real_abils.con = rand_number(start, finish);
    mob->real_abils.cha = rand_number(start, finish);
   } else if (IS_ANDROID(mob)) {
    mob->real_abils.str = rand_number(start, finish);
    mob->real_abils.intel = rand_number(start, finish);
    mob->real_abils.wis = rand_number(start, finish - 10);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start, finish);
    mob->real_abils.cha = rand_number(start, finish);
   } else if (IS_MAJIN(mob)) {
    mob->real_abils.str = rand_number(start, finish);
    mob->real_abils.intel = rand_number(start, finish - 10);
    mob->real_abils.wis = rand_number(start, finish - 5);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start + 15, finish);
    mob->real_abils.cha = rand_number(start, finish);
   } else if (IS_TRUFFLE(mob)) {
    mob->real_abils.str = rand_number(start, finish - 10);
    mob->real_abils.intel = rand_number(start + 15, finish);
    mob->real_abils.wis = rand_number(start, finish);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start, finish);
    mob->real_abils.cha = rand_number(start, finish);
   } else if (IS_ICER(mob)) {
    mob->real_abils.str = rand_number(start + 5, finish);
    mob->real_abils.intel = rand_number(start, finish);
    mob->real_abils.wis = rand_number(start, finish);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start, finish);
    mob->real_abils.cha = rand_number(start + 10, finish);
   } else {
    mob->real_abils.str = rand_number(start, finish);
    mob->real_abils.intel = rand_number(start, finish);
    mob->real_abils.wis = rand_number(start, finish);
    mob->real_abils.dex = rand_number(start, finish);
    mob->real_abils.con = rand_number(start, finish);
    mob->real_abils.cha = rand_number(start, finish);
   }
  }

  if (mob->real_abils.str > 100)
   mob->real_abils.str = 100;
  else if (mob->real_abils.str < 5)
   mob->real_abils.str = rand_number(5, 8);

  if (mob->real_abils.intel > 100)
   mob->real_abils.intel = 100;
  else if (mob->real_abils.intel < 5)
   mob->real_abils.intel = rand_number(5, 8);

  if (mob->real_abils.wis > 100)
   mob->real_abils.wis = 100;
  else if (mob->real_abils.wis < 5)
   mob->real_abils.wis = rand_number(5, 8);

  if (mob->real_abils.con > 100)
   mob->real_abils.con = 100;
  else if (mob->real_abils.con < 5)
   mob->real_abils.con = rand_number(5, 8);

  if (mob->real_abils.cha > 100)
   mob->real_abils.cha = 100;
  else if (mob->real_abils.cha < 5)
   mob->real_abils.cha = rand_number(5, 8);

  if (mob->real_abils.dex > 100)
   mob->real_abils.dex = 100;
  else if (mob->real_abils.dex < 5)
   mob->real_abils.dex = rand_number(5, 8);
}

/* Convert CWG-SunTzu armor objects to new armor types */

static int suntzu_armor_convert(struct obj_data *obj)
{
  int i;
  int conv = 0;
  int conv_table[][3] = {
    { 100, 0, 0 },
    { 8, 0, 5 },
    { 6, 0, 10 },
    { 5, 1, 15 },
    { 4, 2, 20 },
    { 2, 5, 30 },
    { 0, 7, 40 },
    { 0, 7, 40 },
    { 1, 6, 35 },
  };
  int shield_table[][2] = {
    { 0, 0 },
    { 1, 5 },
    { 2, 15 },
    { 3, 30 },
    { 4, 40 },
    { 5, 50 },
    { 6, 60 },
    { 7, 70 },
    { 8, 80 },
  };

  i = GET_OBJ_VAL(obj, 0);
  if (i && i < 10) {
    GET_OBJ_VAL(obj, 0) = 10 * i;
    conv = 1;
  } else
    i /= 10;

  i = MAX(0, MIN(8, i));

  if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
    if (GET_OBJ_VAL(obj, 6))
      return conv;
    GET_OBJ_VAL(obj, 1) = ARMOR_TYPE_SHIELD;
    GET_OBJ_VAL(obj, 2) = 100;
    GET_OBJ_VAL(obj, 3) = shield_table[i][0];
    GET_OBJ_VAL(obj, 6) = shield_table[i][1];
    conv = 1;
  } else if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
    if (GET_OBJ_VAL(obj, 6))
      return conv;
    GET_OBJ_VAL(obj, 2) = conv_table[i][0];
    GET_OBJ_VAL(obj, 3) = conv_table[i][1];
    GET_OBJ_VAL(obj, 6) = conv_table[i][2];
    conv = 1;
  } else if (GET_OBJ_VAL(obj, 2) || GET_OBJ_VAL(obj, 3)) {
    return conv;
  } else {
    GET_OBJ_VAL(obj, 2) = 100;
    GET_OBJ_VAL(obj, 3) = 0;
    GET_OBJ_VAL(obj, 6) = 0;
    conv = 1;
  }
  log("Converted armor #%d [%s] armor=%d i=%d maxdex=%d acheck=%d sfail=%d",
      obj_index[obj - obj_proto].vnum, GET_OBJ_SHORT(obj), GET_OBJ_VAL(obj, 0),
      i, GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 6));
  return conv;
}

/* Convert CWG-SunTzu weapon objects to new weapon types */

static int suntzu_weapon_convert(int wp_type)
{
  int new_type;

  switch (wp_type) {
    case 170:
      new_type = WEAPON_TYPE_DAGGER;
      break;
    case 171:
      new_type = WEAPON_TYPE_SHORTSWORD;
      break;
    case 172:
      new_type =  WEAPON_TYPE_LONGSWORD;
      break;
    case 173:
      new_type = WEAPON_TYPE_GREATSWORD;
      break;
    case 174:
      new_type = WEAPON_TYPE_MACE;
      break;
    case 175:
      new_type = WEAPON_TYPE_AXE;
      break;
    case 176:
      new_type = WEAPON_TYPE_WHIP;
      break;
    case 177:
      new_type = WEAPON_TYPE_SPEAR;
      break;
    case 178:
      new_type = WEAPON_TYPE_POLEARM;
      break;
    case 179:
      new_type = WEAPON_TYPE_UNARMED;
      break;
    case 180:
      new_type = WEAPON_TYPE_FLAIL;
      break;
    case 181:
      new_type = WEAPON_TYPE_STAFF;
      break;
    case 182:
      new_type = WEAPON_TYPE_HAMMER;
      break;
    default:
      new_type = WEAPON_TYPE_UNDEFINED;
      break;
  }
  log("Converted weapon from [%d] to [%d].", wp_type, new_type);
  return new_type;
}


/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

/* Wipe out all the loaded text files, for shutting down. */
void free_text_files(void)
{
  char **textfiles[] = {
	&wizlist, &immlist, &news, &credits, &motd, &imotd, &help, &info,
	&policies, &handbook, &background, &GREETINGS, &GREETANSI, &ihelp, NULL
  };
  int rf;

  for (rf = 0; textfiles[rf]; rf++)
    if (*textfiles[rf]) {
      free(*textfiles[rf]);
      *textfiles[rf] = NULL;
    }
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!strcasecmp(arg, "all") || *arg == '*') {
    if (load_levels() < 0)                                                                         
      send_to_char(ch, "Cannot read level configurations\r\n");                                   
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
	if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0) 
      send_to_char(ch, "Cannot read wizlist\r\n");
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0) 
      send_to_char(ch, "Cannot read immlist\r\n");
    if (file_to_string_alloc(NEWS_FILE, &news) < 0) 
      send_to_char(ch, "Cannot read news\r\n");
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0) 
      send_to_char(ch, "Cannot read credits\r\n");
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0) 
      send_to_char(ch, "Cannot read motd\r\n");
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0) 
      send_to_char(ch, "Cannot read imotd\r\n");
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0) 
      send_to_char(ch, "Cannot read help front page\r\n");
    if (file_to_string_alloc(INFO_FILE, &info) < 0) 
      send_to_char(ch, "Cannot read info file\r\n");
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0) 
      send_to_char(ch, "Cannot read policies\r\n");
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0) 
      send_to_char(ch, "Cannot read handbook\r\n");
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0) 
      send_to_char(ch, "Cannot read background\r\n");
    if (help_table)
      free_help_table();
    index_boot(DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "levels")) {
    if (load_levels() < 0)                                                                         
      send_to_char(ch, "Cannot read level configurations\r\n");                                   
  } else if (!strcasecmp(arg, "wizlist")) {
    if (file_to_string_alloc(WIZLIST_FILE, &wizlist) < 0) 
      send_to_char(ch, "Cannot read wizlist\r\n");
  } else if (!strcasecmp(arg, "immlist")) {
    if (file_to_string_alloc(IMMLIST_FILE, &immlist) < 0) 
      send_to_char(ch, "Cannot read immlist\r\n");
  } else if (!strcasecmp(arg, "news")) {
    if (file_to_string_alloc(NEWS_FILE, &news) < 0) 
      send_to_char(ch, "Cannot read news\r\n");
  } else if (!strcasecmp(arg, "credits")) {
    if (file_to_string_alloc(CREDITS_FILE, &credits) < 0) 
      send_to_char(ch, "Cannot read credits\r\n");
  } else if (!strcasecmp(arg, "motd")) {
    if (file_to_string_alloc(MOTD_FILE, &motd) < 0) 
      send_to_char(ch, "Cannot read motd\r\n");
  } else if (!strcasecmp(arg, "imotd")) {
    if (file_to_string_alloc(IMOTD_FILE, &imotd) < 0) 
      send_to_char(ch, "Cannot read imotd\r\n");
  } else if (!strcasecmp(arg, "help")) {
    if (file_to_string_alloc(HELP_PAGE_FILE, &help) < 0) 
      send_to_char(ch, "Cannot read help front page\r\n");
  } else if (!strcasecmp(arg, "info")) {
    if (file_to_string_alloc(INFO_FILE, &info) < 0) 
      send_to_char(ch, "Cannot read info\r\n");
  } else if (!strcasecmp(arg, "policy")) {
    if (file_to_string_alloc(POLICIES_FILE, &policies) < 0) 
      send_to_char(ch, "Cannot read policy\r\n");
  } else if (!strcasecmp(arg, "handbook")) {
    if (file_to_string_alloc(HANDBOOK_FILE, &handbook) < 0) 
      send_to_char(ch, "Cannot read handbook\r\n");
  } else if (!strcasecmp(arg, "background")) {
    if (file_to_string_alloc(BACKGROUND_FILE, &background) < 0) 
      send_to_char(ch, "Cannot read background\r\n");
  } else if (!strcasecmp(arg, "greetings")) {
    if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
      prune_crlf(GREETINGS);
    else
      send_to_char(ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "greetansi")) {
    if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
      prune_crlf(GREETANSI);
    else
      send_to_char(ch, "Cannot read greetings.\r\n");
  } else if (!strcasecmp(arg, "xhelp")) {
    if (help_table)
      free_help_table();
    index_boot(DB_BOOT_HLP);
  } else if (!strcasecmp(arg, "ihelp")) {
      if (file_to_string_alloc(IHELP_PAGE_FILE, &ihelp) < 0)
        send_to_char(ch, "Cannot read help front page\r\n");
  } else {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", CONFIG_OK);
}


void boot_world(void)
{
  log("Loading level tables.");
  load_levels();

  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading triggers and generating index.");
  index_boot(DB_BOOT_TRG);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  log("Renumbering zone table.");
  renum_zone_table();

  log("Loading disabled commands list...");
  load_disabled();

  if(converting) {
    log("Saving converted worldfiles to disk.");
      save_all();
  }

  if (!no_specials) {
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);

  log("Loading guild masters.");
  index_boot(DB_BOOT_GLD);
  }
  if (SELFISHMETER >= 10) { 
   log("Loading Shadow Dragons.");
   load_shadow_dragons();
  }

}


void free_extra_descriptions(struct extra_descr_data *edesc)
{
  struct extra_descr_data *enext;

  for (; edesc; edesc = enext) {
    enext = edesc->next;

    free(edesc->keyword);
    free(edesc->description);
    free(edesc);
  }
}


/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
  ssize_t cnt, itr;
  struct char_data *chtmp;
  struct obj_data *objtmp;

  /* Active Mobiles & Players */
  while (character_list) {
    chtmp = character_list;
    character_list = character_list->next;
    if (chtmp->master)
      stop_follower(chtmp);
    free_char(chtmp);
  }

  /* Active Objects */
  while (object_list) {
    objtmp = object_list;
    object_list = object_list->next;
    free_obj(objtmp);
  }

  /* Rooms */
  for (cnt = 0; cnt <= top_of_world; cnt++) {
    if (world[cnt].name)
      free(world[cnt].name);
    if (world[cnt].description)
      free(world[cnt].description);
    free_extra_descriptions(world[cnt].ex_description);

    /* free any assigned scripts */
    if (SCRIPT(&world[cnt]))
      extract_script(&world[cnt], WLD_TRIGGER);
    /* free script proto list */
    free_proto_script(&world[cnt], WLD_TRIGGER);
    
    for (itr = 0; itr < NUM_OF_DIRS; itr++) {
      if (!world[cnt].dir_option[itr])
        continue;

      if (world[cnt].dir_option[itr]->general_description)
        free(world[cnt].dir_option[itr]->general_description);
      if (world[cnt].dir_option[itr]->keyword)
        free(world[cnt].dir_option[itr]->keyword);
      free(world[cnt].dir_option[itr]);
    }
  }
  free(world);
  top_of_world = 0;
  htree_free(room_htree);

  /* Objects */
  for (cnt = 0; cnt <= top_of_objt; cnt++) {
    if (obj_proto[cnt].name)
      free(obj_proto[cnt].name);
    if (obj_proto[cnt].description)
      free(obj_proto[cnt].description);
    if (obj_proto[cnt].short_description)
      free(obj_proto[cnt].short_description);
    if (obj_proto[cnt].action_description)
      free(obj_proto[cnt].action_description);
    if (obj_proto[cnt].ex_description)
    free_extra_descriptions(obj_proto[cnt].ex_description);

    /* free script proto list */
    free_proto_script(&obj_proto[cnt], OBJ_TRIGGER);
  }
  free(obj_proto);
  free(obj_index);
  htree_free(obj_htree);

  /* Mobiles */
  for (cnt = 0; cnt <= top_of_mobt; cnt++) {
    if (mob_proto[cnt].name)
      free(mob_proto[cnt].name);
    if (mob_proto[cnt].title)
      free(mob_proto[cnt].title);
    if (mob_proto[cnt].short_descr)
      free(mob_proto[cnt].short_descr);
    if (mob_proto[cnt].long_descr)
      free(mob_proto[cnt].long_descr);
    if (mob_proto[cnt].description)
      free(mob_proto[cnt].description);

    /* free script proto list */
    free_proto_script(&mob_proto[cnt], MOB_TRIGGER);

    while (mob_proto[cnt].affected)
      affect_remove(&mob_proto[cnt], mob_proto[cnt].affected);
  }
  free(mob_proto);
  free(mob_index);
  htree_free(mob_htree);

  /* Shops */
  destroy_shops();

  /* Guilds */
  destroy_guilds();

  /* Zones */
  /* zone table reset queue */
  if (reset_q.head) {
    struct reset_q_element *ftemp=reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

#define THIS_CMD zone_table[cnt].cmd[itr]

  for (cnt = 0; cnt <= top_of_zone_table; cnt++) {
    if (zone_table[cnt].name)
      free(zone_table[cnt].name);
    if (zone_table[cnt].builders)
      free(zone_table[cnt].builders);
    if (zone_table[cnt].cmd) {
       /* first see if any vars were defined in this zone */
       for (itr = 0;THIS_CMD.command != 'S';itr++)
         if (THIS_CMD.command == 'V') {
           if (THIS_CMD.sarg1)
             free(THIS_CMD.sarg1);
           if (THIS_CMD.sarg2)
             free(THIS_CMD.sarg2);
         }
       /* then free the command list */
      free(zone_table[cnt].cmd);
    }
  }
  free(zone_table);

#undef THIS_CMD

  /* zone table reset queue */
  if (reset_q.head) {
    struct reset_q_element *ftemp=reset_q.head, *temp;
    while (ftemp) {
      temp = ftemp->next;
      free(ftemp);
      ftemp = temp;
    }
  }

  /* Triggers */
  for (cnt=0; cnt < top_of_trigt; cnt++) {
    if (trig_index[cnt]->proto) {
      /* make sure to nuke the command list (memory leak) */
      /* free_trigger() doesn't free the command list */
      if (trig_index[cnt]->proto->cmdlist) {
        struct cmdlist_element *i, *j;
        i = trig_index[cnt]->proto->cmdlist;
        while (i) {
          j = i->next;
          if (i->cmd)
            free(i->cmd);
          free(i);
          i = j;
        }
      }
      free_trigger(trig_index[cnt]->proto);
    }
    free(trig_index[cnt]);
  }
  free(trig_index);

  /* Events */
  event_free_all();

  /* context sensitive help system */
  free_context_help();

  free_feats();

  free_obj_unique_hash();

  htree_shutdown();

  log("Freeing Assemblies.");
  free_assemblies();
}


/* You can define this to anything you want; 1 would work but it would
   be very inefficient. I would recommend that it actually be close to
   your total number of in-game objects if not double or triple it just
   to minimize collisions. The only O(n) [n=NUM_OBJ_UNIQUE_POOLS]
   operation is initialization of the hash table, all other operations
   that have to traverse are O(n) [n=num elements in pool], so more
   pools are better.
     - Elie Rosenblum Dec. 12 2003 */
#define NUM_OBJ_UNIQUE_POOLS 5000

struct obj_unique_hash_elem **obj_unique_hash_pools = NULL;

void init_obj_unique_hash()
{
  int i;
  CREATE(obj_unique_hash_pools, struct obj_unique_hash_elem *, NUM_OBJ_UNIQUE_POOLS);
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    obj_unique_hash_pools[i] = NULL;
  }
}

/* body of the booting system */
void boot_db(void)
{
  zone_rnum i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Reading news, credits, help, ihelp, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(IHELP_PAGE_FILE, &ihelp);
  if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
    prune_crlf(GREETINGS);
  if (file_to_string_alloc(GREETANSI_FILE, &GREETANSI) == 0)
    prune_crlf(GREETANSI);

  log("Loading spell definitions.");
  mag_assign_spells();

  log("Loading feats.");
  assign_feats();

  boot_world();

  htree_test();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

  log("Setting up context sensitive help system for OLC");
  boot_context_help();

  log("Generating player index.");
  build_player_index();

  if (ERAPLAYERS <= 0)
   ERAPLAYERS = top_of_p_table + 1;

  insure_directory(LIB_PLROBJS "CRASH", 0);

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }

  if (auto_pwipe) {
    log("Cleaning out inactive players.");
    clean_pfiles();
  }

  log("Loading social messages.");
  boot_social_messages();

  log("Loading Clans.");
  clanBoot();

  log("Building command list.");
  create_command_list(); /* aedit patch -- M. Scott */

  log("Assigning function pointers:");

  if (!no_specials) {
    log("   Mobiles.");
    assign_mobiles();
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
    log("   Guildmasters.");
    assign_the_guilds();
  }

  log("Init Object Unique Hash");
  init_obj_unique_hash();

  log("Booting assembled objects.");
  assemblyBootAssemblies();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();
  sort_feats();

  log("Booting boards system.");
  init_boards();
  
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }

  for (i = 0; i <= top_of_zone_table; i++) {
    log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
	zone_table[i].name, zone_table[i].bot, zone_table[i].top);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  log("Boot db -- DONE.");
}


/* save the auction file */
void auc_save()
{
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "w")) == NULL)
    log("SYSERR: Can't write to '%s' auction file.", AUCTION_FILE);
  else {
    struct obj_data *obj, *next_obj;

    for (obj = world[real_room(80)].contents; obj; obj = next_obj) {
     next_obj = obj->next_content;
     if (obj) {
      fprintf(fl, "%"I64T" %s %d %d %d %d %ld\n", obj->unique_id, GET_AUCTERN(obj), GET_AUCTER(obj), GET_CURBID(obj), GET_STARTBID(obj), GET_BID(obj), GET_AUCTIME(obj));
     }
    }
    fprintf(fl, "~END~\n");
    fclose(fl);
  }
}

/* load from auction file */
void auc_load(struct obj_data *obj)
{
  char line[500], filler[50];
  int64_t oID;
  time_t timer;
  int aID, bID, cost, startc;
  FILE *fl;

  if ((fl = fopen(AUCTION_FILE, "r")) == NULL)
    log("SYSERR: Can't read from '%s' auction file.", AUCTION_FILE);
  else {
   while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%"I64T" %s %d %d %d %d %ld\n", &oID, filler, &aID, &bID, &startc, &cost, &timer);
    if (obj->unique_id == oID) {
     GET_AUCTERN(obj) = strdup(filler);
     GET_AUCTER(obj) = aID;
     GET_CURBID(obj) = bID;
     GET_STARTBID(obj) = startc;
     GET_BID(obj) = cost;
     GET_AUCTIME(obj) = timer;
    }
   }
    fclose(fl);
  }
}

/* reset the time in the game from file */
static void reset_time(void)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf(bgtime, "%ld\n", &beginning_of_time);
    fscanf(bgtime, "%ld\n", &NEWSUPDATE);
    fscanf(bgtime, "%ld\n", &BOARDNEWMORT);
    fscanf(bgtime, "%ld\n", &BOARDNEWDUO);
    fscanf(bgtime, "%ld\n", &BOARDNEWCOD);
    fscanf(bgtime, "%ld\n", &BOARDNEWBUI);
    fscanf(bgtime, "%ld\n", &BOARDNEWIMM);
    fscanf(bgtime, "%ld\n", &INTERESTTIME);
    fscanf(bgtime, "%ld\n", &LASTINTEREST);
    fscanf(bgtime, "%d\n", &HIGHPCOUNT);
    fscanf(bgtime, "%ld\n", &PCOUNTDATE);
    fscanf(bgtime, "%d\n", &WISHTIME);
    fscanf(bgtime, "%d\n", &PCOUNT);
    fscanf(bgtime, "%ld\n", &LASTPAYOUT);
    fscanf(bgtime, "%d\n", &LASTPAYTYPE);
    fscanf(bgtime, "%d\n", &LASTNEWS);
    fscanf(bgtime, "%d\n", &dballtime);
    fscanf(bgtime, "%d\n", &SELFISHMETER);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON1);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON2);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON3);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON4);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON5);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON6);
    fscanf(bgtime, "%d\n", &SHADOW_DRAGON7);
    fscanf(bgtime, "%d\n", &ERAPLAYERS);
    fclose(bgtime);
  }

  if (dballtime == 0)
   dballtime = 604800;

  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when)
{
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
    fprintf(bgtime, "%ld\n", NEWSUPDATE);
    fprintf(bgtime, "%ld\n", BOARDNEWMORT);
    fprintf(bgtime, "%ld\n", BOARDNEWDUO);
    fprintf(bgtime, "%ld\n", BOARDNEWCOD);
    fprintf(bgtime, "%ld\n", BOARDNEWBUI);
    fprintf(bgtime, "%ld\n", BOARDNEWIMM);
    fprintf(bgtime, "%ld\n", INTERESTTIME);
    fprintf(bgtime, "%ld\n", LASTINTEREST);
    fprintf(bgtime, "%d\n", HIGHPCOUNT);
    fprintf(bgtime, "%ld\n", PCOUNTDATE);
    fprintf(bgtime, "%d\n", WISHTIME);
    fprintf(bgtime, "%d\n", PCOUNT);
    fprintf(bgtime, "%ld\n", LASTPAYOUT);
    fprintf(bgtime, "%d\n", LASTPAYTYPE);
    fprintf(bgtime, "%d\n", LASTNEWS);
    fprintf(bgtime, "%d\n", dballtime);
    fprintf(bgtime, "%d\n", SELFISHMETER);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON1);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON2);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON3);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON4);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON5);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON6);
    fprintf(bgtime, "%d\n", SHADOW_DRAGON7);
    fprintf(bgtime, "%d\n", ERAPLAYERS);
    fclose(bgtime);
  }
}


/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
static int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
static int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}



void index_boot(int mode)
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2];
  char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_TRG:
    prefix = TRG_PREFIX;
    break;
  case DB_BOOT_GLD:
    prefix = GLD_PREFIX;
    break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: File '%s' listed in '%s%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(db_index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON)
	rec_count++;
      else if (mode == DB_BOOT_HLP)
	rec_count += count_alias_records(db_file);
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP || mode == DB_BOOT_GLD)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  }

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_TRG:
    CREATE(trig_index, struct index_data *, rec_count);
    break;
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    size[0] = sizeof(struct room_data) * rec_count;
    log("   %d rooms, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct char_data) * rec_count;
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    size[0] = sizeof(struct index_data) * rec_count;
    size[1] = sizeof(struct obj_data) * rec_count;
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    size[0] = sizeof(struct zone_data) * rec_count;
    log("   %d zones, %d bytes.", rec_count, size[0]);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    size[0] = sizeof(struct help_index_element) * rec_count;
    log("   %d entries, %d bytes.", rec_count, size[0]);
    break;
  }

  rewind(db_index);
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
    case DB_BOOT_TRG:
      discrete_load(db_file, mode, buf2);
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      load_help(db_file, buf2);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_GLD:
      boot_the_guilds(db_file, buf2, rec_count);
      break;
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  /* Sort the help index. */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
}


static void discrete_load(FILE *fl, int mode, char *filename)
{
  int nr = -1, last;
  char line[READ_SIZE];

  const char *modes[] = {"world", "mob", "obj", "ZON", "SHP", "HLP", "trg"};
  /* modes positions correspond to DB_BOOT_xxx in db.h */

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
        case DB_BOOT_TRG:
          parse_trigger(fl, nr);
          break;
	case DB_BOOT_OBJ:
	  strlcpy(line, parse_object(fl, nr), sizeof(line));
	  break;
	}
    } else {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      log("SYSERR: ... offending line: '%s'", line);
      exit(1);
    }
  }
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);  
  } while (isspace(c));
  return c;
}

bitvector_t asciiflag_conv(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}

static bitvector_t asciiflag_conv_aff(char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (1 + (*p - 'a'));
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!(isdigit(*p) || (*p == '-')))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}
/* load the rooms */
static void parse_room(FILE *fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i, retval;
  char line[READ_SIZE], flags[128], flags2[128], flags3[128];
  char flags4[128], buf2[MAX_STRING_LENGTH], buf[128];
  struct extra_descr_data *new_descr;
  char letter;

  /* This really had better fit or there are other problems. */
  snprintf(buf2, sizeof(buf2), "room #%d", virtual_nr);

  if (virtual_nr < zone_table[zone].bot) {
    log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (! room_htree)
    room_htree = htree_init();
  htree_add(room_htree, virtual_nr, room_nr);

  if (!get_line(fl, line)) {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }
 
  if (((retval = sscanf(line, " %d %s %s %s %s %d ", t, flags, flags2, flags3, flags4, t + 2)) == 3) && (bitwarning == TRUE)) {
    log("WARNING: Conventional worldfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if ((retval == 3) && (bitwarning == FALSE)) {
  /* 
   * Looks like the implementor is ready, so let's load the worldfiles. We 
   * load the extra three flags as 0, since they won't be anything anyway. We 
   * will save the entire world later on, when every room, mobile, and object 
   * is converted.
   */

    log("Converting room #%d to 128bits..", virtual_nr);
    world[room_nr].room_flags[0] = asciiflag_conv(flags);
    world[room_nr].room_flags[1] = 0;
    world[room_nr].room_flags[2] = 0;
    world[room_nr].room_flags[3] = 0;

    sprintf(flags, "room #%d", virtual_nr);	/* sprintf: OK (until 399-bit integers) */
    
    /* No need to scan the other three sections; they're 0 anyway */
    check_bitvector_names(world[room_nr].room_flags[0], room_bits_count, flags, "room"); 
	
    if(bitsavetodisk) { /* Maybe the implementor just wants to look at the 128bit files */
      add_to_save_list(zone_table[real_zone_by_thing(virtual_nr)].number, 3);
      converting = TRUE;
    }

    log("   done.");
  } else if (retval == 6) {
  int taeller;
  world[room_nr].room_flags[0] = asciiflag_conv(flags);
  world[room_nr].room_flags[1] = asciiflag_conv(flags2);
  world[room_nr].room_flags[2] = asciiflag_conv(flags3);
  world[room_nr].room_flags[3] = asciiflag_conv(flags4);

  world[room_nr].sector_type = t[2];
  sprintf(flags, "object #%d", virtual_nr);	/* sprintf: OK (until 399-bit integers) */
  for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
    check_bitvector_names(world[room_nr].room_flags[taeller], room_bits_count, flags, "room");
  } else {
  log("SYSERR: Format error in roomflags/sector type of room #%d", virtual_nr);
  exit(1);
  }

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */
  world[room_nr].timed = -1;
  world[room_nr].dmg = 0;
  world[room_nr].gravity = 0;

  if (ROOM_FLAGGED(room_nr, ROOM_VEGETA) || ROOM_FLAGGED(room_nr, ROOM_GRAVITYX10)) {
     world[room_nr].gravity = 10;
  }
  if (world[room_nr].number >= 19800 && world[room_nr].number <= 19899) {
     world[room_nr].gravity = 1000;
  }
  if (world[room_nr].number >= 64000 && world[room_nr].number <= 64006) {
     world[room_nr].gravity = 100;
  }
  if (world[room_nr].number >= 64007 && world[room_nr].number <= 64016) {
     world[room_nr].gravity = 300;
  }
  if (world[room_nr].number >= 64017 && world[room_nr].number <= 64030) {
     world[room_nr].gravity = 500;
  }
  if (world[room_nr].number >= 64031 && world[room_nr].number <= 64048) {
     world[room_nr].gravity = 1000;
  }
  if (world[room_nr].number >= 64049 && world[room_nr].number <= 64070) {
     world[room_nr].gravity = 5000;
  }
  if (world[room_nr].number >= 64071 && world[room_nr].number <= 64096) {
     world[room_nr].gravity = 10000;
  }
  if (world[room_nr].number == 64097) {
     world[room_nr].gravity = 1000;
  }
  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  snprintf(buf, sizeof(buf), "SYSERR: Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      log("%s", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1));
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      /* fix for crashes in the editor when formatting 
       * - e-descs are assumed to end with a \r\n
       * -- Welcor 09/03 
       */
      { 
      	char *tmp = strchr(new_descr->description, '\0');
      	if (tmp > new_descr->description && *(tmp-1) != '\n') {
      	  CREATE(tmp, char, strlen(new_descr->description)+3);
      	  sprintf(tmp, "%s\r\n", new_descr->description); /* sprintf ok : size checked above*/
      	  free(new_descr->description);
      	  new_descr->description = tmp;
      	}
      }
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      break;
    case 'S':			/* end of room */
      /* DG triggers -- script is defined after the end of the room */
      letter = fread_letter(fl);
      ungetc(letter, fl);
      while (letter=='T') {
        dg_read_trigger(fl, &world[room_nr], WLD_TRIGGER);
        letter = fread_letter(fl);
        ungetc(letter, fl);
      }
      top_of_world = room_nr++;
      return;
    default:
      log("%s", buf);
      exit(1);
    }
  }
}

/* read direction data */
static void setup_dir(FILE *fl, int room, int dir)
{
  int t[11], retval;
  char line[READ_SIZE], buf2[128];

  snprintf(buf2, sizeof(buf2), "room #%d, direction D%d", GET_ROOM_VNUM(room)+1, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  }
  if (((retval = sscanf(line, " %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, t + 10 )) == 3) && (bitwarning == TRUE)) {
    log("SYSERR: Format error, %s", buf2);
    exit(1);
  } else if (bitwarning == FALSE) {

  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else if (t[0] == 3)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_SECRET;
  else if (t[0] == 4)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR|EX_PICKPROOF|EX_SECRET;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = ((t[1] == -1 || t[1] == 65535) ? NOTHING : t[1]);
  world[room].dir_option[dir]->to_room = ((t[2] == -1  || t[2] == 65535) ? NOWHERE : t[2]);

  if (retval == 3) {
    log("Converting world files to include DC add ons.");
    world[room].dir_option[dir]->dclock = 20;
    world[room].dir_option[dir]->dchide = 20;
    world[room].dir_option[dir]->dcskill = 0;
    world[room].dir_option[dir]->dcmove = 0;
    world[room].dir_option[dir]->failsavetype = 0;
    world[room].dir_option[dir]->dcfailsave = 0;
    world[room].dir_option[dir]->failroom = NOWHERE;
    world[room].dir_option[dir]->totalfailroom = NOWHERE;
    if (bitsavetodisk) { 
      add_to_save_list(zone_table[world[room].zone].number, 3);
      converting = TRUE;
    }
  } else if (retval == 5) {
    world[room].dir_option[dir]->dclock = t[3];
    world[room].dir_option[dir]->dchide = t[4];
    world[room].dir_option[dir]->dcskill = 0;
    world[room].dir_option[dir]->dcmove = 0;
    world[room].dir_option[dir]->failsavetype = 0;
    world[room].dir_option[dir]->dcfailsave = 0;
    world[room].dir_option[dir]->failroom = NOWHERE;
    world[room].dir_option[dir]->totalfailroom = NOWHERE;
    if (bitsavetodisk) { 
      add_to_save_list(zone_table[world[room].zone].number, 3);
      converting = TRUE;
    }
  } else if (retval == 7) {
    world[room].dir_option[dir]->dclock = t[3];
    world[room].dir_option[dir]->dchide = t[4];
    world[room].dir_option[dir]->dcskill = t[5];
    world[room].dir_option[dir]->dcmove = t[6];
    world[room].dir_option[dir]->failsavetype = 0;
    world[room].dir_option[dir]->dcfailsave = 0;
    world[room].dir_option[dir]->failroom = NOWHERE;
    world[room].dir_option[dir]->totalfailroom = NOWHERE;
    if (bitsavetodisk) { 
      add_to_save_list(zone_table[world[room].zone].number, 3);
      converting = TRUE;
    }
  } else if (retval == 11) {
    world[room].dir_option[dir]->dclock = t[3];
    world[room].dir_option[dir]->dchide = t[4];
    world[room].dir_option[dir]->dcskill = t[5];
    world[room].dir_option[dir]->dcmove = t[6];
    world[room].dir_option[dir]->failsavetype = t[7];
    world[room].dir_option[dir]->dcfailsave = t[8];
    world[room].dir_option[dir]->failroom = t[9];
    world[room].dir_option[dir]->totalfailroom = t[10];
  }
  }
}


/* make sure the start rooms exist & resolve their vnums to rnums */
static void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(CONFIG_MORTAL_START)) == NOWHERE) {
    log("SYSERR:  Mortal start room does not exist.  Change mortal_start_room in lib/etc/config.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(CONFIG_IMMORTAL_START)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change immort_start_room in /lib/etc/config.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(CONFIG_FROZEN_START)) == NOWHERE) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change frozen_start_room in /lib/etc/config.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
static void renum_world(void)
{
  int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD2 zone_table[zone].cmd[cmd_no]

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */
static void renum_zone_table(void)
{
  int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD2.arg1;
      oldb = ZCMD2.arg2;
      oldc = ZCMD2.arg3;
      switch (ZCMD2.command) {
      case 'M':
	a = ZCMD2.arg1 = real_mobile(ZCMD2.arg1);
	c = ZCMD2.arg3 = real_room(ZCMD2.arg3);
	break;
      case 'O':
	a = ZCMD2.arg1 = real_object(ZCMD2.arg1);
	if (ZCMD2.arg3 != NOWHERE)
	  c = ZCMD2.arg3 = real_room(ZCMD2.arg3);
	break;
      case 'G':
	a = ZCMD2.arg1 = real_object(ZCMD2.arg1);
	break;
      case 'E':
	a = ZCMD2.arg1 = real_object(ZCMD2.arg1);
	break;
      case 'P':
	a = ZCMD2.arg1 = real_object(ZCMD2.arg1);
	c = ZCMD2.arg3 = real_object(ZCMD2.arg3);
	break;
      case 'D':
	a = ZCMD2.arg1 = real_room(ZCMD2.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD2.arg1 = real_room(ZCMD2.arg1);
	b = ZCMD2.arg2 = real_object(ZCMD2.arg2);
        break;
      case 'T': /* a trigger */
        b = ZCMD2.arg2 = real_trigger(ZCMD2.arg2);
        c = ZCMD2.arg3 = real_room(ZCMD2.arg3);
        break;
      case 'V': /* trigger variable assignment */
        b = ZCMD2.arg3 = real_room(ZCMD2.arg3);
        break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
	if (!mini_mud) {
	  snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
			 a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD2.command = '*';
      }
    }
}

static void mob_autobalance(struct char_data *ch)
{
    /* Try to add some baseline defaults based on level choice. */
    GET_HIT(ch) = 0;
    GET_MANA(ch) = 0;
    GET_MOVE(ch) = 0;
    GET_EXP(ch) = 0;
    GET_ARMOR(ch) = 0;
    GET_NDD(ch) = 0;
    GET_SDD(ch) = 0;
    GET_DAMAGE_MOD(ch) = 0;
}

static int parse_simple_mob(FILE *mob_f, struct char_data *ch, int nr)
{
  int j, t[10];
  char line[READ_SIZE];
 
  ch->real_abils.str = 0;
  ch->real_abils.intel = 0;
  ch->real_abils.wis = 0;
  ch->real_abils.dex = 0;
  ch->real_abils.con = 0;
  ch->real_abils.cha = 0;
 
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    return 0;
  }
 
  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	     t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    log("SYSERR: Format error in mob #%d, first line after S flag\n"
 	"...expecting line of form '# # # #d#+# #d#+#'", nr);
    return 0;
  }
 
  GET_HITDICE(ch) = t[0];
  GET_LEVEL_ADJ(ch) = 0;
  GET_CLASS_LEVEL(ch) = 0;
  GET_ARMOR(ch) = 10 * (10 - t[2]);
 
  /* max hit = 0 is a flag that H, M, V is xdy+z */
  GET_MAX_HIT(ch) = 0;
  GET_HIT(ch) = t[3];
  GET_MANA(ch) = t[4];
  GET_MOVE(ch) = t[5];

  ch->mob_specials.damnodice = t[6];
  ch->mob_specials.damsizedice = t[7];
  GET_DAMAGE_MOD(ch) = t[8];
 
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# #', but file ended!", nr);
    return 0;
  }
 
  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# # # #'", nr);
    return 0;
  }
 
  GET_GOLD(ch) = t[0];
  GET_EXP(ch) = 0;
  GET_RACE(ch) = t[2];
  GET_CLASS(ch) = t[3];

  GET_SAVE_BASE(ch, SAVING_FORTITUDE) = 0;
  GET_SAVE_BASE(ch, SAVING_REFLEX) = 0;
  GET_SAVE_BASE(ch, SAVING_WILL) = 0;

  /* GET_CLASS_RANKS(ch, t[3]) = GET_LEVEL(ch); */
 
  if (!IS_HUMAN(ch))
    if (!AFF_FLAGGED(ch, AFF_INFRAVISION))
      SET_BIT_AR(AFF_FLAGS(ch), AFF_INFRAVISION);
 
  SPEAKING(ch) = SKILL_LANG_COMMON;
 
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #', but file ended!", nr);
    return 0;
  }
 
  if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #'", nr);
    return 0;
  }
 
  GET_POS(ch) = t[0];
  GET_DEFAULT_POS(ch) = t[1];
  GET_SEX(ch) = t[2];
 
  SPEAKING(ch) = MIN_LANGUAGES;
  set_height_and_weight_by_race(ch);
 
  for (j = 0; j < 3; j++)
    GET_SAVE_MOD(ch, j) = 0;
 
  if (MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
    mob_autobalance(ch); 
 }

  return 1;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test)	\
	if (value && !matched && !strcasecmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)	\
	if (!value && !matched && !strcasecmp(keyword, test) && (matched = TRUE))

#define RANGE(low, high)	\
	(num_arg = MAX((low), MIN((high), (num_arg))))

static void interpret_espec(const char *keyword, const char *value, struct char_data *ch, int nr)
{
  int num_arg = 0, matched = FALSE;
  int num, num2, num3, num4, num5, num6;
  struct affected_type af;

  /*
   * If there isn't a colon, there is no value.  While Boolean options are
   * possible, we don't actually have any.  Feel free to make some.
  */
  if (value)
    num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    ch->mob_specials.attack_type = num_arg;
  }

  CASE("Size") {
    RANGE(SIZE_UNDEFINED, NUM_SIZES - 1);
    ch->size = num_arg;
  }

  CASE("Str") {
    RANGE(0, 200);
    ch->real_abils.str = num_arg;
  }

  CASE("StrAdd") {
    log("mob #%d trying to set StrAdd, rebalance its strength.",
        GET_MOB_VNUM(ch));
  }

  CASE("Int") {
    RANGE(0, 200);
    ch->real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(0, 200);
    ch->real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(0, 200);
    ch->real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(0, 200);
    ch->real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(0, 200);
    ch->real_abils.cha = num_arg;
  }
 
  CASE("Hit") {
    RANGE(0, 99999);
    GET_HIT(ch) = num_arg;
  }
 
  CASE("MaxHit") {
    RANGE(0, 99999);
    GET_MAX_HIT(ch) = num_arg;
  }
 
  CASE("Mana") {
    RANGE(0, 99999);
    GET_MANA(ch) = num_arg;
  }
 
  CASE("MaxMana") {
    RANGE(0, 99999);
    GET_MAX_MANA(ch) = num_arg;
  }
 
  CASE("Moves") {
    RANGE(0, 99999);
    GET_MOVE(ch) = num_arg;
  }
 
  CASE("MaxMoves") {
    RANGE(0, 99999);
    GET_MAX_MOVE(ch) = num_arg;
  }
 
  CASE("Affect") {
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(value, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num > 0) {
      af.type = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      af.bitvector = num5;
      af.specific = num6;
      affect_to_char(ch, &af);
    }
  }

  CASE("AffectV") {
    num = num2 = num3 = num4 = num5 = num6 = 0;
    sscanf(value, "%d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5, &num6);
    if (num > 0) {
      af.type = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      af.bitvector = num5;
      af.specific = num6;
      affectv_to_char(ch, &af);
    }
  }

  CASE("Feat") {
    sscanf(value, "%d %d", &num, &num2);
    HAS_FEAT(ch, num) = num2;
  }

  CASE("Skill") {
    sscanf(value, "%d %d", &num, &num2);
    SET_SKILL(ch, num, num2);
  }

  CASE("SkillMod") {
    sscanf(value, "%d %d", &num, &num2);
    SET_SKILL_BONUS(ch, num, num2);
  }

  CASE("Class") {
    sscanf(value, "%d %d", &num, &num2);
    GET_CLASS_NONEPIC(ch, num) = num2;
    GET_CLASS_LEVEL(ch) += num2;
  }

  CASE("EpicClass") {
    sscanf(value, "%d %d", &num, &num2);
    GET_CLASS_EPIC(ch, num) = num2;
    GET_CLASS_LEVEL(ch) += num2;
  }

  if (!matched) {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

static void parse_espec(char *buf, struct char_data *ch, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  }
  interpret_espec(buf, ptr, ch, nr);
}

static int parse_enhanced_mob(FILE *mob_f, struct char_data *ch, int nr)
{
  char line[READ_SIZE];

  parse_simple_mob(mob_f, ch, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))	/* end of the enhanced section */
      return 1;
    else if (*line == '#') {	/* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      return 0;
    } else
      parse_espec(line, ch, nr);
  }

  log("SYSERR: Unexpected end of file reached after mob #%d", nr);
  return 0;
}

int parse_mobile_from_file(FILE *mob_f, struct char_data *ch)
{
  int j, t[10], retval;
  char line[READ_SIZE], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128];
  char f7[128], f8[128], buf2[128];
  mob_vnum nr = mob_index[ch->nr].vnum;
 
  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  ch->player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);   /* sprintf: OK (for 'buf2 >= 19') */

  /***** String data *****/
  ch->name = fread_string(mob_f, buf2);
  tmpptr = ch->short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
	!strcasecmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  ch->long_descr = fread_string(mob_f, buf2);
  ch->description = fread_string(mob_f, buf2);

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error after string section of mob #%d\n"
      "...expecting line of form '# # # {S | E}', but file ended!", nr);
    return 0;
  }
 
  if (((retval = sscanf(line, "%s %s %s %s %s %s %s %s %d %c", f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter)) == 10) && (bitwarning == TRUE)) {
/* Let's make the implementor read some, before converting his worldfiles */
    log("WARNING: Conventional mobilefiles detected. Please read 128bit.readme.");
    return 0;
  } else if ((retval == 4) && (bitwarning == FALSE)) {

    log("Converting mobile #%d to 128bits..", nr);
    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = 0;
    MOB_FLAGS(ch)[2] = 0;
    MOB_FLAGS(ch)[3] = 0;
    check_bitvector_names(MOB_FLAGS(ch)[0], action_bits_count, buf2, "mobile");

    AFF_FLAGS(ch)[0] = asciiflag_conv_aff(f2);
    AFF_FLAGS(ch)[1] = 0;
    AFF_FLAGS(ch)[2] = 0;
    AFF_FLAGS(ch)[3] = 0;

    GET_ALIGNMENT(ch) = atoi(f3);
    
	/* Make some basic checks. */
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_POISON);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SLEEP);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_GOOD))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_GOOD);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_NEUTRAL);
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) && MOB_FLAGGED(ch, MOB_AGGR_EVIL))
      REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_AGGR_EVIL);

    check_bitvector_names(AFF_FLAGS(ch)[0], affected_bits_count, buf2, "mobile affect");

    /* 
     * This is necessary, since if we have conventional worldfiles, &letter
     * is loaded into f4 instead of the letter characters. So what we do, is 
     * copy f4 into letter. Disadvantage is that &letter cannot be longer
     * then 128 characters, but this shouldn't occur anyway.
     */
    letter = *f4;

    if(bitsavetodisk) {
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 0);
      converting =TRUE;
    }

    log("   done.");
  } else if (retval == 10) {
    int taeller;

    MOB_FLAGS(ch)[0] = asciiflag_conv(f1);
    MOB_FLAGS(ch)[1] = asciiflag_conv(f2);
    MOB_FLAGS(ch)[2] = asciiflag_conv(f3);
    MOB_FLAGS(ch)[3] = asciiflag_conv(f4);
    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
      check_bitvector_names(MOB_FLAGS(ch)[taeller], action_bits_count, buf2, "mobile");
  
    AFF_FLAGS(ch)[0] = asciiflag_conv(f5);
    AFF_FLAGS(ch)[1] = asciiflag_conv(f6);
    AFF_FLAGS(ch)[2] = asciiflag_conv(f7);
    AFF_FLAGS(ch)[3] = asciiflag_conv(f8);

    GET_ALIGNMENT(ch) = t[2];

    for(taeller=0; taeller < AF_ARRAY_MAX; taeller++) 
      check_bitvector_names(AFF_FLAGS(ch)[taeller], affected_bits_count, buf2, "mobile affect");
  } else {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}'", nr);
    exit(1);
  }

  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  if (MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
    /* Rather bad to load mobiles with this bit already set. */
    log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
    REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
  }

  /* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE.
  if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
    log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", nr); */

  /* Convert mobs to use AUTOBALANCE. Uncomment and reboot to flag all mobs AUTOBALANCE.
   * if (!MOB_FLAGGED(ch, MOB_AUTOBALANCE)) {
   *   SET_BIT_AR(MOB_FLAGS(ch), MOB_AUTOBALANCE);
   * } */

  switch (UPPER(letter)) {
  case 'S':	/* Simple monsters */
    parse_simple_mob(mob_f, ch, nr);
    break;
  case 'E':	/* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, ch, nr);
    mob_stats(ch);
    break;
  /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  /* DG triggers -- script info follows mob S/E section */
  letter = fread_letter(mob_f);
  ungetc(letter, mob_f);
  while (letter=='T') {
    dg_read_trigger(mob_f, ch, MOB_TRIGGER);
    letter = fread_letter(mob_f);
    ungetc(letter, mob_f);
  }

  ch->aff_abils = ch->real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    ch->equipment[j] = NULL;

  /* Uncomment to force all mob files to be rewritten. Good for initial AUTOBALANCE setup.
   * if (bitsavetodisk) {
   *   add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 0);
   *   converting = TRUE;
   * } */

  return 1;
}


static void parse_mobile(FILE *mob_f, int nr)
{
  static int i = 0;

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  if (parse_mobile_from_file(mob_f, mob_proto + i)) {
    if (! mob_htree)
      mob_htree = htree_init();
    htree_add(mob_htree, nr, i);

    top_of_mobt = i++;
  } else { /* We used to exit in the file reading code, but now we do it here */
    exit(1);
  }
}



/* read all objects from obj file; generate index and prototypes */
static char *parse_object(FILE *obj_f, int nr)
{
  static int i = 0;
  static char line[READ_SIZE];
  int t[NUM_OBJ_VAL_POSITIONS + 2], j, retval;
  char *tmpptr, buf2[128];
  char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
  char f5[READ_SIZE], f6[READ_SIZE], f7[READ_SIZE], f8[READ_SIZE];
  char f9[READ_SIZE], f10[READ_SIZE], f11[READ_SIZE], f12[READ_SIZE];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  if (! obj_htree)
    obj_htree = htree_init();
  htree_add(obj_htree, nr, i);

  clear_object(obj_proto + i);
  obj_proto[i].item_number = i;

  sprintf(buf2, "object #%d", nr);	/* sprintf: OK (for 'buf2 >= 19') */

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!strcasecmp(fname(tmpptr), "a") || !strcasecmp(fname(tmpptr), "an") ||
	!strcasecmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    CAP(tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  if (((retval = sscanf(line, " %d %s %s %s %s %s %s %s %s %s %s %s %s", t, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)) == 4) && (bitwarning == TRUE)) {
  /* Let's make the implementor read some, before converting his worldfiles */
  log("WARNING: Conventional objectfiles detected. Please read 128bit.readme.");
    exit(1);
  } else if (((retval == 4) || (retval == 3)) && (bitwarning == FALSE)) {
    
	if (retval == 3)
          t[3] = 0;
        else if (retval == 4)
          t[3] = asciiflag_conv_aff(f3);
	    
	log("Converting object #%d to 128bits..", nr);
    GET_OBJ_EXTRA(obj_proto + i)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)[1] = 0;
    GET_OBJ_EXTRA(obj_proto + i)[2] = 0;
    GET_OBJ_EXTRA(obj_proto + i)[3] = 0;
    GET_OBJ_WEAR(obj_proto + i)[0] = asciiflag_conv(f2);
    GET_OBJ_WEAR(obj_proto + i)[1] = 0;
    GET_OBJ_WEAR(obj_proto + i)[2] = 0;
    GET_OBJ_WEAR(obj_proto + i)[3] = 0;
    GET_OBJ_PERM(obj_proto + i)[0] = asciiflag_conv_aff(f3);
    GET_OBJ_PERM(obj_proto + i)[1] = 0;
    GET_OBJ_PERM(obj_proto + i)[2] = 0;
    GET_OBJ_PERM(obj_proto + i)[3] = 0;
    
    if(bitsavetodisk) { 
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
      converting = TRUE;
    }
	
	log("   done.");
  } else if (retval == 13) {
 
    GET_OBJ_EXTRA(obj_proto + i)[0] = asciiflag_conv(f1);
    GET_OBJ_EXTRA(obj_proto + i)[1] = asciiflag_conv(f2);
    GET_OBJ_EXTRA(obj_proto + i)[2] = asciiflag_conv(f3);
    GET_OBJ_EXTRA(obj_proto + i)[3] = asciiflag_conv(f4);
    GET_OBJ_WEAR(obj_proto + i)[0] = asciiflag_conv(f5);
    GET_OBJ_WEAR(obj_proto + i)[1] = asciiflag_conv(f6);
    GET_OBJ_WEAR(obj_proto + i)[2] = asciiflag_conv(f7);
    GET_OBJ_WEAR(obj_proto + i)[3] = asciiflag_conv(f8);
    GET_OBJ_PERM(obj_proto + i)[0] = asciiflag_conv(f9);
    GET_OBJ_PERM(obj_proto + i)[1] = asciiflag_conv(f10);
    GET_OBJ_PERM(obj_proto + i)[2] = asciiflag_conv(f11);
    GET_OBJ_PERM(obj_proto + i)[3] = asciiflag_conv(f12);

  } else {
    log("SYSERR: Format error in first numeric line (expecting 13 args, got %d), %s", retval, buf2);
    exit(1);
  }
  
  /* Object flags checked in check_object(). */
  GET_OBJ_TYPE(obj_proto + i) = t[0];
  
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    t[j] = 0;

  if ((retval = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8, t + 9, t + 10, t + 11, t + 12, t + 13, t + 14, t + 15)) > NUM_OBJ_VAL_POSITIONS) {
    log("SYSERR: Format error in second numeric line (expecting <=%d args, got %d), %s", NUM_OBJ_VAL_POSITIONS, retval, buf2);
    exit(1);
  }

  for (j = 0; j < NUM_OBJ_VAL_POSITIONS; j++)
    GET_OBJ_VAL(obj_proto + i, j) = t[j];

  if ((GET_OBJ_TYPE(obj_proto + i) == ITEM_PORTAL || \
       GET_OBJ_TYPE(obj_proto + i) == ITEM_HATCH) && \
       (!GET_OBJ_VAL(obj_proto + i, VAL_DOOR_DCLOCK) || \
        !GET_OBJ_VAL(obj_proto + i, VAL_DOOR_DCHIDE))) {
    GET_OBJ_VAL(obj_proto + i, VAL_DOOR_DCLOCK) = 20;
    GET_OBJ_VAL(obj_proto + i, VAL_DOOR_DCHIDE) = 20;
    if(bitsavetodisk) {
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
      converting = TRUE;
    }
  }

  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_WEAPON && GET_OBJ_VAL(obj_proto + i, 0) > 169) {
    GET_OBJ_VAL(obj_proto + i, 0) = suntzu_weapon_convert(t[0]);

    if(bitsavetodisk) {
      add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
      converting = TRUE;
    }
  }

  /* Convert old CWG-SunTzu style armor values to CWG-Rasputin. Should no longer be needed I think.
   * if (GET_OBJ_TYPE(obj_proto + i) == ITEM_ARMOR) {
   *   if (suntzu_armor_convert(obj_proto + i)) {
   *     if(bitsavetodisk) {
   *       add_to_save_list(zone_table[real_zone_by_thing(nr)].number, 1);
   *       converting = TRUE;
   *     }
   *   }
   * }*/

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    if (retval == 3)
      t[3] = 0;
    else {
      log("SYSERR: Format error in third numeric line (expecting 4 args, got %d), %s", retval, buf2);
    exit(1);
  }
  }
  GET_OBJ_WEIGHT(obj_proto + i) = t[0];
  GET_OBJ_COST(obj_proto + i) = t[1];
  GET_OBJ_RENT(obj_proto + i) = t[2];
  GET_OBJ_LEVEL(obj_proto + i) = t[3];
  GET_OBJ_SIZE(obj_proto + i) = SIZE_MEDIUM;

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON ||
      GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN) {
    if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1))
      GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
  }
  /* *** make sure portal objects have their timer set correctly *** */
  if (GET_OBJ_TYPE(obj_proto + i) == ITEM_PORTAL) {
    GET_OBJ_TIMER(obj_proto + i) =  -1;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
    obj_proto[i].affected[j].specific = 0;
  }

  strcat(buf2, ", after numeric constants\n"	/* strcat: OK (for 'buf2 >= 87') */
	 "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      t[1] = 0;
      if ((retval = sscanf(line, " %d %d %d ", t, t + 1, t + 2)) != 3) {
        if (retval != 2) {
	  log("SYSERR: Format error in 'A' field, %s\n"
	      "...expecting 2 numeric arguments, got %d\n"
	      "...offending line: '%s'", buf2, retval, line);
	  exit(1);
        }
      }

      if (t[0] >= APPLY_UNUSED3 && t[0] <= APPLY_UNUSED4) {
        log("Warning: object #%d (%s) uses deprecated saving throw applies",
            nr, GET_OBJ_SHORT(obj_proto + i));
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      obj_proto[i].affected[j].specific = t[2];
      j++;
      break;
    case 'S':  /* Spells for Spellbooks*/
      if (j >= SPELLBOOK_SIZE) {
	log("SYSERR: Unknown spellbook slot in S field, %s", buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'S' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'S' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      if (!obj_proto[i].sbinfo) {
        CREATE(obj_proto[i].sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
        memset((char *) obj_proto[i].sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
      }
      obj_proto[i].sbinfo[j].spellname = t[0];
      obj_proto[i].sbinfo[j].pages = t[1];
      j++;
      break;
    case 'T':  /* DG triggers */
      dg_obj_trigger(line, &obj_proto[i]);
      break;
    case 'Z':
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'Z' field, %s\n"
	    "...expecting numeric constant but file ended!", buf2);
	exit(1);
      }
      if (sscanf(line, "%d", t) != 1) {
	log("SYSERR: Format error in 'Z' field, %s\n"
	    "...expecting numeric argument\n"
	    "...offending line: '%s'", buf2, line);
	exit(1);
      }
      GET_OBJ_SIZE(obj_proto + i) = t[0];
      break;
    case '$':
    case '#':
      /* Objects that set CHARM on players are bad. */
      if (OBJAFF_FLAGGED(obj_proto + i, AFF_CHARM)) {
        log("SYSERR: Object #%d has reserved bit AFF_CHARM set.", nr);
        REMOVE_BIT_AR(GET_OBJ_PERM(obj_proto + i), AFF_CHARM);
      }
      top_of_objt = i;
      check_object(obj_proto + i);
      i++;
      return (line);
    default:
      log("SYSERR: Format error in (%c): %s", *line, buf2);
      exit(1);
    }
  }
  return line;
}


#define Z	zone_table[zone]

/* load the zone table and command tables */
static void load_zones(FILE *fl, char *zonename)
{
  static zone_rnum zone = 0;
  int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error, arg_num, version = 1;
  char *ptr, buf[READ_SIZE], zname[READ_SIZE], buf2[MAX_STRING_LENGTH];
  int zone_fix = FALSE;
  char t1[80], t2[80], line[MAX_STRING_LENGTH];

  strlcpy(zname, zonename, sizeof(zname));

  /* Skip first 3 lines lest we mistake the zone name for a command. */
  for (tmp = 0; tmp < 3; tmp++)
    get_line(fl, buf);

  /*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
   *  Note that if a new zone command is added to reset_zone(), this string
   *  will need to be updated to suit. - ae.
   */
  while (get_line(fl, buf))
    if ((strchr("MOPGERDTV", buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
      num_of_cmds++;

  rewind(fl);

  if (num_of_cmds == 0) {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  line_num += get_line(fl, buf);

  if(*buf=='@') {
    if(sscanf(buf,"@Version: %d", &version)!=1) {
      log("SYSERR: Format error in %s (version)", zname);
      log("SYSERR: ...Line: %s", line);
      exit(1);
    }
    line_num+=get_line(fl,buf);
  }

  if (sscanf(buf, "#%hd", &Z.number) != 1) {
    log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  snprintf(buf2, sizeof(buf2), "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.builders = strdup(buf);
  
  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  if (version >= 2) {

    char zbuf1[MAX_STRING_LENGTH];
    char zbuf2[MAX_STRING_LENGTH];
    char zbuf3[MAX_STRING_LENGTH];
    char zbuf4[MAX_STRING_LENGTH];

    if  (sscanf(buf, " %hd %hd %d %d %s %s %s %s %d %d", &Z.bot, &Z.top, &Z.lifespan,
      &Z.reset_mode, zbuf1, zbuf2, zbuf3, zbuf4, &Z.min_level, &Z.max_level) != 10) {
      log("SYSERR: Format error in 10-constant line of %s", zname);
      exit(1);
    }

    Z.zone_flags[0] = asciiflag_conv(zbuf1);
    Z.zone_flags[1] = asciiflag_conv(zbuf2);
    Z.zone_flags[2] = asciiflag_conv(zbuf3);
    Z.zone_flags[3] = asciiflag_conv(zbuf4);

  } else if (sscanf(buf, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
    /*
     * This may be due to the fact that the zone has no builder.  So, we just attempt
     * to fix this by copying the previous 2 last reads into this variable and the
     * last one.
     */
    log("SYSERR: Format error in numeric constant line of %s, attempting to fix.", zname);
    if (sscanf(Z.name, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
      log("SYSERR: Could not fix previous error, aborting game.");
    exit(1);
    } else {
      free(Z.name);
      Z.name = strdup(Z.builders);
      free(Z.builders);
      Z.builders = strdup("None.");
      zone_fix = TRUE;
    }
  }
  if (Z.bot > Z.top) {
    log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
    exit(1);
  }

  cmd_no = 0;

  for (;;) {
    /* skip reading one line if we fixed above (line is correct already) */
    if (zone_fix != TRUE) {
    if ((tmp = get_line(fl, buf)) == 0) {
      log("SYSERR: Format error in %s - premature end of file", zname);
      exit(1);
    }
    } else
      zone_fix = FALSE;
    
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD2.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD2.command == 'S' || ZCMD2.command == '$') {
      ZCMD2.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPDTVG", ZCMD2.command) == NULL) {	/* a 4-arg command */
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3) != 4)
	error = 1;
    } else if (ZCMD2.command=='V') { /* a string-arg command */
      if (sscanf(ptr, " %d %d %d %d %d %d %79s %79[^\f\n\r\t\v]", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3, &ZCMD2.arg4, &ZCMD2.arg5, t1, t2) != 8) 
      error = 1;
      else {
        ZCMD2.sarg1 = strdup(t1);
        ZCMD2.sarg2 = strdup(t2);
      }
    } else {
      if ((arg_num = sscanf(ptr, " %d %d %d %d %d %d ", &tmp, &ZCMD2.arg1, &ZCMD2.arg2, &ZCMD2.arg3, &ZCMD2.arg4, &ZCMD2.arg5)) != 6){
        if (arg_num != 5) {
	error = 1;
        } else {
          ZCMD2.arg5 = 0;
        }
    }
    }

    ZCMD2.if_flag = tmp;

    if (error) {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD2.line = line_num;
    cmd_no++;
  }

  if (num_of_cmds != cmd_no + 1) {
    log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
    exit(1);
  }

  top_of_zone_table = zone++;
}

#undef Z


static void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

void free_help(struct help_index_element *cmhelp) 
{
  if (cmhelp->keywords)
    free(cmhelp->keywords);
  if (cmhelp->entry && !cmhelp->duplicate)
    free(cmhelp->entry);

  free(cmhelp);
}

void free_help_table(void)
{
  if (help_table) {
    int hp;
    for (hp = 0; hp < top_of_helpt; hp++) {
      if (help_table[hp].keywords)
        free(help_table[hp].keywords);
      if (help_table[hp].entry && !help_table[hp].duplicate)
        free(help_table[hp].entry);
    }
    free(help_table);
    help_table = NULL;
  }
  top_of_helpt = 0;
}

void load_help(FILE * fl, char *name)
{
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
  size_t entrylen;
  char line[READ_SIZE + 1], hname[READ_SIZE + 1], *scan;
  struct help_index_element el;

  strlcpy(hname, name, sizeof(hname));

  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n"); /* strcat: OK (READ_SIZE - "\n"  "\r\n" == READ_SIZE  1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* Read in the corresponding help entry. */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n"); /* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg); /* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
      get_one_line(fl, line);
    }

    if (*line == '#') {
      if (sscanf(line, "#%d", &el.min_level) != 1) {
        log("SYSERR: Help entry does not have a min level. %s", key);
        el.min_level = 0;
      }
    }

    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);

    while (*next_key) {
      el.keywords = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }
  get_one_line(fl, key);
  }
}

int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (strcasecmp(a1->keywords, b1->keywords));
}

/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/


int vnum_mobile(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++)
    if (isname(searchname, mob_proto[nr].name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                   ++found, mob_index[nr].vnum, mob_proto[nr].short_descr,
                   mob_proto[nr].proto_script ? "[TRIG]" : "" );

  return (found);
}



int vnum_object(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (isname(searchname, obj_proto[nr].name))
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                    ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                    obj_proto[nr].proto_script ? "[TRIG]" : "" );

  return (found);
}



int vnum_material(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (isname(searchname, material_names[obj_proto[nr].value[VAL_ALL_MATERIAL]])) {
      send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                    ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                    obj_proto[nr].proto_script ? "[TRIG]" : "" );
    }

  return (found);
}



int vnum_weapontype(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (obj_proto[nr].type_flag == ITEM_WEAPON) {
      if (isname(searchname, weapon_type[obj_proto[nr].value[VAL_WEAPON_SKILL]])) {
        send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                    ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                    obj_proto[nr].proto_script ? "[TRIG]" : "" );
      }
    }

  return (found);
}



int vnum_armortype(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++)
    if (obj_proto[nr].type_flag == ITEM_ARMOR) {
      if (isname(searchname, armor_type[obj_proto[nr].value[VAL_ARMOR_SKILL]])) {
        send_to_char(ch, "%3d. [%5d] %-40s %s\r\n",
                    ++found, obj_index[nr].vnum, obj_proto[nr].short_description,
                    obj_proto[nr].proto_script ? "[TRIG]" : "" );
      }
    }

  return (found);
}

/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;
  ch->next_affect = NULL;
  ch->next_affectv = NULL;
  GET_ID(ch) = max_mob_id++;
  /* find_char helper */
  add_to_lookup_table(GET_ID(ch), (void *)ch);
  
  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) == NOBODY) {
      log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return (NULL);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;
  mob->next_affect = NULL;
  mob->next_affectv = NULL;
  
  if (IS_HOSHIJIN(mob) && GET_SEX(mob) == SEX_MALE) {
   mob->hairl = 0;
   mob->hairc = 0;
   mob->hairs = 0;
  } else {
   mob->hairl = rand_number(0, 4);
   mob->hairc = rand_number(1, 13);
   mob->hairs = rand_number(1, 11);
  }
 
  mob->eye = rand_number(0, 11);

  GET_ABSORBS(mob) = 0;
  ABSORBING(mob) = NULL;
  ABSORBBY(mob) = NULL;
  SITS(mob) = NULL;
  BLOCKED(mob) = NULL;
  BLOCKS(mob) = NULL;

  if (!IS_HUMAN(mob) && !IS_SAIYAN(mob) && !IS_HALFBREED(mob) && !IS_NAMEK(mob)) {
  mob->skin = rand_number(0, 11);
  }
  if (IS_NAMEK(mob)) {
   mob->skin = 2;
  }
  if (IS_HUMAN(mob) || IS_SAIYAN(mob) || IS_HALFBREED(mob)) {
   if (rand_number(1, 5) <= 2) {
    mob->skin = rand_number(0, 1);
   }
   else if (rand_number(1, 5) <= 4) {
    mob->skin = rand_number(4, 5);
   }
   else if (rand_number(1, 5) <= 5) {
    mob->skin = rand_number(9, 10);
   }
  }
  if (IS_SAIYAN(mob)) {
   mob->hairc = rand_number(1, 2);
   mob->eye = 1;
  }

  if (GET_MOB_VNUM(mob) >= 81 && GET_MOB_VNUM(mob) <= 87) {
   dragon_level(mob);
  }
 
  int64_t mult = 0;

  switch (GET_LEVEL(mob))
  {
   case 1:
    mult = rand_number(50, 80);
    break;
   case 2:
    mult = rand_number(90, 120);
    break;
   case 3:
    mult = rand_number(100, 140);
    break;
   case 4:
    mult = rand_number(120, 180);
    break;
   case 5:
    mult = rand_number(200, 250);
    break;
   case 6:
    mult = rand_number(240, 300);
    break;
   case 7:
    mult = rand_number(280, 350);
    break;
   case 8:
    mult = rand_number(320, 400);
    break;
   case 9:
    mult = rand_number(380, 480);
    break;
   case 10:
    mult = rand_number(500, 600);
    break;
   case 11:
   case 12:
   case 13:
   case 14:
   case 15:
    mult = rand_number(1200, 1600);
    break;
   case 16:
   case 17:
   case 18:
   case 19:
   case 20:
    mult = rand_number(2400, 3000);
    break;
   case 21:
   case 22:
   case 23:
   case 24:
   case 25:
    mult = rand_number(5500, 8000);
    break;
   case 26:
   case 27:
   case 28:
   case 29:
   case 30:
    mult = rand_number(10000, 14000);
    break;
   case 31:
   case 32:
   case 33:
   case 34:
   case 35:
    mult = rand_number(16000, 20000);
    break;
   case 36:
   case 37:
   case 38:
   case 39:
   case 40:
    mult = rand_number(22000, 30000);
    break;
   case 41:
   case 42:
   case 43:
   case 44:
   case 45:
    mult = rand_number(50000, 70000);
    break;
   case 46:
   case 47:
   case 48:
   case 49:
   case 50:
    mult = rand_number(95000, 140000);
    break;
   case 51:
   case 52:
   case 53:
   case 54:
   case 55:
    mult = rand_number(180000, 250000);
    break;
   case 56:
   case 57:
   case 58:
   case 59:
   case 60:
    mult = rand_number(400000, 480000);
    break;
   case 61:
   case 62:
   case 63:
   case 64:
   case 65:
    mult = rand_number(700000, 900000);
    break;
   case 66:
   case 67:
   case 68:
   case 69:
   case 70:
    mult = rand_number(1400000, 1600000);
    break;
   case 71:
   case 72:
   case 73:
   case 74:
   case 75:
    mult = rand_number(2200000, 2500000);
    break;
   case 76:
   case 77:
   case 78:
   case 79:
   case 80:
    mult = rand_number(3000000, 3500000);
    break;
   case 81:
   case 82:
   case 83:
   case 84:
   case 85:
    mult = rand_number(4250000, 4750000);
    break;
   case 86:
   case 87:
   case 88:
   case 89:
   case 90:
    mult = rand_number(6500000, 8500000);
    break;
   case 91:
   case 92:
   case 93:
   case 94:
   case 95:
    mult = rand_number(15000000, 18000000);
    break;
   case 96:
   case 97:
   case 98:
   case 99:
   case 100:
    mult = rand_number(22000000, 30000000);
    break;
   case 101:
    mult = rand_number(32000000, 40000000);
    break;
   case 102:
    mult = rand_number(42000000, 55000000);
    break;
   case 103:
    mult = rand_number(80000000, 95000000);
    break;
   case 104:
    mult = rand_number(150000000, 200000000);
    break;
   case 105:
    mult = rand_number(220000000, 250000000);
    break;
   case 106:
   case 107:
   case 108:
   case 109:
   case 110:
    mult = rand_number(500000000, 750000000);
    break;
   case 111:
   case 112:
   case 113:
   case 114:
   case 115:
   case 116:
   case 117:
   case 118:
   case 119:
   case 120:
    mult = rand_number(800000000, 900000000);
    break;
   default:
    if (GET_LEVEL(mob) >= 150) {
     mult = rand_number(1500000000, 2000000000);
    } else {
     mult = rand_number(1250000000, 1500000000);
    }
    break;
  }

  GET_LPLAY(mob) = time(0);

  if (GET_MAX_HIT(mob) <= 1) {
   GET_MAX_HIT(mob) = GET_LEVEL(mob) * mult;
   if (GET_LEVEL(mob) > 140) {
    GET_MAX_HIT(mob) *= 8;
   } else if (GET_LEVEL(mob) > 130) {
    GET_MAX_HIT(mob) *= 6;
   } else if (GET_LEVEL(mob) > 120) {
    GET_MAX_HIT(mob) *= 3; 
   } else if (GET_LEVEL(mob) > 110) {
    GET_MAX_HIT(mob) *= 2;
   }
   GET_HIT(mob) = GET_MAX_HIT(mob);
   GET_BASE_PL(mob) = GET_MAX_HIT(mob);
  }
  if (GET_MAX_MANA(mob) <= 1) {
   GET_MAX_MANA(mob) = GET_LEVEL(mob) * mult;
   if (GET_LEVEL(mob) > 140) {
    GET_MAX_MANA(mob) *= 8;
   } else if (GET_LEVEL(mob) > 130) {
    GET_MAX_MANA(mob) *= 6;
   } else if (GET_LEVEL(mob) > 120) {
    GET_MAX_MANA(mob) *= 3;
   } else if (GET_LEVEL(mob) > 110) {
    GET_MAX_MANA(mob) *= 2;
   }
   GET_MANA(mob) = GET_MAX_MANA(mob);
   GET_BASE_KI(mob) = GET_MAX_MANA(mob);
  }
  if (GET_MAX_MOVE(mob) <= 1) {
   GET_MAX_MOVE(mob) = GET_LEVEL(mob) * mult;
   if (GET_LEVEL(mob) > 140) {
    GET_MAX_MOVE(mob) *= 8;
   } else if (GET_LEVEL(mob) > 130) {
    GET_MAX_MOVE(mob) *= 6;
   } else if (GET_LEVEL(mob) > 120) {
    GET_MAX_MOVE(mob) *= 3;
   } else if (GET_LEVEL(mob) > 110) {
    GET_MAX_MOVE(mob) *= 2;
   }
   GET_MOVE(mob) = GET_MAX_MOVE(mob);
   GET_BASE_ST(mob) = GET_MAX_MOVE(mob);
  }
  if (GET_MOB_VNUM(mob) == 2245) {
   GET_MAX_HIT(mob) = rand_number(1, 4);
   GET_HIT(mob) = GET_MAX_HIT(mob);
   GET_BASE_PL(mob) = GET_MAX_HIT(mob);
   GET_MAX_MANA(mob) = rand_number(1, 4);
   GET_MANA(mob) = GET_MAX_MANA(mob);
   GET_BASE_KI(mob) = GET_MAX_MANA(mob);
   GET_MAX_MOVE(mob) = rand_number(1, 4);
   GET_MOVE(mob) = GET_MAX_MOVE(mob);
   GET_BASE_ST(mob) = GET_MAX_MOVE(mob);
  }

  int base = 0;
  switch (GET_LEVEL(mob))
  {
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
     base = rand_number(80, 120);
    break;
   case 6:
     base = rand_number(200, 280);
    break;
   case 7:
     base = rand_number(250, 350);
    break;
   case 8:
     base = rand_number(275, 375);
    break;
   case 9:
     base = rand_number(300, 400);
    break;
   case 10:
     base = rand_number(325, 450);
    break;
   case 11:
   case 12:
   case 13:
   case 14:
   case 15:
     base = rand_number(500, 700);
    break;
   case 16:
   case 17:
   case 18:
   case 19:
   case 20:
     base = rand_number(700, 1000);
    break;
   case 21:
   case 22:
   case 23:
   case 24:
   case 25:
     base = rand_number(1000, 1200);
    break;
   case 26:
   case 27:
   case 28:
   case 29:
   case 30:
     base = rand_number(1200, 1400);
    break;
   case 31:
   case 32:
   case 33:
   case 34:
   case 35:
     base = rand_number(1400, 1600);
    break;
   case 36:
   case 37:
   case 38:
   case 39:
   case 40:
     base = rand_number(1600, 1800);
    break;
   case 41:
   case 42:
   case 43:
   case 44:
   case 45:
     base = rand_number(1800, 2000);
    break;
   case 46:
   case 47:
   case 48:
   case 49:
   case 50:
     base = rand_number(2000, 2200);
    break;
   case 51:
   case 52:
   case 53:
   case 54:
   case 55:
     base = rand_number(2200, 2500);
    break;
   case 56:
   case 57:
   case 58:
   case 59:
   case 60:
     base = rand_number(2500, 2800);
    break;
   case 61:
   case 62:
   case 63:
   case 64:
   case 65:
     base = rand_number(2800, 3000);
    break;
   case 66:
   case 67:
   case 68:
   case 69:
   case 70:
     base = rand_number(3000, 3200);
    break;
   case 71:
   case 72:
   case 73:
   case 74:
   case 75:
     base = rand_number(3200, 3500);
    break;
   case 76:
   case 77:
   case 78:
   case 79:
     base = rand_number(3500, 3800);
    break;
   case 80:
   case 81:
   case 82:
   case 83:
   case 84:
   case 85:
     base = rand_number(4000, 4500);
    break;
   case 86:
   case 87:
   case 88:
   case 89:
   case 90:
     base = rand_number(4500, 5500);
    break;
   case 91:
   case 92:
   case 93:
   case 94:
   case 95:
     base = rand_number(5500, 7000);
    break;
   case 96:
   case 97:
   case 98:
   case 99:
     base = rand_number(8000, 10000);
    break;
   case 100:
     base = rand_number(10000, 15000);
    break;
   case 101:
     base = rand_number(15000, 25000);
    break;
   case 102:
     base = rand_number(35000, 40000);
    break;
   case 103:
     base = rand_number(40000, 50000);
    break;
   case 104:
     base = rand_number(60000, 80000);
    break;
   case 105:
     base = rand_number(80000, 100000);
    break;
   default:
     base = rand_number(130000, 180000);
    break;
  }
  MOB_COOLDOWN(mob) = 0;
  if (GET_GOLD(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
   if (GET_LEVEL(mob) < 4) {
    GET_GOLD(mob) = GET_LEVEL(mob) * rand_number(1, 2);
   } else if (GET_LEVEL(mob) < 10) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(1, 2)) - 1;
   } else if (GET_LEVEL(mob) < 20) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(1, 3)) - 2;
   } else if (GET_LEVEL(mob) < 30) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(1, 3)) - 4;
   } else if (GET_LEVEL(mob) < 40) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(1, 3)) - 6;
   } else if (GET_LEVEL(mob) < 50) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(2, 3)) - 25;
   } else if (GET_LEVEL(mob) < 60) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(2, 3)) - 40;
   } else if (GET_LEVEL(mob) < 70) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(2, 3)) - 50;
   } else if (GET_LEVEL(mob) < 80) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(2, 4)) - 60;
   } else if (GET_LEVEL(mob) < 90) {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(2, 4)) - 70;
   } else {
    GET_GOLD(mob) = (GET_LEVEL(mob) * rand_number(3, 4)) - 85;
   }
   if (!IS_HUMANOID(mob)) {
    GET_GOLD(mob) = GET_GOLD(mob) * 0.5;
    if (GET_GOLD(mob) <= 0)
     GET_GOLD(mob) = 1;
   }
  }
  if (GET_EXP(mob) <= 0 && !MOB_FLAGGED(mob, MOB_DUMMY)) {
   GET_EXP(mob) = GET_LEVEL(mob) * base;
   GET_EXP(mob) = GET_EXP(mob) * .9;
   GET_EXP(mob) += GET_LEVEL(mob) / 2;
   GET_EXP(mob) += GET_LEVEL(mob) / 3;
   if (IS_DRAGON(mob)) {
    GET_EXP(mob) *= 1.4;
   } else if (IS_ANDROID(mob)) {
    GET_EXP(mob) *= 1.25;
   } else if (IS_SAIYAN(mob)) {
    GET_EXP(mob) *= 1.1;
   } else if (IS_BIO(mob)) {
    GET_EXP(mob) *= 1.2;
   } else if (IS_MAJIN(mob)) {
    GET_EXP(mob) *= 1.25;
   } else if (IS_DEMON(mob)) {
    GET_EXP(mob) *= 1.1;
   } else if (GET_CLASS(mob) == CLASS_SHADOWDANCER) {
    GET_EXP(mob) *= 2;
   }
   if (GET_CLASS(mob) == CLASS_NPC_COMMONER && IS_HUMANOID(mob) && !IS_DRAGON(mob)) {
    if (!IS_ANDROID(mob) && !IS_SAIYAN(mob) && !IS_BIO(mob) && !IS_MAJIN(mob)) {
     GET_EXP(mob) *= 0.75;
    }
   }

   if (GET_LEVEL(mob) > 90) {
    GET_EXP(mob) = GET_EXP(mob) * .7;
   }
   else if (GET_LEVEL(mob) > 80) {
    GET_EXP(mob) = GET_EXP(mob) * .75;
   }
   else if (GET_LEVEL(mob) > 70) {
    GET_EXP(mob) = GET_EXP(mob) * .8;
   }
   else if (GET_LEVEL(mob) > 60) {
    GET_EXP(mob) = GET_EXP(mob) * .85;
   }
   else if (GET_LEVEL(mob) > 40) {
    GET_EXP(mob) = GET_EXP(mob) * .9;
   }
   else if (GET_LEVEL(mob) > 30) {
    GET_EXP(mob) = GET_EXP(mob) * .95;
   }
  
   if (GET_EXP(mob) > 20000000) {
    GET_EXP(mob) = 20000000;
   }
  }

  GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_MANA(mob) = GET_MAX_HIT(mob);
  GET_MOVE(mob) = GET_MAX_HIT(mob);

  mob->time.birth = time(0) - birth_age(mob);
  mob->time.created = mob->time.logon = time(0); /* why not */
  mob->time.maxage = mob->time.birth + max_age(mob);
  mob->time.played = 0;
  mob->time.logon = time(0);
  MOB_LOADROOM(mob) = NOWHERE;

  if (IS_HUMANOID(mob)) {
   SET_BIT_AR(MOB_FLAGS(mob), MOB_RARM);
   SET_BIT_AR(MOB_FLAGS(mob), MOB_LARM);
   SET_BIT_AR(MOB_FLAGS(mob), MOB_RLEG);
   SET_BIT_AR(MOB_FLAGS(mob), MOB_LLEG);
  }
 
  mob_index[i].number++;

  GET_ID(mob) = max_mob_id++;
  /* find_char helper */
  add_to_lookup_table(GET_ID(mob), (void *)mob);

  copy_proto_script(&mob_proto[i], mob, MOB_TRIGGER);
  assign_triggers(mob, MOB_TRIGGER);
  racial_body_parts(mob); 

  if (GET_MOB_VNUM(mob) >= 800 && GET_MOB_VNUM(mob) <= 805) {
   number_of_assassins += 1;
  }

  return (mob);
}


struct obj_unique_hash_elem {
  time_t generation;
  int64_t unique_id;
  struct obj_data *obj;
  struct obj_unique_hash_elem *next_e;
};

static void free_obj_unique_hash()
{
  int i;
  struct obj_unique_hash_elem *elem;
  struct obj_unique_hash_elem *next_elem;
  if (obj_unique_hash_pools) {
    for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
      elem = obj_unique_hash_pools[i];
      while (elem) {
        next_elem = elem->next_e;
        free(elem);
        elem = next_elem;
      }
    }
    free(obj_unique_hash_pools);
  }
}

void add_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  int i;
  if (!obj_unique_hash_pools)
    init_obj_unique_hash();
  if (obj->unique_id == -1) {
    if (sizeof(long long) > sizeof(long))
      obj->unique_id = (((long long)circle_random()) << (sizeof(long long) * 4)) +
                       circle_random();
    else
      obj->unique_id = circle_random();
  }
  if (CONFIG_ALL_ITEMS_UNIQUE) {
    if (!IS_SET_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
  }
  CREATE(elem, struct obj_unique_hash_elem, 1);
  elem->generation = obj->generation;
  elem->unique_id = obj->unique_id;
  elem->obj = obj;
  i = obj->unique_id % NUM_OBJ_UNIQUE_POOLS;
  elem->next_e = obj_unique_hash_pools[i];
  obj_unique_hash_pools[i] = elem;
}

void remove_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem, **ptr, *tmp;

  if (obj == NULL || obj->unique_id < 0)
    return;

  ptr = obj_unique_hash_pools + (obj->unique_id % NUM_OBJ_UNIQUE_POOLS);

  if (!(ptr && *ptr))
    return;

  elem = *ptr;

  while (elem) {
    tmp = elem->next_e;
    if (elem->obj == obj) {
      free(elem);
      *ptr = tmp;
    } else {
      ptr = &(elem->next_e);
    } 
    elem = tmp;
  }
}

void log_dupe_objects(struct obj_data *obj1, struct obj_data *obj2)
{
  mudlog(BRF, ADMLVL_GOD, TRUE, "DUPE: Dupe object found: %s [%d] [%" TMT ":%" I64T "]",
        obj1->short_description ? obj1->short_description : "<No name>",
        GET_OBJ_VNUM(obj1), obj1->generation, obj1->unique_id);
  mudlog(BRF, ADMLVL_GOD, TRUE, "DUPE: First: In room: %d (%s), "
                             "In object: %s, Carried by: %s, Worn by: %s",
        GET_ROOM_VNUM(IN_ROOM(obj1)),
        IN_ROOM(obj1) == NOWHERE ? "Nowhere" : world[IN_ROOM(obj1)].name,
        obj1->in_obj ? obj1->in_obj->short_description : "None",
        obj1->carried_by ? GET_NAME(obj1->carried_by) : "Nobody",
        obj1->worn_by ? GET_NAME(obj1->worn_by) : "Nobody");
  mudlog(BRF, ADMLVL_GOD, TRUE, "DUPE: Newer: In room: %d (%s), "
                             "In object: %s, Carried by: %s, Worn by: %s",
        GET_ROOM_VNUM(IN_ROOM(obj2)),
        IN_ROOM(obj2) == NOWHERE ? "Nowhere" : world[IN_ROOM(obj2)].name,
        obj2->in_obj ? obj2->in_obj->short_description : "None",
        obj2->carried_by ? GET_NAME(obj2->carried_by) : "Nobody",
        obj2->worn_by ? GET_NAME(obj2->worn_by) : "Nobody");
}

void check_unique_id(struct obj_data *obj)
{
  struct obj_unique_hash_elem *elem;
  if (!obj || obj->unique_id == -1)
    return;
  elem = obj_unique_hash_pools[obj->unique_id % NUM_OBJ_UNIQUE_POOLS];
  while (elem) {
    if (elem->obj == obj) {
      log("SYSERR: check_unique_id checking for existing object?!");
    }
    if (elem->generation == obj->generation && elem->unique_id == obj->unique_id) {
      log_dupe_objects(elem->obj, obj);
      SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_PURGE);
    }
    elem = elem->next_e;
  }
}

char *sprintuniques(int low, int high)
{
  int i, count = 0, remain, header;
  struct obj_unique_hash_elem *q;
  char *str, *ptr;
  remain = 40;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    q = obj_unique_hash_pools[i];
    remain += 40;
    while (q) {
      count++;
      remain += 80 + (q->obj->short_description ? strlen(q->obj->short_description) : 20);
      q = q->next_e;
    }
  }
  if (count < 1) {
    return strdup("No objects in unique hash.\r\n");
  }
  CREATE(str, char, remain + 1);
  ptr = str;
  count = snprintf(ptr, remain, "Unique object hashes (vnums %d - %d)\r\n",
                low, high);
  ptr += count;
  remain -= count;
  for (i = 0; i < NUM_OBJ_UNIQUE_POOLS; i++) {
    header = 0;
    q = obj_unique_hash_pools[i];
    while (q) {
      if (GET_OBJ_VNUM(q->obj) >= low && GET_OBJ_VNUM(q->obj) <= high) {
        if (!header) {
          header = 1;
          count = snprintf(ptr, remain, "|-Hash %d\r\n", i);
          ptr += count;
          remain -= count;
        }
        count = snprintf(ptr, remain, "| |- [@g%6d@n] - [@y%10"TMT":%-19"I64T"@n] - %s\r\n",
                GET_OBJ_VNUM(q->obj), q->generation, q->unique_id,
                q->obj->short_description ? q->obj->short_description : "<Unknown>");
        ptr += count;
        remain -= count;
      }
      q = q->next_e;
    }
  }
  return str;
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  obj->generation = time(0);
  obj->unique_id = -1;

  assign_triggers(obj, OBJ_TRIGGER);
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  return (obj);
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;
  int j;

  if (i == NOTHING || i > top_of_objt) {
    log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (NULL);
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;
  OBJ_LOADROOM(obj) = NOWHERE;

  obj_index[i].number++;

  GET_ID(obj) = max_obj_id++;
  /* find_obj helper */
  add_to_lookup_table(GET_ID(obj), (void *)obj);

  obj->generation = time(0);
  obj->unique_id = -1;

  if (obj_proto[i].sbinfo) {
    CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
    for (j = 0; j < SPELLBOOK_SIZE; j++) {
      obj->sbinfo[j].spellname = obj_proto[i].sbinfo[j].spellname;
      obj->sbinfo[j].pages = obj_proto[i].sbinfo[j].pages;
    }
  }
  copy_proto_script(&obj_proto[i], obj, OBJ_TRIGGER);
  assign_triggers(obj, OBJ_TRIGGER);
  if (GET_OBJ_VNUM(obj) == 65) {
   HCHARGE(obj) = 20;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_FOOD) {
   if (GET_OBJ_VAL(obj, 1) == 0) {
    GET_OBJ_VAL(obj, 1) = GET_OBJ_VAL(obj, VAL_FOOD_FOODVAL);
   }
   FOOB(obj) = GET_OBJ_VAL(obj, 1);
  }
  return (obj);
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      mudlog(CMP, ADMLVL_GOD, FALSE, "Auto zone reset: %s (Zone %d)", 
          zone_table[update_u->zone_to_reset].name, zone_table[update_u->zone_to_reset].number);
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

static void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: zone file: %s", message);
  mudlog(NRM, ADMLVL_GOD, TRUE, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	ZCMD2.command, zone_table[zone].number, ZCMD2.line);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  room_vnum rvnum;
  room_rnum rrnum;
  struct char_data *tmob=NULL; /* for trigger assignment */
  struct obj_data *tobj=NULL;  /* for trigger assignment */
  int mob_load = FALSE; /* ### */
  int obj_load = FALSE; /* ### */

 if (pre_reset(zone_table[zone].number) == FALSE) {
  for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {

    if (ZCMD2.if_flag && !last_cmd && !mob_load && !obj_load)
      continue;

     if (!ZCMD2.if_flag) { /* ### */
       mob_load = FALSE;
       obj_load = FALSE;
     }

    /*  This is the list of actual zone commands.  If any new
     *  zone commands are added to the game, be certain to update
     *  the list of commands in load_zone() so that the counting
     *  will still be correct. - ae.
     */
    switch (ZCMD2.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */

      if ((mob_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
           (rand_number(1, 100) >= ZCMD2.arg5)) {
        int room_max = 0;
        struct char_data *i;
	mob = read_mobile(ZCMD2.arg1, REAL);
	
	/* First find out how many mobs of VNUM are in the mud with this rooms */
               /* VNUM as a load point for max from room checks. */
       /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

        if (ZCMD2.arg4 > 0) {
          for (i = character_list; i; i = i->next) {
            if ((MOB_LOADROOM(i) == GET_ROOM_VNUM(ZCMD2.arg3)) 
               && (GET_MOB_VNUM(i) == GET_MOB_VNUM(mob))) {
  	      room_max++;
            }
	  }
	}
	char_to_room(mob, ZCMD2.arg3);
	
           /* Get rid of it if room_max has been met, ignore room_max if zero */

	if (room_max && (room_max >= ZCMD2.arg4)){
	   extract_char(mob);
	   extract_pending_chars();
	   break;
	   }
	
	   /*  Set the mobs loadroom for room_max checks. */
	MOB_LOADROOM(mob) = GET_ROOM_VNUM(ZCMD2.arg3);
	
        load_mtrigger(mob);
        tmob = mob;
	last_cmd = 1;
        mob_load = TRUE;
      } else
	last_cmd = 0;
        tobj = NULL;
        break;

    case 'O':			/* read an object */
       if ((obj_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
           (rand_number(1, 100) >= ZCMD2.arg5)) {
	if (ZCMD2.arg3 != NOWHERE) {
          int room_max = 0;
          struct obj_data *k;
	  obj = read_object(ZCMD2.arg1, REAL);
	  
	  	/* First find out how many obj of VNUM are in the mud with this rooms */
               /* VNUM as a load point for max from room checks. */
       /* Let's only count if room_max is in use.  If left at zero, max_in_mud will handle*/

        if (ZCMD2.arg4 > 0) {
          for (k = object_list; k; k = k->next) {
            if (((OBJ_LOADROOM(k)==GET_ROOM_VNUM(ZCMD2.arg3)) 
                && (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj))) || (GET_OBJ_VNUM(k) == GET_OBJ_VNUM(obj) && GET_ROOM_VNUM(ZCMD2.arg3) ==  GET_ROOM_VNUM(IN_ROOM(k)))) {
              /*  For objects, lets not count them if they've been removed from the room */
              /*  We'll let max_in_mud handle those. */
               if (IN_ROOM(k) == NOWHERE || GET_ROOM_VNUM(IN_ROOM(k)) 
                   != GET_ROOM_VNUM(ZCMD2.arg3)) {
                 continue;
               }
	       room_max++;
            }
          }
        }

          add_unique_id(obj);
	  obj_to_room(obj, ZCMD2.arg3);
	  
           /* Get rid of it if room_max has been met. */

	  if (room_max && (room_max >= ZCMD2.arg4)){
	     extract_obj(obj);
	     break;
	     }
	  
	  /* Set the loadroom for room_max checks */

	  OBJ_LOADROOM(obj) = GET_ROOM_VNUM(ZCMD2.arg3);
	
	  last_cmd = 1;
          load_otrigger(obj);
          tobj = obj;
	  obj_load = TRUE;
	} else {
	  obj = read_object(ZCMD2.arg1, REAL);
          add_unique_id(obj);
	  IN_ROOM(obj) = NOWHERE;
	  last_cmd = 1;
          tobj = obj;
	  obj_load = TRUE;
	}
      } else
	last_cmd = 0;
	tmob = NULL;
      break;

    case 'P':			/* object to object */
       if ((obj_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
           obj_load && (rand_number(1, 100) >= ZCMD2.arg5)) {
	obj = read_object(ZCMD2.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD2.arg3))) {
	  ZONE_ERROR("target obj not found, command disabled");
	  ZCMD2.command = '*';
	  break;
	}
        add_unique_id(obj);
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
        load_otrigger(obj);
        tobj = obj;
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
	ZCMD2.command = '*';
	break;
      }
      if ((obj_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
          mob_load && (rand_number(1, 100) >= ZCMD2.arg5)) {
	obj = read_object(ZCMD2.arg1, REAL);
        add_unique_id(obj);
	obj_to_char(obj, mob);
        if (GET_MOB_SPEC(mob) != shop_keeper) {
         randomize_eq(obj);
        }
	last_cmd = 1;
        load_otrigger(obj);
        tobj = obj;
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob, command disabled");
	ZCMD2.command = '*';
	break;
      }
      if ((obj_index[ZCMD2.arg1].number < ZCMD2.arg2) &&
          mob_load && (rand_number(1, 100) >= ZCMD2.arg5)) {
	if (ZCMD2.arg3 < 0 || ZCMD2.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD2.arg1, REAL);
          add_unique_id(obj);
          IN_ROOM(obj) = IN_ROOM(mob);
          load_otrigger(obj);
          if (wear_otrigger(obj, mob, ZCMD2.arg3)) {
            IN_ROOM(obj) = NOWHERE;
	    equip_char(mob, obj, ZCMD2.arg3);
          } else
            obj_to_char(obj, mob);
            tobj = obj;
	    last_cmd = 1;
	  }
      } else
	last_cmd = 0;
        tmob = NULL;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD2.arg2, world[ZCMD2.arg1].contents)) != NULL)
        extract_obj(obj);
        last_cmd = 1;
        tmob = NULL;
        tobj = NULL;
      break;


    case 'D':			/* set state of door */
      if (ZCMD2.arg2 < 0 || ZCMD2.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD2.arg1].dir_option[ZCMD2.arg2] == NULL)) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD2.command = '*';
      } else
	switch (ZCMD2.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD2.arg1].dir_option[ZCMD2.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      tmob = NULL;
      tobj = NULL;
      break;

    case 'T': /* trigger command */
      if (ZCMD2.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob))
          CREATE(SCRIPT(tmob), struct script_data, 1);
        add_trigger(SCRIPT(tmob), read_trigger(ZCMD2.arg2), -1);
        last_cmd = 1;
      } else if (ZCMD2.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj))
          CREATE(SCRIPT(tobj), struct script_data, 1);
        add_trigger(SCRIPT(tobj), read_trigger(ZCMD2.arg2), -1);
        last_cmd = 1;
      } else if (ZCMD2.arg1==WLD_TRIGGER) {
        if (ZCMD2.arg3 == NOWHERE || ZCMD2.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in trigger assignment");
        }
        if (!world[ZCMD2.arg3].script)
          CREATE(world[ZCMD2.arg3].script, struct script_data, 1);
        add_trigger(world[ZCMD2.arg3].script, read_trigger(ZCMD2.arg2), -1);
        last_cmd = 1;
      }

      break;

    case 'V':
      if (ZCMD2.arg1==MOB_TRIGGER && tmob) {
        if (!SCRIPT(tmob)) {
          ZONE_ERROR("Attempt to give variable to scriptless mobile");
        } else
          add_var(&(SCRIPT(tmob)->global_vars), ZCMD2.sarg1, ZCMD2.sarg2,
                  ZCMD2.arg3);
        last_cmd = 1;
      } else if (ZCMD2.arg1==OBJ_TRIGGER && tobj) {
        if (!SCRIPT(tobj)) {
          ZONE_ERROR("Attempt to give variable to scriptless object");
        } else
          add_var(&(SCRIPT(tobj)->global_vars), ZCMD2.sarg1, ZCMD2.sarg2,
                  ZCMD2.arg3);
        last_cmd = 1;
      } else if (ZCMD2.arg1==WLD_TRIGGER) {
        if (ZCMD2.arg3 == NOWHERE || ZCMD2.arg3>top_of_world) {
          ZONE_ERROR("Invalid room number in variable assignment");
        } else {
          if (!(world[ZCMD2.arg3].script)) {
            ZONE_ERROR("Attempt to give variable to scriptless object");
          } else
            add_var(&(world[ZCMD2.arg3].script->global_vars),
                    ZCMD2.sarg1, ZCMD2.sarg2, ZCMD2.arg2);
          last_cmd = 1;
        }
      }
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD2.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;

  /* handle reset_wtrigger's */
  rvnum = zone_table[zone].bot;
  while (rvnum <= zone_table[zone].top) {
    rrnum = real_room(rvnum);
    if (rrnum != NOWHERE) {
     reset_wtrigger(&world[rrnum]);
     if (ROOM_FLAGGED(rrnum, ROOM_AURA) && rand_number(1, 5) >= 4) {
      send_to_room(rrnum, "The aura of regeneration covering the surrounding area disappears.\r\n");
      REMOVE_BIT_AR(ROOM_FLAGS(rrnum), ROOM_AURA);
     }
     if (SECT(rrnum) == SECT_LAVA) {
      ROOM_EFFECT(rrnum) = 5;
     }
     if (ROOM_EFFECT(rrnum) < -1) {
      send_to_room(rrnum, "The area loses some of the water flooding it.\r\n");
      ROOM_EFFECT(rrnum) += 1;
     }
     else if (ROOM_EFFECT(rrnum) == -1) {
      send_to_room(rrnum, "The area loses the last of the water flooding it in one large rush.\r\n");
      ROOM_EFFECT(rrnum) = 0;
     }
     if (ROOM_DAMAGE(rrnum) >= 100) {
      send_to_room(rrnum, "The area gets rebuilt a little.\r\n");
      ROOM_DAMAGE(rrnum) -= rand_number(5, 10);
     }
     else if (ROOM_DAMAGE(rrnum) >= 50) {
      send_to_room(rrnum, "The area gets rebuilt a little.\r\n");
      ROOM_DAMAGE(rrnum) -= rand_number(1, 10);
     }
     else if (ROOM_DAMAGE(rrnum) >= 10) {
      send_to_room(rrnum, "The area gets rebuilt a little.\r\n");
      ROOM_DAMAGE(rrnum) -= rand_number(1, 10);
     }
     else if (ROOM_DAMAGE(rrnum) > 1) {
      send_to_room(rrnum, "The area gets rebuilt a little.\r\n");
      ROOM_DAMAGE(rrnum) -= rand_number(1, ROOM_DAMAGE(rrnum));
     }
     else if (ROOM_DAMAGE(rrnum) > 0) {
      send_to_room(rrnum, "The area gets rebuilt a little.\r\n");
      ROOM_DAMAGE(rrnum)--;
     }
     if (ROOM_EFFECT(rrnum) >= 1 && rand_number(1, 4) == 4 && !SUNKEN(rrnum) && SECT(rrnum) != SECT_LAVA) {
      send_to_room(rrnum, "The lava has cooled and become solid rock.\r\n");
      ROOM_EFFECT(rrnum) = 0;
     } else if (ROOM_EFFECT(rrnum) >= 1 && rand_number(1, 2) == 2 && SUNKEN(rrnum) && SECT(rrnum) != SECT_LAVA) {
      send_to_room(rrnum, "The water has cooled the lava and it has become solid rock.\r\n");
      ROOM_EFFECT(rrnum) = 0;
     }
    }
    rvnum++;
  }
 } 
  else 
  { 
   /* even if reset is blocked, age should be reset */ 
   zone_table[zone].age = 0; 
  } 
  post_reset(zone_table[zone].number);
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (world[IN_ROOM(i->character)].zone != zone_nr)
      continue;
    /*
     * if an immortal has nohassle off, he counts as present 
     * added for testing zone reset triggers - Welcor 
     */
    if (IS_NPC(i->character))
      continue; /* immortal switched into a mob */

    if ((GET_ADMLEVEL(i->character) >= ADMLVL_IMMORT) && (PRF_FLAGGED(i->character, PRF_NOHASSLE)))
      continue;

    return (0);
  }

  return (1);
}


/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[520];
  char *point;
  int done = 0, length = 0, templength;

  *buf = *tmp = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      log("SYSERR: fread_string: format error at string (pos %ld): %s at or near %s",
          ftell(fl), feof(fl) ? "EOF" : ferror(fl) ? "read error" : "unknown error", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    /* now only removes trailing ~'s -- Welcor */
    for (point = tmp; *point && *point!='\r' && *point!='\n'; point++);
    if (point > tmp && point[-1] == '~') {
      *(--point)='\0';
      done = 1;
    } else {
      *point = '\r';
      *(++point) = '\n';
      *(++point) = '\0';
    }

    templength = point - tmp;

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log("%s", error);
      exit(1);
    } else {
      strcat(buf + length, tmp);	/* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}

/* Called to free all allocated follow_type structs - Update by Jamie Nelson */
void free_followers(struct follow_type *k)
{
   if (!k)
    return;

  if (k->next)
    free_followers(k->next);

  k->follower = NULL;
    free(k);
}

/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  int i;
  struct alias_data *a;
  struct levelup_data *data, *next_data;
  struct level_learn_entry *learn, *next_learn;

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {

    if (CONFIG_IMC_ENABLED) {
      imc_freechardata(ch);
    }

    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    if (ch->player_specials->host)
      free(ch->player_specials->host);
    for (i = 0; i < NUM_COLOR; i++)
      if (ch->player_specials->color_choices[i])
        free(ch->player_specials->color_choices[i]);
    if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == NOBODY)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (GET_VOICE(ch))
      free(GET_VOICE(ch));
    if (GET_CLAN(ch))
      free(GET_CLAN(ch));
    if (ch->title)
      free(ch->title);
    if (ch->short_descr)
      free(ch->short_descr);
    if (ch->long_descr)
      free(ch->long_descr);
    if (ch->description)
      free(ch->description);
    for (i = 0; i < NUM_HIST; i++)
      if (GET_HISTORY(ch, i))
        free(GET_HISTORY(ch, i));
    if (ch->player_specials)
      free(ch->player_specials);
    
    /* free script proto list */
    free_proto_script(ch, MOB_TRIGGER);
    
  } else if ((i = GET_MOB_RNUM(ch)) != NOBODY) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->name && ch->name != mob_proto[i].name)
      free(ch->name);
    if (ch->title && ch->title != mob_proto[i].title)
      free(ch->title);
    if (ch->short_descr && ch->short_descr != mob_proto[i].short_descr)
      free(ch->short_descr);
    if (ch->long_descr && ch->long_descr != mob_proto[i].long_descr)
      free(ch->long_descr);
    if (ch->description && ch->description != mob_proto[i].description)
      free(ch->description);
    /* free script proto list if it's not the prototype */
    if (ch->proto_script && ch->proto_script != mob_proto[i].proto_script)
      free_proto_script(ch, MOB_TRIGGER);
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* free any assigned scripts */
  if (SCRIPT(ch))
    extract_script(ch, MOB_TRIGGER);

  /* new version of free_followers take the followers pointer as arg */
  free_followers(ch->followers);

  if (ch->desc)
    ch->desc->character = NULL;

  if (ch->level_info) {
    for (data = ch->level_info; data; data = next_data) {
      next_data = data->next;
      for (learn = data->skills; learn; learn = next_learn) {
        next_learn = learn->next;
        free(learn);
      }
      for (learn = data->feats; learn; learn = next_learn) {
        next_learn = learn->next;
        free(learn);
      }
      free(data);
    }
  }
  ch->level_info = NULL;

  /* find_char helper */
  /*
  * when free_char is called with a blank character struct, ID is set
  * to 0, and has not yet been added to the lookup table.
  */
  if (GET_ID(ch) != 0)
    remove_from_lookup_table(GET_ID(ch));

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data *obj)
{
  remove_unique_id(obj);
  if (GET_OBJ_RNUM(obj) == NOWHERE) {
    free_object_strings(obj);
    /* free script proto list */
    free_proto_script(obj, OBJ_TRIGGER);
  } else {
    free_object_strings_proto(obj);
    if (obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
      free_proto_script(obj, OBJ_TRIGGER);

  }
  
  /* Let's make sure that we free up this memory */
  if (obj->auctname) {
   free(obj->auctname);
  }

  /* free any assigned scripts */
  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  /* find_obj helper */
  remove_from_lookup_table(GET_ID(obj));

  if (obj->sbinfo)
    free(obj->sbinfo);

  free(obj);
}


/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
static int file_to_string_alloc(const char *name, char **buf)
{
  int temppage;
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  for (in_use = descriptor_list; in_use; in_use = in_use->next) {
    if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
      continue;

    /* Let's be nice and leave them at the page they were on. */
    temppage = in_use->showstr_page;
    paginate_string((in_use->showstr_head = strdup(*in_use->showstr_vector)), in_use);
    in_use->showstr_page = temppage;
  }

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
static int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  for (;;) {
    if (!fgets(tmp, READ_SIZE, fl))	/* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");	/* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp);	/* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  IN_ROOM(ch) = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->carry_weight = 0;
  ch->carry_items = 0;
  ch->time.logon = time(0);

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;
  if (GET_KI(ch) < 0)
    GET_KI(ch) = 0;

  GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  ch->size = SIZE_UNDEFINED;

  GET_ARMOR(ch) = 0;		/* Basic Armor */

}


void clear_object(struct obj_data *obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = NOWHERE;
}




/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL) {
    CREATE(ch->player_specials, struct player_special_data, 1);
    memset((void *) ch->player_specials, 0, sizeof(struct player_special_data));
  }

  GET_ADMLEVEL(ch) = ADMLVL_NONE;
  GET_CRANK(ch) = 0;
  GET_CLAN(ch) = strdup("None.");
  GET_ABSORBS(ch) = 0;
  ABSORBING(ch) = NULL;
  ABSORBBY(ch) = NULL;
  SITS(ch) = NULL;
  BLOCKED(ch) = NULL;
  BLOCKS(ch) = NULL;

  /* If this is our first player make him LVL_IMPL. */
  if (top_of_p_table == 0) {
    admin_set(ch, ADMLVL_IMPL);
    GET_CLASS_NONEPIC(ch, GET_CLASS(ch)) = GET_LEVEL(ch);

    /* The implementor never goes through do_start(). */
    GET_MAX_HIT(ch) = 1000;
    GET_MAX_MANA(ch) = 1000;
    GET_MAX_MOVE(ch) = 1000;
    GET_MAX_KI(ch) = 1000;
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_KI(ch) = GET_MAX_KI(ch);
  }

  set_title(ch, NULL);
  ch->short_descr = NULL;
  ch->long_descr = NULL;
  ch->description = NULL;

  /*ch->time.birth = time(0) - birth_age(ch);*/
  ch->time.logon = ch->time.created = time(0);
  ch->time.maxage = ch->time.birth + max_age(ch);
  ch->time.played = 0;

  GET_HOME(ch) = 1;
  GET_ARMOR(ch) = 0;

  set_height_and_weight_by_race(ch);

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i < SKILL_TABLE_SIZE; i++) {
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
    SET_SKILL_BONUS(ch, i, 0);
  }

  for (i = 0; i < AF_ARRAY_MAX; i++)
    AFF_FLAGS(ch)[i] = 0;

  for (i = 0; i < 3; i++)
    GET_SAVE_MOD(ch, i) = 0;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_ADMLEVEL(ch) == ADMLVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
  SPEAKING(ch) = SKILL_LANG_COMMON;
  GET_FEAT_POINTS(ch) = 1;
}

/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  room_rnum bot, top, mid, i, last_top;

  i = htree_find(room_htree, vnum);

  if (i != NOWHERE && world[i].number == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_world;

    /* perform binary search on world-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((world + mid)->number == vnum) {
        log("room_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(room_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOWHERE);
      if ((world + mid)->number > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}



/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  mob_rnum bot, top, mid, i, last_top;

  i = htree_find(mob_htree, vnum);

  if (i != NOBODY && mob_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_mobt;

    /* perform binary search on mob-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((mob_index + mid)->vnum == vnum) {
        log("mob_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(mob_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOBODY);
      if ((mob_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  obj_rnum bot, top, mid, i, last_top;

  i = htree_find(obj_htree, vnum);

  if (i != NOWHERE && obj_index[i].vnum == vnum)
    return i;
  else {
    bot = 0;
    top = top_of_objt;

    /* perform binary search on obj-table */
    for (;;) {
      last_top = top;
      mid = (bot + top) / 2;

      if ((obj_index + mid)->vnum == vnum) {
        log("obj_htree sync fix: %d: %d -> %d", vnum, i, mid);
        htree_add(obj_htree, vnum, mid);
        return (mid);
      }
      if (bot >= top)
        return (NOTHING);
      if ((obj_index + mid)->vnum > vnum)
        top = mid - 1;
      else
        bot = mid + 1;

      if (top > last_top)
        return NOWHERE;
    }
  }
}

/* returns the real number of the room with given virtual number */
zone_rnum real_zone(zone_vnum vnum)
{
  zone_rnum bot, top, mid, last_top;

  bot = 0;
  top = top_of_zone_table;

  /* perform binary search on world-table */
  for (;;) {
    last_top = top;
    mid = (bot + top) / 2;

    if ((zone_table + mid)->number == vnum)
      return (mid);
    if (bot >= top)
      return (NOWHERE);
    if ((zone_table + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
 
    if (top > last_top)
      return NOWHERE;
  }
}



/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
static int check_object(struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE, y;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative weight (%"I64T").",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", GET_OBJ_VNUM(obj), obj->short_description);
  for(y = 0; y < TW_ARRAY_MAX; y++) {
    error |= check_bitvector_names(GET_OBJ_WEAR(obj)[y], wear_bits_count, objname, "object wear");
    error |= check_bitvector_names(GET_OBJ_EXTRA(obj)[y], extra_bits_count, objname, "object extra");
    error |= check_bitvector_names(GET_OBJ_PERM(obj)[y], affected_bits_count, objname, "object affect");
  }

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
  {
    char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name, sizeof(onealias));
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE)) {
     //log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)", GET_OBJ_VNUM(obj), obj->short_description, obj->name);
    }
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if ((GET_OBJ_VAL(obj,0) > 0) && (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE)))
      log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		GET_OBJ_VNUM(obj), obj->short_description,
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }

  return (error);
}


static int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1 ||
      GET_OBJ_VAL(obj, val) == 0)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) >= SKILL_TABLE_SIZE)
    error = TRUE;
  if (skill_type(GET_OBJ_VAL(obj, val)) != SKTYPE_SPELL)
    error = TRUE;
  if (error)
    log("SYSERR: Object #%d (%s) has out of range spell #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !strcasecmp("UNDEFINED", spellname)) && (error = TRUE))
    log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
		GET_OBJ_VNUM(obj), obj->short_description, spellname,
		GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0) && (error = TRUE))
    log("SYSERR: Object #%d (%s) has out of range level #%d.",
	GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));

  return (error);
}

static int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1 << flagnum) & bits) {
      log("SYSERR: %s has unknown %s flag, bit %d (0 through %"SZT" known).", whatami, whatbits, flagnum, namecount - 1);
      error = TRUE;
    }

  return (error);
}

int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate)
{
  int counter2, i;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH +1];
  char ebuf0[MAX_STRING_LENGTH], ebuf1[MAX_STRING_LENGTH];
  char ebuf2[MAX_STRING_LENGTH], ebuf3[MAX_STRING_LENGTH];


  if (obj->action_description) {
    strcpy(buf1, obj->action_description);
    strip_string(buf1);
    } else
      *buf1 = 0;

    sprintascii(ebuf0, GET_OBJ_EXTRA(obj)[0]);
    sprintascii(ebuf1, GET_OBJ_EXTRA(obj)[1]);
    sprintascii(ebuf2, GET_OBJ_EXTRA(obj)[2]);
    sprintascii(ebuf3, GET_OBJ_EXTRA(obj)[3]);

    fprintf(fp,
      "#%d\n"
      "%d %d %d %d %d %d %d %d %d %s %s %s %s %d %d %d %d %d %d %d %d\n",
      GET_OBJ_VNUM(obj), locate, GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
      GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), 
      GET_OBJ_VAL(obj, 5), GET_OBJ_VAL(obj, 6), GET_OBJ_VAL(obj, 7),
      ebuf0, ebuf1, ebuf2, ebuf3,
      GET_OBJ_VAL(obj, 8), GET_OBJ_VAL(obj, 9),
      GET_OBJ_VAL(obj, 10), GET_OBJ_VAL(obj, 11), GET_OBJ_VAL(obj, 12),
      GET_OBJ_VAL(obj, 13), GET_OBJ_VAL(obj, 14), GET_OBJ_VAL(obj, 15));

  if (!(OBJ_FLAGGED(obj,ITEM_UNIQUE_SAVE)) && !GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
    return 1;
  }

  fprintf(fp,
    "XAP\n"
    "%s~\n"
    "%s~\n"
    "%s~\n"
    "%s~\n"
    "%d %d %d %d %d %"I64T" %d %d\n", obj->name ? obj->name : "undefined",
    obj->short_description ? obj->short_description : "undefined",
    obj->description ? obj->description : "undefined",
    buf1, GET_OBJ_TYPE(obj), GET_OBJ_WEAR(obj)[0], 
    GET_OBJ_WEAR(obj)[1], GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3], 
    GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));

  if (obj->generation)
  fprintf(fp, "G\n%ld\n", obj->generation);
  if (obj->unique_id)
  fprintf(fp, "U\n%"I64T"\n", obj->unique_id);

  fprintf(fp, "Z\n%d\n", GET_OBJ_SIZE(obj));

  /* Do we have affects? */
  for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
    if (obj->affected[counter2].modifier)
      fprintf(fp, "A\n"
        "%d %d %d\n",
        obj->affected[counter2].location, obj->affected[counter2].modifier,
        obj->affected[counter2].specific);

  /* Do we have extra descriptions? */
    if (obj->ex_description) {        /*. Yep, save them too . */
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
        /*. Sanity check to prevent nasty protection faults . */
        if (!*ex_desc->keyword || !*ex_desc->description) {
          continue;
        }
        strcpy(buf1, ex_desc->description);
        strip_string(buf1);
        fprintf(fp, "E\n"
          "%s~\n"
          "%s~\n",
          ex_desc->keyword,
          buf1);
      }
    }

  /* Do we have spells? */
    if (obj->sbinfo) {        /*. Yep, save them too . */
      for (i=0; i < SPELLBOOK_SIZE; i++) {
        if (obj->sbinfo[i].spellname == 0) {
          break;
        }
        fprintf(fp, "S\n" "%d %d\n", obj->sbinfo[i].spellname, obj->sbinfo[i].pages);
        continue;
      }
    }
    return 1;
}

 /* This procedure removes the '\r\n' from a string so that it may be
   saved to a file.  Use it only on buffers, not on the orginal
   strings. */

 void strip_string(char *buffer)
 {
   register char *ptr, *str;

   ptr = buffer;
   str = ptr;

   while ((*str = *ptr)) {
     str++;
     ptr++;
     if (*ptr == '\r')
       ptr++;
   }
 }

/* External variables from config.c */
extern int crash_file_timeout;
extern int rent_file_timeout;

void load_default_config( void )
{
  /****************************************************************************/
  /** This function is called only once, at boot-time.                       **/
  /** - We assume config_info is empty                          -- Welcor    **/
  /****************************************************************************/
  /****************************************************************************/
  /** Game play options.                                                     **/
  /****************************************************************************/
  CONFIG_PK_ALLOWED 	        = pk_allowed;
  CONFIG_PT_ALLOWED             = pt_allowed;
  CONFIG_LEVEL_CAN_SHOUT 	= level_can_shout;
  CONFIG_HOLLER_MOVE_COST 	= holler_move_cost;
  CONFIG_TUNNEL_SIZE 	        = tunnel_size;
  CONFIG_MAX_EXP_GAIN	        = max_exp_gain;
  CONFIG_MAX_EXP_LOSS 	        = max_exp_loss;
  CONFIG_MAX_NPC_CORPSE_TIME    = max_npc_corpse_time;
  CONFIG_MAX_PC_CORPSE_TIME	= max_pc_corpse_time;
  CONFIG_IDLE_VOID		= idle_void;
  CONFIG_IDLE_RENT_TIME	        = idle_rent_time;
  CONFIG_IDLE_MAX_LEVEL	        = idle_max_level;
  CONFIG_DTS_ARE_DUMPS	        = dts_are_dumps;
  CONFIG_LOAD_INVENTORY         = load_into_inventory;
  CONFIG_OK			= strdup(OK);
  CONFIG_NOPERSON		= strdup(NOPERSON);
  CONFIG_NOEFFECT		= strdup(NOEFFECT);
  CONFIG_TRACK_T_DOORS          = track_through_doors;
  CONFIG_LEVEL_CAP		= level_cap;
  CONFIG_STACK_MOBS		= show_mob_stacking;
  CONFIG_STACK_OBJS		= show_obj_stacking;
  CONFIG_MOB_FIGHTING           = mob_fighting;  
  CONFIG_DISP_CLOSED_DOORS      = disp_closed_doors;
  CONFIG_REROLL_PLAYER_CREATION       = reroll_status;
  CONFIG_INITIAL_POINTS_POOL	= initial_points;
  CONFIG_ENABLE_COMPRESSION	= enable_compression;
  CONFIG_ENABLE_LANGUAGES	= enable_languages;
  CONFIG_ALL_ITEMS_UNIQUE	= all_items_unique;
  CONFIG_EXP_MULTIPLIER		= exp_multiplier;
  /****************************************************************************/
  /** Rent / crashsave options.                                              **/
  /****************************************************************************/
  CONFIG_FREE_RENT              = free_rent;
  CONFIG_MAX_OBJ_SAVE           = max_obj_save;
  CONFIG_MIN_RENT_COST	        = min_rent_cost;
  CONFIG_AUTO_SAVE		= auto_save;
  CONFIG_AUTOSAVE_TIME	        = autosave_time;
  CONFIG_CRASH_TIMEOUT          = crash_file_timeout;
  CONFIG_RENT_TIMEOUT	        = rent_file_timeout;
  
  /****************************************************************************/
  /** Room numbers.                                                          **/
  /****************************************************************************/
  CONFIG_MORTAL_START           = mortal_start_room;
  CONFIG_IMMORTAL_START         = immort_start_room;
  CONFIG_FROZEN_START           = frozen_start_room;
  CONFIG_DON_ROOM_1             = donation_room_1;
  CONFIG_DON_ROOM_2             = donation_room_2;
  CONFIG_DON_ROOM_3             = donation_room_3;
  
  /****************************************************************************/
  /** Game operation options.                                                **/
  /****************************************************************************/
  CONFIG_DFLT_PORT              = DFLT_PORT;
  
  if (DFLT_IP)
    CONFIG_DFLT_IP              = strdup(DFLT_IP);
  else
    CONFIG_DFLT_IP              = NULL;
  
  CONFIG_DFLT_DIR               = strdup(DFLT_DIR);
  
  if (LOGNAME)
    CONFIG_LOGNAME              = strdup(LOGNAME);
  else
    CONFIG_LOGNAME              = NULL;
  
  CONFIG_MAX_PLAYING            = max_playing;
  CONFIG_MAX_FILESIZE           = max_filesize;
  CONFIG_MAX_BAD_PWS            = max_bad_pws;
  CONFIG_SITEOK_ALL             = siteok_everyone;
  CONFIG_NS_IS_SLOW             = nameserver_is_slow;
  CONFIG_NEW_SOCIALS            = use_new_socials;
  CONFIG_OLC_SAVE               = auto_save_olc;
  CONFIG_MENU                   = strdup(MENU);
  CONFIG_WELC_MESSG             = strdup(WELC_MESSG);
  CONFIG_START_MESSG            = strdup(START_MESSG);
  CONFIG_IMC_ENABLED            = imc_is_enabled;
  CONFIG_EXP_MULTIPLIER		= 1.0;
  
  /****************************************************************************/
  /** Autowiz options.                                                       **/
  /****************************************************************************/
  CONFIG_USE_AUTOWIZ            = use_autowiz;
  CONFIG_MIN_WIZLIST_LEV        = min_wizlist_lev;
  
  /****************************************************************************/
  /** Character advancement options.                                         **/
  /****************************************************************************/
  CONFIG_ALLOW_MULTICLASS	= allow_multiclass;
  CONFIG_ALLOW_PRESTIGE 	= allow_prestige;

  /****************************************************************************/
  /** ticks menu                                                             **/
  /****************************************************************************/
  CONFIG_PULSE_VIOLENCE         = pulse_violence;
  CONFIG_PULSE_MOBILE           = pulse_mobile;
  CONFIG_PULSE_ZONE             = pulse_zone;
  CONFIG_PULSE_CURRENT          = pulse_current;
  CONFIG_PULSE_SANITY           = pulse_sanity;
  CONFIG_PULSE_IDLEPWD          = pulse_idlepwd;
  CONFIG_PULSE_AUTOSAVE         = pulse_autosave;
  CONFIG_PULSE_USAGE            = pulse_usage;
  CONFIG_PULSE_TIMESAVE         = pulse_timesave;

  /****************************************************************************/
  /** Character Creation Method                                              **/
  /****************************************************************************/
  CONFIG_CREATION_METHOD	= method;
}

void load_config( void )
{
  FILE *fl;
  char line[MAX_STRING_LENGTH];
  char tag[MAX_INPUT_LENGTH];
  int  num;
  float  fum;
  char buf[MAX_INPUT_LENGTH];

  load_default_config();
  
  snprintf(buf, sizeof(buf), "%s/%s", DFLT_DIR, CONFIG_CONFFILE);
  if ( !(fl = fopen(CONFIG_CONFFILE, "r")) && !(fl = fopen(buf, "r")) ) {
    snprintf(buf, sizeof(buf), "Game Config File: %s", CONFIG_CONFFILE);
    perror(buf);
    return;
  }
  
  /****************************************************************************/
  /** Load the game configuration file.                                      **/
  /****************************************************************************/
  while (get_line(fl, line)) {
    split_argument(line, tag);
    num = atoi(line);
    fum = atof(line);
    
    switch (LOWER(*tag)) {
      case 'a':
        if (!strcasecmp(tag, "auto_save"))
          CONFIG_AUTO_SAVE = num;
        else if (!strcasecmp(tag, "autosave_time"))
          CONFIG_AUTOSAVE_TIME = num;
        else if (!strcasecmp(tag, "auto_save_olc"))
          CONFIG_OLC_SAVE = num;
        else if (!strcasecmp(tag, "allow_multiclass"))
          CONFIG_ALLOW_MULTICLASS = num;
        else if (!strcasecmp(tag, "allow_prestige"))
          CONFIG_ALLOW_PRESTIGE = num;
        else if (!strcasecmp(tag, "auto_level"))
          log("ignoring obsolete config option auto_level");
        else if (!strcasecmp(tag, "all_items_unique"))
          CONFIG_ALL_ITEMS_UNIQUE = num;
        break;
        
      case 'c':
        if (!strcasecmp(tag, "crash_file_timeout"))
          CONFIG_CRASH_TIMEOUT = num;
        else if (!strcasecmp(tag, "compression")) {
          CONFIG_ENABLE_COMPRESSION = num;
        }
        break;
        
      case 'd':
        if (!strcasecmp(tag, "disp_closed_doors"))
          CONFIG_DISP_CLOSED_DOORS = num;
        else if (!strcasecmp(tag, "dts_are_dumps"))
          CONFIG_DTS_ARE_DUMPS = num;
        else if (!strcasecmp(tag, "donation_room_1"))
          if (num == -1)
            CONFIG_DON_ROOM_1 = NOWHERE;
          else
            CONFIG_DON_ROOM_1 = num;
        else if (!strcasecmp(tag, "donation_room_2"))
          if (num == -1)
            CONFIG_DON_ROOM_2 = NOWHERE;
          else
            CONFIG_DON_ROOM_2 = num;
        else if (!strcasecmp(tag, "donation_room_3"))
          if (num == -1)
            CONFIG_DON_ROOM_3 = NOWHERE;
          else
            CONFIG_DON_ROOM_3 = num;
        else if (!strcasecmp(tag, "dflt_dir")) {
          if (CONFIG_DFLT_DIR)
            free(CONFIG_DFLT_DIR);
          if (line != NULL && *line)
            CONFIG_DFLT_DIR = strdup(line);
          else
            CONFIG_DFLT_DIR = strdup(DFLT_DIR);
        } else if (!strcasecmp(tag, "dflt_ip")) {
          if (CONFIG_DFLT_IP)
            free(CONFIG_DFLT_IP);
          if (line != NULL && *line)
            CONFIG_DFLT_IP = strdup(line);
          else
            CONFIG_DFLT_IP = NULL;
        } else if (!strcasecmp(tag, "dflt_port"))
          CONFIG_DFLT_PORT = num;
        break;
        
      case 'e':
        if (!strcasecmp(tag, "enable_languages"))
          CONFIG_ENABLE_LANGUAGES = num;
        else if (!strcasecmp(tag, "exp_multiplier"))
          CONFIG_EXP_MULTIPLIER = fum;
        break;

      case 'f':
        if (!strcasecmp(tag, "free_rent"))
          CONFIG_FREE_RENT = num;
        else if (!strcasecmp(tag, "frozen_start_room"))
          CONFIG_FROZEN_START = num;
        break;
        
      case 'h':
        if (!strcasecmp(tag, "holler_move_cost"))
          CONFIG_HOLLER_MOVE_COST = num;
        break;
        
      case 'i':
        if (!strcasecmp(tag, "idle_void"))
          CONFIG_IDLE_VOID = num;
        else if (!strcasecmp(tag, "idle_rent_time"))
          CONFIG_IDLE_RENT_TIME = num;
        else if (!strcasecmp(tag, "idle_max_level")) {
          if (num >= CONFIG_LEVEL_CAP)
            num += 1 - CONFIG_LEVEL_CAP;
          CONFIG_IDLE_MAX_LEVEL = num;
        } else if (!strcasecmp(tag, "immort_level_ok"))
          log("Ignoring immort_level_ok obsolete config");
        else if (!strcasecmp(tag, "immort_start_room"))
          CONFIG_IMMORTAL_START = num;
        else if (!strcasecmp(tag, "imc_enabled"))
          CONFIG_IMC_ENABLED = num;
        else if (!strcasecmp(tag, "initial_points"))
          CONFIG_INITIAL_POINTS_POOL = num;
        break;
        
      case 'l':
        if (!strcasecmp(tag, "level_can_shout"))
          CONFIG_LEVEL_CAN_SHOUT = num;
        else if (!strcasecmp(tag, "level_cap"))
          CONFIG_LEVEL_CAP = num;
        else if (!strcasecmp(tag, "load_into_inventory"))
          CONFIG_LOAD_INVENTORY = num;
        else if (!strcasecmp(tag, "logname")) {
          if (CONFIG_LOGNAME)
            free(CONFIG_LOGNAME);
          if (line != NULL && *line)
            CONFIG_LOGNAME = strdup(line);
          else
            CONFIG_LOGNAME = NULL;
        }
        break;
        
      case 'm':
        if (!strcasecmp(tag, "max_bad_pws"))
          CONFIG_MAX_BAD_PWS = num;
        else if (!strcasecmp(tag, "max_exp_gain"))
          CONFIG_MAX_EXP_GAIN = num;
        else if (!strcasecmp(tag, "max_exp_loss"))
          CONFIG_MAX_EXP_LOSS = num;
        else if (!strcasecmp(tag, "max_filesize"))
          CONFIG_MAX_FILESIZE = num;
        else if (!strcasecmp(tag, "max_npc_corpse_time"))
          CONFIG_MAX_NPC_CORPSE_TIME = num;
        else if (!strcasecmp(tag, "max_obj_save"))
          CONFIG_MAX_OBJ_SAVE = num;
        else if (!strcasecmp(tag, "max_pc_corpse_time"))
          CONFIG_MAX_PC_CORPSE_TIME = num;
        else if (!strcasecmp(tag, "max_playing"))
          CONFIG_MAX_PLAYING = num;
        else if (!strcasecmp(tag, "menu")) {
          if (CONFIG_MENU)
            free(CONFIG_MENU);
          strncpy(buf, "Reading menu in load_config()", sizeof(buf));
          CONFIG_MENU = fread_string(fl, buf);
        } else if (!strcasecmp(tag, "min_rent_cost"))
          CONFIG_MIN_RENT_COST = num;
        else if (!strcasecmp(tag, "min_wizlist_lev")) {
          if (num >= CONFIG_LEVEL_CAP)
            num += 1 - CONFIG_LEVEL_CAP;
          CONFIG_MIN_WIZLIST_LEV = num;
        }
        else if (!strcasecmp(tag, "mob_fighting"))
          CONFIG_MOB_FIGHTING = num;
        else if (!strcasecmp(tag, "mortal_start_room"))
          CONFIG_MORTAL_START = num;
        else if (!strcasecmp(tag, "method"))
          CONFIG_CREATION_METHOD = num;
        break;
        
      case 'n':
        if (!strcasecmp(tag, "nameserver_is_slow"))
          CONFIG_NS_IS_SLOW = num;
        else if (!strcasecmp(tag, "noperson")) {
          char tmp[READ_SIZE];
          if (CONFIG_NOPERSON)
            free(CONFIG_NOPERSON);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_NOPERSON = strdup(tmp);
        } else if (!strcasecmp(tag, "noeffect")) {
          char tmp[READ_SIZE];
          if (CONFIG_NOEFFECT)
            free(CONFIG_NOEFFECT);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_NOEFFECT = strdup(tmp);
        }
        break;
      
      case 'o':
        if (!strcasecmp(tag, "ok")) {
          char tmp[READ_SIZE];
          if (CONFIG_OK)
            free(CONFIG_OK);
          snprintf(tmp, sizeof(tmp), "%s\r\n", line);
          CONFIG_OK = strdup(tmp);
        }
        break;
                  
      case 'p':
        if (!strcasecmp(tag, "pk_allowed"))
          CONFIG_PK_ALLOWED = num;
        else if (!strcasecmp(tag, "pt_allowed"))
          CONFIG_PT_ALLOWED = num;
        else if (!strcasecmp(tag, "pulse_viol"))
          CONFIG_PULSE_VIOLENCE = num;
        else if (!strcasecmp(tag, "pulse_mobile"))
          CONFIG_PULSE_MOBILE = num;
        else if (!strcasecmp(tag, "pulse_current"))
          CONFIG_PULSE_CURRENT = num;
        else if (!strcasecmp(tag, "pulse_zone"))
          CONFIG_PULSE_ZONE = num;
        else if (!strcasecmp(tag, "pulse_autosave"))
          CONFIG_PULSE_AUTOSAVE = num;
        else if (!strcasecmp(tag, "pulse_usage"))
          CONFIG_PULSE_USAGE = num;
        else if (!strcasecmp(tag, "pulse_sanity"))
          CONFIG_PULSE_SANITY = num;
        else if (!strcasecmp(tag, "pulse_timesave"))
          CONFIG_PULSE_TIMESAVE = num;
        else if (!strcasecmp(tag, "pulse_idlepwd"))
          CONFIG_PULSE_IDLEPWD = num;
        break;
        
      case 'r':
        if (!strcasecmp(tag, "rent_file_timeout"))
          CONFIG_RENT_TIMEOUT = num;
        else if (!strcasecmp(tag, "reroll_stats"))
          CONFIG_REROLL_PLAYER_CREATION = num;
        break;
        
      case 's':
        if (!strcasecmp(tag, "siteok_everyone"))
          CONFIG_SITEOK_ALL = num;
        else if (!strcasecmp(tag, "start_messg")) {
          strncpy(buf, "Reading start message in load_config()", sizeof(buf));
          if (CONFIG_START_MESSG)
            free(CONFIG_START_MESSG);
          CONFIG_START_MESSG = fread_string(fl, buf);
        }
	else if (!strcasecmp(tag, "stack_mobs"))
	  CONFIG_STACK_MOBS = num;
	else if (!strcasecmp(tag, "stack_objs"))
	  CONFIG_STACK_OBJS = num;
        break;
        
      case 't':
        if (!strcasecmp(tag, "tunnel_size"))
          CONFIG_TUNNEL_SIZE = num;
        else if (!strcasecmp(tag, "track_through_doors"))
          CONFIG_TRACK_T_DOORS = num;
        break;
        
      case 'u':
        if (!strcasecmp(tag, "use_autowiz"))
          CONFIG_USE_AUTOWIZ = num;
        else if (!strcasecmp(tag, "use_new_socials"))
          CONFIG_NEW_SOCIALS = num;
        break;
        
      case 'w':
        if (!strcasecmp(tag, "welc_messg")) {
          strncpy(buf, "Reading welcome message in load_config()", sizeof(buf));
          if (CONFIG_WELC_MESSG)
            free(CONFIG_WELC_MESSG);
          CONFIG_WELC_MESSG = fread_string(fl, buf);
        }
        break;
        
      default:
        break;
    }
  }
  
  fclose(fl);
}


void read_level_data(struct char_data *ch, FILE *fl)
{
  char buf[READ_SIZE], *p;
  int i = 1;
  int t[16];
  struct levelup_data *curr = NULL;
  struct level_learn_entry *learn;

  ch->level_info = NULL;
  while (!feof(fl)) {
    i++;
    if (!get_line(fl, buf)) {
      log("read_level_data: get_line() failed reading level data line %d for %s", i, GET_NAME(ch));
      return;
    }
    for (p = buf; *p && *p != ' '; p++);
    if (!strcmp(buf, "end")) {
      return;
    }
    if (!*p) {
      log("read_level_data: malformed line reading level data line %d for %s: %s", i, GET_NAME(ch), buf);
      return;
    }
    *(p++) = 0;
    if (!strcmp(buf, "level")) {
      if (sscanf(p, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", t,t+1,t+2,t+3,
                 t+4,t+5,t+6,t+7,t+8,t+9,t+10,t+11,t+12,t+13,t+14,t+15) != 16) {
        log("read_level_data: missing fields on level_data line %d for %s", i, GET_NAME(ch));
        curr = NULL;
        continue;
      }
      CREATE(curr, struct levelup_data, 1);
      curr->prev = NULL;
      curr->next = ch->level_info;
      if ((curr->next = ch->level_info)) {
        curr->next->prev = curr;
      }
      ch->level_info = curr;
      curr->type = t[0];
      curr->spec = t[1];
      curr->level = t[2];
      curr->hp_roll = t[3];
      curr->mana_roll = t[4];
      curr->ki_roll = t[5];
      curr->move_roll = t[6];
      curr->fort = t[8];
      curr->reflex = t[9];
      curr->will = t[10];
      curr->add_skill = t[11];
      curr->add_gen_feats = t[12];
      curr->add_epic_feats = t[13];
      curr->add_class_feats = t[14];
      curr->add_class_epic_feats = t[15];
      curr->skills = curr->feats = NULL;
      continue;
    }
    if (!curr) {
      log("read_level_data: found continuation entry without current level for %s", GET_NAME(ch));
      continue;
    }
    if (sscanf(p, "%d %d %d", t, t+1, t+2) != 3) {
      log("read_level_data: missing fields on level_data %s line %d for %s", buf, i, GET_NAME(ch));
      continue;
    }
    CREATE(learn, struct level_learn_entry, 1);
    learn->location = t[0];
    learn->specific = t[1];
    learn->value = t[2];
    if (!strcmp(buf, "skill")) {
      learn->next = curr->skills;
      curr->skills = learn;
    } else if (!strcmp(buf, "feat")) {
      learn->next = curr->feats;
      curr->feats = learn;
    }
  }
  log("read_level_data: EOF reached reading level_data for %s", GET_NAME(ch));
  return;
}

void write_level_data(struct char_data *ch, FILE *fl)
{
  struct levelup_data *lev;
  struct level_learn_entry *learn;

  for (lev = ch->level_info; lev && lev->next; lev = lev->next);

  while (lev) {
    fprintf(fl, "level %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
            lev->type, lev->spec, lev->level, lev->hp_roll, lev->mana_roll,
            lev->ki_roll, lev->move_roll, lev->accuracy, lev->fort,
            lev->reflex, lev->will, lev->add_skill, lev->add_gen_feats,
            lev->add_epic_feats, lev->add_class_feats, lev->add_class_epic_feats);
    for (learn = lev->skills; learn; learn = learn->next)
      fprintf(fl, "skill %d %d %d", learn->location, learn->specific, learn->value);
    for (learn = lev->feats; learn; learn = learn->next)
      fprintf(fl, "feat %d %d %d", learn->location, learn->specific, learn->value);
    lev = lev->prev;
  }
  fprintf(fl, "end\n");
}

