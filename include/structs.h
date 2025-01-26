#pragma once
/* ************************************************************************
 *   File: structs.h                                     Part of CircleMUD *
 *  Usage: header file for central structures and constants                *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */
#include "defs.h"

/**********************************************************************
 * Structures                                                          *
 **********************************************************************/

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
    char *keyword; /* Keyword in look/examine          */
    char *description; /* What to see                      */
    struct extra_descr_data *next; /* Next in list                     */
};

struct obj_affected_type {
    int location; /* Which ability to change (APPLY_XXX) */
    int specific; /* Some locations have parameters      */
    int modifier; /* How much it changes by              */
};

struct obj_spellbook_spell {
    int spellname; /* Which spell is written */
    int pages; /* How many pages does it take up */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
    obj_vnum item_number; /* Where in data-base			*/
    room_rnum in_room; /* In what room -1 when conta/carr	*/
    room_vnum room_loaded; /* Room loaded in, for room_max checks	*/

    int value[NUM_OBJ_VAL_POSITIONS]; /* Values of the item (see list)    */
    int8_t type_flag; /* Type of item                        */
    int level; /* Minimum level of object.            */
    bitvector_t wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
    bitvector_t extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
    int64_t weight; /* Weigt what else                     */
    int cost; /* Value when sold (gp.)               */
    int cost_per_day; /* Cost to keep pr. real day           */
    int timer; /* Timer for object                    */
    bitvector_t bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
    int size; /* Size class of object                */

    struct obj_affected_type affected[MAX_OBJ_AFFECT]; /* affects */

    char *name; /* Title of object :get etc.        */
    char *description; /* When in room                     */
    char *short_description; /* when worn/carry/in cont.         */
    char *action_description; /* What to write when used          */
    struct extra_descr_data *ex_description; /* extra descriptions     */
    struct char_data *carried_by; /* Carried by :NULL in room/conta   */
    struct char_data *worn_by; /* Worn by?			      */
    int16_t worn_on; /* Worn where?		      */

    struct obj_data *in_obj; /* In what object NULL when none    */
    struct obj_data *contains; /* Contains objects                 */

    int32_t id; /* used by DG triggers              */
    time_t generation; /* creation time for dupe check     */
    int64_t unique_id; /* random bits for dupe check       */

    struct trig_proto_list *proto_script; /* list of default triggers  */
    struct script_data *script; /* script info for the object       */

    struct obj_data *next_content; /* For 'contains' lists             */
    struct obj_data *next; /* For the object list              */

    struct obj_spellbook_spell *sbinfo; /* For spellbook info */
    struct char_data *sitting; /* Who is sitting on me? */
    int scoutfreq;
    time_t lload;
    int healcharge;
    int64_t kicharge;
    int kitype;
    struct char_data *user;
    struct char_data *target;
    int distance;
    int foob;
    int32_t aucter;
    int32_t curBidder;
    time_t aucTime;
    int bid;
    int startbid;
    char *auctname;
    int posttype;
    struct obj_data *posted_to;
    struct obj_data *fellow_wall;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
    char *general_description; /* When look DIR.			*/

    char *keyword; /* for open/close			*/

    int16_t exit_info; /* Exit info			*/
    obj_vnum key; /* Key's number (-1 for no key)		*/
    room_rnum to_room; /* Where direction leads (NOWHERE)	*/
    int dclock; /* DC to pick the lock			*/
    int dchide; /* DC to find hidden			*/
    int dcskill; /* Skill req. to move through exit	*/
    int dcmove; /* DC for skill to move through exit	*/
    int failsavetype; /* Saving Throw type on skill fail	*/
    int dcfailsave; /* DC to save against on fail		*/
    int failroom; /* Room # to put char in when fail > 5  */
    int totalfailroom; /* Room # if char fails save < 5	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
    room_vnum number; /* Rooms number	(vnum)		      */
    zone_rnum zone; /* Room zone (for resetting)          */
    int sector_type; /* sector type (move/hide)            */
    char *name; /* Rooms name 'You are ...'           */
    char *description; /* Shown when entered                 */
    struct extra_descr_data *ex_description; /* for examine/look       */
    struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
    bitvector_t room_flags[RF_ARRAY_MAX]; /* DEATH,DARK ... etc */

