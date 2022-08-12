/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __DB_H__
#define __DB_H__

#include "structs.h"


/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_TRG	6
#define DB_BOOT_GLD	7

#define LIB_USER        "user/"
#define LIB_INTRO       "intro/"
#define LIB_SENSE       "sense/"
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_PLRVARS	"plrvars/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_PLRFILES	"plrfiles/"
#define LIB_HOUSE	"house/"
#define LIB_PLRIMC      "plrimc/"
#define SLASH		"/"

#define SUF_OBJS	"new"
#define SUF_TEXT	"text"
#define SUF_ALIAS	"alias"
#define SUF_MEM	        "mem"
#define SUF_PLR	        "plr"
#define SUF_PET		"pet"
#define SUF_IMC         "imc"
#define SUF_USER        "usr"
#define SUF_INTRO       "itr"
#define SUF_SENSE       "sen"
#define SUF_CUSTOM      "cus"

#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	LIB_WORLD "wld" SLASH	/* room definitions	*/
#define MOB_PREFIX	LIB_WORLD "mob" SLASH	/* monster prototypes	*/
#define OBJ_PREFIX	LIB_WORLD "obj" SLASH	/* object prototypes	*/
#define ZON_PREFIX	LIB_WORLD "zon" SLASH	/* zon defs & command tables */
#define SHP_PREFIX	LIB_WORLD "shp" SLASH	/* shop definitions	*/
#define HLP_PREFIX	LIB_TEXT "help" SLASH	/* for HELP <keyword>	*/
#define TRG_PREFIX	LIB_WORLD "trg" SLASH	/* trigger files	*/
#define GLD_PREFIX	LIB_WORLD "gld" SLASH	/* guild files		*/

#define CREDITS_FILE	LIB_TEXT "credits" /* for the 'credits' command	*/
#define NEWS_FILE	LIB_TEXT "news"	/* for the 'news' command	*/
#define MOTD_FILE	LIB_TEXT "motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	LIB_TEXT "imotd"	/* messages of the day / immort	*/
#define GREETINGS_FILE	LIB_TEXT "greetings"	/* The opening screen.	*/
#define GREETANSI_FILE	LIB_TEXT "greetansi"	/* The opening screen.	*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP "screen"	/* for HELP <CR>	*/
#define CONTEXT_HELP_FILE LIB_TEXT "contexthelp"	/* The opening screen.	*/
#define INFO_FILE	LIB_TEXT "info"		/* for INFO		*/
#define WIZLIST_FILE	LIB_TEXT "wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE	LIB_TEXT "immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE	LIB_TEXT "background"/* for the background story	*/
#define POLICIES_FILE	LIB_TEXT "policies"  /* player policies/rules	*/
#define HANDBOOK_FILE	LIB_TEXT "handbook"  /* handbook for new immorts	*/
#define IHELP_PAGE_FILE	LIB_TEXT_HELP "iscreen"	/* for HELP <CR>	*/
#define HELP_FILE       "help.hlp"

#define IDEA_FILE	LIB_MISC "ideas"	   /* for the 'idea'-command	*/
#define TYPO_FILE	LIB_MISC "typos"	   /*         'typo'		*/
#define BUG_FILE	LIB_MISC "bugs"	   /*         'bug'		*/
#define REQUEST_FILE    LIB_MISC "request"  /*      RPP Requests         */
#define CUSTOM_FILE     LIB_MISC "customs"  /*      Custom EQ            */
#define MESS_FILE	LIB_MISC "messages" /* damage messages		*/
#define SOCMESS_FILE	LIB_MISC "socials"  /* messages for social acts	*/
#define SOCMESS_FILE_NEW LIB_MISC "socials.new"  /* messages for social acts with aedit patch*/
#define XNAME_FILE	LIB_MISC "xnames"   /* invalid name substrings	*/

#define CONFIG_FILE   LIB_ETC "config"      /* OasisOLC * GAME CONFIG FL */
#define PLAYER_FILE	LIB_ETC "players"   /* the player database	*/
#define MAIL_FILE	LIB_ETC "plrmail"   /* for the mudmail system	*/
#define BAN_FILE	LIB_ETC "badsites"  /* for the siteban system	*/
#define HCONTROL_FILE	LIB_ETC "hcontrol"  /* for the house system	*/
#define TIME_FILE	LIB_ETC "time"	   /* for calendar system	*/
#define AUCTION_FILE    LIB_ETC "auction"   /* for the auction house system */
#define ASSEMBLIES_FILE LIB_ETC "assemblies"/* for assemblies system 	*/
#define LEVEL_CONFIG	LIB_ETC "levels"	   /* set various level values  */

/* new bitvector data for use in player_index_element */
#define PINDEX_DELETED		(1 << 0)	/* deleted player	*/
#define PINDEX_NODELETE		(1 << 1)	/* protected player	*/
#define PINDEX_SELFDELETE	(1 << 2)	/* player is selfdeleting*/
#define PINDEX_NOWIZLIST	(1 << 3)	/* Player shouldn't be on wizlist*/


// global variables
extern struct time_info_data time_info;/* the infomation about the time    */
extern struct weather_data weather_info;	/* the infomation about the weather */
extern struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
extern struct reset_q_type reset_q;	/* queue of zones to be reset	 */
extern struct char_data *EDRAGON;
extern int WISH[2];
extern int DRAGONR, DRAGONZ, DRAGONC, SHENRON;
extern int circle_restrict;
extern struct help_index_element *help_table;
extern char *help, *ihelp, *credits, *news, *info, *wizlist, *immlist, *background;
extern char *policies, *handbook, *motd, *imotd, *GREETINGS, *GREETANSI;
extern int top_of_helpt, dballtime;
extern int mini_mud, no_rent_check, no_mail;
extern room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
extern room_rnum r_immort_start_room;	/* rnum of immort start room	 */
extern room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */

