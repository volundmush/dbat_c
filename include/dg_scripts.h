/**************************************************************************
*  File: dg_scripts.h                                                     *
*  Usage: header file for script structures and constants, and            *
*         function prototypes for dg_scripts.c                            *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#ifndef __DG_SCRIPTS_H__
#define __DG_SCRIPTS_H__

#include "structs.h"
#include "db.h"
#include "genzon.h"
#include "oasis.h"

#define DG_SCRIPT_VERSION "DG Scripts 1.0.14"

#define    MOB_TRIGGER   0
#define    OBJ_TRIGGER   1
#define    WLD_TRIGGER   2

/* unless you change this, Puff casts all your dg spells */
#define DG_CASTER_PROXY 1
/* spells cast by objects and rooms use this level */
#define DG_SPELL_LEVEL  25

/*
 * define this if you don't want wear/remove triggers to fire when
 * a player is saved.
 */
#define NO_EXTRANEOUS_TRIGGERS
/*
 * %actor.room% behaviour :
 * Until pl 7 %actor.room% returned a room vnum.
 * Working with this number in scripts was unnecessarily hard,
 * especially in those situations one needed the id of the room,
 * the items in it, etc. As a result of this, the output
 * has been changed (as of pl 8) to a room variable.
 * This means old scripts will need a minor adjustment;
 *
 * Before:
 * if %actor.room%==3001
 *   %echo% You are at the main temple.
 *
 * After:
 * eval room %actor.room%
 * if %room.vnum%==3001
 *   %echo% You are at the main temple.
 *
 * If you wish to continue using the old style, comment out the line below.
 *
 * Welcor
 */
#define ACTOR_ROOM_IS_UID 1

/* mob trigger types */
#define MTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define MTRIG_RANDOM           (1 << 1)      /* checked randomly           */
#define MTRIG_COMMAND          (1 << 2)         /* character types a command  */
#define MTRIG_SPEECH           (1 << 3)         /* a char says a word/phrase  */
#define MTRIG_ACT              (1 << 4)      /* word or phrase sent to act */
#define MTRIG_DEATH            (1 << 5)      /* character dies             */
#define MTRIG_GREET            (1 << 6)      /* something enters room seen */
#define MTRIG_GREET_ALL        (1 << 7)      /* anything enters room       */
#define MTRIG_ENTRY            (1 << 8)      /* the mob enters a room      */
#define MTRIG_RECEIVE          (1 << 9)      /* character is given obj     */
#define MTRIG_FIGHT            (1 << 10)     /* each pulse while fighting  */
#define MTRIG_HITPRCNT         (1 << 11)     /* fighting and below some hp */
#define MTRIG_BRIBE           (1 << 12)     /* coins are given to mob     */
#define MTRIG_LOAD             (1 << 13)     /* the mob is loaded          */
#define MTRIG_MEMORY           (1 << 14)     /* mob see's someone remembered */
#define MTRIG_CAST             (1 << 15)     /* mob targetted by spell     */
#define MTRIG_LEAVE            (1 << 16)     /* someone leaves room seen   */
#define MTRIG_DOOR             (1 << 17)     /* door manipulated in room   */

#define MTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* obj trigger types */
#define OTRIG_GLOBAL           (1 << 0)         /* unused                     */
#define OTRIG_RANDOM           (1 << 1)         /* checked randomly           */
#define OTRIG_COMMAND          (1 << 2)      /* character types a command  */

#define OTRIG_TIMER            (1 << 5)     /* item's timer expires       */
#define OTRIG_GET              (1 << 6)     /* item is picked up          */
#define OTRIG_DROP             (1 << 7)     /* character trys to drop obj */
#define OTRIG_GIVE             (1 << 8)     /* character trys to give obj */
#define OTRIG_WEAR             (1 << 9)     /* character trys to wear obj */
#define OTRIG_REMOVE           (1 << 11)    /* character trys to remove obj */

#define OTRIG_LOAD             (1 << 13)    /* the object is loaded        */

#define OTRIG_CAST             (1 << 15)    /* object targetted by spell   */
#define OTRIG_LEAVE            (1 << 16)    /* someone leaves room seen    */

#define OTRIG_CONSUME          (1 << 18)    /* char tries to eat/drink obj */
#define OTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* wld trigger types */
#define WTRIG_GLOBAL           (1 << 0)      /* check even if zone empty   */
#define WTRIG_RANDOM           (1 << 1)         /* checked randomly           */
#define WTRIG_COMMAND          (1 << 2)         /* character types a command  */
#define WTRIG_SPEECH           (1 << 3)      /* a char says word/phrase    */

#define WTRIG_RESET            (1 << 5)      /* zone has been reset        */
#define WTRIG_ENTER            (1 << 6)         /* character enters room      */
#define WTRIG_DROP             (1 << 7)      /* something dropped in room  */