    struct trig_proto_list *proto_script; /* list of default triggers  */
    struct script_data *script; /* script info for the object         */

    int8_t light; /* Number of lightsources in room     */
    SPECIAL(*func);

    struct obj_data *contents; /* List of items in room              */
    struct char_data *people; /* List of NPC / PC in room           */

    int timed; /* For timed Dt's                     */
    int dmg; /* How damaged the room is            */
    int gravity; /* What is the level of gravity?      */
    int geffect; /* Effect of ground destruction       */
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
    int32_t id;
    struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
    int hours, day, month;
    int16_t year;
};


/* These data contain information about a players time data */
struct time_data {
    time_t birth; /* This represents the characters current age        */
    time_t created; /* This does not change                              */
    time_t maxage; /* This represents death by natural causes           */
    time_t logon; /* Time of the last logon (used to calculate played) */
    time_t played; /* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
    int level; /* max level for this time limit	*/
    int days; /* time limit in days			*/
};


/* Char's abilities. */
struct abil_data {
    int8_t str; /* New stats can go over 18 freely, no more /xx */
    int8_t intel;
    int8_t wis;
    int8_t dex;
    int8_t con;
    int8_t cha;
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct player_special_data {
    char *poofin; /* Description on arrival of a god.     */
    char *poofout; /* Description upon a god's exit.       */
    struct alias_data *aliases; /* Character's aliases                  */
    int32_t last_tell; /* idnum of last tell from              */
    void *last_olc_targ; /* olc control                          */
    int last_olc_mode; /* olc control                          */
    char *host; /* host of last logon                   */
    struct imcchar_data *imcchardata; /**< IMC2 Data */
    int spell_level[MAX_SPELL_LEVEL];
    /* bonus to number of spells memorized */
    int memcursor; /* points to the next free slot in spellmem */
    int wimp_level; /* Below this # of hit points, flee!	*/
    int8_t freeze_level; /* Level of god who froze char, if any	*/
    int16_t invis_level; /* level of invisibility		*/
    room_vnum load_room; /* Which room to place char in		*/
    bitvector_t pref[PR_ARRAY_MAX]; /* preference flags for PC's.		*/
    uint8_t bad_pws; /* number of bad password attemps	*/
    int8_t conditions[NUM_CONDITIONS]; /* Drunk, full, thirsty			*/
    int skill_points; /* Skill points earned from race HD	*/
    int class_skill_points[NUM_CLASSES];
    /* Skill points earned from a class	*/
    struct txt_block *comm_hist[NUM_HIST]; /* Player's communcations history     */
    int olc_zone; /* Zone where OLC is permitted		*/
    int gauntlet; /* Highest Gauntlet Position */
    int speaking; /* Language currently speaking		*/
    int tlevel; /* Turning level			*/
    int ability_trains; /* How many stat points can you train?	*/
    int spellmem[MAX_MEM]; /* Spell slots				*/
    int feat_points; /* How many general feats you can take	*/
    int epic_feat_points; /* How many epic feats you can take	*/
    int class_feat_points[NUM_CLASSES];
    /* How many class feats you can take	*/
    int epic_class_feat_points[NUM_CLASSES];
    /* How many epic class feats 		*/
    int domain[NUM_DOMAINS];
    int school[NUM_SCHOOLS];
    int deity;
    int spell_mastery_points;
    char *color_choices[NUM_COLOR]; /* Choices for custom colors		*/
    uint8_t page_length;
    int murder; /* Murder of PC's count                 */
    int trainstr;
    int trainint;
    int traincon;
    int trainwis;
    int trainagl;
    int trainspd;

    struct char_data *carrying;
    struct char_data *carried_by;

    int racial_pref;

};


/* this can be used for skills that can be used per-day */
struct memorize_node {
    int timer; /* how many ticks till memorized */
    int spell; /* the spell number */
    struct memorize_node *next; /* link to the next node */
};

struct innate_node {
    int timer;
    int spellnum;
    struct innate_node *next;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    memory_rec *memory; /* List of attackers to remember	       */
    int8_t attack_type; /* The Attack Type Bitvector for NPC's     */
    int8_t default_pos; /* Default position for NPC                */
    int8_t damnodice; /* The number of damage dice's	       */
    int8_t damsizedice; /* The size of the damage dice's           */
    int newitem; /* Check if mob has new inv item       */
};


/* An affect structure. */
struct affected_type {
    int16_t type; /* The type of spell that caused this      */
    int16_t duration; /* For how long its effects will last      */
    int modifier; /* This is added to apropriate ability     */
    int location; /* Tells which ability to change(APPLY_XXX)*/
    int specific; /* Some locations have parameters          */
    bitvector_t bitvector; /* Tells which bits to set (AFF_XXX) */