/* public procedures in db.c */
void boot_world(void);
int is_empty(zone_rnum zone_nr);
void index_boot(int mode);
void	boot_db(void);
void	destroy_db(void);
int	create_entry(char *name);
void	zone_update(void);
char	*fread_string(FILE *fl, const char *error);
long	get_id_by_name(const char *name);
char	*get_name_by_id(long id);
void	save_mud_time(struct time_info_data *when);
void	free_extra_descriptions(struct extra_descr_data *edesc);
void	free_text_files(void);
void	free_player_index(void);
void load_disabled(void);
void save_disabled(void);
void free_disabled(void);
void    free_help_table(void);
void    load_help(FILE *fl, char *name);
void auc_save(void);
void load_config(void);

zone_rnum real_zone(zone_vnum vnum);
room_rnum real_room(room_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);

int	load_char(const char *name, struct char_data *ch);
void	load_char_pets(struct char_data *ch);
void	save_char(struct char_data *ch);
void	save_char_pets(struct char_data *ch);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);
void	save_player_index(void);
long  get_ptable_by_name(const char *name);
void	read_level_data(struct char_data *ch, FILE *fl);
void	write_level_data(struct char_data *ch, FILE *fl);

int parse_mobile_from_file(FILE *mob_f, struct char_data *ch);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
struct obj_data *read_object(obj_vnum nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
int my_obj_save_to_disk(FILE *fp, struct obj_data *obj, int locate);
void add_unique_id(struct obj_data *obj);
void check_unique_id(struct obj_data *obj);
char *sprintuniques(int low, int high);
int	vnum_material(char *searchname, struct char_data *ch);
int	vnum_weapontype(char *searchname, struct char_data *ch);
int	vnum_armortype(char *searchname, struct char_data *ch);

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int  arg4;		/* room_max  default 0			*/
   int  arg5;           /* percentages variable                 */
   int line;		/* line number this command appears on  */
   char *sarg1;		/* string argument                      */
   char *sarg2;		/* string argument                      */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
	*  'T': Trigger command   *
        *  'V': Assign a variable *
   */
};



/* zone definition structure. for the 'zone-table'   */
#define CUR_WORLD_VERSION 1
#define CUR_ZONE_VERSION  2

struct zone_data {
   char	*name;		    /* name of this zone                  */
   char *builders;          /* namelist of builders allowed to    */
                            /* modify this zone.		  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   room_vnum bot;           /* starting room number for this zone */
   room_vnum top;           /* upper limit for rooms in this zone */

   int	reset_mode;         /* conditions for reset (see below)   */
   zone_vnum number;	    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */
   int min_level;           /* Minimum level to enter zone        */
   int max_level;           /* Max Mortal level to enter zone     */
   int zone_flags[ZF_ARRAY_MAX];          /* Flags for the zone.                */


   /*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
    */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};


/* Added level, flags, and last, primarily for pfile autocleaning.  You
   can also use them to keep online statistics, and can add race, class,
   etc if you like.
*/
struct player_index_element {
   char	*name;
   long id;
   int level;
   int admlevel;
   int flags;
   time_t last;
   int ship;
   int shiproom;
   time_t played;
   char *clan;
};


struct help_index_element {
   char *index;      /*Future Use */
   char *keywords;   /*Keyword Place holder and sorter */
   char *entry;      /*Entries for help files with Keywords at very top*/
   int duplicate;    /*Duplicate entries for multple keywords*/
   int min_level;    /*Min Level to read help entry*/
};


/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};


/* global buffering system */
extern time_t boot_time;

extern struct config_data config_info;

extern struct room_data *world;
extern room_rnum top_of_world;

extern struct zone_data *zone_table;
extern zone_rnum top_of_zone_table;

extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct char_data *affect_list;
extern struct char_data *affectv_list;
extern struct player_special_data dummy_mob;

extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern mob_rnum top_of_mobt;

extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern obj_rnum top_of_objt;

extern struct htree_node *room_htree;
extern struct htree_node *mob_htree;
extern struct htree_node *obj_htree;

extern struct social_messg *soc_mess_list;
extern int top_of_socialt;
extern struct index_data **trig_index;

extern struct index_data **trig_index;
extern struct trig_data *trigger_list;
extern int top_of_trigt;
extern long max_mob_id;
extern long max_obj_id;
extern int dg_owner_purged;
extern int xap_objs;

void strip_string(char *buffer);
int read_xap_objects(FILE *fl,struct char_data *ch);
bitvector_t asciiflag_conv(char *flag);
void reset_zone(zone_rnum zone);

/* For disabled commands code by Erwin S. Andreasen, */
/* ported to CircleMUD by Myrdred (Alexei Svitkine)  */

#define DISABLED_FILE    "disabled.cmds"  /* disabled commands */
#define END_MARKER    "END" /* for load_disabled() and save_disabled() */

typedef struct disabled_data DISABLED_DATA;

extern DISABLED_DATA *disabled_first; /* interpreter.c */

/* one disabled command */
struct disabled_data {
       DISABLED_DATA *next;                /* pointer to next node          */
       struct command_info const *command; /* pointer to the command struct */
       char *disabled_by;                  /* name of disabler              */
       int16_t level;                       /* level of disabler             */
       int subcmd;                         /* the subcmd, if any            */
};

// commands
ACMD(do_reboot);

#endif