#define WTRIG_CAST             (1 << 15)     /* spell cast in room */
#define WTRIG_LEAVE            (1 << 16)     /* character leaves the room */
#define WTRIG_DOOR             (1 << 17)     /* door manipulated in room  */

#define WTRIG_TIME             (1 << 19)     /* trigger based on game hour */

/* obj command trigger types */
#define OCMD_EQUIP             (1 << 0)         /* obj must be in char's equip */
#define OCMD_INVEN             (1 << 1)         /* obj must be in char's inven */
#define OCMD_ROOM              (1 << 2)         /* obj must be in char's room  */

/* obj consume trigger commands */
#define OCMD_EAT    1
#define OCMD_DRINK  2
#define OCMD_QUAFF  3

#define TRIG_NEW                0         /* trigger starts from top  */
#define TRIG_RESTART            1         /* trigger restarting       */


/*
 * These are slightly off of PULSE_MOBILE so
 * everything isnt happening at the same time
 */
#define PULSE_DG_SCRIPT         (13 RL_SEC)


#define MAX_SCRIPT_DEPTH      10          /* maximum depth triggers can
					     recurse into each other */

#define SCRIPT_ERROR_CODE     (-9999999)   /* this shouldn't happen too often */

/* one line of the trigger */
struct cmdlist_element {
    char *cmd;                /* one line of a trigger */
    struct cmdlist_element *original;
    struct cmdlist_element *next;
};

struct trig_var_data {
    char *name;                /* name of variable  */
    char *value;                /* value of variable */
    long context;                /* 0: global context */

    struct trig_var_data *next;
};

/* structure for triggers */
struct trig_data {
    IDXTYPE nr;                    /* trigger's rnum                  */
    int8_t attach_type;            /* mob/obj/wld intentions          */
    int8_t data_type;                /* type of game_data for trig      */
    char *name;                    /* name of trigger                 */
    long trigger_type;            /* type of trigger (for bitvector) */
    struct cmdlist_element *cmdlist;    /* top of command list             */
    struct cmdlist_element *curr_state;    /* ptr to current line of trigger  */
    int narg;                /* numerical argument              */
    char *arglist;            /* argument list                   */
    int depth;                /* depth into nest ifs/whiles/etc  */
    int loops;                /* loop iteration counter          */
    struct event *wait_event;    /* event to pause the trigger      */
    bool purged;            /* trigger is set to be purged     */
    struct trig_var_data *var_list;    /* list of local vars for trigger  */

    struct trig_data *next;
    struct trig_data *next_in_world;    /* next in the global trigger list */
};


/* a complete script (composed of several triggers) */
struct script_data {
    long types;                /* bitvector of trigger types */
    struct trig_data *trig_list;            /* list of triggers           */
    struct trig_var_data *global_vars;    /* list of global variables   */
    bool purged;                /* script is set to be purged */
    long context;                /* current context for statics */

    struct script_data *next;        /* used for purged_scripts    */
};

/* The event data for the wait command */
struct wait_event_data {
    struct trig_data *trigger;
    void *gohere;
    int type;
};

/* used for actor memory triggers */
struct script_memory {
    long id;                /* id of who to remember */
    char *cmd;                /* command, or NULL for generic */
    struct script_memory *next;
};


/* function prototypes from dg_triggers.c */
extern char *one_phrase(char *arg, char *first_arg);

extern bool is_substring(char *sub, char *string);

extern bool word_check(char *str, char *wordlist);

extern void act_mtrigger(const struct char_data *ch, char *str, struct char_data *actor,
                         const struct char_data *victim, struct obj_data *object, const struct obj_data *target,
                         const char *arg);

extern void speech_mtrigger(struct char_data *actor, char *str);

extern void speech_wtrigger(struct char_data *actor, char *str);

extern void greet_memory_mtrigger(struct char_data *ch);

extern bool greet_mtrigger(struct char_data *actor, int dir);

extern bool entry_mtrigger(struct char_data *ch);

extern void entry_memory_mtrigger(struct char_data *ch);

extern bool enter_wtrigger(struct room_data *room, struct char_data *actor, int dir);

extern bool drop_otrigger(struct obj_data *obj, struct char_data *actor);

extern void timer_otrigger(struct obj_data *obj);

extern bool get_otrigger(struct obj_data *obj, struct char_data *actor);

extern bool drop_wtrigger(struct obj_data *obj, struct char_data *actor);

extern bool give_otrigger(struct obj_data *obj, struct char_data *actor,
                          struct char_data *victim);

extern bool receive_mtrigger(struct char_data *ch, struct char_data *actor,
                             struct obj_data *obj);

extern void bribe_mtrigger(struct char_data *ch, struct char_data *actor,
                           int amount);

extern bool wear_otrigger(struct obj_data *obj, struct char_data *actor, int where);