    struct affected_type *next;
};

/* Queued spell entry */
struct queued_act {
    int level;
    int spellnum;
};

/* Structure used for chars following other chars */
struct follow_type {
    struct char_data *follower;
    struct follow_type *next;
};


struct level_learn_entry {
    struct level_learn_entry *next;
    int location;
    int specific;
    int8_t value;
};

struct levelup_data {
    struct levelup_data *next; /* Form a linked list			*/
    struct levelup_data *prev; /* Form a linked list			*/
    int8_t type; /* LEVELTYPE_ value			*/
    int8_t spec; /* Specific class or race		*/
    int8_t level; /* Level ir HD # for that class or race	*/

    int8_t hp_roll; /* Straight die-roll value with no mods	*/
    int8_t mana_roll; /* Straight die-roll value with no mods	*/
    int8_t ki_roll; /* Straight die-roll value with no mods	*/
    int8_t move_roll; /* Straight die-roll value with no mods	*/

    int8_t accuracy; /* Hit accuracy change			*/
    int8_t fort; /* Fortitude change			*/
    int8_t reflex; /* Reflex change			*/
    int8_t will; /* Will change				*/

    int8_t add_skill; /* Total added skill points		*/
    int8_t add_gen_feats; /* General feat points			*/
    int8_t add_epic_feats; /* General epic feat points		*/
    int8_t add_class_feats; /* Class feat points			*/
    int8_t add_class_epic_feats; /* Epic class feat points		*/

    struct level_learn_entry *skills; /* Head of linked list		*/
    struct level_learn_entry *feats; /* Head of linked list		*/
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
    int pfilepos; /* playerfile pos			*/
    mob_rnum nr; /* Mob's rnum				*/
    room_rnum in_room; /* Location (real room number)		*/
    room_rnum was_in_room; /* location for linkdead people		*/
    int wait; /* wait for how many loops		*/

    char *name; /* PC / NPC s name (kill ...  )		*/
    char *short_descr; /* for NPC 'actions'			*/
    char *long_descr; /* for 'look'				*/
    char *description; /* Extra descriptions                   */
    char *title; /* PC / NPC's title                     */
    int size; /* Size class of char                   */
    int8_t sex; /* PC / NPC's sex                       */
    int8_t race; /* PC / NPC's race                      */
    int8_t hairl; /* PC hair length                       */
    int8_t hairs; /* PC hair style                        */
    int8_t hairc; /* PC hair color                        */
    int8_t skin; /* PC skin color                        */
    int8_t eye; /* PC eye color                         */
    int8_t distfea; /* PC's Distinguishing Feature          */
    int race_level; /* PC / NPC's racial level / hit dice   */
    int level_adj; /* PC level adjustment                  */
    int8_t chclass; /* Last class taken                     */
    int chclasses[NUM_CLASSES]; /* Ranks in all classes        */
    int epicclasses[NUM_CLASSES]; /* Ranks in all epic classes */
    struct levelup_data *level_info;
    /* Info on gained levels */
    int level; /* PC / NPC's level                     */
    int admlevel; /* PC / NPC's admin level               */
    bitvector_t admflags[AD_ARRAY_MAX]; /* Bitvector for admin privs		*/
    room_vnum hometown; /* PC Hometown / NPC spawn room         */
    struct time_data time; /* PC's AGE in days			*/
    uint8_t weight; /* PC / NPC's weight                    */
    uint8_t height; /* PC / NPC's height                    */