extern bool remove_otrigger(struct obj_data *obj, struct char_data *actor);

extern bool cmd_otrig(struct obj_data *obj, struct char_data *actor, char *cmd,
                      char *argument, int type);

extern bool command_mtrigger(struct char_data *actor, char *cmd, char *argument);

extern bool command_otrigger(struct char_data *actor, char *cmd, char *argument);

extern bool command_wtrigger(struct char_data *actor, char *cmd, char *argument);

extern bool death_mtrigger(struct char_data *ch, struct char_data *actor);

extern void fight_mtrigger(struct char_data *ch);

extern void hitprcnt_mtrigger(struct char_data *ch);

extern void random_mtrigger(struct char_data *ch);

extern void random_otrigger(struct obj_data *obj);

extern void random_wtrigger(struct room_data *ch);

extern void reset_wtrigger(struct room_data *ch);

extern void load_mtrigger(struct char_data *ch);

extern void load_otrigger(struct obj_data *obj);

extern bool cast_mtrigger(struct char_data *actor, struct char_data *ch, int spellnum);

extern bool cast_otrigger(struct char_data *actor, struct obj_data *obj, int spellnum);

extern bool cast_wtrigger(struct char_data *actor, struct char_data *vict, struct obj_data *obj, int spellnum);

extern bool leave_mtrigger(struct char_data *actor, int dir);

extern bool leave_wtrigger(struct room_data *room, struct char_data *actor, int dir);

extern bool leave_otrigger(struct room_data *room, struct char_data *actor, int dir);

extern bool door_mtrigger(struct char_data *actor, int subcmd, int dir);

extern bool door_wtrigger(struct char_data *actor, int subcmd, int dir);

extern bool consume_otrigger(struct obj_data *obj, struct char_data *actor, int cmd);

extern void time_mtrigger(struct char_data *ch);

extern void time_otrigger(struct obj_data *obj);

extern void time_wtrigger(struct room_data *room);

/* function prototypes from dg_scripts.c */
extern char *str_str(char *cs, char *ct);

extern int find_eq_pos_script(char *arg);

extern bool can_wear_on_pos(struct obj_data *obj, int pos);

extern struct char_data *find_char(long n);

extern struct char_data *get_char(char *name);

extern struct char_data *get_char_near_obj(struct obj_data *obj, char *name);

extern struct char_data *get_char_in_room(struct room_data *room, char *name);

extern struct obj_data *get_obj_near_obj(struct obj_data *obj, char *name);

extern struct obj_data *get_obj(char *name);

extern struct room_data *get_room(char *name);

extern struct char_data *get_char_by_obj(struct obj_data *obj, char *name);

extern struct char_data *get_char_by_room(struct room_data *room, char *name);

extern struct obj_data *get_obj_by_obj(struct obj_data *obj, char *name);

extern struct obj_data *get_obj_in_room(struct room_data *room, char *name);

extern struct obj_data *get_obj_by_room(struct room_data *room, char *name);

extern int trgvar_in_room(room_vnum vnum);

extern struct obj_data *get_obj_in_list(char *name, struct obj_data *list);

extern struct obj_data *get_object_in_equip(struct char_data *ch, char *name);

extern void script_trigger_check(void);

extern void check_timed_triggers(void);

extern void find_uid_name(char *uid, char *name, size_t nlen);

extern void do_sstat_room(struct char_data *ch, struct room_data *rm);

extern void do_sstat_object(struct char_data *ch, struct obj_data *j);

extern void do_sstat_character(struct char_data *ch, struct char_data *k);

extern void add_trigger(struct script_data *sc, struct trig_data *t, int loc);

extern void script_vlog(const char *format, va_list args);

extern void script_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

extern char *matching_quote(char *p);

extern struct room_data *dg_room_of_obj(struct obj_data *obj);

/* To maintain strict-aliasing we'll have to do this trick with a union */
/* Thanks to Chris Gilbert for reminding me that there are other options. */
extern int script_driver(void *go_adress, struct trig_data *trig, int type, int mode);

extern trig_rnum real_trigger(trig_vnum vnum);

extern void process_eval(void *gohere, struct script_data *sc, struct trig_data *trig,
                         int type, char *cmd);

extern void read_saved_vars(struct char_data *ch);

extern void save_char_vars(struct char_data *ch);

extern void init_lookup_table(void);

extern void add_to_lookup_table(long uid, void *c);

extern void remove_from_lookup_table(long uid);

/* from dg_db_scripts.c */
extern void parse_trigger(FILE *trig_f, int nr);

extern struct trig_data *read_trigger(int nr);

extern void trig_data_copy(struct trig_data *this_data, const struct trig_data *trg);

extern void dg_read_trigger(FILE *fp, void *proto, int type);

extern void dg_obj_trigger(char *line, struct obj_data *obj);