    struct abil_data real_abils; /* Abilities without modifiers   */
    struct abil_data aff_abils; /* Abils with spells/stones/etc  */
    struct player_special_data *player_specials;
    /* PC specials				*/
    struct mob_special_data mob_specials;
    /* NPC specials				*/

    struct affected_type *affected;
    /* affected by what spells		*/
    struct affected_type *affectedv;
    /* affected by what combat spells	*/
    struct queued_act *actq; /* queued spells / other actions	*/

    struct obj_data *equipment[NUM_WEARS];
    /* Equipment array			*/
    struct obj_data *carrying; /* Head of list				*/

    struct descriptor_data *desc; /* NULL for mobiles			*/
    int32_t id; /* used by DG triggers			*/

    struct trig_proto_list *proto_script;
    /* list of default triggers		*/
    struct script_data *script; /* script info for the object		*/
    struct script_memory *memory; /* for mob memory triggers		*/

    struct char_data *next_in_room;
    /* For room->people - list		*/
    struct char_data *next; /* For either monster or ppl-list	*/
    struct char_data *next_fighting;
    /* For fighting list			*/
    struct char_data *next_affect; /* For affect wearoff			*/
    struct char_data *next_affectv;
    /* For round based affect wearoff	*/

    struct follow_type *followers; /* List of chars followers		*/
    struct char_data *master; /* Who is char following?		*/
    int32_t master_id;

    struct memorize_node *memorized;
    struct innate_node *innate;

    struct char_data *fighting; /* Opponent				*/

    int8_t position; /* Standing, fighting, sleeping, etc.	*/

    int carry_weight; /* Carried weight			*/
    int8_t carry_items; /* Number of items carried		*/
    int timer; /* Timer for update			*/

    struct obj_data *sits; /* What am I sitting on? */
    struct char_data *blocks; /* Who am I blocking?    */
    struct char_data *blocked; /* Who is blocking me?    */
    struct char_data *absorbing; /* Who am I absorbing */
    struct char_data *absorbby; /* Who is absorbing me */

    int8_t feats[MAX_FEATS + 1]; /* Feats (booleans and counters)	*/
    int combat_feats[CFEAT_MAX + 1][FT_ARRAY_MAX];
    /* One bitvector array per CFEAT_ type	*/
    int school_feats[SFEAT_MAX + 1]; /* One bitvector array per CFEAT_ type	*/

    int8_t skills[SKILL_TABLE_SIZE + 1];
    /* array of skills/spells/arts/etc	*/
    int8_t skillmods[SKILL_TABLE_SIZE + 1];
    /* array of skill mods			*/
    int8_t skillperfs[SKILL_TABLE_SIZE + 1];
    /* array of skill mods                  */

    int alignment; /* +-1000 for alignment good vs. evil	*/
    int alignment_ethic; /* +-1000 for alignment law vs. chaos	*/
    int32_t idnum; /* player's idnum; -1 for mobiles	*/

    bitvector_t act[PM_ARRAY_MAX]; /* act flag for NPC's; player flag for PC's */
    bitvector_t affected_by[AF_ARRAY_MAX]; /* Bitvector for current affects	*/
    bitvector_t bodyparts[AF_ARRAY_MAX]; /* Bitvector for current bodyparts      */
    int16_t saving_throw[3]; /* Saving throw				*/
    int16_t apply_saving_throw[3]; /* Saving throw bonuses			*/

    int powerattack; /* Setting for power attack level	*/
    int combatexpertise; /* Setting for Combat expertise level   */

    int64_t baseki;
    int64_t mana;
    int64_t max_mana; /* Max mana for PC/NPC			*/

    int64_t basepl;
    int64_t hit;
    int64_t max_hit; /* Max hit for PC/NPC			*/

    int64_t basest;
    int64_t move;
    int64_t max_move; /* Max move for PC/NPC			*/

    bool bonuses[MAX_BONUSES];
    int armor; /* Internally stored *10		*/
    int16_t shield_bonus; /* Shield bonus for AC			*/
    int gold; /* Money carried			*/
    int bank_gold; /* Gold the char has in a bank account	*/
    int64_t exp; /* The experience of the player		*/

    int accuracy; /* Base hit accuracy			*/
    int accuracy_mod; /* Any bonus or penalty to the accuracy	*/
    int damage_mod; /* Any bonus or penalty to the damage	*/

    int16_t spellfail; /* Total spell failure %                 */
    int16_t armorcheck; /* Total armorcheck penalty with proficiency forgiveness */
    int16_t armorcheckall; /* Total armorcheck penalty regardless of proficiency */

    /* All below added by Iovan for sure o.o */ int64_t charge;
    int64_t chargeto;
    int64_t barrier;

    char *clan;

    room_vnum droom;
    int choice;
    int sleeptime;
    int foodr;
    int altitude;
    int overf;
    int spam;

    room_vnum radar1;
    room_vnum radar2;
    room_vnum radar3;
    int ship;
    room_vnum shipr;
    time_t lastpl;
    time_t lboard[5];

    room_vnum listenroom;
    int crank;
    int kaioken;
    int absorbs;
    int boosts;
    int upgrade;
    time_t lastint;
    int majinize;
    short fury;
    short btime;
    int eavesdir;
    time_t deathtime;
    int rp;
    int64_t suppression;
    int64_t suppressed;
    struct char_data *drag;
    struct char_data *dragged;
    int trp;
    struct char_data *mindlink;
    int lasthit;
    int dcount;
    char *voice; /* PC's snet voice */
    int limbs[5]; /* 1 Right Arm, 2 Left Arm, 3 Right Leg, 4 Left Leg, existing code starts at 1 so we don't use 0. */
    int aura;
    time_t rewtime;
    struct char_data *grappling;
    struct char_data *grappled;
    int grap;
    int genome[2]; /* Bio racial bonus, Genome */
    int combo;
    int lastattack;
    int combhits;
    int ping;
    int starphase;
    int mimic;

    int ccpoints;
    int negcount;
    int cooldown;
    int death_type;

    int64_t moltexp;
    int moltlevel;

    char *loguser; /* What user was I last saved as?      */
    int arenawatch;
    int64_t majinizer;
    int speedboost;
    int skill_slots;
    int tail_growth;
    int rage_meter;
    char *feature;
    int transclass;
    int transcost[6];
    int armor_last;
    int forgeting;
    int forgetcount;
    int backstabcool;
    int con_cooldown;
    short stupidkiss;
    char *temp_prompt;

    int personality;
    int combine;
    int linker;
    int fishstate;
    int throws;

    struct char_data *defender;
    struct char_data *defending;

    int64_t lifeforce;
    int lifeperc;
    int gooptime;
    int blesslvl;
    struct char_data *poisonby;

    int mobcharge;
    int preference;
    int aggtimer;

    int lifebonus;
    int asb;
    int regen;
    int rbank;
    int con_sdcooldown;

    int limb_condition[4];

    char placeholder[2];

    char *rdisplay;