extern void assign_triggers(void *i, int type);


/* From dg_variables.c */
extern void add_var(struct trig_var_data **var_list, char *name, const char *value, long id);

extern int item_in_list(char *item, struct obj_data *list);

extern char *skill_percent(struct char_data *ch, char *skill);

extern bool char_has_item(char *item, struct char_data *ch);

extern void var_subst(void *gohere, struct script_data *sc, struct trig_data *trig,
                      int type, char *line, char *buf);

extern bool text_processed(char *field, char *subfield, struct trig_var_data *vd,
                           char *str, size_t slen);

extern void find_replacement(void *gohere, struct script_data *sc, struct trig_data *trig,
                             int type, char *var, char *field, char *subfield, char *str, size_t slen);


/* From dg_handler.c */
extern void free_var_el(struct trig_var_data *var);

extern void free_varlist(struct trig_var_data *vd);

extern bool remove_var(struct trig_var_data **var_list, char *name);

extern void free_trigger(struct trig_data *trig);

extern void extract_trigger(struct trig_data *trig);

extern void extract_script(void *thing, int type);

extern void extract_script_mem(struct script_memory *sc);

extern void free_proto_script(void *thing, int type);

extern void copy_proto_script(void *source, void *dest, int type);

extern void delete_variables(const char *charname);

extern void update_wait_events(struct room_data *to, struct room_data *from);

/* from dg_comm.c */
extern char *any_one_name(char *argument, char *first_arg);

extern void sub_write(char *arg, struct char_data *ch, int8_t find_invis, int targets);

extern void send_to_zone(char *messg, zone_rnum zone);

/* from dg_misc.c */
extern void do_dg_cast(void *gohere, struct script_data *sc, struct trig_data *trig,
                       int type, char *cmd);

extern void do_dg_affect(void *gohere, struct script_data *sc, struct trig_data *trig,
                         int type, char *cmd);

extern void send_char_pos(struct char_data *ch, int dam);

extern bool valid_dg_target(struct char_data *ch, int bitvector);

extern void script_damage(struct char_data *vict, int dam);

extern bool check_flags_by_name_ar(int *array, int numflags, char *search, const char *namelist[]);

/* from dg_objcmd.c */
extern room_rnum obj_room(struct obj_data *obj);

// commands
extern ACMD(do_attach);

extern ACMD(do_detach);

extern ACMD(do_vdelete);

extern ACMD(do_tstat);

/* defines for valid_dg_target */
#define DG_ALLOW_GODS (1<<0)

/* Macros for scripts */

#define UID_CHAR   '}'
#define GET_TRIG_NAME(t)          ((t)->name)
#define GET_TRIG_RNUM(t)          ((t)->nr)
#define GET_TRIG_VNUM(t)      (trig_index[(t)->nr]->vnum)
#define GET_TRIG_TYPE(t)          ((t)->trigger_type)
#define GET_TRIG_DATA_TYPE(t)      ((t)->data_type)
#define GET_TRIG_NARG(t)          ((t)->narg)
#define GET_TRIG_ARG(t)           ((t)->arglist)
#define GET_TRIG_VARS(t)      ((t)->var_list)
#define GET_TRIG_WAIT(t)      ((t)->wait_event)
#define GET_TRIG_DEPTH(t)         ((t)->depth)
#define GET_TRIG_LOOPS(t)         ((t)->loops)

/* player id's: 0 to MOB_ID_BASE - 1            */
/* mob id's: MOB_ID_BASE to ROOM_ID_BASE - 1      */
/* room id's: ROOM_ID_BASE to OBJ_ID_BASE - 1    */
/* object id's: OBJ_ID_BASE and higher           */
#define MOB_ID_BASE      50000  /* 50000 player IDNUMS should suffice */
#define ROOM_ID_BASE    1050000 /* 1000000 Mobs */
#define OBJ_ID_BASE     1300000 /* 250000 Rooms */

#define SCRIPT(o)          ((o)->script)
#define SCRIPT_MEM(c)             ((c)->memory)

#define SCRIPT_TYPES(s)          ((s)->types)
#define TRIGGERS(s)          ((s)->trig_list)

#define GET_SHORT(ch)    ((ch)->short_descr)


#define SCRIPT_CHECK(go, type)   (SCRIPT(go) && \
                  IS_SET(SCRIPT_TYPES(SCRIPT(go)), type))
#define TRIGGER_CHECK(t, type)   (IS_SET(GET_TRIG_TYPE(t), type) && \
                  !GET_TRIG_DEPTH(t))

#define ADD_UID_VAR(buf, trig, go, name, context) do { \
                 sprintf(buf, "%c%d", UID_CHAR, GET_ID(go)); \
                         add_var(&GET_TRIG_VARS(trig), name, buf, context); } while (0)


#endif