    short song;
    struct char_data *original;
    short clones;
    int relax_count;
    int ingestLearned;
};

/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
    char *text;
    int aliased;
    struct txt_block *next;
};


struct txt_q {
    struct txt_block *head;
    struct txt_block *tail;
};

struct compr {
    int state; /* 0 - off. 1 - waiting for response. 2 - compress2 on */

    Bytef *buff_out;
    int total_out; /* size of input buffer */
    int size_out; /* size of data in output buffer */

    Bytef *buff_in;
    int total_in; /* size of input buffer */
    int size_in; /* size of data in input buffer */

    z_streamp stream;
};

struct descriptor_data {
    socklen_t descriptor; /* file descriptor for socket		*/
    char host[HOST_LENGTH + 1]; /* hostname				*/
    int8_t bad_pws; /* number of bad pw attemps this login	*/
    int8_t idle_tics; /* tics idle at password prompt		*/
    int connected; /* mode of 'connectedness'		*/
    int desc_num; /* unique num assigned to desc		*/
    time_t login_time; /* when the person connected		*/
    char *showstr_head; /* for keeping track of an internal str	*/
    char **showstr_vector; /* for paging through texts		*/
    int showstr_count; /* number of pages to page through	*/
    int showstr_page; /* which page are we currently showing?	*/
    char **str; /* for the modify-str system		*/
    char *backstr; /* backup string for modify-str system	*/
    size_t max_str; /* maximum size of string in modify-str	*/
    int32_t mail_to; /* name for mail system			*/
    int has_prompt; /* is the user at a prompt?             */
    char inbuf[MAX_RAW_INPUT_LENGTH]; /* buffer for raw input		*/
    char last_input[MAX_INPUT_LENGTH]; /* the last input			*/
    char small_outbuf[SMALL_BUFSIZE]; /* standard output buffer		*/
    char *output; /* ptr to the current output buffer	*/
    char **history; /* History of commands, for ! mostly.	*/
    int history_pos; /* Circular array position.		*/
    int bufptr; /* ptr to end of current output		*/
    int bufspace; /* space left in the output buffer	*/
    struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
    struct txt_q input; /* q of unprocessed input		*/
    struct char_data *character; /* linked to char			*/
    struct char_data *original; /* original char if switched		*/
    struct descriptor_data *snooping; /* Who is this char snooping	*/
    struct descriptor_data *snoop_by; /* And who is snooping this char	*/
    struct descriptor_data *next; /* link to next descriptor		*/
    struct oasis_olc_data *olc; /* OLC info                            */
    struct compr *comp; /* compression info */
    char *user; /* What user am I?                     */
    char *email; /* User Account Email.                 */
    char *pass; /* User Account Password.              */
    char *loadplay; /* What character am I loading?        */
    int writenew; /* What slot am I writing to?          */
    int total; /* What Is My Total Character Limit?   */
    int rpp; /* What is my total RPP?               */
    char *tmp1;
    char *tmp2;
    char *tmp3;
    char *tmp4;
    char *tmp5;
    int level;
    char *newsbuf;
    /*---------------Player Level Object Editing Variables-------------------*/
    int obj_editval;
    int obj_editflag;
    char *obj_was;
    char *obj_name;
    char *obj_short;
    char *obj_long;
    int obj_type;
    int obj_weapon;
    struct obj_data *obj_point;
    /*---------------Ship Construction Editing Variables---------------------*/
    int shipmenu;
    int shipsize;
    char *ship_name;
    int shipextra[4];
    int shields;
    int armor;
    int drive;
    int shipweap;
    /*-----------------------------------------------------------------------*/
    int user_freed;
    int customfile;
    char *title;
    int rbank;
};


/* other miscellaneous structures ***************************************/


struct msg_type {
    char *attacker_msg; /* message to attacker */
    char *victim_msg; /* message to victim   */
    char *room_msg; /* message to room     */
};


struct message_type {
    struct msg_type die_msg; /* messages when death			*/
    struct msg_type miss_msg; /* messages when miss			*/
    struct msg_type hit_msg; /* messages when hit			*/
    struct msg_type god_msg; /* messages when hit on god		*/
    struct message_type *next; /* to next messages of this kind.	*/
};


struct message_list {
    int a_type; /* Attack type				*/
    int number_of_attacks; /* How many attack messages to chose from. */
    struct message_type *msg; /* List of messages.			*/
};

/* used in the socials */
struct social_messg {
    int act_nr;
    char *command; /* holds copy of activating command */
    char *sort_as; /* holds a copy of a similar command or
                    * abbreviation to sort by for the parser */
    int hide; /* ? */
    int min_victim_position; /* Position of victim */
    int min_char_position; /* Position of char */
    int min_level_char; /* Minimum level of socialing char */

    /* No argument was supplied */
    char *char_no_arg;
    char *others_no_arg;

    /* An argument was there, and a victim was found */
    char *char_found;
    char *others_found;
    char *vict_found;

    /* An argument was there, as well as a body part, and a victim was found */
    char *char_body_found;
    char *others_body_found;
    char *vict_body_found;

    /* An argument was there, but no victim was found */
    char *not_found;

    /* The victim turned out to be the character */
    char *char_auto;
    char *others_auto;

    /* If the char cant be found search the char's inven and do these: */
    char *char_obj_found;
    char *others_obj_found;
};


struct weather_data {
    int pressure; /* How is the pressure ( Mb ) */
    int change; /* How fast and what way does it change. */
    int sky; /* How is the sky. */
    int sunlight; /* And how much sun. */
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
    mob_vnum vnum; /* virtual number of this mob/obj		*/
    int number; /* number of existing units of this mob/obj	*/
    SPECIAL(*func);

    char *farg; /* string argument for special function     */
    struct trig_data *proto; /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
    int vnum; /* vnum of the trigger   */
    struct trig_proto_list *next; /* next trigger          */
};

struct guild_info_type {
    int pc_class;
    room_vnum guild_room;
    int direction;
};

/*
 * Config structs
 *
 */

/*
 * The game configuration structure used for configurating the game play
 * variables.
 */
struct game_data {
    int pk_allowed; /* Is player killing allowed? 	  */
    int pt_allowed; /* Is player thieving allowed?	  */
    int level_can_shout; /* Level player must be to shout.	  */
    int holler_move_cost; /* Cost to holler in move points.	  */
    int tunnel_size; /* Number of people allowed in a tunnel.*/
    int max_exp_gain; /* Maximum experience gainable per kill.*/
    int max_exp_loss; /* Maximum experience losable per death.*/
    int max_npc_corpse_time; /* Num tics before NPC corpses decompose*/
    int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
    int idle_void; /* Num tics before PC sent to void(idle)*/
    int idle_rent_time; /* Num tics before PC is autorented.	  */
    int idle_max_level; /* Level of players immune to idle.     */
    int dts_are_dumps; /* Should items in dt's be junked?	  */
    int load_into_inventory; /* Objects load in immortals inventory. */
    int track_through_doors; /* Track through doors while closed?    */
    int level_cap; /* You cannot level to this level       */
    int stack_mobs; /* Turn mob stacking on                 */
    int stack_objs; /* Turn obj stacking on                 */
    int mob_fighting; /* Allow mobs to attack other mobs.     */
    char *OK; /* When player receives 'Okay.' text.	  */
    char *NOPERSON; /* 'No-one by that name here.'	  */
    char *NOEFFECT; /* 'Nothing seems to happen.'	          */
    int disp_closed_doors; /* Display closed doors in autoexit?	  */
    int reroll_player; /* Players can reroll stats on creation */
    int initial_points; /* Initial points pool size		  */
    int enable_compression; /* Enable MCCP2 stream compression      */
    int enable_languages; /* Enable spoken languages              */
    int all_items_unique; /* Treat all items as unique 		  */
    float exp_multiplier; /* Experience gain  multiplier	  */
};


/*
 * The rent and crashsave options.
 */
struct crash_save_data {
    int free_rent; /* Should the MUD allow rent for free?  */
    int max_obj_save; /* Max items players can rent.          */
    int min_rent_cost; /* surcharge on top of item costs.	  */
    int auto_save; /* Does the game automatically save ppl?*/
    int autosave_time; /* if auto_save=TRUE, how often?        */
    int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
    int rent_file_timeout; /* Lifetime of normal rent files in days*/
};


/*
 * The room numbers.
 */
struct room_numbers {
    room_vnum mortal_start_room; /* vnum of room that mortals enter at.  */
    room_vnum immort_start_room; /* vnum of room that immorts enter at.  */
    room_vnum frozen_start_room; /* vnum of room that frozen ppl enter.  */
    room_vnum donation_room_1; /* vnum of donation room #1.            */
    room_vnum donation_room_2; /* vnum of donation room #2.            */
    room_vnum donation_room_3; /* vnum of donation room #3.	        */
};


/*
 * The game operational constants.
 */
struct game_operation {
    uint16_t DFLT_PORT; /* The default port to run the game.  */
    char *DFLT_IP; /* Bind to all interfaces.		  */
    char *DFLT_DIR; /* The default directory (lib).	  */
    char *LOGNAME; /* The file to log messages to.	  */
    int max_playing; /* Maximum number of players allowed. */
    int max_filesize; /* Maximum size of misc files.	  */
    int max_bad_pws; /* Maximum number of pword attempts.  */
    int siteok_everyone; /* Everyone from all sites are SITEOK.*/
    int nameserver_is_slow; /* Is the nameserver slow or fast?	  */
    int use_new_socials; /* Use new or old socials file ?      */
    int auto_save_olc; /* Does OLC save to disk right away ? */
    char *MENU; /* The MAIN MENU.			  */
    char *WELC_MESSG; /* The welcome message.		  */
    char *START_MESSG; /* The start msg for new characters.  */
    int imc_enabled; /**< Is connection to IMC allowed ? */
};

/*
 * The Autowizard options.
 */
struct autowiz_data {
    int use_autowiz; /* Use the autowiz feature?		*/
    int min_wizlist_lev; /* Minimun level to show on wizlist.	*/
};

/* This is for the tick system.
 *
 */

struct tick_data {
    int pulse_violence;
    int pulse_mobile;
    int pulse_zone;
    int pulse_autosave;
    int pulse_idlepwd;
    int pulse_sanity;
    int pulse_usage;
    int pulse_timesave;
    int pulse_current;
};

/*
 * The character advancement (leveling) options.
 */
struct advance_data {
    int allow_multiclass; /* Allow advancement in multiple classes     */
    int allow_prestige; /* Allow advancement in prestige classes     */
};

/*
 * The new character creation method options.
 */
struct creation_data {
    int method; /* What method to use for new character creation */
};

/*
 * The main configuration structure;
 */
struct config_data {
    char *CONFFILE; /* config file path	 */
    struct game_data play; /* play related config   */
    struct crash_save_data csd; /* rent and save related */
    struct room_numbers room_nums; /* room numbers          */
    struct game_operation operation; /* basic operation       */
    struct autowiz_data autowiz; /* autowiz related stuff */
    struct advance_data advance; /* char advancement stuff */
    struct tick_data ticks; /* game tick stuff 	 */
    struct creation_data creation; /* char creation method	 */
};

/*
 * Data about character aging
 */
struct aging_data {
    int adult; /* Adulthood */
    int classdice[3][2]; /* Dice info for starting age based on class age type */
    int middle; /* Middle age */
    int old; /* Old age */
    int venerable; /* Venerable age */
    int maxdice[2]; /* For roll to determine natural death beyond venerable */
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif
