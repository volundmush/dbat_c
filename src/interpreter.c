/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#define __INTERPRETER_C__
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "tedit.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "shop.h"
#include "guild.h"
#include "imc.h"
#include "clan.h"
#include "class.h"
#include "races.h"
#include "act.movement.h"
#include "config.h"
#include "objsave.h"
#include "statedit.h"
#include "weather.h"
#include "act.informative.h"
#include "players.h"
#include "act.wizard.h"
#include "alias.h"
#include "dg_comm.h"
#include "ban.h"
#include "assedit.h"
#include "obj_edit.h"

/* local global variables */
DISABLED_DATA *disabled_first = NULL;

/* local functions */
int roll_stats(struct char_data *ch, int type, int bonus);
void userRead(struct descriptor_data *d);
int opp_bonus(struct char_data *ch, int value, int type);
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int reserved_word(char *argument);
void display_bonus_menu(struct char_data *ch, int type);
int parse_bonuses(const char *arg);
void exchange_ccpoints(struct char_data *ch, int value);
int command_pass(char *cmd, struct char_data *ch);
void payout(int num);


/* prototypes for all do_x functions. */
ACMD(do_lag);
ACMD(do_runic);
ACMD(do_refuel);
ACMD(do_extract);
ACMD(do_combine);
ACMD(do_channel);
ACMD(do_teach);
ACMD(do_action);
ACMD(do_scry);
ACMD(do_spiritcontrol);
ACMD(do_song);
ACMD(do_shell);
ACMD(do_moondust);
ACMD(do_multiform);
ACMD(do_amnisiac);
ACMD(do_arena);
ACMD(do_approve);
ACMD(do_ensnare);
ACMD(do_absorb);
ACMD(do_ingest);
ACMD(do_instill);
ACMD(do_infuse);
ACMD(do_implant);
ACMD(do_interest);
ACMD(do_appraise);
ACMD(do_forgery);
ACMD(do_fish);
ACMD(do_fireshield);
ACMD(do_feed);
ACMD(do_future);
ACMD(do_fury);
ACMD(do_aid);
ACMD(do_advance);
ACMD(do_adrenaline);
ACMD(do_aedit);
ACMD(do_alias);
ACMD(do_assemble);
ACMD(do_ashcloud);
ACMD(do_assedit);
ACMD(do_assist);
ACMD(do_rescue);
ACMD(do_resize);
ACMD(do_rpp);
ACMD(do_rip);
ACMD(do_intro);
ACMD(do_astat);
ACMD(do_at);
ACMD(do_hydromancy);
ACMD(do_liquefy);
ACMD(do_lightgrenade);
ACMD(do_attack);
ACMD(do_attack2);
ACMD(do_auction);
ACMD(do_autoexit);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_head);
ACMD(do_healglow);
ACMD(do_hayasa);
ACMD(do_nova);
ACMD(do_beacon);
ACMD(do_breaker);
ACMD(do_block);
ACMD(do_barrier);
ACMD(do_beam);
ACMD(do_tsuihidan);
ACMD(do_table);
ACMD(do_play);
ACMD(do_preference);
ACMD(do_energize);
ACMD(do_evolve);
ACMD(do_post);
ACMD(do_hand);
ACMD(do_trip);
ACMD(do_train);
ACMD(do_taisha);
ACMD(do_twohand);
ACMD(do_tailwhip);
ACMD(do_shogekiha);
ACMD(do_shimmer);
ACMD(do_sunder);
ACMD(do_seishou);
ACMD(do_spiral);
ACMD(do_spoil);
ACMD(do_selfd);
ACMD(do_shuffle);
ACMD(do_bid);
ACMD(do_boom);
ACMD(do_break);
ACMD(do_disguise);
ACMD(do_defend);
ACMD(do_deploy);
ACMD(do_pack);
ACMD(do_dimizu);
ACMD(do_obstruct);
ACMD(do_draw);
ACMD(do_diagnose);
ACMD(do_drag);
ACMD(do_carry);
ACMD(do_cook);
ACMD(do_stop);
ACMD(do_silk);
ACMD(do_focus);
ACMD(do_charge);
ACMD(do_candy);
ACMD(do_form);
ACMD(do_checkloadstatus);
ACMD(do_finger);
ACMD(do_chown);
ACMD(do_clan);
ACMD(do_color);
ACMD(do_compare);
ACMD(do_copyover);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_disable);
ACMD(do_dig);
ACMD(do_bury);
ACMD(do_disarm);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drive);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_commune);
ACMD(do_rptrans);
ACMD(do_rpbank);
ACMD(do_aura);
ACMD(do_escape);
ACMD(do_eavesdrop);
ACMD(do_eyec);
ACMD(do_solar);
ACMD(do_spit);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_feats);
ACMD(do_file);
ACMD(do_finddoor);
ACMD(do_findkey);
ACMD(do_fix);
ACMD(do_srepair);
ACMD(do_recharge);
ACMD(do_renzo);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_garden);
ACMD(do_grapple);
ACMD(do_ginfo);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_hell);
ACMD(do_hspiral);
ACMD(do_heal);
ACMD(do_help);
ACMD(do_hindex);
ACMD(do_history);
ACMD(do_helpcheck);
ACMD(do_hide);
ACMD(do_pushup);
ACMD(do_paralyze);
ACMD(do_perf);
ACMD(do_permission);
ACMD(do_rbank);
ACMD(do_reward);
ACMD(do_plant);
ACMD(do_meditate);
ACMD(do_metamorph);
ACMD(do_malice);
ACMD(do_mimic);
ACMD(do_masenko);
ACMD(do_dodonpa);
ACMD(do_deathball);
ACMD(do_spiritball);
ACMD(do_genki);
ACMD(do_geno);
ACMD(do_deathbeam);
ACMD(do_dualbeam);
ACMD(do_pose);
ACMD(do_rogafufuken);
ACMD(do_baku);
ACMD(do_kienzan);
ACMD(do_kanso);
ACMD(do_koteiru);
ACMD(do_kyodaika);
ACMD(do_kousengan);
ACMD(do_kura);
ACMD(do_tribeam);
ACMD(do_sbc);
ACMD(do_final);
ACMD(do_crusher);
ACMD(do_ddslash);
ACMD(do_pbarrage);
ACMD(do_hellflash);
ACMD(do_hellspear);
ACMD(do_scatter);
ACMD(do_bigbang);
ACMD(do_pslash);
ACMD(do_kakusanha);
ACMD(do_hass);
ACMD(do_handout);
ACMD(do_eraser);
ACMD(do_tslash);
ACMD(do_psyblast);
ACMD(do_honoo);
ACMD(do_galikgun);
ACMD(do_majinize);
ACMD(do_potential);
ACMD(do_map);
ACMD(do_situp);
ACMD(do_house);
ACMD(do_iedit);
ACMD(do_instant);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kill);
ACMD(do_kamehameha);
ACMD(do_kaioken);
ACMD(do_kiball);
ACMD(do_kiblast);
ACMD(do_kick);
ACMD(do_elbow);
ACMD(do_knee);
ACMD(do_languages);
ACMD(do_lifeforce);
ACMD(do_land);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_balefire);
ACMD(do_blessedhammer);
ACMD(do_rbanktrans);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_not_here);
ACMD(do_news);
ACMD(do_newsedit);
ACMD(do_nickname);
ACMD(do_oasis_copy);
ACMD(do_oasis);
//ACMD(do_olc);
ACMD(do_page);
ACMD(do_powerup);
ACMD(do_suppress);
ACMD(do_punch);
ACMD(do_bite);
ACMD(do_roundhouse);
ACMD(do_regenerate);
ACMD(do_uppercut);
ACMD(do_upgrade);
ACMD(do_pagelength);
ACMD(do_peace);
ACMD(do_plist);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_raise);
ACMD(do_radar);
ACMD(do_sradar);
ACMD(do_recall);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_respond);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_resurrect);
ACMD(do_return);
ACMD(do_room_copy);
ACMD(do_sac);
ACMD(do_spar);
ACMD(do_slam);
ACMD(do_heeldrop);
ACMD(do_scouter);
ACMD(do_snet);
ACMD(do_save);
ACMD(do_skills);
ACMD(do_saveall);
ACMD(do_say);
ACMD(do_osay);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_status);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_showoff);
ACMD(do_show_save_list);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_rcopy);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_spells);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_fly);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_telepathy);
ACMD(do_think);
ACMD(do_transform);
ACMD(do_transobj);
ACMD(do_throw);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_toplist);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_varstat);
ACMD(do_voice);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_warp);
ACMD(do_warppool);
ACMD(do_razor);
ACMD(do_spike);
ACMD(do_willpower);
ACMD(do_summon);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);
ACMD(do_wield);
ACMD(do_value);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zcheck);
ACMD(do_zen);
ACMD(do_zanzoken);
ACMD(do_zreset);
ACMD(do_zpurge);
ACMD(do_tailhide);
ACMD(do_nogrow);
ACMD(do_restring);

/* DG Script ACMD's */
ACMD(do_attach);
ACMD(do_detach);
ACMD(do_tlist);
ACMD(do_tstat);
ACMD(do_masound);
ACMD(do_mkill);
ACMD(do_mheal);
ACMD(do_mjunk);
ACMD(do_mdoor);
ACMD(do_mechoaround);
ACMD(do_msend);
ACMD(do_mecho);
ACMD(do_mload);
ACMD(do_mpurge);
ACMD(do_mgoto);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mteleport);
ACMD(do_mforce);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mzoneecho);
ACMD(do_vdelete);
ACMD(do_mfollow);
ACMD(do_dig);
ACMD(do_rdisplay);

struct command_info *complete_cmd_info;

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, ADMLVL_NONE	, 0 },     /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NORTH },
  { "east"     , "e"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_EAST },
  { "south"    , "s"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SOUTH },
  { "west"     , "w"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_WEST },
  { "up"       , "u"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_UP },
  { "down"     , "d"       , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_DOWN },
  { "northwest", "northw"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "nw"       , "nw"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NW },
  { "northeast", "northe"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "ne"       , "ne"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_NE },
  { "southeast", "southe"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "se"       , "se"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SE },
  { "southwest", "southw"  , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "sw"       , "sw"      , POS_RESTING, do_move     , 0, ADMLVL_NONE	, SCMD_SW },
  { "i"        , "i"            , POS_DEAD    , do_inventory, 0, ADMLVL_NONE    , 0 },
  { "inside"   , "in"      , POS_RESTING, do_move     , 0, ADMLVL_NONE , SCMD_IN },
  { "outside"  , "out"     , POS_RESTING, do_move     , 0, ADMLVL_NONE , SCMD_OUT },

  /* now, the main list */
  { "absorb"   , "absor"        , POS_STANDING, do_absorb   , 0, ADMLVL_NONE    , 0 },
  { "at"       , "at"		, POS_DEAD    , do_at       , 0, ADMLVL_BUILDER	, 0 },
  { "adrenaline"  , "adrenalin" , POS_DEAD    , do_adrenaline, 0, ADMLVL_NONE    , 0 },
  { "advance"  , "adv"		, POS_DEAD    , do_advance  , 0, ADMLVL_IMPL	, 0 },
  { "aedit"    , "aed"	 	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_AEDIT },
  { "alias"    , "ali"		, POS_DEAD    , do_alias    , 0, ADMLVL_NONE	, 0 },
  { "afk"      , "afk"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AFK },
  { "aid"      , "aid"          , POS_STANDING, do_aid      , 0, ADMLVL_NONE    , 0 },
  { "amnesiac" , "amnesia"      , POS_STANDING, do_amnisiac , 0, ADMLVL_NONE    , 0 },
  { "appraise" , "apprais"      , POS_STANDING, do_appraise , 0, ADMLVL_NONE    , 0 },
  { "approve"  , "approve"      , POS_STANDING, do_approve  , 0, ADMLVL_IMMORT  , 0 },
  { "arena"    , "aren"         , POS_RESTING , do_arena    , 0, ADMLVL_NONE    , 0 },
  { "ashcloud" , "ashclou"      , POS_RESTING , do_ashcloud , 0, ADMLVL_NONE    , 0 },
  { "assedit"  , "assed"	, POS_STANDING, do_assedit  , 0, ADMLVL_GOD	, 0},
  { "assist"   , "assis"        , POS_STANDING, do_assist   , 0, ADMLVL_NONE    , 0 },
  { "astat"    , "ast"		, POS_DEAD    , do_astat    , 0, ADMLVL_GOD	, 0 },
  { "ask"      , "ask"		, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_ASK },
  { "attack"   , "attack"       , POS_FIGHTING, do_attack   , 0, 0, 0 },
  { "auction"  , "auctio"       , POS_RESTING , do_not_here , 0, 0, 0 },
  { "augment"  , "augmen"       , POS_SITTING , do_not_here , 1, ADMLVL_NONE    , 0 },
  { "aura"     , "aura"         , POS_RESTING , do_aura , 0,  ADMLVL_NONE     , 0 },
  { "autoexit" , "autoex"	, POS_DEAD    , do_autoexit , 0, ADMLVL_NONE	, 0 },
  { "autogold" , "autogo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOGOLD },
  { "autoloot" , "autolo"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_AUTOLOOT },
  { "autosplit", "autosp"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_AUTOSPLIT },

  { "bakuhatsuha", "baku"       , POS_FIGHTING, do_baku   , 0, 0, 0 },
  { "ban"      , "ban"		, POS_DEAD    , do_ban      , 0, ADMLVL_VICE	, 0 },
  { "balance"  , "bal"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "balefire"  , "balef"       , POS_FIGHTING, do_balefire  , 0, ADMLVL_NONE    , 0 },
  { "barrage"  , "barrage"      , POS_FIGHTING, do_pbarrage , 0, ADMLVL_NONE     , 0 },
  { "barrier"  , "barri"        , POS_FIGHTING, do_barrier  , 0, ADMLVL_NONE    , 0 },
  { "bash"     , "bas"          , POS_FIGHTING, do_bash     , 0, ADMLVL_NONE    , 0 },
  { "beam"     , "bea"          , POS_FIGHTING, do_beam     , 0, ADMLVL_NONE    , 0 },
  { "bexchange" , "bexchan"       , POS_RESTING , do_rbanktrans  , 0, ADMLVL_NONE    , 0 },
  { "bid"      , "bi"           , POS_RESTING , do_bid      , 0, 0, 0 },
  { "bigbang"  , "bigban"       , POS_FIGHTING, do_bigbang  , 0, 0, 0 },
  { "bite"     , "bit"          , POS_FIGHTING, do_bite     , 0, 0, 0 },
  { "blessedhammer", "bham"    , POS_FIGHTING, do_blessedhammer, 0, ADMLVL_NONE    , 0 },
  { "block"    , "block"        , POS_FIGHTING, do_block    , 0, 0, 0 },
  { "book"     , "boo"          , POS_SLEEPING, do_gen_ps   , 0, ADMLVL_IMMORT  , SCMD_INFO },
  { "break"    , "break"	, POS_STANDING, do_break    , 0, ADMLVL_IMMORT	, 0 },
  { "brief"    , "br"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_BRIEF },
  { "build"    , "bui"          , POS_SITTING , do_assemble , 0, ADMLVL_NONE    , SCMD_BREW },
  { "buildwalk", "buildwalk"    , POS_STANDING, do_gen_tog, 0, ADMLVL_IMMORT    , SCMD_BUILDWALK },
  { "buy"      , "bu"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "bug"      , "bug"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_BUG },

  { "cancel"   , "cance"        , POS_RESTING , do_not_here , 0, 0, 0 },
  { "candy"    , "cand"         , POS_FIGHTING, do_candy    , 0, 0, 0 },
  { "carry"    , "carr"         , POS_STANDING, do_carry    , 0, 0, 0 },
  { "carve"    , "carv"         , POS_SLEEPING, do_gen_tog   , 0, 0, SCMD_CARVE },
  { "cedit"    , "cedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMPL	, SCMD_OASIS_CEDIT },
  { "channel"  , "channe"       , POS_FIGHTING, do_channel  , 0, 0, 0 },
  { "charge"   , "char"         , POS_FIGHTING, do_charge   , 0, 0, 0 },
  { "check"    , "ch"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "checkload", "checkl"	, POS_DEAD    , do_checkloadstatus, 0, ADMLVL_GOD, 0 },
  { "chown"    , "cho"		, POS_DEAD    , do_chown    , 1, ADMLVL_IMPL	, 0 },
  { "clan"     , "cla"          , POS_DEAD    , do_clan     , 0, ADMLVL_NONE     , 0 },
  { "clear"    , "cle"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "close"    , "cl"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_CLOSE },
  { "closeeyes", "closeey"      , POS_RESTING , do_eyec     , 0, ADMLVL_NONE    , 0 },
  { "cls"      , "cls"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CLEAR },
  { "clsolc"   , "clsolc"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_BUILDER	, SCMD_CLS },
  { "consider" , "con"		, POS_RESTING , do_consider , 0, ADMLVL_NONE	, 0 },
  { "color"    , "col"		, POS_DEAD    , do_color    , 0, ADMLVL_NONE	, 0 },
  { "combine"  , "comb"         , POS_RESTING , do_combine  , 0, ADMLVL_NONE    , 0 },
  { "compare"  , "comp"		, POS_RESTING , do_compare  , 0, ADMLVL_NONE	, 0 },
  { "commands" , "com"		, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_COMMANDS },
  { "commune"  , "comm"         , POS_DEAD    , do_commune  , 0, ADMLVL_NONE    , 0 },
  { "compact"  , "compact"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_COMPACT },
  { "cook"     , "coo"          , POS_RESTING , do_cook     , 0, ADMLVL_NONE    , 0 },
  { "copyover" , "copyover"	, POS_DEAD    , do_copyover , 0, ADMLVL_GOD	, 0 },
  { "create"   , "crea"         , POS_STANDING, do_form     , 0, ADMLVL_NONE    , 0 },
  { "credits"  , "cred"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_CREDITS },
  { "crusher"  , "crushe"       , POS_FIGHTING, do_crusher  , 0, 0, 0 },

  { "date"     , "da"		, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_DATE },
  { "darkness" , "darknes"      , POS_FIGHTING, do_ddslash  , 0, ADMLVL_NONE    , 0 },
  { "dc"       , "dc"		, POS_DEAD    , do_dc       , 0, ADMLVL_GOD	, 0 },
  { "deathball", "deathbal"     , POS_FIGHTING, do_deathball, 0, ADMLVL_NONE    , 0 },
  { "deathbeam", "deathbea"     , POS_FIGHTING, do_deathbeam, 0, ADMLVL_NONE    , 0 },
  { "decapitate", "decapit"     , POS_STANDING, do_spoil    , 0, ADMLVL_NONE    , 0 },
  { "defend"   , "defen"        , POS_STANDING, do_defend   , 0, ADMLVL_NONE    , 0 },
  { "deploy"   , "deplo"        , POS_STANDING, do_deploy   , 0, ADMLVL_NONE    , 0 },
  { "dualbeam" , "dualbea"      , POS_FIGHTING, do_dualbeam , 0, ADMLVL_NONE    , 0 },
  { "deposit"  , "depo"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "diagnose" , "diagnos"      , POS_RESTING , do_diagnose , 0, ADMLVL_NONE    , 0 },
  { "dimizu"   , "dimizu"       , POS_STANDING, do_dimizu   , 0, 0   , 0 },
  { "disable"  , "disa"		, POS_DEAD    , do_disable  , 0, ADMLVL_VICE	, 0 },
  { "disguise" , "disguis"      , POS_DEAD    , do_disguise , 0, 0, 0 },
  { "dig"      , "dig"		, POS_DEAD    , do_bury     , 0, ADMLVL_NONE	, 0 },
  { "display"  , "disp"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "dodonpa"  , "dodon"        , POS_FIGHTING, do_dodonpa  , 0, ADMLVL_NONE    , 0 },
  { "donate"   , "don"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DONATE },
  { "drag"     , "dra"          , POS_STANDING, do_drag     , 0, ADMLVL_NONE    , 0 },
  { "draw"     , "dra"          , POS_SITTING , do_draw     , 0, ADMLVL_NONE    , 0 },
  { "drink"    , "dri"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_DRINK },
  { "drop"     , "dro"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_DROP },
  { "dub"      , "du"           , POS_STANDING, do_intro  , 0, ADMLVL_NONE    , 0 },

  { "eat"      , "ea"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_EAT },
  { "eavesdrop", "eaves"       , POS_RESTING , do_eavesdrop, 0, ADMLVL_NONE    , 0 },
  { "echo"     , "ec"		, POS_SLEEPING, do_echo     , 0, ADMLVL_IMMORT	, SCMD_ECHO },
  { "elbow"    , "elb"          , POS_FIGHTING, do_elbow    , 0, ADMLVL_NONE    , 0 },
  { "emote"    , "em"		, POS_RESTING , do_echo     , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { "energize" , "energiz"      , POS_RESTING , do_energize , 1, ADMLVL_NONE    , 0 },
  { ":"        , ":"		, POS_RESTING, do_echo      , 1, ADMLVL_NONE	, SCMD_EMOTE },
  { "ensnare"  , "ensnar"       , POS_FIGHTING, do_ensnare  , 0, ADMLVL_NONE    , 0 },
  { "enter"    , "ent"		, POS_STANDING, do_enter    , 0, ADMLVL_NONE	, 0 },
  { "equipment", "eq"		, POS_SLEEPING, do_equipment, 0, ADMLVL_NONE	, 0 },
  { "eraser"   , "eras"         , POS_FIGHTING, do_eraser   , 0, ADMLVL_NONE    , 0 },
  { "escape"   , "esca"         , POS_RESTING,  do_escape   , 0, ADMLVL_NONE    , 0 },
  { "evolve"   , "evolv"        , POS_RESTING, do_evolve    , 0, ADMLVL_NONE    , 0 },
  { "exchange" , "exchan"       , POS_RESTING , do_rptrans  , 0, ADMLVL_NONE    , 0 },
  { "exits"    , "ex"		, POS_RESTING , do_exits    , 0, ADMLVL_NONE	, 0 },
  { "examine"  , "exa"		, POS_SITTING , do_examine  , 0, ADMLVL_NONE	, 0 },
  { "extract"  , "extrac"       , POS_STANDING, do_extract  , 0, ADMLVL_NONE    , 0 },

  { "feed"     , "fee"          , POS_STANDING, do_feed     , 0, ADMLVL_NONE    , 0 },
  { "fill"     , "fil"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_FILL },
  { "file"     , "fi"		, POS_SLEEPING, do_file     , 0, ADMLVL_IMMORT	, 0 },
  { "finalflash" , "finalflash" , POS_FIGHTING, do_final    , 0, ADMLVL_NONE    , 0 },
  { "finddoor" , "findd"	, POS_SLEEPING, do_finddoor , 0, ADMLVL_IMMORT, 0 },
  { "findkey"  , "findk"	, POS_SLEEPING, do_findkey  , 0, ADMLVL_IMMORT, 0 },
  { "finger"   , "finge"        , POS_SLEEPING, do_finger   , 0, ADMLVL_NONE    , 0 },
  { "fireshield"  , "firesh"    , POS_STANDING, do_fireshield   , 0, ADMLVL_NONE    , 0 },
  { "fish"     , "fis"          , POS_STANDING, do_fish     , 0, ADMLVL_NONE  , 0 },
  { "fix"      , "fix"		, POS_STANDING, do_fix      , 0, ADMLVL_NONE	, 0 },
  { "flee"     , "fl"		, POS_FIGHTING, do_flee     , 1, ADMLVL_NONE	, 0 },
  { "fly"      , "fly"          , POS_RESTING, do_fly      , 0, ADMLVL_NONE    , 0 },
  { "focus"    , "foc"          , POS_STANDING, do_focus    , 0, ADMLVL_NONE    , 0 },
  { "follow"   , "fol"		, POS_RESTING , do_follow   , 0, ADMLVL_NONE	, 0 },
  { "force"    , "force"        , POS_SLEEPING, do_force    , 0, ADMLVL_IMMORT  , 0 },
  { "forgery"  , "forg"         , POS_RESTING, do_forgery   , 0, ADMLVL_NONE    , 0 },
  { "forget"   , "forg"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "freeze"   , "freeze"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_FREEZE },
  { "fury"     , "fury"         , POS_FIGHTING, do_fury     , 0, ADMLVL_NONE    , 0 },
  { "future"   , "futu"         , POS_STANDING, do_future   , 0, ADMLVL_NONE    , 0 },

  { "gain"     , "ga"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "galikgun" , "galik"        , POS_FIGHTING, do_galikgun , 0, ADMLVL_NONE    , 0 },
  { "game"     , "gam"          , POS_RESTING , do_show     , 0, ADMLVL_IMMORT  , 0 },
  { "garden"   , "garde"        , POS_STANDING, do_garden   , 0, ADMLVL_NONE    , 0 },
  { "genkidama", "genkidam"     , POS_FIGHTING, do_genki    , 0, ADMLVL_NONE    , 0 },
  { "genocide" , "genocid"      , POS_FIGHTING, do_geno     , 0, ADMLVL_NONE    , 0 },
  { "get"      , "get"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "gecho"    , "gecho"	, POS_DEAD    , do_gecho    , 0, ADMLVL_BUILDER	, 0 },
  { "gedit"    , "gedit"      	, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GEDIT },
  { "gemote"   , "gem"	 	, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GEMOTE },
  { "generator", "genr"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "glist"    , "glist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_GLIST },
  { "give"     , "giv"		, POS_RESTING , do_give     , 0, ADMLVL_NONE	, 0 },
  { "goto"     , "go"		, POS_SLEEPING, do_goto     , 0, ADMLVL_IMMORT	, 0 },
  { "gold"     , "gol"		, POS_RESTING , do_gold     , 0, ADMLVL_NONE	, 0 },
  { "group"    , "gro"		, POS_RESTING , do_group    , 1, ADMLVL_NONE	, 0 },
  { "grab"     , "grab"		, POS_RESTING , do_grab     , 0, ADMLVL_NONE	, 0 },
  { "grand"    , "gran"         , POS_RESTING , do_not_here , 0, ADMLVL_NONE    , 0 },
  { "grapple"  , "grapp"        , POS_FIGHTING, do_grapple  , 0, ADMLVL_NONE    , 0 },
  { "grats"    , "grat"		, POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE	, SCMD_GRATZ },
  { "gravity"  , "grav"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "gsay"     , "gsay"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },
  { "gtell"    , "gt"		, POS_SLEEPING, do_gsay     , 0, ADMLVL_NONE	, 0 },

  { "hand"     , "han"          , POS_SITTING , do_hand     , 0, ADMLVL_NONE    , 0 },
  { "handout"  , "hand"         , POS_STANDING, do_handout  , 0, ADMLVL_GOD     , 0 },
  { "hasshuken", "hasshuke"     , POS_STANDING, do_hass     , 0, ADMLVL_NONE    , 0 },
  { "hayasa"   , "hayas"       , POS_STANDING, do_hayasa   , 0, ADMLVL_NONE    , 0 },
  { "headbutt" , "headbut"      , POS_FIGHTING, do_head     , 0, ADMLVL_NONE    , 0 },
  { "heal"     , "hea"          , POS_STANDING, do_heal     , 0, ADMLVL_NONE    , 0 },
  { "health"   , "hea"          , POS_DEAD,     do_gen_tog  , 0, ADMLVL_NONE    , SCMD_GHEALTH },
  { "healingglow" , "healing"   , POS_STANDING, do_healglow     , 0, ADMLVL_NONE    , 0 },
  { "heeldrop" , "heeldr"       , POS_FIGHTING, do_heeldrop , 0, ADMLVL_NONE    , 0 },
  { "hellflash", "hellflas"     , POS_FIGHTING, do_hellflash, 0, ADMLVL_NONE     , 0 },
  { "hellspear", "hellspea"     , POS_FIGHTING, do_hellspear, 0, ADMLVL_NONE     , 0 },
  { "help"     , "h"            , POS_DEAD    , do_help     , 0, ADMLVL_NONE    , 0 },
  { "hedit"    , "hedit"        , POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT  , SCMD_OASIS_HEDIT },
  { "hindex"   , "hind"         , POS_DEAD    , do_hindex   , 0, ADMLVL_NONE    , 0 },
  { "helpcheck", "helpch"       , POS_DEAD    , do_helpcheck, 0, ADMLVL_NONE    , 0 },
  { "handbook" , "handb"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_HANDBOOK },
  { "hide"     , "hide"		, POS_RESTING , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_HIDE },
  { "hints"    , "hints"        , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_HINTS },
  { "history"  , "hist"         , POS_DEAD    , do_history  , 0, ADMLVL_NONE    , 0 },
  { "hold"     , "hold"		, POS_RESTING , do_grab     , 1, ADMLVL_NONE	, 0 },
  { "holylight", "holy"		, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_HOLYLIGHT },
  { "honoo"    , "hono"         , POS_FIGHTING, do_honoo    , 0, ADMLVL_NONE    , 0 },
  { "house"    , "house"	, POS_RESTING , do_house    , 0, ADMLVL_NONE	, 0 },
  { "hsedit"   , "hsedit"       , POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER , SCMD_OASIS_HSEDIT },
  { "hspiral"  , "hspira"       , POS_FIGHTING, do_hspiral  , 0, ADMLVL_NONE    , 0 },
  { "htank"    , "htan"         , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "hydromancy", "hydrom"      , POS_STANDING, do_hydromancy , 0, ADMLVL_NONE    , 0 },
  { "hyoga"    , "hyoga"        , POS_STANDING, do_obstruct , 0, ADMLVL_NONE    , 0 },

  { "ihealth"  , "ihea"         , POS_DEAD,     do_gen_tog  , 0, ADMLVL_NONE    , SCMD_IHEALTH },
  { "info"     , "info"         , POS_DEAD    , do_ginfo    , 0, ADMLVL_IMMORT  , 0 },
  { "infuse"   , "infus"        , POS_STANDING, do_infuse   , 0, ADMLVL_NONE    , 0 },
  { "ingest"   , "inges"        , POS_STANDING, do_ingest   , 0, ADMLVL_NONE    , 0 },
  { "imotd"    , "imotd"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_IMMORT	, SCMD_IMOTD },
  { "immlist"  , "imm"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "implant"  , "implan"       , POS_RESTING , do_implant  , 0, ADMLVL_NONE    , 0 },
  { "instant"  , "insta"        , POS_STANDING, do_instant  , 0, ADMLVL_NONE    , 0 },
  { "instill"  , "instil"       , POS_STANDING, do_instill  , 0, ADMLVL_NONE    , 0 },
  { "instruct" , "instruc"      , POS_STANDING, do_gen_tog   , 0, 0, SCMD_INSTRUCT },
  /*{ "insult"   , "insult"	, POS_RESTING , do_insult   , 0, ADMLVL_NONE	, 0 },*/
  { "inventory", "inv"		, POS_DEAD    , do_inventory, 0, ADMLVL_NONE	, 0 },
  { "interest" , "inter"        , POS_DEAD    , do_interest , 0, ADMLVL_IMPL    , 0 },
  { "iedit"    , "ie"   	, POS_DEAD    , do_iedit    , 0, ADMLVL_IMPL	, 0 },
  { "invis"    , "invi"		, POS_DEAD    , do_invis    , 0, ADMLVL_IMMORT	, 0 },
  { "iwarp"    , "iwarp"        , POS_RESTING , do_warp     , 0, ADMLVL_NONE    , 0 },

  { "junk"     , "junk"		, POS_RESTING , do_drop     , 0, ADMLVL_NONE	, SCMD_JUNK },

  { "kaioken"  , "kaioken"      , POS_STANDING, do_kaioken  , 0, ADMLVL_NONE    , 0 },
  { "kakusanha", "kakusan"      , POS_FIGHTING, do_kakusanha, 0, ADMLVL_NONE    , 0 },
  { "kamehameha", "kame"        , POS_FIGHTING, do_kamehameha, 0, ADMLVL_NONE    , 0 },
  { "kanso"    , "kans"         , POS_FIGHTING, do_kanso    , 0, ADMLVL_NONE    , 0 },
  { "kiball"   , "kibal"        , POS_FIGHTING, do_kiball   , 0, ADMLVL_NONE    , 0 },
  { "kiblast"  , "kiblas"       , POS_FIGHTING, do_kiblast  , 0, ADMLVL_NONE    , 0 },
  { "kienzan"  , "kienza"       , POS_FIGHTING, do_kienzan  , 0, ADMLVL_NONE    , 0 },
  { "kill"     , "kil"		, POS_FIGHTING, do_kill     , 0, ADMLVL_IMMORT	, 0 },
  { "kick"     , "kic"          , POS_FIGHTING, do_kick     , 0, ADMLVL_NONE     , 0 },
  { "knee"     , "kne"          , POS_FIGHTING, do_knee     , 0, ADMLVL_NONE     , 0 },
  { "koteiru"  , "koteiru"      , POS_FIGHTING, do_koteiru  , 0, ADMLVL_NONE    , 0 },
  { "kousengan", "kousengan"    , POS_FIGHTING, do_kousengan, 0, ADMLVL_NONE    , 0 },
  { "kuraiiro" , "kuraiir"      , POS_FIGHTING, do_kura     , 0, ADMLVL_NONE     , 0 },
  { "kyodaika" , "kyodaik"      , POS_STANDING, do_kyodaika , 0, ADMLVL_NONE    , 0 },

  { "look"     , "lo"		, POS_RESTING , do_look     , 0, ADMLVL_NONE	, SCMD_LOOK },
  { "lag"      , "la"           , POS_RESTING , do_lag      , 0, 5   , 0 },
  { "land"     , "lan"          , POS_RESTING , do_land     , 0, ADMLVL_NONE    , 0 },
  { "languages", "lang"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "last"     , "last"		, POS_DEAD    , do_last     , 0, ADMLVL_GOD	, 0 },
  { "learn"    , "lear"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "leave"    , "lea"		, POS_STANDING, do_leave    , 0, ADMLVL_NONE	, 0 },
  { "levels"   , "lev"		, POS_DEAD    , do_levels   , 0, ADMLVL_NONE	, 0 },
  { "light"    , "ligh"         , POS_STANDING, do_lightgrenade, 0, ADMLVL_NONE , 0 },
  { "list"     , "lis"          , POS_STANDING, do_not_here , 0, ADMLVL_NONE    , 0 },
  { "life"     , "lif"		, POS_SLEEPING, do_lifeforce, 0, ADMLVL_NONE	, 0 },
  { "links"    , "lin"		, POS_DEAD    , do_oasis    , 0, ADMLVL_BUILDER	, SCMD_OASIS_LINKS },
  { "liquefy"  , "liquef"       , POS_SLEEPING, do_liquefy  , 0, ADMLVL_NONE    , 0 },
  { "lkeep"    , "lkee"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_LKEEP },
  { "lock"     , "loc"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_LOCK },
  { "lockout"  , "lock"         , POS_STANDING, do_hell     , 0, ADMLVL_IMMORT  , 0 },
  { "load"     , "load"		, POS_DEAD    , do_load     , 0, ADMLVL_IMMORT	, 0 },

  { "majinize" , "majini"       , POS_STANDING, do_majinize , 0, ADMLVL_NONE    , 0 },
  { "malice"   , "malic"        , POS_FIGHTING, do_malice   , 0, ADMLVL_NONE    , 0 },
  { "masenko"  , "masenk"       , POS_FIGHTING, do_masenko  , 0, ADMLVL_NONE    , 0 },
  { "motd"     , "motd"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_MOTD },
  { "mail"     , "mail"		, POS_STANDING, do_not_here , 2, ADMLVL_NONE	, 0 },
  { "map"      , "map"          , POS_STANDING, do_map      , 0, ADMLVL_NONE    , 0 },
  /*{ "mcopy"    , "mcopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_MEDIT },*/
  { "medit"    , "medit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_MEDIT },
  { "meditate" , "medita"       , POS_SITTING, do_meditate, 0, ADMLVL_NONE    , 0 },
  { "metamorph", "metamorp"     , POS_STANDING, do_metamorph, 0, ADMLVL_NONE    , 0 },
  { "mimic"    , "mimi"         , POS_STANDING, do_mimic    , 0, ADMLVL_NONE    , 0 },
  { "mlist"    , "mlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_MLIST },
  { "moondust" , "moondus"      , POS_STANDING, do_moondust , 0, ADMLVL_NONE    , 0 },
  { "multiform", "multifor"     , POS_STANDING, do_multiform , 0, ADMLVL_NONE    , 0 },
  { "mute"     , "mute"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_SQUELCH },
  { "music"    , "musi"         , POS_RESTING , do_gen_comm , 1, ADMLVL_NONE  , SCMD_HOLLER },

  { "newbie"   , "newbie"       , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE    , SCMD_AUCTION },
  { "news"     , "news"		, POS_SLEEPING, do_news     , 0, ADMLVL_NONE	, 0 },
  { "newsedit" , "newsedi"      , POS_SLEEPING, do_newsedit , 0, ADMLVL_IMMORT  , 0 },
  { "nickname" , "nicknam"      , POS_RESTING , do_nickname , 0, ADMLVL_NONE    , 0 },
  { "nocompress","nocompress"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOCOMPRESS },
  { "noeq"     , "noeq"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOEQSEE },
  { "nolin"    , "nolin"        , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NODEC },
  { "nomusic"  , "nomusi"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOMUSIC },
  { "noooc"    , "noooc"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGOSSIP },
  { "nogive"   , "nogiv"        , POS_DEAD    , do_gen_tog   , 0, 0, SCMD_NOGIVE },
  { "nograts"  , "nograts"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOGRATZ },
  { "nogrow"   , "nogro"        , POS_DEAD    , do_nogrow   , 0, ADMLVL_NONE    , 0 },
  { "nohassle" , "nohassle"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOHASSLE },
  { "nomail"   , "nomail"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NMWARN },
  { "nonewbie" , "nonewbie"     , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOAUCTION },
  { "noparry"  , "noparr"       , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_NOPARRY },
  { "norepeat" , "norepeat"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_NOREPEAT },
  { "noshout"  , "noshout"	, POS_SLEEPING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_DEAF },
  { "nosummon" , "nosummon"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOSUMMON },
  { "notell"   , "notell"	, POS_DEAD    , do_gen_tog  , 1, ADMLVL_NONE	, SCMD_NOTELL },
  { "notitle"  , "notitle"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_NOTITLE },
  { "nova"     , "nov"          , POS_STANDING, do_nova     , 0, ADMLVL_NONE    , 0 },
  { "nowiz"    , "nowiz"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_NOWIZ },

  /*{ "ocopy"    , "ocopy"	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_OEDIT },*/
  { "ooc"      , "ooc"          , POS_SLEEPING, do_gen_comm , 0, ADMLVL_NONE     , SCMD_GOSSIP },
  { "offer"    , "off"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "open"     , "ope"		, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_OPEN },
  { "olc"      , "olc"		, POS_DEAD    , do_show_save_list, 0, ADMLVL_IMMORT, 0 },
  { "olist"    , "olist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_OEDIT },
  { "osay"     , "osay"         , POS_RESTING , do_osay     , 0, ADMLVL_NONE     , 0 },

  { "pack"     , "pac"          , POS_STANDING, do_pack     , 0, 0 , 0 },
  { "page"     , "pag"		, POS_DEAD    , do_page     , 0, ADMLVL_BUILDER	, 0 },
  { "paralyze" , "paralyz"      , POS_FIGHTING, do_paralyze , 0, ADMLVL_NONE    , 0 },
  { "pagelength", "pagel"	, POS_DEAD    , do_pagelength, 0, 0, 0 },
  { "peace"    , "pea"		, POS_DEAD    , do_peace    , 0, ADMLVL_BUILDER	, 0 },
  { "perfect"  , "perfec"       , POS_DEAD    , do_perf     , 0, ADMLVL_NONE , 0 },
  { "permission", "permiss"     , POS_DEAD    , do_permission, 0, ADMLVL_IMMORT , 0 },
  { "phoenix"  , "phoeni"       , POS_FIGHTING, do_pslash   , 0, ADMLVL_NONE    , 0 },
  { "pick"     , "pi"		, POS_STANDING, do_gen_door , 1, ADMLVL_NONE	, SCMD_PICK },
  { "pickup"   , "picku"        , POS_RESTING , do_not_here , 0, 0, 0 },
  { "pilot"    , "pilot"	, POS_SITTING , do_drive    , 0, ADMLVL_NONE	, 0 },
  { "plant"    , "plan"         , POS_STANDING, do_plant    , 0, ADMLVL_NONE    , 0 },
  { "play"     , "pla"          , POS_SITTING , do_play     , 0, ADMLVL_NONE    , 0 },
  { "players"  , "play"		, POS_DEAD    , do_plist    , 0, ADMLVL_IMPL	, 0 },
  { "poofin"   , "poofi"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFIN },
  { "poofout"  , "poofo"	, POS_DEAD    , do_poofset  , 0, ADMLVL_IMMORT	, SCMD_POOFOUT },
  { "pose"     , "pos"          , POS_STANDING, do_pose     , 0, ADMLVL_NONE    , 0 },
  { "post"     , "pos"          , POS_STANDING, do_post     , 0, ADMLVL_NONE    , 0 },
  { "potential", "poten"        , POS_STANDING, do_potential, 0, ADMLVL_NONE    , 0 },
  { "pour"     , "pour"		, POS_STANDING, do_pour     , 0, ADMLVL_NONE	, SCMD_POUR },
  { "powerup"  , "poweru"       , POS_FIGHTING, do_powerup  , 0, ADMLVL_NONE    , 0 },
  { "preference", "preferenc"   , POS_DEAD    , do_preference , 0, ADMLVL_NONE    , 0 },
  { "program"  , "progra"       , POS_DEAD    , do_oasis    , 0, ADMLVL_NONE  , SCMD_OASIS_REDIT },
  { "prompt"   , "pro"		, POS_DEAD    , do_display  , 0, ADMLVL_NONE	, 0 },
  { "practice" , "pra"		, POS_RESTING , do_practice , 1, ADMLVL_NONE	, 0 },
  { "psychic"  , "psychi"       , POS_FIGHTING, do_psyblast , 0, ADMLVL_NONE     , 0 },
  { "punch"    , "punc"         , POS_FIGHTING, do_punch    , 0, ADMLVL_NONE     , 0 },
  { "pushup"   , "pushu"        , POS_STANDING, do_pushup   , 0, ADMLVL_NONE     , 0 },
  { "put"      , "put"          , POS_RESTING , do_put      , 0, ADMLVL_NONE    , 0 },
  { "purge"    , "purge"	, POS_DEAD    , do_purge    , 0, ADMLVL_BUILDER	, 0 },

  { "qui"      , "qui"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, 0 },
  { "quit"     , "quit"		, POS_DEAD    , do_quit     , 0, ADMLVL_NONE	, SCMD_QUIT },

  { "radar"    , "rada"         , POS_RESTING , do_sradar   , 0, ADMLVL_NONE    , 0 },
  { "raise"    , "rai"		, POS_DEAD    , do_raise    , 0, ADMLVL_NONE	, 0 },
  { "rbank"    , "rban"         , POS_RESTING , do_rbank    , 0, ADMLVL_IMMORT  , 0 },
  { "refuel"   , "refue"        , POS_SITTING, do_refuel   , 0, ADMLVL_NONE    , 0 },
  { "resize"   , "resiz"        , POS_STANDING, do_resize   , 0, ADMLVL_NONE    , 0 },
  { "restring" , "restring"       , POS_STANDING, do_restring , 0, ADMLVL_NONE    , 0 },
  { "rclone"   , "rclon"        , POS_DEAD    , do_rcopy    , 0, ADMLVL_BUILDER      , 0 },
  { "rcopy"    , "rcopy"        , POS_DEAD    , do_rcopy    , 0, ADMLVL_BUILDER      , 0 },
  { "roomdisplay" , "roomdisplay"     , POS_STANDING	, do_rdisplay	, 0, ADMLVL_NONE	, 0 },
  { "read"     , "rea"          , POS_RESTING , do_look     , 0, ADMLVL_NONE    , SCMD_READ },
  { "recall"   , "reca"         , POS_STANDING, do_recall   , 0, ADMLVL_IMMORT     , 0 },
  { "recharge" , "rechar"       , POS_STANDING, do_recharge , 0, ADMLVL_NONE    , 0 },
  { "regenerate", "regen"       , POS_RESTING , do_regenerate, 0, ADMLVL_NONE    , 0 },
  { "renzokou" , "renzo"        , POS_FIGHTING, do_renzo     , 0, ADMLVL_NONE    , 0 },
  { "repair"   , "repai"        , POS_STANDING, do_srepair   , 0, ADMLVL_NONE    , 0 },
  { "reply"    , "rep"		, POS_SLEEPING, do_reply    , 0, ADMLVL_NONE	, 0 },
  { "rescue"   , "rescu"        , POS_STANDING, do_rescue   , 0, ADMLVL_NONE    , 0 },
  { "rest"     , "re"		, POS_RESTING , do_rest     , 0, ADMLVL_NONE	, 0 },
  { "reward"   , "rewar"        , POS_RESTING , do_reward   , 0, ADMLVL_IMMORT  , 0 },
  { "reload"   , "reload"	, POS_DEAD    , do_reboot   , 0, 5	, 0 },
  { "receive"  , "rece"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "remove"   , "rem"		, POS_RESTING , do_remove   , 0, ADMLVL_NONE	, 0 },
  { "rent"     , "rent"		, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "report"   , "repor"        , POS_DEAD    , do_gen_write, 0, ADMLVL_NONE    , SCMD_IDEA },
  { "reroll"   , "rero"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMPL	, SCMD_REROLL },
  { "respond"  , "resp" 	, POS_RESTING,  do_respond  , 1, ADMLVL_NONE	, 0 },
  { "restore"  , "resto"	, POS_DEAD    , do_restore  , 0, ADMLVL_GOD	, 0 },
  { "return"   , "retu"		, POS_DEAD    , do_return   , 0, ADMLVL_NONE	, 0 },
  { "redit"    , "redit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_REDIT },
  { "rip"      , "ri"           , POS_DEAD    , do_rip      , 0, ADMLVL_NONE    , 0 },
  { "rlist"    , "rlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_RLIST },
  { "rogafufuken" , "rogafu"    , POS_FIGHTING, do_rogafufuken, 0, ADMLVL_NONE    , 0 },
  { "roomflags", "roomf"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMMORT	, SCMD_ROOMFLAGS },
  { "roundhouse", "roundhou"    , POS_FIGHTING, do_roundhouse, 0, ADMLVL_NONE   , 0 },
  { "rpbank"   , "rpban"        , POS_SLEEPING, do_rpbank    , 0, ADMLVL_NONE   , 0 },
  { "rpp"      , "rpp"          , POS_SLEEPING, do_rpp       , 0, ADMLVL_NONE   , 0 },
  { "runic"    , "runi"         , POS_STANDING, do_runic     , 0, ADMLVL_NONE   , 0 },

  { "say"      , "say"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "'"        , "'"		, POS_RESTING , do_say      , 0, ADMLVL_NONE	, 0 },
  { "save"     , "sav"		, POS_SLEEPING, do_save     , 0, ADMLVL_NONE	, 0 },
  { "saveall"  , "saveall"	, POS_DEAD    , do_saveall  , 0, ADMLVL_BUILDER	, 0},
  { "sbc"      , "sbc"          , POS_FIGHTING, do_sbc      , 0, ADMLVL_NONE    , 0 },
  { "scan"     , "sca"          , POS_FIGHTING, do_scan     , 0, ADMLVL_NONE     , 0 },
  { "scatter"  , "scatte"       , POS_FIGHTING, do_scatter  , 0, ADMLVL_NONE    , 0 },
  { "score"    , "sc"		, POS_DEAD    , do_score    , 0, ADMLVL_NONE	, 0 },
  /*{ "scopy"    , "scopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, CON_SEDIT },*/
  { "scouter"  , "scou"         , POS_RESTING, do_scouter  , 0, ADMLVL_NONE     , 0 },
  { "scry"     , "scr"          , POS_STANDING, do_scry     , 0, ADMLVL_NONE     , 0 },
  { "seishou"  , "seisho"       , POS_FIGHTING, do_seishou  , 0, ADMLVL_NONE     , 0 },
  { "shell"    , "she"          , POS_STANDING, do_shell    , 0, ADMLVL_NONE    , 0 },
  { "shimmer"  , "shimme"       , POS_STANDING, do_shimmer  , 0, ADMLVL_NONE    , 0 },
  { "shogekiha", "shog"         , POS_STANDING, do_shogekiha, 0, ADMLVL_NONE    , 0 },
  { "shuffle"  , "shuff"        , POS_SITTING , do_shuffle  , 0, ADMLVL_NONE    , 0 },
  { "snet"     , "snet"         , POS_RESTING , do_snet     , 0, ADMLVL_NONE     , 0 },
  { "search"   , "sea"		, POS_STANDING, do_look     , 0, ADMLVL_NONE	, SCMD_SEARCH },
  { "sell"     , "sell"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "selfdestruct", "selfdest", POS_STANDING, do_selfd     , 0, ADMLVL_NONE    , 0 },
  { "sedit"    , "sedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_SEDIT },
  { "send"     , "send"		, POS_SLEEPING, do_send     , 0, ADMLVL_GOD	, 0 },
  { "sense"    , "sense"        , POS_RESTING, do_track    , 0, ADMLVL_NONE    , 0 },
  { "set"      , "set"		, POS_DEAD    , do_set      , 0, ADMLVL_IMMORT	, 0 },
  { "shout"    , "sho"		, POS_RESTING , do_gen_comm , 0, ADMLVL_NONE	, SCMD_SHOUT },
  { "show"     , "show"		, POS_DEAD    , do_showoff  , 0, ADMLVL_NONE	, 0 },
  { "shutdow"  , "shutdow"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, 0 },
  { "shutdown" , "shutdown"	, POS_DEAD    , do_shutdown , 0, ADMLVL_IMPL	, SCMD_SHUTDOWN },
  { "silk"     , "sil"          , POS_RESTING , do_silk     , 0, ADMLVL_NONE    , 0 },
  { "sip"      , "sip"		, POS_RESTING , do_drink    , 0, ADMLVL_NONE	, SCMD_SIP },
  { "sit"      , "sit"		, POS_RESTING , do_sit      , 0, ADMLVL_NONE	, 0 },
  { "situp"    , "situp"        , POS_STANDING, do_situp    , 0, ADMLVL_NONE     , 0 },
  { "skills"   , "skills"       , POS_SLEEPING, do_skills   , 0, ADMLVL_NONE    , 0 },
  { "skillset" , "skillset"	, POS_SLEEPING, do_skillset , 0, 5      	, 0 },
  { "slam"     , "sla"          , POS_FIGHTING, do_slam     , 0, ADMLVL_NONE    , 0 },
  { "sleep"    , "sl"		, POS_SLEEPING, do_sleep    , 0, ADMLVL_NONE	, 0 },
  { "slist"    , "slist"	, POS_SLEEPING, do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_SLIST },
  { "slowns"   , "slowns"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_SLOWNS },
  { "smote"    , "sm"           , POS_RESTING , do_echo     , 1, ADMLVL_NONE    , SCMD_SMOTE },
  { "sneak"    , "sneak"	, POS_STANDING, do_gen_tog  , 1, ADMLVL_NONE	, SCMD_SNEAK },
  { "snoop"    , "snoop"	, POS_DEAD    , do_snoop    , 0, ADMLVL_IMMORT	, 0 },
  { "song"     , "son"          , POS_RESTING , do_song     , 0, 0  , 0 },
  { "spiral"   , "spiral"       , POS_STANDING, do_spiral   , 0, ADMLVL_NONE    , 0 },
  { "socials"  , "socials"	, POS_DEAD    , do_commands , 0, ADMLVL_NONE	, SCMD_SOCIALS },
  { "solarflare", "solarflare"  , POS_FIGHTING, do_solar    , 0, ADMLVL_NONE    , 0 },
  { "spar"     , "spa"          , POS_FIGHTING, do_spar     , 0, ADMLVL_NONE    , 0 },
  { "spit"     , "spi"          , POS_STANDING, do_spit     , 0, ADMLVL_NONE    , 0 },
  { "spiritball", "spiritball"  , POS_FIGHTING, do_spiritball, 0, ADMLVL_NONE    , 0 },
  { "spiritcontrol", "spiritcontro", POS_RESTING, do_spiritcontrol, 0, ADMLVL_NONE, 0 },
  { "split"    , "split"	, POS_SITTING , do_split    , 1, ADMLVL_IMMORT	, 0 },
  { "speak"    , "spe"		, POS_RESTING , do_languages, 0, ADMLVL_NONE	, 0 },
  { "spells"   , "spel"		, POS_RESTING , do_spells   , 0, ADMLVL_IMMORT	, 0 },
  { "stand"    , "st"		, POS_RESTING , do_stand    , 0, ADMLVL_NONE	, 0 },
  { "starbreaker", "starbr"     , POS_FIGHTING, do_breaker  , 0, ADMLVL_NONE  , 0 },
  { "stake"    , "stak"         , POS_SLEEPING, do_beacon   , 0, 0 , 0 },
  { "stat"     , "stat"		, POS_DEAD    , do_stat     , 0, ADMLVL_IMMORT	, 0 },
  { "status"   , "statu"        , POS_DEAD    , do_status   , 0, 0 , 0 },  
  { "steal"    , "ste"		, POS_STANDING, do_steal    , 1, ADMLVL_NONE	, 0 },
  { "stone"    , "ston"         , POS_STANDING, do_spit     , 0, ADMLVL_NONE    , 0 },
  { "stop"     , "sto"          , POS_STANDING, do_stop     , 0, ADMLVL_NONE    , 0 },
  { "study"    , "stu"		, POS_RESTING , do_not_here , 0, ADMLVL_NONE	, 0 },
  { "summon"   , "summo"        , POS_STANDING, do_summon   , 0, ADMLVL_NONE    , 0 },
  { "sunder"   , "sunde"        , POS_STANDING, do_sunder   , 0, ADMLVL_NONE    , 0 },
  { "suppress" , "suppres"      , POS_STANDING, do_suppress , 0, ADMLVL_NONE    , 0 },
  { "swallow"  , "swall"        , POS_RESTING , do_use      , 0, ADMLVL_NONE  , SCMD_QUAFF },
  { "switch"   , "switch"	, POS_DEAD    , do_switch   , 0, ADMLVL_VICE	, 0 },
  { "syslog"   , "syslog"	, POS_DEAD    , do_syslog   , 0, ADMLVL_IMMORT	, 0 },

  /*{ "tcopy"    , "tcopy"  	, POS_DEAD    , do_oasis_copy, 0, ADMLVL_GOD	, SCMD_TEDIT },*/
  { "tailhide" , "tailh"  , POS_RESTING , do_tailhide       , 0, ADMLVL_NONE    , 0 },
  { "table"    , "tabl"         , POS_SITTING , do_table    , 0, ADMLVL_NONE    , 0 },
  { "teach"    , "teac"         , POS_STANDING, do_teach    , 0, ADMLVL_NONE    , 0 },
  { "tell"     , "tel"		, POS_DEAD    , do_tell     , 0, ADMLVL_NONE	, 0 },
  { "take"     , "tak"		, POS_RESTING , do_get      , 0, ADMLVL_NONE	, 0 },
  { "tailwhip" , "tailw"        , POS_FIGHTING, do_tailwhip , 0, ADMLVL_NONE    , 0 },
  { "taisha"   , "taish"        , POS_FIGHTING, do_taisha , 0, ADMLVL_NONE    , 0 },
  { "taste"    , "tas"		, POS_RESTING , do_eat      , 0, ADMLVL_NONE	, SCMD_TASTE },
  { "teleport" , "tele"		, POS_DEAD    , do_teleport , 0, ADMLVL_IMMORT	, 0 },
  { "telepathy", "telepa"       , POS_DEAD    , do_telepathy, 0, ADMLVL_NONE    , 0 },
  { "tedit"    , "tedit"	, POS_DEAD    , do_tedit    , 0, ADMLVL_GRGOD	, 0 },  
  { "test"     , "test"         , POS_DEAD    , do_gen_tog  , 0, ADMLVL_BUILDER    , SCMD_TEST },
  { "thaw"     , "thaw"		, POS_DEAD    , do_wizutil  , 0, ADMLVL_IMMORT	, SCMD_THAW },
  { "think"    , "thin"         , POS_DEAD    , do_think    , 0, ADMLVL_NONE    , 0 },
  { "throw"    , "thro"         , POS_FIGHTING, do_throw    , 0, ADMLVL_NONE  , 0 },
  { "title"    , "title"	, POS_DEAD    , do_title    , 0, ADMLVL_NONE	, 0 },
  { "time"     , "time"		, POS_DEAD    , do_time     , 0, ADMLVL_NONE	, 0 },
  { "toggle"   , "toggle"	, POS_DEAD    , do_toggle   , 0, ADMLVL_NONE	, 0 },
  { "toplist"  , "toplis"       , POS_DEAD    , do_toplist  , 0, ADMLVL_NONE    , 0 },
  { "trackthru", "trackthru"	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_IMPL	, SCMD_TRACK },
  { "train"    , "train"	, POS_STANDING, do_train    , 0, ADMLVL_NONE	, 0 },
  { "transfer" , "transfer"	, POS_SLEEPING, do_trans    , 0, ADMLVL_IMMORT	, 0 },
  { "transform", "transform"    , POS_FIGHTING, do_transform, 0, ADMLVL_NONE    , 0 },
  { "transo"   , "trans"        , POS_STANDING, do_transobj , 0, 5    , 0 },
  { "tribeam"  , "tribe"        , POS_FIGHTING, do_tribeam  , 0, ADMLVL_NONE    , 0},
  { "trigedit" , "trigedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_TRIGEDIT},
  { "trip"     , "trip"         , POS_FIGHTING, do_trip     , 0, ADMLVL_NONE    , 0},
  { "tsuihidan", "tsuihida"     , POS_FIGHTING, do_tsuihidan, 0, ADMLVL_NONE    , 0},
  { "tunnel"   , "tunne"        , POS_DEAD    , do_dig      , 0, ADMLVL_IMMORT  , 0 },
  { "twinslash", "twins"        , POS_FIGHTING, do_tslash   , 0, ADMLVL_NONE    , 0},
  { "twohand"  , "twohand"      , POS_DEAD    , do_twohand  , 0, ADMLVL_NONE    , 0},
  { "typo"     , "typo"		, POS_DEAD    , do_gen_write, 0, ADMLVL_NONE	, SCMD_TYPO },

  { "unlock"   , "unlock"	, POS_SITTING , do_gen_door , 0, ADMLVL_NONE	, SCMD_UNLOCK },
  { "ungroup"  , "ungroup"	, POS_DEAD    , do_ungroup  , 0, ADMLVL_NONE	, 0 },
  { "unban"    , "unban"	, POS_DEAD    , do_unban    , 0, ADMLVL_GRGOD	, 0 },
  { "unaffect" , "unaffect"	, POS_DEAD    , do_wizutil  , 0, ADMLVL_GOD	, SCMD_UNAFFECT },
  { "uppercut" , "upperc"       , POS_FIGHTING, do_uppercut , 0, ADMLVL_NONE   , 0 },
  { "upgrade"  , "upgrad"       , POS_RESTING , do_upgrade  , 0, ADMLVL_NONE    , 0 },
  { "uptime"   , "uptime"	, POS_DEAD    , do_date     , 0, ADMLVL_IMMORT	, SCMD_UPTIME },
  { "use"      , "use"		, POS_SITTING , do_use      , 1, ADMLVL_NONE	, SCMD_USE },
  { "users"    , "users"	, POS_DEAD    , do_users    , 0, ADMLVL_IMMORT	, 0 },

  { "value"    , "val"		, POS_STANDING, do_not_here , 0, ADMLVL_NONE	, 0 },
  { "varstat"  , "varst"        , POS_DEAD    , do_varstat  , 0, ADMLVL_IMMORT  , 0 },
  { "version"  , "ver"		, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_VERSION },
  { "vieworder", "view" 	, POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE	, SCMD_VIEWORDER },
  { "visible"  , "vis"		, POS_RESTING , do_visible  , 1, ADMLVL_NONE	, 0 },
  { "vnum"     , "vnum"		, POS_DEAD    , do_vnum     , 0, ADMLVL_IMMORT	, 0 },
  { "voice"    , "voic"         , POS_RESTING , do_voice    , 0, ADMLVL_NONE  , 0 },
  { "vstat"    , "vstat"	, POS_DEAD    , do_vstat    , 0, ADMLVL_IMMORT	, 0 },

  { "wake"     , "wa"		, POS_SLEEPING, do_wake     , 0, ADMLVL_NONE	, 0 },
  { "warppool" , "warppoo"      , POS_STANDING, do_warppool , 0, ADMLVL_NONE    , 0 },
  { "waterrazor", "waterraz"    , POS_STANDING, do_razor    , 0, ADMLVL_NONE    , 0 },
  { "waterspikes", "waterspik"  , POS_STANDING, do_spike    , 0, ADMLVL_NONE    , 0 },
  { "wear"     , "wea"		, POS_RESTING , do_wear     , 0, ADMLVL_NONE	, 0 },
  { "weather"  , "weather"	, POS_RESTING , do_weather  , 0, ADMLVL_NONE	, 0 },
  { "who"      , "who"		, POS_DEAD    , do_who      , 0, ADMLVL_NONE	, 0 },
  { "whoami"   , "whoami"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WHOAMI },
  { "whohide"  , "whohide"      , POS_DEAD    , do_gen_tog  , 0, ADMLVL_NONE    , SCMD_WHOHIDE },
  { "whois"    , "whois"	, POS_DEAD    , do_whois    , 0, ADMLVL_NONE	, 0 },
  { "where"    , "where"	, POS_RESTING , do_where    , 1, ADMLVL_IMMORT	, 0 },
  { "whisper"  , "whisper"	, POS_RESTING , do_spec_comm, 0, ADMLVL_NONE	, SCMD_WHISPER },
  { "wield"    , "wie"		, POS_RESTING , do_wield    , 0, ADMLVL_NONE	, 0 },
  { "will"     , "wil"          , POS_RESTING , do_willpower, 0, ADMLVL_NONE    , 0 },
  { "wimpy"    , "wimpy"	, POS_DEAD    , do_value    , 0, ADMLVL_NONE	, SCMD_WIMPY },
  { "withdraw" , "withdraw"	, POS_STANDING, do_not_here , 1, ADMLVL_NONE	, 0 },
  { "wire"     , "wir"          , POS_STANDING, do_not_here , 1, ADMLVL_NONE    , 0 },
  { "wiznet"   , "wiz"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { ";"        , ";"		, POS_DEAD    , do_wiznet   , 0, ADMLVL_IMMORT	, 0 },
  { "wizhelp"  , "wizhelp"	, POS_SLEEPING, do_commands , 0, ADMLVL_IMMORT	, SCMD_WIZHELP },
  { "wizlist"  , "wizlist"	, POS_DEAD    , do_gen_ps   , 0, ADMLVL_NONE	, SCMD_WIZLIST },
  { "wizlock"  , "wizlock"	, POS_DEAD    , do_wizlock  , 0, ADMLVL_IMMORT	, 0 },
  { "wizupdate", "wizupdate"    , POS_DEAD    , do_wizupdate, 0, ADMLVL_IMPL	, 0 },
  { "write"    , "write"	, POS_STANDING, do_write    , 1, ADMLVL_NONE	, 0 },


  { "zanzoken" , "zanzo"        , POS_FIGHTING, do_zanzoken , 0, ADMLVL_NONE, 0 },
  { "zen"      , "ze"           , POS_FIGHTING, do_zen      , 0, ADMLVL_NONE, 0 },
  { "zcheck"   , "zcheck"       , POS_DEAD    , do_zcheck   , 0, ADMLVL_GOD, 0 },
  { "zreset"   , "zreset"	, POS_DEAD    , do_zreset   , 0, ADMLVL_IMMORT	, 0 },
  { "zedit"    , "zedit"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_ZEDIT },
  { "zlist"    , "zlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_ZLIST },
  { "zpurge"   , "zpurge"       , POS_DEAD    , do_zpurge   , 0, ADMLVL_GRGOD, 0 },

  /* DG trigger commands */
  { "attach"   , "attach"	, POS_DEAD    , do_attach   , 0, ADMLVL_BUILDER	, 0 },
  { "detach"   , "detach"	, POS_DEAD    , do_detach   , 0, ADMLVL_BUILDER	, 0 },
  { "detect"   , "detec"        , POS_STANDING, do_radar    , 0, ADMLVL_NONE     , 0 },
  { "tlist"    , "tlist"	, POS_DEAD    , do_oasis    , 0, ADMLVL_IMMORT	, SCMD_OASIS_TLIST },
  { "tstat"    , "tstat"	, POS_DEAD    , do_tstat    , 0, ADMLVL_IMMORT	, 0 },
  { "masound"  , "masound"	, POS_DEAD    , do_masound  , -1, ADMLVL_NONE	, 0 },
  { "mheal"    , "mhea"         , POS_SITTING , do_mheal    , -1, ADMLVL_NONE   , 0 },
  { "mkill"    , "mkill"	, POS_STANDING, do_mkill    , -1, ADMLVL_NONE	, 0 },
  { "mjunk"    , "mjunk"	, POS_SITTING , do_mjunk    , -1, ADMLVL_NONE	, 0 },
  { "mdamage"  , "mdamage"	, POS_DEAD    , do_mdamage  , -1, ADMLVL_NONE	, 0 },
  { "mdoor"    , "mdoor"	, POS_DEAD    , do_mdoor    , -1, ADMLVL_NONE	, 0 },
  { "mecho"    , "mecho"	, POS_DEAD    , do_mecho    , -1, ADMLVL_NONE	, 0 },
  { "mechoaround", "mechoaround", POS_DEAD    , do_mechoaround, -1, ADMLVL_NONE	, 0 },
  { "msend"    , "msend"	, POS_DEAD    , do_msend    , -1, ADMLVL_NONE	, 0 },
  { "mload"    , "mload"	, POS_DEAD    , do_mload    , -1, ADMLVL_NONE	, 0 },
  { "mpurge"   , "mpurge"	, POS_DEAD    , do_mpurge   , -1, ADMLVL_NONE	, 0 },
  { "mgoto"    , "mgoto"	, POS_DEAD    , do_mgoto    , -1, ADMLVL_NONE	, 0 },
  { "mat"      , "mat"		, POS_DEAD    , do_mat      , -1, ADMLVL_NONE	, 0 },
  { "mteleport", "mteleport"	, POS_DEAD    , do_mteleport, -1, ADMLVL_NONE	, 0 },
  { "mforce"   , "mforce"	, POS_DEAD    , do_mforce   , -1, ADMLVL_NONE	, 0 },
  { "mremember", "mremember"	, POS_DEAD    , do_mremember, -1, ADMLVL_NONE	, 0 },
  { "mforget"  , "mforget"	, POS_DEAD    , do_mforget  , -1, ADMLVL_NONE	, 0 },
  { "mtransform", "mtransform"	, POS_DEAD    , do_mtransform, -1, ADMLVL_NONE	, 0 },
  { "mzoneecho", "mzoneecho"	, POS_DEAD    , do_mzoneecho, -1, ADMLVL_NONE	, 0 },
  { "vdelete"  , "vdelete"	, POS_DEAD    , do_vdelete  , 0, ADMLVL_BUILDER	, 0 },
  { "mfollow"  , "mfollow"	, POS_DEAD    , do_mfollow  , -1, ADMLVL_NONE	, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, ADMLVL_NONE	, 0 } };	/* this must be last */

const char *fill[] =
{
  "in",
  "into",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/* Used to roll starting PL/KI/ST in character creation */
int roll_stats(struct char_data *ch, int type, int bonus)
{

  int pool = 0, base_num = bonus, max_num = bonus;
  int powerlevel = 0, ki = 1, stamina = 2;

  if (type == powerlevel) {
   base_num = ch->real_abils.str * 3;
   max_num = ch->real_abils.str * 5;
  } else if (type == ki) {
   base_num = ch->real_abils.intel * 3;
   max_num = ch->real_abils.intel * 5;
  } else if (type == stamina) {
   base_num = ch->real_abils.con * 3;
   max_num = ch->real_abils.con * 5;
  }

 pool = rand_number(base_num, max_num) + bonus;
 
 return (pool);
}

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  int skip_ld = 0;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  switch (GET_POS(ch)) {
    case POS_DEAD:
    case POS_INCAP:
    case POS_MORTALLYW:
    case POS_STUNNED:
      GET_POS(ch) = POS_SITTING;
      break;
  }

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;


  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);


  if (!strcasecmp(arg, "-")) {
   return;
  }
  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. 
   */
  /* otherwise, find the command */
  {
  int cont;                                            /* continue the command checks */
  cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
  if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
  if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
  if (cont) return;                                    /* yes, command trigger took over */
  }
  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++) {
    if (!strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level &&
          GET_ADMLEVEL(ch) >= complete_cmd_info[cmd].minimum_admlevel)
	break;
  }

  char blah[MAX_INPUT_LENGTH];

  sprintf(blah, "%s", complete_cmd_info[cmd].command);
  if (!strcasecmp(blah, "throw"))
      ch->throws = rand_number(1, 3);


  if (*complete_cmd_info[cmd].command == '\n') {
     if (CONFIG_IMC_ENABLED && !IS_NPC(ch)) {
       if(!IS_NPC(ch) && !imc_command_hook(ch, arg, line)) {
         send_to_char(ch, "Huh!?!\r\n");
       } else {
         skip_ld = 1;
       }
     } else {
      send_to_char(ch, "Huh!?!\r\n");
      return;
     }
  }

  else if (!command_pass(blah, ch) && GET_ADMLEVEL(ch) < 1)
      send_to_char(ch, "It's unfortunate...\r\n");
  else if (check_disabled(&complete_cmd_info[cmd]))    /* is it disabled? */
      send_to_char(ch, "This command has been temporarily disabled.\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_GOOP) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You only have your internal thoughts until your body has finished regenerating!\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_ADMLEVEL(ch) < ADMLVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_SPIRAL))
    send_to_char(ch, "You are occupied with your Spiral Comet attack!\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_admlevel >= ADMLVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position && GET_POS(ch) != POS_FIGHTING) {
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
    }
  } else if (no_specials || !special(ch, cmd, line)) {
    if (!skip_ld) {
     ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
    } 
   }
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}


void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next, temp);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!strcasecmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char(ch, "Alias added.\r\n");
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae.
   */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}

void topLoad()
{
  FILE *file;
  char fname[40], line[256], filler[50];
  int x = 0;

  /* Read Toplist File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist")) {
    log("ERROR: Toplist file does not exist.");
    return;
  }
  else if (!(file = fopen(fname, "r"))) {
    log("ERROR: Toplist file does not exist.");
    return;
  }

 
  TOPLOADED = TRUE;

  while (!feof(file)) {
    get_line(file, line);
    sscanf(line, "%s %"I64T"\n", filler, &toppoint[x]);
    topname[x] = strdup(filler);
    *filler = '\0';
    x++;
  }
  fclose(file);
}

/* Write the toplist to file */
void topWrite(struct char_data *ch)
{
  if (GET_ADMLEVEL(ch) > 0 || IS_NPC(ch))
   return;

  if (TOPLOADED == FALSE) {
   return;
  }

  char fname[40];
  FILE *fl;
  char *positions[25];
  int64_t points[25] = {0};
  int x = 0, writeEm = FALSE, placed = FALSE, start = 0, finish = 25, location = -1;
  int progress = FALSE;

  if (!ch) {
   return;
  }

  if (!ch->desc || !GET_USER(ch)) {
   return;
  }

  for (x = start; x < finish; x++) { /* Save the places as they are right now */
   positions[x] = strdup(topname[x]);
   points[x] = toppoint[x];
  }

  /* Powerlevel Section */
   /* Set the start and finish for this section */
    start = 0;
    finish = 5;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_HIT(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_HIT(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the powerlevel section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the powerlevel section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;
  /* Ki Section         */
   /* Set the start and finish for this section */
    start = 5;
    finish = 10;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_MANA(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_MANA(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the ki section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the ki section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* Stamina Section    */
   /* Set the start and finish for this section */
    start = 10;
    finish = 15;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_MAX_MOVE(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_MAX_MOVE(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the stamina section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the stamina section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* Zenni Section      */
   /* Set the start and finish for this section */
    start = 15;
    finish = 20;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_NAME(ch))) { /* Name doesn't match */
      if (GET_BANK_GOLD(ch) + GET_GOLD(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_BANK_GOLD(ch) + GET_GOLD(ch);
       topname[x] = strdup(GET_NAME(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_NAME(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the zenni section.@D]\r\n", GET_NAME(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the zenni section.@D]\r\n", GET_NAME(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  /* RPP Section        */
   /* Set the start and finish for this section */
    start = 20;
    finish = 25;

   for (x = start; x < finish; x++) { /* Save the new spots */
    if (placed == FALSE) { /* They Haven't Placed */
     if (strcasecmp(topname[x], GET_USER(ch))) { /* Name doesn't match */
      if (GET_TRP(ch) > toppoint[x]) {
       free(topname[x]);
       toppoint[x] = GET_TRP(ch);
       topname[x] = strdup(GET_USER(ch));
       placed = TRUE;
       writeEm = TRUE;
       location = x;
      }
     } else { /* This is their spot already */
       placed = TRUE;
       location = finish;
     }
    } else { /* They have placed */
      if (x < finish && location < finish) {
         if (strcasecmp(positions[location], GET_USER(ch))) { /* This isn't their old spot */
          free(topname[x]);
          toppoint[x] = points[location];
          topname[x] = strdup(positions[location]);
          location += 1;
         } else { /* This IS their old spot */
           progress = TRUE;
           location += 1;
           free(topname[x]);
           toppoint[x] = points[location];
           topname[x] = strdup(positions[location]);
           location += 1;
          }
     }
    }
   } /* End Save New Spots*/

   if (progress == TRUE) {
    send_to_all("@D[@GToplist@W: @C%s @Whas moved up in rank in the RPP section.@D]\r\n", GET_USER(ch));
   } else if (placed == TRUE && location != finish) {
    send_to_all("@D[@GToplist@W: @C%s @Whas placed in the RPP section.@D]\r\n", GET_USER(ch));
   }

   location = -1;
   placed = FALSE;
   progress = FALSE;

  for(x = 0; x < 25; x++) {
   free(positions[x]);
  }

  if (writeEm == TRUE) {
   if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist"))
     return;

   if( !(fl = fopen(fname, "w")) ) {
     log("ERROR: could not save Toplist File, %s.", fname);
     return;
   }
    x = 0;
    while(x < 25) {
     fprintf(fl, "%s %"I64T"\n", topname[x], toppoint[x]);
     x++;
    }

   fclose(fl);
  }
  return;
}

int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}


/*
 * one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word.
 *
 * No longer ignores fill words.  -dak, 6 Jan 2003.
 */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

/*
 * Same as two_arguments only, well you get the idea... - Iovan
 *
 */
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg)
{
 return (one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg)); /* >.> */
}

void display_races(struct descriptor_data *d)
{
  int x, i=0;
  const char *cost[23] = {"Free",  /* 0 */
                          "60 RPP", /* 1 */
                          "Free", /* 2 */
                          "Free",  /* 3 */
                          "Free",  /* 4 */
                          "Free",  /* 5 */
                          "Free",  /* 6 */
                          "Free",  /* 7 */
                          "35 RPP",  /* 8 */
                          "Free",  /* 9 */
                          "Free",  /* 10 */
                          "55 RPP",  /* 11 */
                          "Free",  /* 12 */
                          "Free",  /* 13 */
                          "30 RPP",  /* 14 */
                          "Free",  /* 15 */
                          "Free", /* 16 */
                          "Free",  /* 17 */
                          "Free", /* 18 */
                          "Free",  /* 19 */
                          "Free", /* 20 */
                          "Free",  /* 21 */
                          "Free", /* 22 */
                          };

  send_to_char(d->character, "\r\n@YRace SELECTION menu:\r\n@D---------------------------------------\r\n@n");
  for (x = 0; x < NUM_RACES; x++)
    if (race_ok_gender[(int)GET_SEX(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%-15s@D[@R%-6s@D]@n%s", (x+1) != 21 ? x+1 : 16, pc_race_types[x], cost[x], !(++i %2) ? "\r\n" : "   ");

      send_to_char(d->character, "\n @BR@W) @CRandom Race Selection!\r\n@n");
      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WRace: @n");
}

void display_classes(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YSensei SELECTION menu:\r\n@D--------------------------------------\r\n@n");
  for (x = 0; x < NUM_BASIC_CLASSES; x++)
    if (class_ok_race[(int)GET_RACE(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%s@n%s", x+1, pc_class_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BR@W) @CRandom Sensei Selection!\r\n@n");
      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WSensei: @n");
}

void display_races_help(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YRace HELP menu:\r\n@G--------------------------------------------\r\n@n");
  for (x = 0; x < NUM_RACES; x++)
    if (race_ok_gender[(int)GET_SEX(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%-15s@n%s", x+1, pc_race_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Race #: @n");
}

void display_classes_help(struct descriptor_data *d)
{
  int x, i=0;

  send_to_char(d->character, "\r\n@YClass HELP menu:\r\n@G-------------------------------------------\r\n@n");
  for (x = 0; x < NUM_BASIC_CLASSES; x++)
    if (class_ok_race[(int)GET_RACE(d->character)][x])
      send_to_char(d->character, "@B%2d@W) @C%s@n%s", x+1, pc_class_types[x], !(++i%2) ? "\r\n" : "	");

      send_to_char(d->character, "\n @BT@W) @CToggle between SELECTION/HELP Menu\r\n@n");
      send_to_char(d->character, "\n@WHelp on Class #: @n");
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/*
 * Return first space-delimited token in arg1; remainder of string in arg2.
 *
 * NOTE: Requires sizeof(arg2) >= sizeof(string)
 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  if (arg2 != temp)
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;
  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);
  return (-1);
}


int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* This function needs to die. */
int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
        if (k->snoop_by) {
         k->snoop_by->snooping = d;
         d->snoop_by = k->snoop_by;
         k->snoop_by = NULL;
        }
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target)
    return (0);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->timer = 0;
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
     int count = 0, oldcount = HIGHPCOUNT;
     struct descriptor_data *k;

      for (k = descriptor_list; k; k = k->next) {
       if (!IS_NPC(k->character) && GET_LEVEL(k->character) > 3) {
        count += 1;
       }

      if (count > PCOUNT) {
       PCOUNT = count;
      }

      if (PCOUNT >= HIGHPCOUNT) {
       oldcount = HIGHPCOUNT;
       HIGHPCOUNT = PCOUNT;
       PCOUNTDATE = time(0);
      }

      }
      if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
       payout(0);
      }
      if (PCOUNT == HIGHPCOUNT) {
       payout(1);
      }
      if (PCOUNT > oldcount) {
       payout(2);
      }
      /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
    d->character->time.logon = time(0);
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_NONE, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    d->character->rp = d->rpp;
    if (has_mail(GET_IDNUM(d->character)))
      write_to_output(d, "You have mail waiting.\r\n");
    if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWIMM > GET_BOARD(d->character, 1))
      send_to_char(d->character, "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWCOD > GET_BOARD(d->character, 2))
      send_to_char(d->character, "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWBUI > GET_BOARD(d->character, 4))
      send_to_char(d->character, "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWDUO > GET_BOARD(d->character, 3))
      send_to_char(d->character, "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(d->character, 0))
      send_to_char(d->character, "\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(d->character))
      send_to_char(d->character, "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n", LASTNEWS);
    if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(d->character)) {
      int diff = (LASTINTEREST - GET_LINTEREST(d->character));
      int mult = 0;
      while (diff > 0) {
       if ((diff - 86400) < 0 && mult == 0) {
        mult = 1;
       }
       else if ((diff - 86400) >= 0) {
        diff -= 86400;
        mult++;
       }
       else {
        diff = 0;
       }
      }
        if (mult > 3) {
         mult = 3;
        }
      GET_LINTEREST(d->character) = LASTINTEREST;
      if (GET_BANK_GOLD(d->character) > 0) {
       int inc = ((GET_BANK_GOLD(d->character) / 100) * 2);
       if (inc >= 7500) {
        inc = 7500;
       }
       inc *= mult;
       GET_BANK_GOLD(d->character) += inc;
       send_to_char(d->character, "Interest happened while you were away, %d times.\r\n"
                                  "@cBank Interest@D: @Y%s@n\r\n", mult, add_commas(inc));
      }
    }
  if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
      d->comp->state = 1;	/* waiting for response to offer */
      write_to_output(d, "%s", compress_offer);
  }
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
        "$n's body has been taken over by a new spirit!",
        TRUE, d->character, 0, 0, TO_ROOM);
    d->character->rp = d->rpp;
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
          if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
              d->comp->state = 1;       /* waiting for response to offer */
              write_to_output(d, "%s", compress_offer);
          }
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
    int load_result;
    IDXTYPE load_room;
    struct char_data *check;


      reset_char(d->character);
      read_aliases(d->character);

      racial_body_parts(d->character);

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

      /*
       * We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE.
       */

      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_ADMLEVEL(d->character))
	  load_room = real_room(CONFIG_IMMORTAL_START);
	else
	  load_room = real_room(CONFIG_MORTAL_START);
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = real_room(CONFIG_FROZEN_START);

      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character);
      if (d->character->player_specials->host) {
        free(d->character->player_specials->host);
        d->character->player_specials->host = NULL;
      }
      d->character->player_specials->host = strdup(d->host);
      GET_ID(d->character) = GET_IDNUM(d->character);
      /* find_char helper */
      add_to_lookup_table(GET_ID(d->character), (void *)d->character);
      read_saved_vars(d->character);
      /*load_char_pets(d->character);*/
      for (check = character_list; check; check = check->next)
        if (!check->master && IS_NPC(check) && check->master_id == GET_IDNUM(d->character) &&
            AFF_FLAGGED(check, AFF_CHARM) && !circle_follow(check, d->character))
          add_follower(check, d->character);
      save_char(d->character);

    if (d->customfile != 1) {
     customCreate(d);
     userWrite(d, 0, 0, 0, "index");
    }

    if (GET_LIFEFORCE(d->character) == 0 || GET_LIFEFORCE(d->character) > GET_LIFEMAX(d->character)) {
     GET_LIFEFORCE(d->character) = GET_LIFEMAX(d->character);
    }   

    if (PLR_FLAGGED(d->character, PLR_RARM)) {
     GET_LIMBCOND(d->character, 0) = 100;
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_RARM);
    }
    if (PLR_FLAGGED(d->character, PLR_LARM)) {
     GET_LIMBCOND(d->character, 1) = 100;
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_LARM);
    }
    if (PLR_FLAGGED(d->character, PLR_LLEG)) {
     GET_LIMBCOND(d->character, 3) = 100;
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_LLEG);
    }
    if (PLR_FLAGGED(d->character, PLR_RLEG)) {
     GET_LIMBCOND(d->character, 2) = 100;
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_RLEG);
    }
    GET_COMBINE(d->character) = -1;
    GET_SLEEPT(d->character) = 8;
    GET_FOODR(d->character) = 2;
    if (AFF_FLAGGED(d->character, AFF_FLYING)) {
     GET_ALT(d->character) = 1;
    } else {
     GET_ALT(d->character) = 0;
    }
    if (AFF_FLAGGED(d->character, AFF_POSITION)) {
     REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_POSITION);
    }
    if (AFF_FLAGGED(d->character, AFF_SANCTUARY)) {
     REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_SANCTUARY);
    }
    if (AFF_FLAGGED(d->character, AFF_ZANZOKEN)) {
     REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_ZANZOKEN);
    }
    if (PLR_FLAGGED(d->character, PLR_KNOCKED)) {
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_KNOCKED);
    }
    if (IS_ANDROID(d->character) && !AFF_FLAGGED(d->character, AFF_INFRAVISION)) {
     SET_BIT_AR(AFF_FLAGS(d->character), AFF_INFRAVISION);
    }
    ABSORBING(d->character) = NULL;
    ABSORBBY(d->character) = NULL;
    SITS(d->character) = NULL;
    BLOCKED(d->character) = NULL;
    BLOCKS(d->character) = NULL;
    GET_OVERFLOW(d->character) = FALSE;
    GET_SPAM(d->character) = 0;
    GET_RMETER(d->character) = 0;
    if (!d->character->affected) {
     if (AFF_FLAGGED(d->character, AFF_HEALGLOW)) {
      REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_HEALGLOW);
     }
    }
    if (AFF_FLAGGED(d->character, AFF_HAYASA)) {
     GET_SPEEDBOOST(d->character) = GET_SPEEDCALC(d->character) * 0.5;
    } else {
     GET_SPEEDBOOST(d->character) = 0;
    }
    if (GET_TRP(d->character) < GET_RP(d->character)) {
     GET_TRP(d->character) = GET_RP(d->character);
    }

    if (IS_NAMEK(d->character) && GET_COND(d->character, HUNGER) >= 0) {
     GET_COND(d->character, HUNGER) = -1;
    }

    if (PLR_FLAGGED(d->character, PLR_HEALT)) {
     REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_HEALT);
    }
    if (CONFIG_IMC_ENABLED) {
     load_imc_pfile(d->character);
    }
    if (readIntro(d->character, d->character) == 2) {
     introCreate(d->character);
    }
    if (read_sense_memory(d->character, d->character) == 2) {
     senseCreate(d->character);
    }
    if (GET_ADMLEVEL(d->character) > 0) {
     d->level = 1;
    }

    if (GET_CLAN(d->character) != NULL && !strstr(GET_CLAN(d->character), "None")) {
     if(!clanIsMember(GET_CLAN(d->character), d->character)) {
       if (!clanIsModerator(GET_CLAN(d->character), d->character)) {
        if (!checkCLAN(d->character)) {
         write_to_output(d, "Your clan no longer exists.\r\n");
         GET_CLAN(d->character) = strdup("None.");
       }
      }
     }
    }
    d->character->rp = d->rpp;
	d->character->rbank = d->rbank;
    if (MOON_UP == FALSE && (IS_SAIYAN(d->character) || IS_HALFBREED(d->character))) {
     oozaru_drop(d->character);
    }
    if (MOON_UP == TRUE && (IS_SAIYAN(d->character) || IS_HALFBREED(d->character))) {
     oozaru_add(d->character);
    }
    if (IS_HOSHIJIN(d->character)) {
       if (time_info.day <= 14) {
        star_phase(d->character, 1);
       } else if (time_info.day <= 21) {
        star_phase(d->character, 2);
       } else {
        star_phase(d->character, 0);
       }
    }

    if (IS_ICER(d->character) && !GET_SKILL(d->character, SKILL_TAILWHIP)) {
     int numb = rand_number(20, 30);
     SET_SKILL(d->character, SKILL_TAILWHIP, numb);
    }
    else if (!IS_ICER(d->character) && GET_SKILL(d->character, SKILL_TAILWHIP)) {
     SET_SKILL(d->character, SKILL_TAILWHIP, 0);
    }
	
	if (IS_MUTANT(d->character) && (GET_GENOME(d->character, 0) == 9 || GET_GENOME(d->character, 1) == 9) && !GET_SKILL(d->character, SKILL_TELEPATHY)) {
	 SET_SKILL(d->character, SKILL_TELEPATHY, 50);
	}
	
	if (IS_BIO(d->character) && (GET_GENOME(d->character, 0) == 7 || GET_GENOME(d->character, 1) == 7) && !GET_SKILL(d->character, SKILL_TELEPATHY) && !GET_SKILL(d->character, SKILL_FOCUS)) {
	 SET_SKILL(d->character, SKILL_TELEPATHY, 30);
	 SET_SKILL(d->character, SKILL_FOCUS, 30);
	}
	
    COMBO(d->character) = -1;
    return load_result;
}

int readUserIndex(char *name) {
  char fname[40];
  FILE *fl;

  /* Read User Index */
  if (!get_filename(fname, sizeof(fname), USER_FILE, name)) {
    return 0;
  }
  else if (!(fl = fopen(fname, "r"))) {
    return 0;
  }
  fclose(fl);
  return 1;
}

void payout(int num)
{

 struct descriptor_data *k;
  if (LASTPAYOUT == 0) {
   LASTPAYOUT = time(0) + 86400;
   LASTPAYTYPE = num;
  }
  else if (num > LASTPAYTYPE) {
   LASTPAYOUT = time(0) + 86400;
   LASTPAYTYPE = num;
  }
  else if (LASTPAYOUT <= time(0)) {
   LASTPAYOUT = time(0) + 86400;
   LASTPAYTYPE = num;
  }
  for (k = descriptor_list; k; k = k->next) {
    if (GET_ADMLEVEL(k->character) <= 0 && IS_PLAYING(k) && GET_RTIME(k->character) < LASTPAYOUT) {
     if (num == 0) {
      k->rpp += 1;
      GET_RP(k->character) = k->rpp;
      GET_TRP(k->character) += 1;
      userWrite(k, 0, 0, 0, "index");
      send_to_char(k->character, "@D[@G+ 1 RPP@D] @cA total logon count within 4 of the highest has been achieved.@n\r\n");
     } else if (num == 1) {
      k->rpp += 2;
      GET_RP(k->character) = k->rpp;
      GET_TRP(k->character) += 2;
      userWrite(k, 0, 0, 0, "index");
      send_to_char(k->character, "@D[@G+ 2 RPP@D] @cThe total logon count has tied with the highest ever.@n\r\n");
     } else {
      k->rpp += 3;
      GET_RP(k->character) = k->rpp;
      GET_TRP(k->character) += 3;
      userWrite(k, 0, 0, 0, "index");
      send_to_char(k->character, "@D[@G+ 3 RPP@D] @cA new logon count record has been achieved!@n\r\n");
     }
     GET_RTIME(k->character) = LASTPAYOUT;
    }
 }
}

int command_pass(char *cmd, struct char_data *ch)
{

 if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
  if (strcasecmp(cmd, "liquefy") && strcasecmp(cmd, "ingest") && strcasecmp(cmd, "look") && strcasecmp(cmd, "score") && strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") && strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") && strcasecmp(cmd, "status")) {
   send_to_char(ch, "You are not capable of performing that action while liquefied!\r\n");
   return (FALSE);
  }
 } else if (IS_AFFECTED(ch, AFF_PARALYZE)) {
  if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") && strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") && strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") && strcasecmp(cmd, "status")) {
   send_to_char(ch, "You are not capable of performing that action while petrified!\r\n");
   return (FALSE);
  }
 } else if (IS_AFFECTED(ch, AFF_FROZEN)) {
  if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") && strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") && strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") && strcasecmp(cmd, "status")) {
   send_to_char(ch, "You are not capable of performing that action while a frozen block of ice!\r\n");
   return (FALSE);
  }
 } else if (IS_AFFECTED(ch, AFF_PARA) && GET_INT(ch) < rand_number(1, 60)) {
  if (strcasecmp(cmd, "look") && strcasecmp(cmd, "score") && strcasecmp(cmd, "ooc") && strcasecmp(cmd, "osay") && strcasecmp(cmd, "emote") && strcasecmp(cmd, "smote") && strcasecmp(cmd, "status")) {
   act("@yYou fail to overcome your paralysis!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@Y$n @ystruggles with $s paralysis!@n", TRUE, ch, 0, 0, TO_ROOM);
   return (FALSE);
  }
 }

 return (TRUE);
}

int lockRead(char *name) {
  char fname[40], filler[50], line[256];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    return 0;
  }
  else if (!(fl = fopen(fname, "r"))) {
    return 0;
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%s\n", filler);
    if (!strcasecmp(CAP(name), CAP(filler))) {
     known = TRUE;
    }
  }
   fclose(fl);

   if (known == TRUE)
    return 1;
   else
    return 0;
}

void userLoad(struct descriptor_data *d, char *name)
{
  char fname[40], filler[100], line[256];
  FILE *fl;

  /* Read and Write User Index */
  if (!get_filename(fname, sizeof(fname), USER_FILE, name)) {
    return;
  }
  else if (!(fl = fopen(fname, "r"))) {
    log("ERROR: could not load user, %s, from filename, %s.", name, fname);
    return;
  }
  int count = 0;
  while (!feof(fl)) {
    get_line(fl, line);
    count += 1;
    switch (count) {
     case 1:
      sscanf(line, "%s\n", filler);
      if (d->user) {
       free(d->user);
       d->user = NULL;
      }
      d->user = strdup(filler);
      break;
     case 2:
      sscanf(line, "%s\n", filler);
      if (d->email) {
       free(d->email);
       d->email = NULL;
      }
      d->email = strdup(filler);
      break;
     case 3:
      sscanf(line, "%s\n", filler);
      if (d->pass) {
       free(d->pass);
       d->pass = NULL;
      }
      d->pass = strdup(filler);
      break;
     case 4:
      sscanf(line, "%d\n", &d->total);
      break;
     case 5:
      sscanf(line, "%d\n", &d->rpp);
      break;
     case 6:
      sscanf(line, "%s\n", filler);
      if (d->tmp1) {
       free(d->tmp1);
       d->tmp1 = NULL;
      }
      d->tmp1 = strdup(filler);
      break;
     case 7:
      sscanf(line, "%s\n", filler);
      if (d->tmp2) {
       free(d->tmp2);
       d->tmp2 = NULL;
      }
      d->tmp2 = strdup(filler);
      break;
     case 8:
      sscanf(line, "%s\n", filler);
      if (d->tmp3) {
       free(d->tmp3);
       d->tmp3 = NULL;
      }
      d->tmp3 = strdup(filler);
      break;
     case 9:
      sscanf(line, "%s\n", filler);
      if (d->tmp4) {
       free(d->tmp4);
       d->tmp4 = NULL;
      }
      d->tmp4 = strdup(filler);
      break;
     case 10:
      sscanf(line, "%s\n", filler);
      if (d->tmp5) {
       free(d->tmp5);
       d->tmp5 = NULL;
      }
      d->tmp5 = strdup(filler);
      break;
     case 11:
      sscanf(line, "%d\n", &d->level);
      break;
     case 12:
      sscanf(line, "%d\n", &d->customfile);
      break;
	case 13:
	  sscanf(line, "%d\n", &d->rbank);
	  break;
    }
   *filler = '\0';
  }
  fclose(fl);
  return;
}

void userCreate(struct descriptor_data *d)
{
  char fname[40];
  FILE *fl;

  /* Write Their User File */

  if (!get_filename(fname, sizeof(fname), USER_FILE, d->user))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save user, %s, to filename, %s.", d->user, fname);
    return;
  }

  /* User's Account Name */
  fprintf(fl, "%s\n", CAP(d->user));
  /* User's Email */
  fprintf(fl, "%s\n", d->email);
  /* User's Password */
  fprintf(fl, "%s\n", d->pass);
  /* User's Allowed Player Total */
  fprintf(fl, "3\n");
  d->total = 3;
  /* User's Total RPP */
  fprintf(fl, "0\n");
  d->rpp = 0;
  /* Player Slots */
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "0\n");
  fprintf(fl, "1\n");
 /* User's Total RPP */
  fprintf(fl, "0\n");
  d->rbank = 0;
  

  fclose(fl);

  if (d->tmp1) {
   free(d->tmp1);
   d->tmp1 = NULL;
  }
  d->tmp1 = strdup("Empty");
  if (d->tmp2) {
   free(d->tmp2);
   d->tmp2 = NULL;
  }
  d->tmp2 = strdup("Empty");
  if (d->tmp3) {
   free(d->tmp3);
   d->tmp3 = NULL;
  }
  d->tmp3 = strdup("Empty");
  if (d->tmp4) {
   free(d->tmp4);
   d->tmp4 = NULL;
  }
  d->tmp4 = strdup("Empty");
  if (d->tmp5) {
   free(d->tmp5);
   d->tmp5 = NULL;
  }
  d->tmp5 = strdup("Empty");
  customCreate(d);

  return;
}

/* Delete A User Account */
void userDelete(struct descriptor_data *d) {
 int player_i;

 char fname[40];

 if (get_filename(fname, sizeof(fname), USER_FILE, d->user)) {
   if (!!strcasecmp(d->tmp1, "Empty")) {
     if ((player_i = get_ptable_by_name(d->tmp1)) >= 0)
       remove_player(player_i);
   }
   if (!!strcasecmp(d->tmp2, "Empty")) {
     if ((player_i = get_ptable_by_name(d->tmp2)) >= 0)
       remove_player(player_i);
   }
   if (!!strcasecmp(d->tmp3, "Empty")) {
     if ((player_i = get_ptable_by_name(d->tmp3)) >= 0)
       remove_player(player_i);
   }
   if (!!strcasecmp(d->tmp4, "Empty")) {
     if ((player_i = get_ptable_by_name(d->tmp4)) >= 0)
       remove_player(player_i);
   }
   if (!!strcasecmp(d->tmp5, "Empty")) {
     if ((player_i = get_ptable_by_name(d->tmp5)) >= 0)
       remove_player(player_i);
   }
   unlink(fname);
   return;
 }
 else {
  write_to_output(d, "Error. Your user file doesn't even exist!\n");
  return;
 }
}

/* For transfering money or doing things with an offline player */
char *rIntro(struct char_data *ch, char *arg) {
  char fname[40], filler[50], scrap[100], line[256];
  static char name[80];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (IS_NPC(ch)) {
   return "NOTHING";
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, GET_NAME(ch))) {
    return "NOTHING";
  }
  else if (!(fl = fopen(fname, "r"))) {
    return "NOTHING";
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%s %s\n", filler, scrap);
    if (!strcasecmp(arg, scrap)) {
     known = TRUE;
     sprintf(name, "%s", filler);
    }
  }
   fclose(fl);

   if (known == TRUE)
    return (name);
   else
    return "NOTHING";
}

/* Update A User Account */
void userWrite(struct descriptor_data *d, int setTot, int setRpp, int setRBank, char *name)
{
  char fname[40];
  FILE *fl;

 if (!strcasecmp(name, "index")) {
  if (!d) {
   return;
  }
  if (d->user == NULL) {
   return;
  }
  if (!get_filename(fname, sizeof(fname), USER_FILE, d->user))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save user, %s, to filename, %s.", d->user, fname);
    return;
  }

  /* User's Account Name */
  fprintf(fl, "%s\n", CAP(d->user));

  /* User's Email */
  fprintf(fl, "%s\n", d->email);

  /* User's Pass */
  fprintf(fl, "%s\n", d->pass);

  /* User's Allowed Player Total */
  if (setTot <= 3 || setTot > 5) {
   fprintf(fl, "%d\n", d->total);
  }
  else if (setTot > 3) {
   fprintf(fl, "%d\n", setTot);
   d->total = setTot;
  }
  /* User's RPP Total */
  if (setRpp == 0) {
    fprintf(fl, "%d\n", d->rpp);
  }
  else {
    d->rpp += setRpp;
    fprintf(fl, "%d\n", d->rpp);
  }

  /* Write Player Slots */
   if (d->tmp1 != NULL) {
    fprintf(fl, "%s\n", d->tmp1);
   } else {
    fprintf(fl, "Empty\n");
   }
   if (d->tmp2 != NULL) {
    fprintf(fl, "%s\n", d->tmp2);
   } else {
    fprintf(fl, "Empty\n");
   }
   if (d->tmp3 != NULL) {
    fprintf(fl, "%s\n", d->tmp3);
   } else {
    fprintf(fl, "Empty\n");
   }
   if (d->tmp4 != NULL) {
    fprintf(fl, "%s\n", d->tmp4);
   } else {
    fprintf(fl, "Empty\n");
   }
   if (d->tmp5 != NULL) {
    fprintf(fl, "%s\n", d->tmp5);
   } else {
    fprintf(fl, "Empty\n");
   }

   fprintf(fl, "%d\n", d->level);

   /* Write whether we have a custom file or not */
   fprintf(fl, "%d\n", d->customfile);
   
  /* User's RPP Bank Total */
  if (setRBank == 0) {
    fprintf(fl, "%d\n", d->rbank);
  }
  else {
    d->rbank += setRBank;
    fprintf(fl, "%d\n", d->rbank);
  }

   fclose(fl);
   return;
 }
 else if (strcasecmp(name, "index")) {
  char filename[40], uname[100], email[100], pass[100], tmp1[100], tmp2[100], tmp3[100], tmp4[100], tmp5[100], line[256];
  int total = 0, rpp = 0, level = 0, custom = 0, rbank = 0;
  FILE *file;

  /* Read and Write User Index */
  if (!get_filename(filename, sizeof(filename), USER_FILE, name)) {
    return;
  }
  else if (!(file = fopen(filename, "r"))) {
    log("ERROR: could not load user, %s, from filename, %s.", name, filename);
    return;
  }

  int count = 0;
  while (!feof(file)) {
    get_line(file, line);
    count += 1;
    switch (count) {
     case 1:
      sscanf(line, "%s\n", uname);
      break;
     case 2:
      sscanf(line, "%s\n", email);
      break;
     case 3:
      sscanf(line, "%s\n", pass);
      break;
     case 4:
      sscanf(line, "%d\n", &total);
      break;
     case 5:
      sscanf(line, "%d\n", &rpp);
      break;
     case 6:
      sscanf(line, "%s\n", tmp1);
      break;
     case 7:
      sscanf(line, "%s\n", tmp2);
      break;
     case 8:
      sscanf(line, "%s\n", tmp3);
      break;
     case 9:
      sscanf(line, "%s\n", tmp4);
      break;
     case 10:
      sscanf(line, "%s\n", tmp5);
      break;
     case 11:
      sscanf(line, "%d\n", &level);
      break;
     case 12:
      sscanf(line, "%d\n", &custom);
      break;
	case 13:
	 sscanf(line, "%d\n",  &rbank);
	 break;
    }
  }
  fclose(file);

  if (!get_filename(fname, sizeof(fname), USER_FILE, name))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save user, %s, to filename, %s.", name, fname);
    return;
  }
  /* User's Account Name */
  fprintf(fl, "%s\n", uname);
  /* User's Email */
  fprintf(fl, "%s\n", email);

  /* User's Pass */
  fprintf(fl, "%s\n", pass);

  /* User's Allowed Player Total */
  if (setTot <= 3 || setTot > 5) {
   fprintf(fl, "%d\n", total);
  }
  else if (setTot > 3) {
   fprintf(fl, "%d\n", setTot);
  }
  /* User's RPP Total */
  if (setRpp == 0) {
    fprintf(fl, "%d\n", rpp);
  }
  else if (rpp + setRpp < 0) {
   send_to_imm("RPP would be below 0, reward canceled.");
   fprintf(fl, "%d\n", rpp);
  }
  else {
    rpp += setRpp;
    fprintf(fl, "%d\n", rpp);
  }

  /* Write Player Slots */
   fprintf(fl, "%s\n", tmp1);
   fprintf(fl, "%s\n", tmp2);
   fprintf(fl, "%s\n", tmp3);
   fprintf(fl, "%s\n", tmp4);
   fprintf(fl, "%s\n", tmp5);
   fprintf(fl, "%d\n", level);
   fprintf(fl, "%d\n", custom);
   
  /* User's RPP Total */
  if (setRBank == 0) {
    fprintf(fl, "%d\n", rbank);
  }
  else if (rbank + setRBank < 0) {
   send_to_imm("RPP Bank would be below 0, reward canceled.");
   fprintf(fl, "%d\n", rbank);
  }
  else {
    rbank += setRBank;
    fprintf(fl, "%d\n", rbank);
  }
  
   fclose(fl);
   return;
 }
 else {
  send_to_imm("Error with userWrite!");
  return;
 }

}

void fingerUser(struct char_data *ch, char *name)
{
  char filename[40], uname[100], email[100], pass[100], tmp1[100], tmp2[100], tmp3[100], tmp4[100], tmp5[100], line[256];
  int total = 0, rpp = 0, rbank = 0;
  FILE *file;

  /* Read and Write User Index */
  if (!get_filename(filename, sizeof(filename), USER_FILE, name)) {
    send_to_char(ch, "That user doesn't exist.\r\n");
    return;
  }
  else if (!(file = fopen(filename, "r"))) {
    send_to_char(ch, "That user is bugged! Report to Iovan.\r\n");
    return;
  }

  int count = 0;
  while (!feof(file)) {
    get_line(file, line);
    count += 1;
    switch (count) {
     case 1:
      sscanf(line, "%s\n", uname);
      break;
     case 2:
      sscanf(line, "%s\n", email);
      break;
     case 3:
      sscanf(line, "%s\n", pass);
      break;
     case 4:
      sscanf(line, "%d\n", &total);
      break;
     case 5:
      sscanf(line, "%d\n", &rpp);
      break;
     case 6:
      sscanf(line, "%s\n", tmp1);
      break;
     case 7:
      sscanf(line, "%s\n", tmp2);
      break;
     case 8:
      sscanf(line, "%s\n", tmp3);
      break;
     case 9:
      sscanf(line, "%s\n", tmp4);
      break;
     case 10:
      sscanf(line, "%s\n", tmp5);
      break;
	case 13:
	  sscanf(line, "%d\n", &rbank);
	  break;
    }
  }
  fclose(file);

  send_to_char(ch, "@D[@gUsername   @D: @w%-30s@D]@n\r\n", uname);
  send_to_char(ch, "@D[@gEmail      @D: @w%-30s@D]@n\r\n", email);
  if (GET_ADMLEVEL(ch) > 4) {
   send_to_char(ch, "@D[@gPass       @D: @w%-30s@D]@n\r\n", pass);
  }
  else if (GET_ADMLEVEL(ch) > 0) {
   send_to_char(ch, "@D[@gPass       @D: @w%-30s@D]@n\r\n", "??????????");
  }
  send_to_char(ch, "@D[@gTotal Slots@D: @w%-30d@D]@n\r\n", total);
  send_to_char(ch, "@D[@gRP Points  @D: @w%-30d@D]@n\r\n", rpp);
  send_to_char(ch, "@D[@gRP Bank    @D: @w%-30d@D]@n\r\n", rbank);
  if (GET_ADMLEVEL(ch) > 0) {
   send_to_char(ch, "@D[@gCh. Slot 1 @D: @w%-30s@D]@n\r\n", tmp1);
   send_to_char(ch, "@D[@gCh. Slot 2 @D: @w%-30s@D]@n\r\n", tmp2);
   send_to_char(ch, "@D[@gCh. Slot 3 @D: @w%-30s@D]@n\r\n", tmp3);
   send_to_char(ch, "@D[@gCh. Slot 4 @D: @w%-30s@D]@n\r\n", tmp4);
   send_to_char(ch, "@D[@gCh. Slot 5 @D: @w%-30s@D]@n\r\n", tmp5);
   send_to_char(ch, "\n");
   customRead(ch->desc, 1, name);
  }
  return;
}

void userRead(struct descriptor_data *d)
{
   write_to_output(d, "                 @RUser Menu@n\n");
   write_to_output(d, "@D=============================================@n\r\n");
   write_to_output(d, "@D|@gUser Account  @D: @w%-27s@D|@n\n", d->user);
   write_to_output(d, "@D|@gEmail Address @D: @w%-27s@D|@n\n", d->email);
   write_to_output(d, "@D|@gMax Characters@D: @w%-27d@D|@n\n", d->total);
   write_to_output(d, "@D|@gRP Points     @D: @w%-27d@D|@n\n", d->rpp);
   write_to_output(d, "@D|@gRP Bank       @D: @w%-27d@D|@n\n", d->rbank);
   write_to_output(d, "@D=============================================@n\r\n\r\n");
   write_to_output(d, "      @D[@y----@YSelect A Character Slot@y----@D]@n\n");
   write_to_output(d, "                @B(@W1@B) @C%s@n\n", d->tmp1);
   write_to_output(d, "                @B(@W2@B) @C%s@n\n", d->tmp2);
   write_to_output(d, "                @B(@W3@B) @C%s@n\n", d->tmp3);
  if (d->total > 3) {
   write_to_output(d, "                @B(@W4@B) @C%s@n\n", d->tmp4);
  }
  if (d->total > 4) {
   write_to_output(d, "                @B(@W5@B) @C%s@n\n", d->tmp5);
  }
   write_to_output(d, "\n");
   write_to_output(d, "      @D[@y---- @YSelect Another Option @y----@D]@n\n");
   write_to_output(d, "                @B(@WB@B) @CBuy New C. Slot @D(@R15 RPP@D)@n\n");
   write_to_output(d, "                @B(@WC@B) @CUser's Customs\n");
   write_to_output(d, "                @B(@WD@B) @RDelete User@n\n");
   write_to_output(d, "                @B(@WE@B) @CEmail@n\n");
   write_to_output(d, "                @B(@WP@B) @CNew Password@n\n");
   write_to_output(d, "                @B(@WQ@B) @CQuit@n\n");
   write_to_output(d, "\r\nMake your choice: \n");
}

/* This is the bonus/negatives menu for Character Creation */
void display_bonus_menu(struct char_data *ch, int type)
{

 int BonusCount = 26, NegCount = 52, x = 0, y = 0;
 const char *bonus[] = {
                   "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost             @D[@G-2pts @D]", /* Bonus 0 */
                   "Prodigy     - +25% Experience Gained Until Level 80                  @D[@G-5pts @D]", /* Bonus 1 */
		   "Quick Study - Character auto-trains skills faster                    @D[@G-3pts @D]", /* Bonus 2 */
		   "Die Hard    - Life Force's PL regen doubled, but cost is the same    @D[@G-6pts @D]", /* Bonus 3 */
		   "Brawler     - Physical attacks do 20% more damage                    @D[@G-4pts @D]", /* Bonus 4 */
		   "Destroyer   - Damaged Rooms act as regen rooms for you               @D[@G-3pts @D]", /* Bonus 5 */
		   "Hard Worker - Physical rewards better + activity drains less stamina @D[@G-3pts @D]", /* Bonus 6 */
		   "Healer      - Heal/First-aid/Vigor/Repair restore +10%               @D[@G-3pts @D]", /* Bonus 7 */
		   "Loyal       - +20% Experience When Grouped As Follower               @D[@G-2pts @D]", /* Bonus 8 */
		   "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%     @D[@G-5pts @D]", /* Bonus 9 */
		   "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75% @D[@G-5pts @D]", /* Bonus 10 */
		   "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%       @D[@G-5pts @D]", /* Bonus 11 */
		   "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%      @D[@G-4pts @D]", /* Bonus 12 */
		   "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%        @D[@G-6pts @D]", /* Bonus 13 */
		   "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%       @D[@G-5pts @D]", /* Bonus 14 */
                   "Thick Skin  - -20% Physical and -10% ki dmg received                 @D[@G-5pts @D]", /* Bonus 15 */
                   "Recipe Int. - Food cooked by you lasts longer/heals better           @D[@G-2pts @D]", /* Bonus 16 */
                   "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn         @D[@G-4pts @D]", /* Bonus 17 */
                   "Powerhitter - 15% critical hits will be x4 instead of x2             @D[@G-4pts @D]", /* Bonus 18 */
                   "Healthy     - 40% chance to recover from ill effects when sleeping   @D[@G-3pts @D]", /* Bonus  19 */
                   "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis    @D[@G-2pts @D]", /* Bonus  20 */
                   "Evasive     - +15% to dodge rolls                                    @D[@G-3pts @D]", /* Bonus  21 */
                   "The Wall    - +20% chance to block                                   @D[@G-3pts @D]", /* Bonus  22 */
                   "Accurate    - +20% chance to hit physical, +10% to hit with ki       @D[@G-3pts @D]", /* Bonus  23 */
                   "Energy Leech- -2% ki damage received for every 5 character levels,   @D[@G-5pts @D]\n                  @cas long as you can take that ki to your charge pool.@D        ", /* Bonus  24*/
                   "Good Memory - +2 Skill Slots initially, +1 every 20 levels after     @D[@G-6pts @D]", /* Bonus 25 */
                   "Soft Touch  - Half damage for all hit locations                      @D[@G+5pts @D]", /* Neg 26 */
                   "Late Sleeper- Can only wake automatically. 33% every hour if maxed   @D[@G+5pts @D]", /* Neg 27 */
                   "Impulse Shop- +25% shop costs                                        @D[@G+3pts @D]", /* Neg 28 */
                   "Sickly      - Suffer from harmful effects longer                     @D[@G+5pts @D]", /* Neg 29 */
                   "Punching Bag- -15% to dodge rolls                                    @D[@G+3pts @D]", /* Neg 30 */
                   "Pushover    - -20% block chance                                      @D[@G+3pts @D]", /* Neg 31 */
                   "Poor D. Perc- -20% chance to hit with physical, -10% with ki         @D[@G+3pts @D]", /* Neg 32 */
                   "Thin Skin   - +20% physical and +10% ki damage received              @D[@G+4pts @D]", /* Neg 33 */
                   "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned            @D[@G+5pts @D]", /* Neg 34 */
                   "Energy Int. - +2% ki damage received for every 5 character levels,   @D[@G+6pts @D]\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
                   "Coward      - Can't Attack Enemy With 150% Your Powerlevel           @D[@G+6pts @D]", /* Neg 36 */
		   "Arrogant    - Cannot Suppress                                        @D[@G+1pt  @D]", /* Neg 37 */
		   "Unfocused   - Charge concentration randomly breaks                   @D[@G+3pts @D]", /* Neg 38 */
		   "Slacker     - Physical activity drains more stamina                  @D[@G+3pts @D]", /* Neg 39 */
		   "Slow Learner- Character auto-trains skills slower                    @D[@G+3pts @D]", /* Neg 40 */
		   "Masochistic - Defense Skills Cap At 75                               @D[@G+5pts @D]", /* Neg 41 */
		   "Mute        - Can't use IC speech related commands                   @D[@G+4pts @D]", /* Neg 42 */
		   "Wimp        - Strength is capped at 45                               @D[@G+6pts @D]", /* Neg 43 */
		   "Dull        - Intelligence is capped at 45                           @D[@G+6pts @D]", /* Neg 44 */
		   "Foolish     - Wisdom is capped at 45                                 @D[@G+6pts @D]", /* Neg 45 */
		   "Clumsy      - Agility is capped at 45                                @D[@G+3pts @D]", /* Neg 46 */
		   "Slow        - Speed is capped at 45                                  @D[@G+6pts @D]", /* Neg 47 */
		   "Frail       - Constitution capped at 45                              @D[@G+4pts @D]", /* Neg 48 */
		   "Sadistic    - Half Experience Gained For Quick Kills                 @D[@G+3pts @D]", /* Neg 49 */
		   "Loner       - Can't Group, +5% Train gains, +10% to physical gains   @D[@G+2pts @D]", /* Neg 50 */
                   "Bad Memory  - -5 Skill Slots                                         @D[@G+6pts @D]"  /* Neg 51 */
                  };

 if (type == 0) {
  send_to_char(ch, "\r\n@YBonus Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
  for (x = 0; x < BonusCount; x++) {
    send_to_char(ch, "@C%-2d@D)@c %s <@g%s@D>\n", x + 1, bonus[x], GET_BONUS(ch, x) > 0 ? "X" : " ");
  }
  send_to_char(ch, "\n");
 }
 
 if (type == 1) {
   y = BonusCount;
   send_to_char(ch, "@YNegative Trait SELECTION menu:\r\n@D---------------------------------------\r\n@n");
   while (y < NegCount) {
    send_to_char(ch, "@R%-2d@D)@r %s <@g%s@D>\n", y - 14, bonus[y], GET_BONUS(ch, y) > 0 ? "X" : " ");
    y += 1;
   }
 }

 if (type == 0) {
   send_to_char(ch, "\n@CN@D)@c Show Negatives@n\n");
 } else {
   send_to_char(ch, "\n@CB@D)@c Show Bonuses@n\n");
 }
  send_to_char(ch, "@CX@D)@c Exit Traits Section and complete your character@n\n");
  send_to_char(ch, "@D---------------------------------------\n[@WCurrent Points Pool@W: @y%d@D] [@WPTS From Neg@W: @y%d@D]@w\n", GET_CCPOINTS(ch), GET_NEGCOUNT(ch));
}

 /* Return -1 if not an acceptable menu option *
 * Return 31 if selection is X                 *
 * Return other value if Bonus/Negative        */

int parse_bonuses(const char *arg)
{
 int value = -1, ident = -1;
 
 switch (*arg) {
  case 'b':
  case 'B':
   value = 53;
   break;
  case 'n':
  case 'N':
   value = 54;
   break;
  case 'x':
  case 'X':
   value = 55;
   break;
 }
 
 if (value < 52) {
  ident = atoi(arg);
 }
  
 switch (ident) {
  /* Valid Selections */
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
  case 13:
  case 14:
  case 15:
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  case 21:
  case 22:
  case 23:
  case 24:
  case 25:
  case 26:
  case 27:
  case 28:
  case 29:
  case 30:
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
  case 36:
  case 37:
  case 38:
  case 39:
  case 40:
  case 41:
  case 42:
  case 43:
  case 44:
  case 45:
  case 46:
  case 47:
  case 48:
  case 49:
  case 50:
  case 51:
   value = ident - 1;
   break;
   
  /* Invalid Selections */
  default:
   value = -1;
   break;
 }

 return (value);
}

/* List names of Bonus/Negative */
const char *list_bonus[] = {
		                   "Thrifty", /* Bonus 0 */
                		   "Prodigy", /* Bonus 1 */
				   "Quick Study", /* Bonus 2 */
				   "Die Hard", /* Bonus 3 */
				   "Brawler", /* Bonus 4 */
				   "Destroyer", /* Bonus 5 */
				   "Hard Worker", /* Bonus 6 */
				   "Healer", /* Bonus 7 */
				   "Loyal", /* Bonus 8 */
				   "Brawny", /* Bonus 9 */
				   "Scholarly", /* Bonus 10 */
				   "Sage", /* Bonus 11 */
				   "Agile", /* Bonus 12 */
				   "Quick", /* Bonus 13 */
				   "Sturdy", /* Bonus 14 */
                                   "Thick Skin", /* Bonus 15 */
                                   "Recipe Int", /* Bonus 16 */
                                   "Fireproof", /* Bonus 17 */
                                   "Powerhitter", /* Bonus 18 */
                                   "Healthy", /* Bonus 19 */
                                   "Insomniac", /* Bonus 20 */
                                   "Evasive", /* Bonus 21 */
                                   "The Wall", /* Bonus 22 */
                                   "Accurate", /* Bonus 23 */
                                   "Energy Leech", /* Bonus 24 */
                                   "Good Memory", /* Bonus 25*/
                                   "Soft Touch", /* Neg 26 */
                                   "Late Sleeper", /* Neg 27 */
                                   "Impulse Shop", /* Neg 28 */
                                   "Sickly", /* Neg 29 */
                                   "Punching Bag", /* Neg 30 */
                                   "Pushover", /* Neg 31 */
                                   "Poor Depth Perception", /* Neg 32 */
                                   "Thin Skin", /* Neg 33 */
                                   "Fireprone", /* Neg 34 */
                                   "Energy Intollerant", /* Neg 35 */
		                   "Coward", /* Neg 36 */
				   "Arrogant", /* Neg 37 */
				   "Unfocused", /* Neg 38 */
				   "Slacker", /* Neg 39 */
				   "Slow Learner", /* Neg 40 */
				   "Masochistic", /* Neg 41 */
				   "Mute", /* Neg 42 */
				   "Wimp", /* Neg 43 */
				   "Dull", /* Neg 44 */
				   "Foolish", /* Neg 45 */
				   "Clumsy", /* Neg 46 */
				   "Slow", /* Neg 47 */
				   "Frail", /* Neg 48 */
				   "Sadistic", /* Neg 49 */
				   "Loner", /* Neg 50 */
                                   "Bad Memory" /* Neg 51 */
};

/* List cost of bonus/negative */
const int list_bonus_cost[] = {
 -2, /* Bonus 0 */
 -5, /* Bonus 1 */
 -3, /* Bonus 2 */
 -6, /* Bonus 3 */
 -4, /* Bonus 4 */
 -3, /* Bonus 5 */
 -3, /* Bonus 6 */
 -3, /* Bonus 7 */
 -2, /* Bonus 8 */
 -5, /* Bonus 9 */
 -5, /* Bonus 10 */
 -5, /* Bonus 11 */
 -4, /* Bonus 12 */
 -6, /* Bonus 13 */
 -5, /* Bonus 14 */
 -5, /* Bonus 15 */
 -2, /* Bonus 16 */
 -4, /* Bonus 17 */
 -4, /* Bonus 18 */
 -3, /* Bonus 19 */
 -2, /* Bonus 20 */
 -3, /* Bonus 21 */
 -3, /* Bonus 22 */
 -4, /* Bonus 23 */
 -5, /* Bonus 24 */
 -6, /* Bonus 25*/
 5, /* Negative 26 */
 5, /* Negative 27 */
 3, /* Negative 28 */
 5, /* Negative 29 */
 3, /* Negative 30 */
 3, /* Negative 31 */
 4, /* Negative 32 */
 4, /* Negative 33 */
 5, /* Negative 34 */
 6, /* Negative 35 */
 6, /* Negative 36 */
 1, /* Negative 37 */
 3, /* Negative 38 */
 3, /* Negative 39 */
 3, /* Negative 40 */
 5, /* Negative 41 */
 4, /* Negative 42 */
 6, /* Negative 43 */
 6, /* Negative 44 */
 6, /* Negative 45 */
 3, /* Negative 46 */
 6, /* Negative 47 */
 4, /* Negative 48 */
 3, /* Negative 49 */
 2, /* Negative 50 */
 6, /* Negative 51 */
};

int opp_bonus(struct char_data *ch, int value, int type)
{
 int give = TRUE;

 switch (value) {
  case 0:
   if (GET_BONUS(ch, BONUS_IMPULSE)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[BONUS_IMPULSE], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 2:
   if (GET_BONUS(ch, 40) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[40], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 3:
   if (IS_ANDROID(ch)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[3]);
    give = FALSE;
   }
   break;
  case 6:
   if (GET_BONUS(ch, 39) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[39], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 8:
   if (GET_BONUS(ch, 50) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[50], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 9:
   if (GET_BONUS(ch, 43) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[43], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 10:
   if (GET_BONUS(ch, 44) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[44], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 11:
   if (GET_BONUS(ch, 45) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[45], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 12:
   if (GET_BONUS(ch, 46) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[46], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 13:
   if (GET_BONUS(ch, 47) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[47], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 14:
   if (GET_BONUS(ch, 48) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[48], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 15:
   if (GET_BONUS(ch, 33) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[33], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 16:
   if (IS_ANDROID(ch)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "You are an android and can not suppress anyway.\n\n");
    give = FALSE;
   }
   break;
  case 17:
   if (IS_DEMON(ch)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "As a demon you are already fireproof.\r\n");
    give = FALSE;
   } else if (GET_BONUS(ch, 34) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[34], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 18:
   if (GET_BONUS(ch, 26) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[26], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 19:
   if (GET_BONUS(ch, 29) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[29], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 20:
   if (GET_BONUS(ch, 27) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[27], list_bonus[value]);
    give = FALSE;
   } else if (IS_ANDROID(ch)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[value]);
    give = FALSE;
   }
   break;
  case 21:
   if (GET_BONUS(ch, 30) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[30], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 22:
   if (GET_BONUS(ch, 31) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[31], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 23:
   if (GET_BONUS(ch, 32) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[32], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 24:
   if (GET_BONUS(ch, 35) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[35], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 25:
   if (GET_BONUS(ch, 51) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[51], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 26:
   if (GET_BONUS(ch, 18) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[18], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 27:
   if (GET_BONUS(ch, 20) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[27], list_bonus[value]);
    give = FALSE;
   } else if (IS_ANDROID(ch)) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "You can not take %s as an android!@n\r\n", list_bonus[value]);
    give = FALSE;
   }
   break;
  case 29:
   if (GET_BONUS(ch, 19) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[19], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 30:
   if (GET_BONUS(ch, 21) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[21], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 31:
   if (GET_BONUS(ch, 22) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[22], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 32:
   if (GET_BONUS(ch, 23) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[23], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 33:
   if (GET_BONUS(ch, 15) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[15], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 34:
   if (GET_BONUS(ch, 17) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[17], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 35:
   if (GET_BONUS(ch, 24) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[24], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 39:
   if (GET_BONUS(ch, 6) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[6], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 40:
   if (GET_BONUS(ch, 2) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[2], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 43:
   if (GET_BONUS(ch, 9) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[9], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 44:
   if (GET_BONUS(ch, 10) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[10], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 45:
   if (GET_BONUS(ch, 11) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[11], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 46:
   if (GET_BONUS(ch, 12) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[12], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 47:
   if (GET_BONUS(ch, 13) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[13], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 48:
   if (GET_BONUS(ch, 14) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[14], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 50:
   if (GET_BONUS(ch, 8) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[8], list_bonus[value]);
    give = FALSE;
   }
   break;
  case 51:
   if (GET_BONUS(ch, 25) > 0) {
    display_bonus_menu(ch, type);
    send_to_char(ch, "@R%s and %s are mutually exclusive.\n\n", list_bonus[25], list_bonus[value]);
    give = FALSE;
   }
   break;
 }


 return (give);
}

/* Handle CC point exchange for Bonus/negative */
void exchange_ccpoints(struct char_data *ch, int value)
{
 int type = 0;

 if (STATE(ch->desc) == CON_BONUS) {
  type = 0;
 } else {
  type = 1;
 }


 if (GET_BONUS(ch, value) > 0 && GET_CCPOINTS(ch) - list_bonus_cost[value] < 0) {
   display_bonus_menu(ch, type);
   send_to_char(ch, "@RYou must unselect some bonus traits first.\r\n");
   return;
 }
 else if (GET_BONUS(ch, value) > 0 && GET_CCPOINTS(ch) - list_bonus_cost[value] >= 0) {
  GET_CCPOINTS(ch) -= list_bonus_cost[value];
  if (list_bonus_cost[value] > 0) {
   GET_NEGCOUNT(ch) -= list_bonus_cost[value];
  }
  GET_BONUS(ch, value) = 0;
  display_bonus_menu(ch, type);
  send_to_char(ch, "@GYou cancel your selection of %s.\r\n", list_bonus[value]);
  return;
 }
 if (type == 0) {
  if (value > 25) {
   display_bonus_menu(ch, type);
   send_to_char(ch, "@RYou are not in the negatives menu, enter B to switch.\r\n");
   return;
  } else if (GET_CCPOINTS(ch) + list_bonus_cost[value] < 0) {
   display_bonus_menu(ch, type);
   send_to_char(ch, "@RYou do not have enough points for %s.\r\n", list_bonus[value]);
   return;
  } else if (!opp_bonus(ch, value, type)) {
   return;
  } else if (list_bonus_cost[value] < 0) {
   GET_CCPOINTS(ch) += list_bonus_cost[value];
   GET_BONUS(ch, value) = 1;
   display_bonus_menu(ch, type);
   send_to_char(ch, "@GYou select the bonus %s\r\n", list_bonus[value]);
   return;
  }
 } else {
 
  if (value < 26) {
   display_bonus_menu(ch, type);
   send_to_char(ch, "@RYou are not in the bonuses menu, enter B to switch.\r\n");
   return;
  }

   int x, count = 0;
 
   for (x = 14; x < 52; x++) {
    if (GET_BONUS(ch, x) > 1) {
     count += list_bonus_cost[x];
    }
   }

  if (list_bonus_cost[value] + count > 10) {
   display_bonus_menu(ch, type);
   send_to_char(ch, "@RYou can not have more than +10 points from negatives.\r\n");
   return;
  } else if (!opp_bonus(ch, value, type)) {
   return;
  } else {
   GET_CCPOINTS(ch) += list_bonus_cost[value];
   GET_NEGCOUNT(ch) += list_bonus_cost[value];
   GET_BONUS(ch, value) = 2;
   display_bonus_menu(ch, type);
   send_to_char(ch, "@GYou select the negative %s\r\n", list_bonus[value]);
   return;
  }
 }
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result=-1;	/* Overloaded variable */
  int total, rr, moveon = FALSE, penalty = FALSE;
  int player_i;
  int value, roll = rand_number(1, 6); /* For parse_bonuses */
  struct descriptor_data *k; 

  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char*);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_IEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_CEDIT, cedit_parse },
    { CON_AEDIT, aedit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_ASSEDIT, assedit_parse },
    { CON_GEDIT, gedit_parse },
    { CON_LEVELUP, levelup_parse },
    { CON_HEDIT, hedit_parse },
    { CON_HSEDIT,   hsedit_parse },
    { CON_POBJ, pobj_edit_parse},
    { -1, NULL }
  };

  skip_spaces(&arg);

  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }

  /*
   * Quick check for the OLC states.
   */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character) {
     free_char(d->character);
    }
    if (!d->character) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
      SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR);
    }
     /*  Check for other chars already logged in */
     /* Jamdog - 3rd May 2007 */
      char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];
     if (d->writenew > 0) {
      if (!*arg) {
       write_to_output(d, "Enter name: ");
       return;
      }
      d->loadplay = strdup(arg);
     }
     if ((_parse_name(d->loadplay, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "Invalid name, please try another.\r\nName: ");
	return;
     }
     if (d->writenew > 0 && (player_i = load_char(tmp_name, d->character)) > -1) {
          userRead(d);
          write_to_output(d, "That character is already taken.\r\n");
          d->writenew = 0;
          STATE(d) = CON_UMENU;
          return;
     }
    else {
     if ((player_i = load_char(tmp_name, d->character)) > -1) {
        if (d->writenew > 0) {
          write_to_output(d, "That character is already taken.\r\n");
          userRead(d);
          STATE(d) = CON_UMENU;
          return;
        }
	GET_PFILEPOS(d->character) = player_i;
	if (PLR_FLAGGED(d->character, PLR_DELETED)) {

	  /* make sure old files are removed so the new player doesn't get
	     the deleted player's equipment (this should probably be a
	     stock behavior)
	  */
	 if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
	    remove_player(player_i);

	  /* We get a false positive from the original deleted character. */
	  free_char(d->character);
	  /* Check for multiple creations... */
	  if (!Valid_Name(tmp_name)) {
	    write_to_output(d, "@YInvalid name@n, please try @Canother.@n\r\nName: ");
	    return;
	  }
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */
	  GET_PFILEPOS(d->character) = player_i;
          SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR);
          display_races(d);
          d->character->rp = d->rpp;
          STATE(d) = CON_QRACE;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
	  REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_GROUP);
          if (isbanned(d->host) == BAN_SELECT &&
           !PLR_FLAGGED(d->character, PLR_SITEOK)) {
           write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
           mudlog(NRM, ADMLVL_GOD, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
           STATE(d) = CON_CLOSE;
           return;
          }
          else if (GET_LEVEL(d->character) < circle_restrict && circle_restrict < 101) {
           userRead(d);
           write_to_output(d, "The game is temporarily restricted to at least %d level.\r\n", circle_restrict);
           STATE(d) = CON_UMENU;
           mudlog(NRM, ADMLVL_GOD, TRUE, "Request for character load denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
           return;
          }
          /* check and make sure no other copies of this player are logged in */
          else if (perform_dupe_check(d)) {
           return;
          }
          else {
	  d->idle_tics = 0;
          write_to_output(d, "\r\n%s\r\n%s", motd, CONFIG_MENU);
          d->character->rp = d->rpp;
          STATE(d) = CON_MENU;
         }
	}
      } else {
        if (d->writenew <= 0) {
         if (!strcasecmp(d->loadplay, d->tmp1)) {
          if (d->tmp1) {
           free(d->tmp1);
           d->tmp1 = NULL;
          }
          d->tmp1 = strdup("Empty");
         }
         if (!strcasecmp(d->loadplay, d->tmp2)) {
          if (d->tmp2) {
           free(d->tmp2);
           d->tmp2 = NULL;
          }
          d->tmp2 = strdup("Empty");
         }
         if (!strcasecmp(d->loadplay, d->tmp3)) {
          if (d->tmp3) {
           free(d->tmp3);
           d->tmp3 = NULL;
          }
          d->tmp3 = strdup("Empty");
         }
         if (!strcasecmp(d->loadplay, d->tmp4)) {
          if (d->tmp4) {
           free(d->tmp4);
           d->tmp4 = NULL;
          }
          d->tmp4 = strdup("Empty");
         }
         if (!strcasecmp(d->loadplay, d->tmp5)) {
          if (d->tmp5) {
           free(d->tmp5);
           d->tmp5 = NULL;
          }
          d->tmp5 = strdup("Empty");
         }
         userWrite(d, 0, 0, 0, "index");
         userRead(d);
         write_to_output(d, "Character missing. Emptying slot.\r\n");
         STATE(d) = CON_UMENU;
        }
        else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	  write_to_output(d, "Invalid name, please try another.\r\nName: ");
	  return;
	}
	CREATE(d->character->name, char, strlen(tmp_name) + 1);
	strcpy(d->character->name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

          display_races(d);
          d->character->rp = d->rpp;
          switch (d->writenew) {
           case 1:
            if (d->tmp1) {
             free(d->tmp1);
             d->tmp1 = NULL;
            }
            d->tmp1 = strdup(d->character->name);
            userWrite(d, 0, 0, 0, "index");
            break;
           case 2:
            if (d->tmp2) {
             free(d->tmp2);
             d->tmp2 = NULL;
            }
            d->tmp2 = strdup(d->character->name);
            userWrite(d, 0, 0, 0, "index");
            break;
           case 3:
            if (d->tmp3) {
             free(d->tmp3);
             d->tmp3 = NULL;
            }
            d->tmp3 = strdup(d->character->name);
            userWrite(d, 0, 0, 0, "index");
            break;
           case 4:
            if (d->tmp4) {
             free(d->tmp4);
             d->tmp4 = NULL;
            }
            d->tmp4 = strdup(d->character->name);
            userWrite(d, 0, 0, 0, "index");
            break;
           case 5:
            if (d->tmp5) {
             free(d->tmp5);
             d->tmp5 = NULL;
            }
            d->tmp5 = strdup(d->character->name);
            userWrite(d, 0, 0, 0, "index");
            break;
          }
          STATE(d) = CON_QRACE;
        }
      }
    }
    break;

  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
	write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	write_to_output(d, "Sorry, new players can't be created at the moment.\r\n");
	mudlog(NRM, ADMLVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      d->idle_tics = 0;
      write_to_output(d, "@MNew character.@n\r\nGive me a @gpassword@n for @C%s@n: ", GET_PC_NAME(d->character));
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "Okay, what IS it, then? ");
      free(d->character->name);
      d->character->name = NULL;
      STATE(d) = CON_GET_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

 case CON_GET_USER:
  if (isbanned(d->host)) {
   write_to_output(d, "You have been banned. Have a nice day.\r\n");
   STATE(d) = CON_CLOSE;
  }
  if (!*arg) {
   write_to_output(d, "Enter your desired username or the username you have already made.\nUsername?\r\n");
   return;
  }
  else if (!strcasecmp(arg, "index")) {
   write_to_output(d, "Try again, username?\r\n");
   return;
  }
  else if (!Valid_Name(CAP(arg))) {
   write_to_output(d, "Invalid name. Username?\r\n");
   return;
  }
  else if (strstr(arg, " ")) {
   write_to_output(d, "No spaces. Username?\r\n");
   return;
  }
  else if (strstr(arg, "1") || strstr(arg, "2") || strstr(arg, "3") || strstr(arg, "4") || strstr(arg, "5") || strstr(arg, "6") || strstr(arg, "7") || strstr(arg, "8") || strstr(arg, "9") || strstr(arg, "0")) {
   write_to_output(d, "No numbers. Username?\r\n");
   return;
  }
  else if (strstr(arg, ".") || strstr(arg, ",") || strstr(arg, "!") || strstr(arg, "?")  || strstr(arg, ";")  || strstr(arg, ":") || strstr(arg, "'")) {
   write_to_output(d, "No punctuation. Username?\r\n");
   return;
  }
  else if (strlen(arg) < 3) {
   write_to_output(d, "Name must at least be 3 characters long, username?\r\n");
   return;
  }
  else if (strlen(arg) > 10) {
   write_to_output(d, "Name must be at most 10 characters long, username?\r\n");
   return;
  }
  else {
   if (!readUserIndex(arg)) {
     if (circle_restrict == 101) {
       write_to_output(d, "The mud is locked for mortals at this time.\r\n");
       write_to_output(d, "Please check: www.advent-truth.com/forum for further details.\r\n");
       send_to_imm("%s rejected by wizlock", CAP(arg));
       STATE(d) = CON_CLOSE;
     }
     else {
      if (d->user) {
       free(d->user);
       d->user = NULL;
      }
      d->user = strdup(arg);
      write_to_output(d, "You want you user name to be, %s?\r\n", d->user);
      write_to_output(d, "Yes or no: \n");
      STATE(d) = CON_USER_CONF;
     }
   }
   else {
    if (circle_restrict == 101) {
      if (d->user) {
       free(d->user);
       d->user = NULL;
      }
     d->user = strdup(arg);
     userLoad(d, d->user);
     if (d->level == 0) {
      write_to_output(d, "The mud is locked for mortals at this time.\r\n");
      send_to_imm("%s rejected by wizlock", d->user);
      STATE(d) = CON_CLOSE;
     }
     else {
      write_to_output(d, "Password: \r\n");
      send_to_imm("Username, %s, logging in.", CAP(arg));
      echo_off(d);
      STATE(d) = CON_PASSWORD;
     }
    }
    else {
      if (d->user) {
       free(d->user);
       d->user = NULL;
      }
     userLoad(d, arg);
     write_to_output(d, "Password: \r\n");
     send_to_imm("Username, %s, logging in.", CAP(arg));
          echo_off(d);
     STATE(d) = CON_PASSWORD;
    }
   }
  }
  break;

  case CON_USER_CONF:
    if (!*arg) {
      write_to_output(d, "You want your user name to be, %s?\r\n", d->user);
      write_to_output(d, "Yes or no: \n");
      return;
    }
    else if (!strcasecmp(arg, "yes") || !strcasecmp(arg, "y")) {
     write_to_output(d, "User Account, %s, created.\r\nEnter Email:\n", d->user);
     write_to_output(d, "Remember your email must be valid and matching the example given.\n");
     write_to_output(d, "[Example: iovan@@advent-truth.com]\n");
     send_to_imm("Username, %s, creating.", CAP(d->user));
     STATE(d) = CON_GET_EMAIL;
    }
    else if (!strcasecmp(arg, "no") || !strcasecmp(arg, "n")) {
      if (d->user) {
       free(d->user);
       d->user = NULL;
      }
      d->user = strdup("Empty");
     write_to_output(d, "Enter Username: \n");
     STATE(d) = CON_GET_USER;
    }
    else {
     write_to_output(d, "You want you user name to be, %s?\r\n", d->user);
     write_to_output(d, "Yes or no: \n");
     return;
    }
   break;

 case CON_GET_EMAIL:
  if (readUserIndex(d->user) && !*arg) {
   write_to_output(d, "Email, or M for menu?\r\n");
   return;
  }
  else if (!*arg) {
   write_to_output(d, "Email?\r\n");
   return;
  }
  else if (!strcasecmp(arg, "M") && readUserIndex(d->user)) {
   userRead(d);
   STATE(d) = CON_UMENU;
  }
  else if (!strstr(arg, ".com") && !strstr(arg, ".net") && !strstr(arg, ".org")) {
   write_to_output(d, "Improper email format missing '.com' or '.net' or '.org'. Email?\r\n");
   return;
  }
  else if (readUserIndex(d->user)) {
   if (strstr(arg, "@")) {
    search_replace(arg, "@", "<AT>");
   }
   if (d->email) {
    free(d->email);
    d->email = NULL;
   }
   d->email = strdup(arg);
   userWrite(d, 0, 0, 0, "index");
   userRead(d);
   STATE(d) = CON_UMENU;
  }
  else {
   if (strstr(arg, "@")) {
    search_replace(arg, "@", "<AT>");
   }
   write_to_output(d, "Your email is: %s\n", arg);
   if (d->email) {
    free(d->email);
    d->email = NULL;
   }
   d->email = strdup(arg);
   write_to_output(d, "Password: \r\n");
          echo_off(d);
   STATE(d) = CON_NEWPASSWD;
  }
  break;

 case CON_NEWPASSWD:
  if (readUserIndex(d->user) && !*arg) {
   write_to_output(d, "Password, or M for menu?\r\n");
   return;
  }
  else if (!*arg) {
   write_to_output(d, "Password?\r\n");
   return;
  }
  else if (!strcasecmp(arg, "M") && readUserIndex(d->user)) {
   userRead(d);
   STATE(d) = CON_UMENU;
  }
  else if (strlen(arg) > MAX_PWD_LENGTH && readUserIndex(d->user)) {
   write_to_output(d, "Password is too long. Password, or M for menu?\r\n");
   return;
  }
  else if (strlen(arg) > MAX_PWD_LENGTH) {
   write_to_output(d, "Password is too long. Password?\r\n");
   return;
  }
  else if (readUserIndex(d->user)) {
   write_to_output(d, "Your password and user account have been fully saved.\r\n");
   if (d->pass) {
    free(d->pass);
    d->pass = NULL;
   }
   d->pass = strdup(arg);
   userWrite(d, 0, 0, 0, "index");
   userRead(d);
   echo_on(d);
   STATE(d) = CON_UMENU;
  }
  else {
   write_to_output(d, "Your password and user account have been fully saved.\r\n");
      if (d->pass) {
       free(d->pass);
       d->pass = NULL;
      }
   d->pass = strdup(arg);
   userCreate(d);
   userRead(d);
   echo_on(d);
   STATE(d) = CON_UMENU;
  }
  break;

 case CON_PASSWORD:
  if (!*arg) {
   write_to_output(d, "Enter Password or Return:\r\n");
   write_to_output(d, "(Return will ask for a different username)\r\n");
   return;
  }
  if (!strcasecmp("return", arg) || !strcasecmp("Return", arg)) {
   if (d->user) {
    free(d->user);
    d->user = NULL;
   }
   d->user = strdup("Empty");
   write_to_output(d, "Username?\r\n");
   STATE(d) = CON_GET_USER;
  }
  if (!strcasecmp(d->pass, arg)) {
   for (k = descriptor_list; k; k = k->next) {
     if (k == d)
      continue;
     if (!k->user || k->user == NULL)
      continue;
     if (!d->user || d->user == NULL)
      continue;
     if (!strcasecmp(k->user, d->user)) {
      if (STATE(k) == CON_PLAYING) {
       STATE(k) = CON_DISCONNECT;
       write_to_output(k, "Your account has been usurped by someone who knows its password!@n");
      } 
      else {
       STATE(k) = CON_CLOSE;
       write_to_output(k, "Your account has been usurped by someone who knows its password!@n");
      }
     }
   }
   userRead(d);
   echo_on(d);
   STATE(d) = CON_UMENU;   
  }
  else {
   write_to_output(d, "Password is wrong. Password or Return?\r\n");
   write_to_output(d, "(Return will ask for a different username)\r\n");
   send_to_imm("Username, %s, password failure!", CAP(d->user));
   log("%s BAD PASSWORD: %s", CAP(d->user), arg);
   return;
  }
  break;

 case CON_UMENU:
  if (!*arg) {
   userRead(d);
   return;
  }
  if (!strcasecmp(arg, "Q")) {
   write_to_output(d, "Thanks for visiting!\n");
   STATE(d) = CON_CLOSE;
  }
  else if (!strcasecmp(arg, "P")) {
   write_to_output(d, "Enter New Password, or M for menu::\n");
   STATE(d) = CON_NEWPASSWD;
  }
  else if (!strcasecmp(arg, "C")) {
   write_to_output(d, "\n");
   customRead(d, 0, NULL);
   write_to_output(d, "\r\n@n--Press Enter--\n@n");
   return;
  }
  else if (!strcasecmp(arg, "E")) {
   write_to_output(d, "Enter New Email, or M for menu:\n");
   STATE(d) = CON_GET_EMAIL;
  }
  else if (!strcasecmp(arg, "D")) {
   write_to_output(d, "Are you sure you want to delete your user file and all its characters? Yes or no:\n");
   STATE(d) = CON_DELCNF1;
  }
  else if (!strcasecmp(arg, "B")) {
   if (d->total == 3 && d->rpp >= 15) {
    d->rpp -= 15;
    d->total = 4;
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    write_to_output(d, "New slot purchased, -15 RPP.\n");
    return;
   }
   else if (d->total == 3 && d->rpp < 15) {
    userRead(d);
    write_to_output(d, "You need at least 15 RPP to purchase a new character slot.\n");
    return;
   }
   else if (d->total == 4 && d->rpp >= 15) {
    d->rpp -= 15;
    d->total = 5;
    userWrite(d, 0, 0, 0, "index");
    userRead(d);
    write_to_output(d, "New slot purchased, -15 RPP.\n");
    return;
   }
   else if (d->total == 4 && d->rpp < 15) {
    userRead(d);
    write_to_output(d, "You need at least 15 RPP to purchase a new character slot.\n");
    return;
   }
   else {
    userRead(d);
    write_to_output(d, "You can't have more than 5 character slots.\n");
    return;
   }
  }
  else {
   switch (atoi(arg)) {
    case 1:
     if (!strcasecmp(d->tmp1, "Empty")) {
     write_to_output(d, "Enter New Character Name: \n");
     d->writenew = 1;
     STATE(d) = CON_GET_NAME;
    }
    else {
     if (lockRead(d->tmp1) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
     write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", d->tmp1);
     d->loadplay = d->tmp1;
     d->writenew = 0;
     STATE(d) = CON_GET_NAME;
     }
    }
    break;
   case 2:
    if (!strcasecmp(d->tmp2, "Empty")) {
     write_to_output(d, "Enter New Character Name: \n");
     d->writenew = 2;
     STATE(d) = CON_GET_NAME;
    }
    else {
     if (lockRead(d->tmp2) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
     write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", d->tmp2);
     d->loadplay = d->tmp2;
     d->writenew = 0;
     STATE(d) = CON_GET_NAME;
     }
    }
    break;
   case 3:
    if (!strcasecmp(d->tmp3, "Empty")) {
     write_to_output(d, "Enter New Character Name: \n");
     d->writenew = 3;
     STATE(d) = CON_GET_NAME;
    }
    else {
     if (lockRead(d->tmp3) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
     write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", d->tmp3);
     d->loadplay = d->tmp3;
     d->writenew = 0;
     STATE(d) = CON_GET_NAME;
     }
    }
    break;
   case 4:
    if (d->total <= 3) {
     userRead(d);
     write_to_output(d, "You only have %d character slots avaialable!\r\n", d->total);
     return;
    }
    if (!strcasecmp(d->tmp4, "Empty")) {
     write_to_output(d, "Enter New Character Name: \n");
     d->writenew = 4;
     STATE(d) = CON_GET_NAME;
    }
    else {
     if (lockRead(d->tmp4) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
     write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", d->tmp4);
     d->loadplay = d->tmp4;
     d->writenew = 0;
     STATE(d) = CON_GET_NAME;
     }
    }
    break;
   case 5:
    if (d->total <= 4) {
     userRead(d);
     write_to_output(d, "You only have %d character slots avaialable!\r\n", d->total);
     return;
    }
    if (!strcasecmp(d->tmp5, "Empty")) {
     write_to_output(d, "Enter New Character Name: \n");
     d->writenew = 5;
     STATE(d) = CON_GET_NAME;
    }
    else {
     if (lockRead(d->tmp5) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
     write_to_output(d, "Loading character, %s.\r\n***Press Return***\n", d->tmp5);
     d->loadplay = d->tmp5;
     d->writenew = 0;
     STATE(d) = CON_GET_NAME;
     }
    }
    break;
   default:
    userRead(d);
    write_to_output(d, "That is not an option.\n");
    return;
    break;
   }
  }
  break;

  case CON_QSEX:		/* query sex of new user         */
   if (!IS_NAMEK(d->character)) {
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->sex = SEX_FEMALE;
      break;
    case 'n':
    case 'N':
      d->character->sex = SEX_NEUTRAL;
      break;
    default:
      write_to_output(d, "That is not a sex..\r\n"
		"What IS your sex? ");
      return;
    }
   }
    if (IS_HUMAN(d->character) || IS_SAIYAN(d->character) || IS_KONATSU(d->character) || IS_MUTANT(d->character) || IS_ANDROID(d->character) || IS_KAI(d->character) || IS_HALFBREED(d->character) || IS_TRUFFLE(d->character) || (IS_HOSHIJIN(d->character) && IS_FEMALE(d->character))) {
     write_to_output(d, "@YHair Length SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Bald  @B2@W)@C Short  @B3@W)@C Medium\r\n");
     write_to_output(d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    STATE(d) = CON_HAIRL;
    }
    else if (IS_DEMON(d->character) || IS_ICER(d->character)) {
     write_to_output(d, "@YHorn Length SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C None  @B2@W)@C Short  @B3@W)@C Medium\r\n");
     write_to_output(d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    STATE(d) = CON_HAIRL;
    }
    else if (IS_MAJIN(d->character)) {
     write_to_output(d, "@YForelock Length SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Tiny  @B2@W)@C Short  @B3@W)@C Medium\r\n");
     write_to_output(d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    STATE(d) = CON_HAIRL;
    }
    else if (IS_NAMEK(d->character) || IS_ARLIAN(d->character)) {
     write_to_output(d, "@YAntenae Length SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Tiny  @B2@W)@C Short  @B3@W)@C Medium\r\n");
     write_to_output(d, "@B4@W)@C Long  @B5@W)@C Really Long@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
    STATE(d) = CON_HAIRL;
    }
    else {
     write_to_output(d, "@YSkin color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
     write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
     write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
     write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_SKIN;
    }
    break;

    case CON_QRACE:
    switch (*arg) {
      case 'r':
      case 'R':
        while (load_result == RACE_UNDEFINED) {
          rr = rand_number(1, NUM_RACES);
          load_result = parse_race(d->character, rr);
        }
        break;
      case 't':
      case 'T':
        display_races_help(d);
        STATE(d) = CON_RACE_HELP;
        return;
    }
    if (load_result == RACE_UNDEFINED)
      load_result = parse_race(d->character, atoi(arg));
      if (load_result == RACE_UNDEFINED && (atoi(arg) != 2 && atoi(arg) != 12 && atoi(arg) != 9)) {
        write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
        return;
      } else if (load_result == RACE_UNDEFINED && (atoi(arg) == 2)) {
        write_to_output(d, "\r\nThat race is restricted, You need 60 RPP to unlock it.\r\nRace: ");
        return;
      } else if (load_result == RACE_UNDEFINED && (atoi(arg) == 12)) {
        write_to_output(d, "\r\nThat race is restricted, You need 55 RPP to unlock it.\r\nRace: ");
        return;
      } else if (load_result == RACE_UNDEFINED && (atoi(arg) == 9)) {
        write_to_output(d, "\r\nThat race is restricted, You need 30 RPP to unlock it.\r\nRace: ");
        return;
      } else if (load_result == RACE_UNDEFINED && (atoi(arg) == 15)) {
        write_to_output(d, "\r\nThat race is restricted, You need 20 RPP to unlock it.\r\nRace: ");
        return;
      }
        else
        GET_RACE(d->character) = load_result;

    if (IS_HALFBREED(d->character)) {
     write_to_output(d, "@YWhat race do you prefer to by identified with?\r\n");
     write_to_output(d, "@cThis controls how others first view you and whether you start with\na tail and how fast it regrows when missing.\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Human\n@B2@W)@C Saiyan\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_RACIAL;
    } else if (IS_ANDROID(d->character)) {
     write_to_output(d, "@YWhat do you want to be identified as at first glance?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Android\n@B2@W)@C Human\n@B3@W)@C Robotic Humanoid\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_RACIAL;
    } else if (IS_NAMEK(d->character)) {
     GET_SEX(d->character) = SEX_NEUTRAL;
     STATE(d) = CON_QSEX;
    } else {
     write_to_output(d, "\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
     STATE(d) = CON_QSEX;
    }
    break;

  case CON_RACIAL:
    switch (*arg) {
      case '1':
      d->character->player_specials->racial_pref = 1;
      break;
      case '2':
      d->character->player_specials->racial_pref = 2;
      break;
      case '3':
      if (IS_HALFBREED(d->character)) {
       write_to_output(d, "That is not an acceptable option.\r\n");
       return;
      } else {
       d->character->player_specials->racial_pref = 3;
      }
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     write_to_output(d, "\r\n@wWhat is your sex @W(@BM@W/@MF@W/@GN@W)@w?@n");
     STATE(d) = CON_QSEX;
   break;

  case CON_RACE_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_races(d);
      STATE(d) = CON_QRACE;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_RACES || player_i < 1) {
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
	break;
      }
      player_i -= 1;
      if (race_ok_gender[(int)GET_SEX(d->character)][player_i])
	show_help(d, race_names[player_i]);
      else
	write_to_output(d, "\r\nThat's not a race.\r\nHelp on Race #: ");
    } else {
      display_races_help(d);
    }
    STATE(d) = CON_RACE_HELP;
    break;
  
  case CON_CLASS_HELP:
    if (*arg == 't' || *arg == 'T') {
      display_classes(d);
      STATE(d) = CON_QCLASS;
      return;
    }
    if (isdigit(*arg)) {
      player_i = atoi(arg);
      if (player_i > NUM_BASIC_CLASSES || player_i < 1) {
	write_to_output(d, "\r\nThat's not a sensei.\r\nHelp on Sensei #: ");
	break;
      }
      player_i -= 1;
      if (class_ok_race[(int)GET_SEX(d->character)][player_i])
        show_help(d, class_names[player_i]);
      else
	write_to_output(d, "\r\nThat's not a sensei.\r\nHelp on Sensei #: ");
    } else {
      display_classes_help(d);
    }
    STATE(d) = CON_CLASS_HELP;
    break;

  case CON_HAIRL:                /* query hair length */
    if (IS_HUMAN(d->character) || IS_SAIYAN(d->character) || IS_KONATSU(d->character) || IS_MUTANT(d->character) || IS_ANDROID(d->character) || IS_KAI(d->character) || IS_HALFBREED(d->character) || IS_TRUFFLE(d->character) || (IS_HOSHIJIN(d->character) && IS_FEMALE(d->character))) {
    switch (*arg) {
      case '1':
      d->character->hairl = HAIRL_BALD;
      d->character->hairc = HAIRC_NONE;
      d->character->hairs = HAIRS_NONE;
      break;
      case '2':
      d->character->hairl = HAIRL_SHORT;
      break;
      case '3':
      d->character->hairl = HAIRL_MEDIUM;
      break;
      case '4':
      d->character->hairl = HAIRL_LONG;
      break;
      case '5':
      d->character->hairl = HAIRL_RLONG;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     if (d->character->hairl == HAIRL_BALD) {
     write_to_output(d, "@YSkin color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
     write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
     write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
     write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_SKIN;
     }
     else {
     write_to_output(d, "@YHair color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
     write_to_output(d, "@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
     write_to_output(d, "@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
     write_to_output(d, "@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
     write_to_output(d, "@BD@W)@C White@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_HAIRC;
     }
    }
    else {
    if (IS_DEMON(d->character) || IS_ICER(d->character)) {
    switch (*arg) {
      case '1':
      d->character->hairl = HAIRL_BALD;
      break;
      case '2':
      d->character->hairl = HAIRL_SHORT;
      break;
      case '3':
      d->character->hairl = HAIRL_MEDIUM;
      break;
      case '4':
      d->character->hairl = HAIRL_LONG;
      break;
      case '5':
      d->character->hairl = HAIRL_RLONG;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
      }
      d->character->hairc = HAIRC_NONE;
      d->character->hairs = HAIRS_NONE;
     write_to_output(d, "@YSkin color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
     write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
     write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
     write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_SKIN;
    }
    if (IS_MAJIN(d->character) || IS_NAMEK(d->character) || IS_ARLIAN(d->character)) {
      switch (*arg) {
      case '1':
      d->character->hairl = HAIRL_BALD;
      break;
      case '2':
      d->character->hairl = HAIRL_SHORT;
      break;
      case '3':
      d->character->hairl = HAIRL_MEDIUM;
      break;
      case '4':
      d->character->hairl = HAIRL_LONG;
      break;
      case '5':
      d->character->hairl = HAIRL_RLONG;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
      }
     if (IS_ARLIAN(d->character) && IS_FEMALE(d->character)) {
      write_to_output(d, "@YWing color SELECTION menu:\r\n");
      write_to_output(d, "@D---------------------------------------@n\r\n");
      write_to_output(d, "@B1@W)@C Black  @B2@W)@C Brown  @B3@W)@C Blonde\r\n");
      write_to_output(d, "@B4@W)@C Grey   @B5@W)@C Red    @B6@W)@C Orange@n\r\n");
      write_to_output(d, "@B7@W)@C Green  @B8@W)@C Blue   @B9@W)@C Pink\r\n");
      write_to_output(d, "@BA@W)@C Purple @BB@W)@C Silver @BC@W)@C Crimson@n\r\n");
      write_to_output(d, "@BD@W)@C White@n\r\n");
      write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_HAIRC;
     } else {
       d->character->hairc = HAIRC_NONE;
       d->character->hairs = HAIRS_NONE;
       write_to_output(d, "@YSkin color SELECTION menu:\r\n");
       write_to_output(d, "@D---------------------------------------@n\r\n");
       write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
       write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
       write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
       write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
       write_to_output(d, "@w\r\nMake a selection:@n\r\n");
       STATE(d) = CON_SKIN;
      }
     }
     else {
     d->character->hairc = HAIRC_NONE;
     d->character->hairs = HAIRS_NONE;
     STATE(d) = CON_SKIN;
     }
    }
   break;

   case CON_HAIRC:     /* query hair color */
    switch (*arg) {
      case '1':
      d->character->hairc = HAIRC_BLACK;
      break;
      case '2':
      d->character->hairc = HAIRC_BROWN;
      break;
      case '3':
      d->character->hairc = HAIRC_BLONDE;
      break;
      case '4':
      d->character->hairc = HAIRC_GREY;
      break;
      case '5':
      d->character->hairc = HAIRC_RED;
      break;
      case '6':
      d->character->hairc = HAIRC_ORANGE;
      break;
      case '7':
      d->character->hairc = HAIRC_GREEN;
      break;
      case '8':
      d->character->hairc = HAIRC_BLUE;
      break;
      case '9':
      d->character->hairc = HAIRC_PINK;
      break;
      case 'A':
      d->character->hairc = HAIRC_PURPLE;
      break;
      case 'a':
      d->character->hairc = HAIRC_PURPLE;
      break;
      case 'B':
      d->character->hairc = HAIRC_SILVER;
      break;
      case 'b':
      d->character->hairc = HAIRC_SILVER;
      break;
      case 'C':
      d->character->hairc = HAIRC_CRIMSON;
      break;
      case 'c':
      d->character->hairc = HAIRC_CRIMSON;
      break;
      case 'D':
      d->character->hairc = HAIRC_WHITE;
      break;
      case 'd':
      d->character->hairc = HAIRC_WHITE;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     if (IS_ARLIAN(d->character)) {
      d->character->hairs = HAIRS_NONE;
      write_to_output(d, "@YSkin color SELECTION menu:\r\n");
      write_to_output(d, "@D---------------------------------------@n\r\n");
      write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
      write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
      write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
      write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");
      write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_SKIN;
     } else {
      write_to_output(d, "@YHair style SELECTION menu:\r\n");
      write_to_output(d, "@D---------------------------------------@n\r\n");
      write_to_output(d, "@B1@W)@C Plain     @B2@W)@C Mohawk    @B3@W)@C Spiky\r\n");
      write_to_output(d, "@B4@W)@C Curly     @B5@W)@C Uneven    @B6@W)@C Ponytail@n\r\n");
      write_to_output(d, "@B7@W)@C Afro      @B8@W)@C Fade      @B9@W)@C Crew Cut\r\n");
      write_to_output(d, "@BA@W)@C Feathered @BB@W)@C Dred Locks@n\r\n");
      write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_HAIRS;
     }
    break;

    case CON_HAIRS:
    switch (*arg) {
      case '1':
      d->character->hairs = HAIRS_PLAIN;
      break;
      case '2':
      d->character->hairs = HAIRS_MOHAWK;
      break;
      case '3':
      d->character->hairs = HAIRS_SPIKY;
      break;
      case '4':
      d->character->hairs = HAIRS_CURLY;
      break;
      case '5':
      d->character->hairs = HAIRS_UNEVEN;
      break;
      case '6':
      d->character->hairs = HAIRS_PONYTAIL;
      break;
      case '7':
      d->character->hairs = HAIRS_AFRO;
      break;
      case '8':
      d->character->hairs = HAIRS_FADE;
      break;
      case '9':
      d->character->hairs = HAIRS_CREW;
      break;
      case 'A':
      d->character->hairs = HAIRS_FEATHERED;
      break;
      case 'a':
      d->character->hairs = HAIRS_FEATHERED;
      break;
      case 'B':
      d->character->hairs = HAIRS_DRED;
      break;
      case 'b':
      d->character->hairs = HAIRS_DRED;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     write_to_output(d, "@YSkin color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C White  @B2@W)@C Black  @B3@W)@C Green\r\n");
     write_to_output(d, "@B4@W)@C Orange @B5@W)@C Yellow @B6@W)@C Red@n\r\n");
     write_to_output(d, "@B7@W)@C Grey   @B8@W)@C Blue   @B9@W)@C Aqua\r\n");
     write_to_output(d, "@BA@W)@C Pink   @BB@W)@C Purple @BC@W)@C Tan@n\r\n");      
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_SKIN;
    break;

    case CON_SKIN:
    switch (*arg) {
      case '1':
      d->character->skin = SKIN_WHITE;
      break;
      case '2':
      d->character->skin = SKIN_BLACK;
      break;
      case '3':
      d->character->skin = SKIN_GREEN;
      break;
      case '4':
      d->character->skin = SKIN_ORANGE;
      break;
      case '5':
      d->character->skin = SKIN_YELLOW;
      break;
      case '6':
      d->character->skin = SKIN_RED;
      break;
      case '7':
      d->character->skin = SKIN_GREY;
      break;
      case '8':
      d->character->skin = SKIN_BLUE;
      break;
      case '9':
      d->character->skin = SKIN_AQUA;
      break;
      case 'A':
      d->character->skin = SKIN_PINK;
      break;
      case 'a':
      d->character->skin = SKIN_PINK;
      break;
      case 'B':
      d->character->skin = SKIN_PURPLE;
      break;
      case 'b':
      d->character->skin = SKIN_PURPLE;
      break;
      case 'C':
      d->character->skin = SKIN_TAN;
      break;
      case 'c':
      d->character->skin = SKIN_TAN;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     write_to_output(d, "@YEye color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Blue  @B2@W)@C Black  @B3@W)@C Green\r\n");
     write_to_output(d, "@B4@W)@C Brown @B5@W)@C Red    @B6@W)@C Aqua@n\r\n");
     write_to_output(d, "@B7@W)@C Pink  @B8@W)@C Purple @B9@W)@C Crimson\r\n");
     write_to_output(d, "@BA@W)@C Gold  @BB@W)@C Amber  @BC@W)@C Emerald@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_EYE;
     break;

   case CON_EYE:

    switch (*arg) {
      case '1':
      d->character->eye = EYE_BLUE;
      break;
      case '2':
      d->character->eye = EYE_BLACK;
      break;
      case '3':
      d->character->eye = EYE_GREEN;
      break;
      case '4':
      d->character->eye = EYE_BROWN;
      break;
      case '5':
      d->character->eye = EYE_RED;
      break;
      case '6':
      d->character->eye = EYE_AQUA;
      break;
      case '7':
      d->character->eye = EYE_PINK;
      break;
      case '8':
      d->character->eye = EYE_PURPLE;
      break;
      case '9':
      d->character->eye = EYE_CRIMSON;
      break;
      case 'A':
      d->character->eye = EYE_GOLD;
      break;
      case 'a':
      d->character->eye = EYE_GOLD;
      break;
      case 'B':
      d->character->eye = EYE_AMBER;
      break;
      case 'b':
      d->character->eye = EYE_AMBER;
      break;
      case 'C':
      d->character->eye = EYE_EMERALD;
      break;
      case 'c':
      d->character->eye = EYE_EMERALD;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
      break;
     }
      write_to_output(d, "@YWhat do you want to be your most distinguishing feature:\r\n");
      write_to_output(d, "@D---------------------------------------@n\r\n");
      write_to_output(d, "@B1@W)@C My Eyes@n\r\n");
      if (IS_MAJIN(d->character)) {
       write_to_output(d, "@B2@W)@C My Forelock@n\r\n");
      }
      else if (IS_NAMEK(d->character) || IS_ARLIAN(d->character)) {
       write_to_output(d, "@B2@W)@C My Antennae@n\r\n");
      }
      else if (IS_ICER(d->character) || IS_DEMON(d->character)) {
       write_to_output(d, "@B2@W)@C My Horns@n\r\n");
      }
      else if (GET_HAIRL(d->character) == HAIRL_BALD) {
       write_to_output(d, "@B2@W)@C My Baldness@n\r\n");
      }
      else {
       write_to_output(d, "@B2@W)@C My Hair@n\r\n");
      }
      write_to_output(d, "@B3@W)@C My Skin@n\r\n");
      write_to_output(d, "@B4@W)@C My Height@n\r\n");
      write_to_output(d, "@B5@W)@C My Weight@n\r\n");
      write_to_output(d, "@w\r\nMake a selection:@n\r\n");

     STATE(d) = CON_DISTFEA;
     break;

    case CON_DISTFEA:
    switch (*arg) {
      case '1':
       d->character->distfea = 0;
      break;
      case '2':
       d->character->distfea = 1;
      break;
      case '3':
       d->character->distfea = 2;
      break;
      case '4':
       d->character->distfea = 3;
      break;
      case '5':
       d->character->distfea = 4;
      break;
      default:
       write_to_output(d, "That is not an acceptable option.\r\n");
       return;
      break;
     }
     write_to_output(d, "@YWhat Height/Weight Range do you prefer:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
      write_to_output(d, "@B1@W)@C 100-120cm, 25-30kg@n\r\n");
      write_to_output(d, "@B2@W)@C 120-140cm, 30-35kg@n\r\n");
      write_to_output(d, "@B3@W)@C 140-160cm, 35-45kg@n\r\n");
      write_to_output(d, "@B4@W)@C 160-180cm, 45-60kg@n\r\n");
      write_to_output(d, "@B5@W)@C 180-200cm, 60-80kg@n\r\n");
      write_to_output(d, "@B6@W)@C 200-220cm, 80-100kg@n\r\n");
     } else if (IS_ICER(d->character)) {
      write_to_output(d, "@B1@W)@C 100-120cm, 25-30kg@n\r\n");
      write_to_output(d, "@B2@W)@C 120-140cm, 30-35kg@n\r\n");
      write_to_output(d, "@B3@W)@C 140-160cm, 35-45kg@n\r\n");
     } else {
      write_to_output(d, "@B1@W)@C 20-35cm, 5-8kg@n\r\n");
      write_to_output(d, "@B2@W)@C 35-40cm, 8-10kg@n\r\n");
      write_to_output(d, "@B3@W)@C 40-50cm, 10-12kg@n\r\n");
      write_to_output(d, "@B4@W)@C 50-60cm, 12-15kg@n\r\n");
      write_to_output(d, "@B5@W)@C 60-70cm, 15-18kg@n\r\n");
     }
     write_to_output(d, "\n@WMake a selection:@n ");
     STATE(d) = CON_HW;
     break;

     case CON_HW:
     switch (*arg) {
      case '1':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(100, 120); 
        d->character->weight = rand_number(25, 30);
       } else if (IS_ICER(d->character)) {
        d->character->height = rand_number(100, 120);
        d->character->weight = rand_number(25, 30);
       } else {
        d->character->height = rand_number(20, 35);
        d->character->weight = rand_number(5, 8);
       }
      break;
      case '2':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(120, 140);
        d->character->weight = rand_number(30, 35);
       } else if (IS_ICER(d->character)) {
        d->character->height = rand_number(120, 140);
        d->character->weight = rand_number(30, 35);
       } else {
        d->character->height = rand_number(35, 40);
        d->character->weight = rand_number(8, 10);
       }
      break;
      case '3':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(140, 160);
        d->character->weight = rand_number(35, 45);
       } else if (IS_ICER(d->character)) {
        d->character->height = rand_number(140, 160);
        d->character->weight = rand_number(35, 45);
       } else {
        d->character->height = rand_number(40, 50);
        d->character->weight = rand_number(10, 12);
       }
      break;
      case '4':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(160, 180);
        d->character->weight = rand_number(45, 60);
       } else if (IS_ICER(d->character)) {
         write_to_output(d, "That is not an acceptable option.\r\n");
         return;
       } else {
        d->character->height = rand_number(50, 60);
        d->character->weight = rand_number(12, 15);
       }
      break;
      case '5':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(180, 200);
        d->character->weight = rand_number(60, 80);
       } else if (IS_ICER(d->character)) {
         write_to_output(d, "That is not an acceptable option.\r\n");
         return;
       } else {
        d->character->height = rand_number(60, 70);
        d->character->weight = rand_number(15, 18);
       }
      break;
      case '6':
       if (!IS_TRUFFLE(d->character) && !IS_ICER(d->character)) {
        d->character->height = rand_number(200, 220);
        d->character->weight = rand_number(80, 100);
       } else {
         write_to_output(d, "That is not an acceptable option.\r\n");
         return;
       }
      break;
      default:
       write_to_output(d, "That is not an acceptable option.\r\n");
       return;
      break;
     }
     write_to_output(d, "@YAura color SELECTION menu:\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C White  @B2@W)@C Blue@n\r\n");
     write_to_output(d, "@B3@W)@C Red    @B4@W)@C Green@n\r\n");
     write_to_output(d, "@B5@W)@C Pink   @B6@W)@C Purple@n\r\n");
     write_to_output(d, "@B7@W)@C Yellow @B8@W)@C Black@n\r\n");
     write_to_output(d, "@B9@W)@C Orange@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_AURA;
     break;

     case CON_AURA:
      switch (*arg) {
       case '1':
         d->character->aura = 0;
        break;
       case '2':
         d->character->aura = 1;
        break;
       case '3':
         d->character->aura = 2;
        break;
       case '4':
         d->character->aura = 3;
        break;
       case '5':
         d->character->aura = 4;
        break;
       case '6':
         d->character->aura = 5;
        break;
       case '7':
         d->character->aura = 6;
        break;
       case '8':
         d->character->aura = 7;
        break;
       case '9':
         d->character->aura = 8;
        break;
       default:
        write_to_output(d, "That is not an acceptable option.\r\n");
        return;
       break;
      }
    
     display_classes(d);
     STATE(d) = CON_QCLASS;
     break;

    case CON_Q1:
      d->character->max_hit = rand_number(30, 50);
      d->character->max_move = rand_number(30, 50);
      d->character->max_mana = rand_number(30, 50);
     if (IS_SAIYAN(d->character)) {
      d->character->real_abils.str = rand_number(12, 18);
      d->character->real_abils.con = rand_number(12, 18);
      d->character->real_abils.wis = rand_number(8, 16);
      d->character->real_abils.intel = rand_number(8, 14);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 16);
     }
     else if (IS_HALFBREED(d->character)) {
      d->character->real_abils.str = rand_number(10, 18);
      d->character->real_abils.con = rand_number(10, 18);
      d->character->real_abils.wis = rand_number(8, 18);
      d->character->real_abils.intel = rand_number(8, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_HUMAN(d->character)) {
      d->character->real_abils.str = rand_number(8, 18);
      d->character->real_abils.con = rand_number(8, 18);
      d->character->real_abils.wis = rand_number(10, 18);
      d->character->real_abils.intel = rand_number(12, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_HOSHIJIN(d->character)) {
      d->character->real_abils.str = rand_number(10, 18);
      d->character->real_abils.con = rand_number(9, 18);
      d->character->real_abils.wis = rand_number(9, 18);
      d->character->real_abils.intel = rand_number(9, 18);
      d->character->real_abils.cha = rand_number(10, 18);
      d->character->real_abils.dex = rand_number(9, 18);
     }
     else if (IS_NAMEK(d->character)) {
      d->character->real_abils.str = rand_number(9, 18);
      d->character->real_abils.con = rand_number(9, 18);
      d->character->real_abils.wis = rand_number(12, 18);
      d->character->real_abils.intel = rand_number(8, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_ARLIAN(d->character)) {
      d->character->real_abils.str = rand_number(15, 20);
      d->character->real_abils.con = rand_number(15, 20);
      d->character->real_abils.wis = rand_number(8, 16);
      d->character->real_abils.intel = rand_number(8, 16);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_ANDROID(d->character)) {
      d->character->real_abils.str = rand_number(12, 18);
      d->character->real_abils.con = rand_number(8, 18);
      d->character->real_abils.wis = rand_number(8, 16);
      d->character->real_abils.intel = rand_number(8, 16);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_BIO(d->character)) {
      d->character->real_abils.str = rand_number(14, 18);
      d->character->real_abils.con = rand_number(8, 18);
      d->character->real_abils.wis = rand_number(8, 18);
      d->character->real_abils.intel = rand_number(8, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 14);
     }
     else if (IS_MAJIN(d->character)) {
      d->character->real_abils.str = rand_number(11, 18);
      d->character->real_abils.con = rand_number(14, 18);
      d->character->real_abils.wis = rand_number(8, 14);
      d->character->real_abils.intel = rand_number(8, 14);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 17);
     }
     else if (IS_TRUFFLE(d->character)) {
      d->character->real_abils.str = rand_number(8, 14);
      d->character->real_abils.con = rand_number(8, 14);
      d->character->real_abils.wis = rand_number(8, 18);
      d->character->real_abils.intel = rand_number(14, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_KAI(d->character)) {
      d->character->real_abils.str = rand_number(9, 18);
      d->character->real_abils.con = rand_number(8, 18);
      d->character->real_abils.wis = rand_number(14, 18);
      d->character->real_abils.intel = rand_number(10, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_ICER(d->character)) {
      d->character->real_abils.str = rand_number(10, 18);
      d->character->real_abils.con = rand_number(12, 18);
      d->character->real_abils.wis = rand_number(8, 18);
      d->character->real_abils.intel = rand_number(8, 18);
      d->character->real_abils.cha = rand_number(8, 15);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_MUTANT(d->character)) {
      d->character->real_abils.str = rand_number(9, 18);
      d->character->real_abils.con = rand_number(9, 18);
      d->character->real_abils.wis = rand_number(9, 18);
      d->character->real_abils.intel = rand_number(9, 18);
      d->character->real_abils.cha = rand_number(9, 18);
      d->character->real_abils.dex = rand_number(9, 18);
     }
     else if (IS_KANASSAN(d->character)) {
      d->character->real_abils.str = rand_number(8, 16);
      d->character->real_abils.con = rand_number(8, 16);
      d->character->real_abils.wis = rand_number(12, 18);
      d->character->real_abils.intel = rand_number(12, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_DEMON(d->character)) {
      d->character->real_abils.str = rand_number(11, 18);
      d->character->real_abils.con = rand_number(8, 18);
      d->character->real_abils.wis = rand_number(10, 18);
      d->character->real_abils.intel = rand_number(10, 18);
      d->character->real_abils.cha = rand_number(8, 18);
      d->character->real_abils.dex = rand_number(8, 18);
     }
     else if (IS_KONATSU(d->character)) {
      d->character->real_abils.str = rand_number(10, 14);
      d->character->real_abils.con = rand_number(10, 14);
      d->character->real_abils.wis = rand_number(10, 16);
      d->character->real_abils.intel = rand_number(10, 14);
      d->character->real_abils.cha = rand_number(12, 18);
      d->character->real_abils.dex = rand_number(14, 18);
     }     

    switch (*arg) {
      case '1':
      d->character->max_hit += roll_stats(d->character, 5, 25);
      d->character->max_move += roll_stats(d->character, 8, 50);
      d->character->max_mana += roll_stats(d->character, 6, 50);
      break;
      case '2':
      d->character->max_hit += roll_stats(d->character, 5, 55);
      d->character->max_move += roll_stats(d->character, 8, 40);
      d->character->max_mana += roll_stats(d->character, 6, 40);
      break;
      case '3':
      d->character->max_hit += roll_stats(d->character, 5, 125);
      d->character->max_move += roll_stats(d->character, 8, 50);
      d->character->max_mana += roll_stats(d->character, 6, 40);
      break;
      case '4':
      d->character->max_hit += roll_stats(d->character, 5, 65);
      d->character->max_move += roll_stats(d->character, 8, 65);
      d->character->max_mana += roll_stats(d->character, 6, 65);
      SET_BIT_AR(PLR_FLAGS(d->character), PLR_SKILLP);
      break;
      case '5':
      d->character->max_hit += roll_stats(d->character, 5, 75);
      d->character->max_move += roll_stats(d->character, 8, 100);
      d->character->max_mana += roll_stats(d->character, 6, 75);
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     write_to_output(d, "\r\n@WQuestion (@G2@W out of @g10@W)\r\n");
     write_to_output(d, "@YAnswer the following question:\r\n");
     write_to_output(d, "@wYou are faced with the strongest opponent you have ever\r\nfaced in your life. You both have beat each other to the\r\nlimits of both your strengths. A situation has presented \r\nan opportunity to win the fight, what do you do?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Kill my opponent in a very brutal fashion!@n\r\n");
     write_to_output(d, "@B2@W)@C Disable my opponent, but spare their life.@n\r\n");
     write_to_output(d, "@B3@W)@C Kill my opponent, I have no other choice.@n\r\n");
     write_to_output(d, "@B4@W)@C Try to evade my opponent till I can get away.@n\r\n");
     write_to_output(d, "@B5@W)@C Take their head clean off and bathe in their blood!@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_Q2;
     break;

     case CON_Q2:
    switch (*arg) {
      case '1':
      d->character->alignment += -200;
      break;
      case '2':
      d->character->alignment += 100;
      break;
      case '3':
      d->character->alignment += 10;
      break;
      case '4':
       d->character->alignment += 0;
      break;
      case '5':
      d->character->alignment += -400;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }
     write_to_output(d, "\r\n@WQuestion (@G3@W out of @g10@W)\r\n");
     write_to_output(d, "@YAnswer the following question:\r\n");
     write_to_output(d, "@wYou are one day offered a means to gain incredible strength\r\nby some extraordinary means. The only problem is it requires\r\nthe lives of innocents to obtain. What do you do?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Hell yeah I take the opportunity to get stronger!@n\r\n");
     write_to_output(d, "@B2@W)@C I refuse to gain unnatural strength.@n\r\n");
     write_to_output(d, "@B3@W)@C I refuse to gain strength at the cost of the innocent.@n\r\n");
     write_to_output(d, "@B4@W)@C I kill that many innocents before breakfast anyway...@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_Q3;
     break;
     case CON_Q3:
    switch (*arg) {
      case '1':
      d->character->alignment += -100;
      d->character->max_hit += 100;
      d->character->max_move += 80;
      d->character->max_mana += 10;
      break;
      case '2':
      d->character->alignment += 10;
      d->character->max_hit += 25;
      d->character->max_move += 25;
      d->character->max_mana += 25;
      break;
      case '3':
      d->character->alignment += 50;
      d->character->max_hit += 20;
      d->character->max_move += 20;
      d->character->max_mana += 20;
      break;
      case '4':
      d->character->alignment += -200;
      d->character->max_hit += 100;
      d->character->max_move += 100;
      d->character->max_mana += 100;
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }    
     write_to_output(d, "\r\n@WQuestion (@G4@W out of @g10@W)\r\n");
     write_to_output(d, "@YAnswer the following question:\r\n");
     write_to_output(d, "@wOne day you are offered a way to make a lot of money, but in order\r\nto do so you will need to stop training for a whole month to\r\nhandle business. What do you do?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C I take the opportunity, with more money I can train better later.@n\r\n");
     write_to_output(d, "@B2@W)@C I refuse to waste my time. What I need is some nice hard training.@n\r\n");
     write_to_output(d, "@B3@W)@C Hmm. With more money I can live better, certainly that is worth the time.@n\r\n");
     write_to_output(d, "@B4@W)@C I choose to earn a little money while still training instead.@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_Q4;
     break;

     case CON_Q4:
    switch (*arg) {
      case '1':
      d->character->gold += 1000;
      d->character->max_hit -= rand_number(10, 30);
      d->character->max_move -= rand_number(10, 30);
      d->character->max_mana -= rand_number(10, 30);
      break;
      case '2':
      d->character->gold = 0;
      d->character->max_hit += rand_number(50, 165);
      d->character->max_move += rand_number(50, 165);
      d->character->max_mana += rand_number(50, 165);
      break;
      case '3':
      d->character->gold = 2500;
      d->character->max_hit -= rand_number(15, 25);
      d->character->max_move -= rand_number(15, 25);
      d->character->max_mana -= rand_number(15, 25);
      break;
      case '4':
      d->character->gold = 150;
      d->character->max_hit += rand_number(25, 80);
      d->character->max_move += rand_number(25, 80);
      d->character->max_mana += rand_number(25, 80);
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }    
     write_to_output(d, "\r\n@WQuestion (@G5@W out of @g10@W)\r\n");
     write_to_output(d, "@YAnswer the following question:\r\n");
     write_to_output(d, "@wYou are introduced to a new way of training one day, what do you do?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C I prefer my way, it has worked so far.@n\r\n");
     write_to_output(d, "@B2@W)@C I am open to new possibilites, sure.@n\r\n");
     write_to_output(d, "@B3@W)@C I will at least try it, for a little while.@n\r\n");
     write_to_output(d, "@B4@W)@C No way is superior to eating spinach everyday...@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
      STATE(d) = CON_Q5;
      break;

   case CON_Q5:
    switch (*arg) {
      case '1':
      d->character->max_hit += rand_number(0, 40);
      break;
      case '2':
      d->character->max_hit += rand_number(-30, 80);
      break;
      case '3':
      d->character->max_hit += rand_number(-25, 60);
      break;
      case '4':
      d->character->max_hit += rand_number(0, 50);
      break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
     }    
     write_to_output(d, "\r\n@WQuestion (@G6@W out of @g10@W)\r\n");
         write_to_output(d, "\r\n@YAnswer the following question:\r\n");
         write_to_output(d, "@wYou have an enemy before you, what is your prefered method to attack him?@n\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C I prefer to defend rather than attack.@n\r\n");
         write_to_output(d, "@B2@W)@C I prefer to throw a strong punch at the enemy's throat.@n\r\n");
         write_to_output(d, "@B3@W)@C I prefer to send a devestating kick at the enemy's neck.@n\r\n");
         write_to_output(d, "@B4@W)@C I prefer to smash them with a two-handed slam!@n\r\n");
         write_to_output(d, "@B5@W)@C I prefer to throw one of my many energy attacks!@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q6;
      break;


     
  case CON_Q6:
    switch (*arg) {
      case '1':
         d->character->max_hit += rand_number(0, 15);
         d->character->max_move += rand_number(0, 15);
         d->character->choice = 1;
        break;
      case '2':
         d->character->max_hit += rand_number(0, 30);
         d->character->choice = 2;
        break;
      case '3':
         d->character->max_hit += rand_number(0, 30);
         d->character->choice = 3;
        break;
      case '4':
         d->character->max_hit += rand_number(0, 30);
         d->character->choice = 4;
        break;
      case '5':
         d->character->max_mana += rand_number(0, 50);
         d->character->choice = 5;
        break;
      default:
      write_to_output(d, "That is not an acceptable option.\r\n");
      return;
    }
     write_to_output(d, "\r\n@WQuestion (@G7@W out of @g10@W)\r\n");
         write_to_output(d, "\r\n@YAnswer the following question:\r\n");
         write_to_output(d, "@wYou are camped out one night in a field, the sky is clear and the\r\nstars visible. Looking at them, what thought crosses your mind?@n\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C One day, I am going to conquer every one of those.@n\r\n");
         write_to_output(d, "@B2@W)@C I really wish I had brought that special someone along as this is a great night for romance@n\r\n");
         write_to_output(d, "@B3@W)@C Those stars hold greater meaning to you than just planets, they dictate\r\nyour life or guide you and those arround you in one form or another.@n\r\n");
         write_to_output(d, "@B4@W)@C You'd very much like to travel into space to get to one of these stars to study all that it holds.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q7;
    break;

    case CON_Q7:
    switch (*arg) {
      case '1':
         d->character->alignment += -10;
         d->character->real_abils.str += 1;
        break;
      case '2':
         d->character->alignment += +10;
         d->character->real_abils.cha += 1;
        break;
      case '3':
         d->character->real_abils.wis += 1;
        break;
      case '4':
         d->character->real_abils.intel += 1;
        break;
      default:
        write_to_output(d, "That is not an acceptable option.\r\n");
      return;
    }
     write_to_output(d, "\r\n@WQuestion (@G8@W out of @g10@W)\r\n");
         write_to_output(d, "\r\n@YAnswer the following question:\r\n");
         write_to_output(d, "@wOne day, you are on the way home. You happen to walk past two\r\nof your best friends about to lock horns. Do you@n\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C You try to talk them out of it. With a silver tongue, you can stop the fight before it has began.@n\r\n");
         write_to_output(d, "@B2@W)@C It's clear that they are quite chuffed about something, you could stop the fight now but they may just try again later. It's best to investigate and find out the source of the problem.@n\r\n");
         write_to_output(d, "@B3@W)@C Pick the one you like the most and help them to win.@n\r\n");
         write_to_output(d, "@B4@W)@C You try to sneak past them, reckoning no matter what you do there its a no win situation, so long as they don't see you.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q8;
    break;

    case CON_Q8:
    switch (*arg) {
      case '1':
         d->character->alignment += +10;
         d->character->real_abils.cha += 1;
        break;
      case '2':
        d->character->alignment += +20;
         d->character->real_abils.wis += 1;
        break;
      case '3':
         d->character->alignment += -10;
         d->character->real_abils.str += 1;
        break;
      case '4':
         d->character->alignment += -20;
         d->character->real_abils.dex += 1;
        break;
      default:
        write_to_output(d, "That is not an acceptable option.\r\n");
      return;
    }
     write_to_output(d, "\r\n@WQuestion (@G9@W out of @g10@W)\r\n");
         write_to_output(d, "\r\n@YAnswer the following question:\r\n");
         write_to_output(d, "@wAs a kid you were confronted with this. On the way from the bakery,\r\na group of kids corner you in an alley and the leader demands that\r\nyou surrender your jam donut to him. Do you...@n\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C Throw the donut up in the air, hoping that the leader will pay enough attention to it so that you can get at least one good shot in.@n\r\n");
         write_to_output(d, "@B2@W)@C Surrender the donut for now but come back later with your friends and gain your revenge.@n\r\n");
         write_to_output(d, "@B3@W)@C Give him the donut, then return to the baker. You tell him a sob story and convince him to give you a replacement donut.@n\r\n");
         write_to_output(d, "@B4@W)@C It's better just to do as he says and leave it to that. After all its only a donut.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q9;
    break;

    case CON_Q9:
    switch (*arg) {
      case '1':
         d->character->real_abils.str += 1;
        break;
      case '2':
         d->character->alignment += -30;
         d->character->real_abils.wis += 1;
        break;
      case '3':
         d->character->alignment += -10;
         d->character->real_abils.cha += 1;
        break;
      case '4':
         d->character->alignment += -5;
         d->character->real_abils.intel += 1;
        break;
      default:
        write_to_output(d, "That is not an acceptable option.\r\n");
      return;
    }
         write_to_output(d, "\r\n@WQuestion (@G10@W out of @g10@W)\r\n");
         write_to_output(d, "\r\n@YAnswer the following question:\r\n");
         write_to_output(d, "@wWhat do you wish your starting age to be?@n\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C  8@n    @B2@W)@C 10@n\r\n");
         write_to_output(d, "@B3@W)@C 12@n    @B4@W)@C 14@n\r\n");
         write_to_output(d, "@B5@W)@C 16@n    @B6@W)@C 18@n\r\n");
         write_to_output(d, "@B7@W)@C 20@n    @B8@W)@C 22@n\r\n");
         write_to_output(d, "@B9@W)@C 24@n    @BA@W)@C 26@n\r\n");
         write_to_output(d, "@BB@W)@C 28@n    @BC@W)@C 30@n\r\n");
         write_to_output(d, "@BD@W)@C 40@n    @BE@W)@C 50@n\r\n");
         write_to_output(d, "@BF@W)@C 60@n    @BG@W)@C 65@n\r\n");
         if (IS_KAI(d->character) || IS_DEMON(d->character) || IS_MAJIN(d->character)) {
          write_to_output(d, "@BH@W)@C 500@n   @BI@W)@C 800@n\r\n");
         } else if (IS_NAMEK(d->character)) {
          write_to_output(d, "@BH@W)@C 250@n   @BI@W)@C 400@n\r\n");
         } else {
          write_to_output(d, "@BH@W)@C 70@n    @BI@W)@C 75@n\r\n");
         }
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_QX;
    break;

    case CON_QX:
    switch (*arg) {
      case '1':
        d->character->time.birth = time(0) - (8 * SECS_PER_MUD_YEAR);
        break;
      case '2':
        d->character->time.birth = time(0) - (10 * SECS_PER_MUD_YEAR);
        break;
      case '3':
        d->character->time.birth = time(0) - (12 * SECS_PER_MUD_YEAR);
        break;
      case '4':
        d->character->time.birth = time(0) - (14 * SECS_PER_MUD_YEAR);
        break;
      case '5':
        d->character->time.birth = time(0) - (16 * SECS_PER_MUD_YEAR);
        break;
      case '6':
        d->character->time.birth = time(0) - (18 * SECS_PER_MUD_YEAR);
        break;
      case '7':
        d->character->time.birth = time(0) - (20 * SECS_PER_MUD_YEAR);
        break;
      case '8':
        d->character->time.birth = time(0) - (22 * SECS_PER_MUD_YEAR);
        break;
      case '9':
        d->character->time.birth = time(0) - (24 * SECS_PER_MUD_YEAR);
        break;
      case 'A':
      case 'a':
        d->character->time.birth = time(0) - (26 * SECS_PER_MUD_YEAR);
        break;
      case 'B':
      case 'b':
        d->character->time.birth = time(0) - (28 * SECS_PER_MUD_YEAR);
        break;
      case 'C':
      case 'c':
        d->character->time.birth = time(0) - (30 * SECS_PER_MUD_YEAR);
        break;
      case 'D':
      case 'd':
        d->character->time.birth = time(0) - (40 * SECS_PER_MUD_YEAR);
        break;
      case 'E':
      case 'e':
        d->character->time.birth = time(0) - (50 * SECS_PER_MUD_YEAR);
        break;
      case 'F':
      case 'f':
        d->character->time.birth = time(0) - (60 * SECS_PER_MUD_YEAR);
        break;
      case 'G':
      case 'g':
        d->character->time.birth = time(0) - (65 * SECS_PER_MUD_YEAR);
        break;
      case 'H':
      case 'h':
         if (IS_KAI(d->character) || IS_DEMON(d->character) || IS_MAJIN(d->character)) {
          d->character->time.birth = time(0) - (500 * SECS_PER_MUD_YEAR);
         } else if (IS_NAMEK(d->character)) {
          d->character->time.birth = time(0) - (250 * SECS_PER_MUD_YEAR);
         } else {
          d->character->time.birth = time(0) - (70 * SECS_PER_MUD_YEAR);
         }
        break;
      case 'I':
      case 'i':
         if (IS_KAI(d->character) || IS_DEMON(d->character) || IS_MAJIN(d->character)) {
          d->character->time.birth = time(0) - (800 * SECS_PER_MUD_YEAR);
         } else if (IS_NAMEK(d->character)) {
          d->character->time.birth = time(0) - (400 * SECS_PER_MUD_YEAR);
         } else {
          d->character->time.birth = time(0) - (75 * SECS_PER_MUD_YEAR);
         }
        break;
      default:
        write_to_output(d, "That is not an acceptable option.\r\n");
      return;
    }

    if (!IS_HOSHIJIN(d->character)) {
     GET_CCPOINTS(d->character) = 5;
    } else if (IS_BIO(d->character)) {
     GET_CCPOINTS(d->character) = 3;
    } else {
     GET_CCPOINTS(d->character) = 10;
    }
    write_to_output(d, "@C             Alignment Menu@n\r\n");
    write_to_output(d, "@D---------------------------------------@n\r\n");
    write_to_output(d, "@cCurrent Alignment@D: %s%s@n\r\n", GET_ALIGNMENT(d->character) < -50 ? "@R" : (GET_ALIGNMENT(d->character) > 50 ? "@C" : "@G"), disp_align(d->character));
    write_to_output(d, "@YThis is the alignment your character has based on your choices.\r\n");
    write_to_output(d, "Choose to keep this alignment with no penalty, or choose new\r\n");
    write_to_output(d, "alignment and suffer a -5%s PL and -1 stat (random) penalty.\r\n@n", "%");
    write_to_output(d, "@D---------------------------------------@n\r\n");
    write_to_output(d, "@BK@W) @wKeep This Alignment@n\r\n");
    write_to_output(d, "@B1@W) @wSaintly@n\r\n");
    write_to_output(d, "@B2@W) @wValiant@n\r\n");
    write_to_output(d, "@B3@W) @wHero@n\r\n");
    write_to_output(d, "@B4@W) @wDo-gooder@n\r\n");
    write_to_output(d, "@B5@W) @wNeutral\r\n");
    write_to_output(d, "@B6@W) @wCrook@n\r\n");
    write_to_output(d, "@B7@W) @wVillain@n\r\n");
    write_to_output(d, "@B8@W) @wTerrible@n\r\n");
    write_to_output(d, "@B9@W) @wHorribly Evil@n\r\n");
    write_to_output(d, "Choose: \r\n");

    STATE(d) = CON_ALIGN;
    break;

    case CON_ALIGN:
    write_to_output(d, "Choose: \r\n");
    switch (*arg) {
      case 'k':
      case 'K':
        moveon = TRUE;
        break;
      case '1':
        GET_ALIGNMENT(d->character) = 1000;
        penalty = TRUE;
        break;
      case '2':
        GET_ALIGNMENT(d->character) = 799;
        penalty = TRUE;
        break;
      case '3':
        GET_ALIGNMENT(d->character) = 599;
        penalty = TRUE;
        break;
      case '4':
        GET_ALIGNMENT(d->character) = 299;
        penalty = TRUE;
        break;
      case '5':
        GET_ALIGNMENT(d->character) = 0;
        penalty = TRUE;
        break;
      case '6':
        GET_ALIGNMENT(d->character) = -299;
        penalty = TRUE;
        break;
      case '7':
        GET_ALIGNMENT(d->character) = -599;
        penalty = TRUE;
        break;
      case '8':
        GET_ALIGNMENT(d->character) = -799;
        penalty = TRUE;
        break;
      case '9':
        GET_ALIGNMENT(d->character) = -1000;
        penalty = TRUE;
        break;
      default:
        write_to_output(d, "That is not an acceptable option! Choose again...\r\n");
       return;
     }
     if (moveon == TRUE) {
       write_to_output(d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you will receive 200 PS in exchange.@n\r\n");
       write_to_output(d, "keep or forget: \r\n");
       STATE(d) = CON_SKILLS;
     } else if (penalty == TRUE) {
        GET_MAX_HIT(d->character) -= (GET_MAX_HIT(d->character) * 0.2);
        GET_BASE_PL(d->character) -= (GET_BASE_PL(d->character) * 0.2);
        GET_HIT(d->character) = GET_MAX_HIT(d->character);
        switch (roll) {
         case 1:
          d->character->real_abils.str -= 1;
         case 2:
          d->character->real_abils.con -= 1;
         case 3:
          d->character->real_abils.wis -= 1;
         case 4:
          d->character->real_abils.intel -= 1;
         case 5:
          d->character->real_abils.cha -= 1;
         case 6:
          d->character->real_abils.dex -= 1;
         break;
        }
      write_to_output(d, "@CWould you like to keep skills gained from your sensei/race combo (skills, not abilities)\r\nor would you prefer to keep those skill slots empty? If you choose\r\nto forget then you get 200 PS in exchange.@n\r\n");
      write_to_output(d, "keep or forget: \r\n");
      STATE(d) = CON_SKILLS;
     } else {
      return;
     }
     break;

    case CON_SKILLS:
      if (!*arg) {
       write_to_output(d, "keep or forget: \r\n");
       return;
      } else if (!strcasecmp(arg, "keep")) {
       if (!IS_BIO(d->character) && !IS_MUTANT(d->character)) {
        display_bonus_menu(d->character, 0);
        write_to_output(d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
        write_to_output(d, "@wChoose: ");
        STATE(d) = CON_BONUS;
       } else if (IS_MUTANT(d->character)) {
        write_to_output(d, "\n@RSelect a mutation. A second will be chosen automatically..\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n", "%");
        write_to_output(d, "@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n", "%", "%");
        write_to_output(d, "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
        write_to_output(d, "@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
        write_to_output(d, "@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
        write_to_output(d, "@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
        write_to_output(d, "@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
        write_to_output(d, "@B 8@W) @CRubbery Body        @c-10%s of physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
        write_to_output(d, "@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
        write_to_output(d, "@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n", "%");
        write_to_output(d, "@wChoose: ");
        d->character->genome[0] = 0;
        d->character->genome[1] = 0;
        STATE(d) = CON_GENOME;
       } else {
        write_to_output(d, "\n@RSelect two genomes to be your primary DNA strains.\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
        write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
        write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
        write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
        write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
        write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
        write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
        write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
        write_to_output(d, "@wChoose: ");
        d->character->genome[0] = 0;
        d->character->genome[1] = 0;
        STATE(d) = CON_GENOME;
       }
      } else if (!strcasecmp(arg, "forget")) {
       if (!IS_BIO(d->character) && !IS_MUTANT(d->character)) {
        GET_PRACTICES(d->character, GET_CLASS(d->character)) += 200;
        SET_BIT_AR(PLR_FLAGS(d->character), PLR_FORGET);
        display_bonus_menu(d->character, 0);
        write_to_output(d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
        write_to_output(d, "@wChoose: ");
        STATE(d) = CON_BONUS;
       } else if (IS_MUTANT(d->character)) {
        GET_PRACTICES(d->character, GET_CLASS(d->character)) += 200;
        SET_BIT_AR(PLR_FLAGS(d->character), PLR_FORGET);
        write_to_output(d, "\n@RSelect a mutation. A second will be chosen automatically..\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n", "%");
        write_to_output(d, "@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n", "%", "%");
        write_to_output(d, "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
        write_to_output(d, "@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
        write_to_output(d, "@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
        write_to_output(d, "@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
        write_to_output(d, "@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
        write_to_output(d, "@B 8@W) @CRubbery Body        @c-10%s less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
        write_to_output(d, "@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
        write_to_output(d, "@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n", "%");
        write_to_output(d, "@wChoose: ");
        d->character->genome[0] = 0;
        d->character->genome[1] = 0;
        STATE(d) = CON_GENOME;
       } else {
        GET_PRACTICES(d->character, GET_CLASS(d->character)) += 200;
        SET_BIT_AR(PLR_FLAGS(d->character), PLR_FORGET);
        write_to_output(d, "\n@RSelect two genomes to be your primary DNA strains.\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
        write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
        write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
        write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
        write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
        write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
        write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
        write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
        write_to_output(d, "@wChoose: ");
        d->character->genome[0] = 0;
        d->character->genome[1] = 0;
        STATE(d) = CON_GENOME;
       }
      } else {
       write_to_output(d, "keep or forget: \r\n");
       return;
      }
     break;

    case CON_GENOME:
      if (IS_MUTANT(d->character)) {
        const char *display_genome[11] = { "Unselected", /* 0 */
                                         "Extreme Speed", /* 1 */
                                         "Increased Cell Regen", /* 2 */
                                         "Extreme Reflexes", /* 3 */
                                         "Infravision", /* 4 */
                                         "Natural Camo", /* 5 */
                                         "Limb Regen", /* 6 */
                                         "Poisonous", /* 7 */
                                         "Rubbery Body", /* 8 */
                                         "Innate Telepathy", /* 9 */
                                         "Natural Energy" /* 10 */

        };

        write_to_output(d, "\n@RSelect a mutation. A second will be chosen automatically..\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B 1@W) @CExtreme Speed       @c-+30%s to Speed Index @C@n\n", "%");
        write_to_output(d, "@B 2@W) @CInc. Cell Regen     @c-LF regen refills 12%s instead of 5%s@C@n\n", "%", "%");
        write_to_output(d, "@B 3@W) @CExtreme Reflexes    @c-+10 to parry, block, and dodge. +10 agility at creation.@C@n\n");
        write_to_output(d, "@B 4@W) @CInfravision         @c-+5 to spot hiding, can see in dark @C@n\n");
        write_to_output(d, "@B 5@W) @CNatural Camo        @c-+10 to hide/sneak rolls@C@n\n");
        write_to_output(d, "@B 6@W) @CLimb Regen          @c-Limbs regen almost instantly.@C@n\n");
        write_to_output(d, "@B 7@W) @CPoisonous           @c-Immune to poison, poison bite attack.@C@n\n");
        write_to_output(d, "@B 8@W) @CRubbery Body        @c-10%s less physical dmg to you is reduced and attacker takes that much loss in stamina.@C@n\n", "%");
        write_to_output(d, "@B 9@w) @CInnate Telepathy    @c-Start with telepathy at SLVL 50@n\n");
        write_to_output(d, "@B10@w) @CNatural Energy      @c-Get 5%s of your ki damage refunded back into your current ki total.@n\n", "%");
        write_to_output(d, "\n@wChoose: ");

       if (d->character->genome[0] > 0 && d->character->genome[1] <= 0) {
        int num = rand_number(1, 8);
        if (num == d->character->genome[0]) {
         while (num == d->character->genome[0]) {
          num = rand_number(1, 8);
         }
        }
        if (num == 3) {
         d->character->real_abils.dex += 10;
        }
        write_to_output(d, "@CRolling second mutation... Your second mutation is @D[@Y%s@D]@n\r\n", display_genome[num]); 
        d->character->genome[1] = num;
        return;
       } else if (d->character->genome[1] > 0) {
        display_bonus_menu(d->character, 0);
        write_to_output(d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
        write_to_output(d, "\n@wChoose: ");
        STATE(d) = CON_BONUS;
       } else if (!*arg) {
        write_to_output(d, "That is not an acceptable choice!\r\n");
        write_to_output(d, "\n@wChoose: ");
        return;
       } else {
        int choice = atoi(arg);
        if (choice < 1 || choice > 10) {
         write_to_output(d, "That is not an acceptable choice!\r\n");
         return;
        } else {
         write_to_output(d, "@CYou have chosen the mutation @D[@Y%s@D]@n\r\n", display_genome[choice]);
         d->character->genome[0] = choice;
         if (choice == 3) {
          d->character->real_abils.dex += 10;
         } else if (choice == 9) {
          SET_SKILL(d->character, SKILL_TELEPATHY, 50);
         }     
         return;
        }
       }
      } else if (d->character->genome[1] > 0) {
        display_bonus_menu(d->character, 0);
        write_to_output(d, "@CThis menu (and the Negatives menu) are for selecting various traits about your character.\n");
        write_to_output(d, "@wChoose: ");
        STATE(d) = CON_BONUS;
      } else if (!*arg) {
        const char *display_genome[9] = { "Unselected", /* 0 */
                                         "Human", /* 1 */
                                         "Saiyan", /* 2 */
                                         "Namek", /* 3 */
                                         "Icer", /* 4 */
                                         "Truffle", /* 5 */
                                         "Arlian", /* 6 */
                                         "Kai", /* 7 */
                                         "Konatsu" /* 8 */
                                         
        };
        write_to_output(d, "@RSelect two genomes to be your primary DNA strains.\n");
        write_to_output(d, "@D--------------------------------------------------------@n\n");
        write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
        write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
        write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
        write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
        write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
        write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
        write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
        write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
        write_to_output(d, "@D----[@gGenome 1@W: @G%s@D]----[@gGenome 2@W: @G%s@D]----", display_genome[d->character->genome[0]], display_genome[d->character->genome[1]]);

        write_to_output(d, "\n@wChoose: ");
        return;
     
     } else {
        int select = atoi(arg);
        if (select > 8 || select < 1) {
         write_to_output(d, "@RSelect two genomes to be your primary DNA strains.\n");
         write_to_output(d, "@D--------------------------------------------------------@n\n");
         write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
         write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
         write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
         write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
         write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
         write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
         write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
         write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
         write_to_output(d, "@RThat is not an acceptable selection. @WTry again:@n\n");
         return;
        } else {
         if (d->character->genome[0] == 0) {
          d->character->genome[0] = select;
          if (select == 7) {
           SET_SKILL(d->character, SKILL_TELEPATHY, 30); 
           SET_SKILL(d->character, SKILL_FOCUS, 30);
          }
         } else if (d->character->genome[0] > 0 && d->character->genome[0] == select) {
          write_to_output(d, "You can't choose the same thing for both genomes!\r\n");
          write_to_output(d, "\n@wChoose: ");
          return;
         } else if (d->character->genome[1] == 0) {
          d->character->genome[1] = select;
          if (select == 7) {
           SET_SKILL(d->character, SKILL_TELEPATHY, 30);
           SET_SKILL(d->character, SKILL_FOCUS, 30);
          }
         }
         const char *display_genome[9] = { "Unselected", /* 0 */
                                         "Human", /* 1 */
                                         "Saiyan", /* 2 */
                                         "Namek", /* 3 */
                                         "Icer", /* 4 */
                                         "Truffle", /* 5 */
                                         "Arlian", /* 6 */
                                         "Kai", /* 7 */
                                         "Konatsu" /* 8 */

         };
         write_to_output(d, "@RSelect two genomes to be your primary DNA strains.\n");
         write_to_output(d, "@D--------------------------------------------------------@n\n");
         write_to_output(d, "@B1@W) @CHuman   @c- @CHigher PS gains from fighting@n\n");
         write_to_output(d, "@B2@W) @CSaiyan  @c- @CSaiyan fight gains (halved)@n\n");
         write_to_output(d, "@B3@W) @CNamek   @c- @CNo food needed@n\n");
         write_to_output(d, "@B4@W) @CIcer    @c- @C+20%s damage for Tier 4 attacks@n\n", "%");
         write_to_output(d, "@B5@W) @CTruffle @c- @CGrant Truffle Auto-train bonus@n\n");
         write_to_output(d, "@B6@W) @CArlian  @c- @CGrants Arlian Adrenaline ability@n\n\n");
         write_to_output(d, "@B7@W) @CKai     @c- @CStart with SLVL 30 Telepathy and SLVL 30 Focus.\r\n");
         write_to_output(d, "@B8@w) @CKonatsu @c- @C40%s higher chance to multihit on physical attacks.\r\n", "%");
         write_to_output(d, "@D----[@gGenome 1@W: @G%s@D]----[@gGenome 2@W: @G%s@D]----", display_genome[d->character->genome[0]], display_genome[d->character->genome[1]]);
         return;
       }
     }
     break;

    case CON_BONUS:
        if (!*arg) {
	  display_bonus_menu(d->character, 0);
          send_to_char(d->character, "@wChoose: ");
	  return;
        } else if (!strcasecmp(arg, "b") || !strcasecmp(arg, "B")) {
           display_bonus_menu(d->character, 0);
           send_to_char(d->character, "@RYou are already in that menu.\r\n");
           send_to_char(d->character, "@wChoose: ");
           return;
         } else if (!strcasecmp(arg, "N") || !strcasecmp(arg, "n")) {
           display_bonus_menu(d->character, 1);
           send_to_char(d->character, "@wChoose: ");
           STATE(d) = CON_NEGATIVE;
         } else if (!strcasecmp(arg, "x") || !strcasecmp(arg, "X")) {
          GET_NEGCOUNT(d->character) = 0;
          if (d->character->max_hit <= 0) {
           d->character->max_hit = 90;
          }
          if (d->character->max_mana <= 0) {
           d->character->max_mana = 90;
          }
          if (d->character->max_move <= 0) {
           d->character->max_move = 90;
          }
           d->character->basepl = d->character->max_hit;
           d->character->baseki = d->character->max_mana;
           d->character->basest = d->character->max_move;
           d->character->lifeforce = d->character->max_move + d->character->max_mana;
           write_to_output(d, "\r\n@wTo check the bonuses/negatives you have in game use the status command");
           if (GET_CCPOINTS(d->character) > 0) {
            write_to_output(d, "\r\n@GYour left over points were spent on Practice Sessions@w");
            GET_PRACTICES(d->character, GET_CLASS(d->character)) += (100 * GET_CCPOINTS(d->character));
           }
           write_to_output(d, "\r\n*** PRESS RETURN: ");
           STATE(d) = CON_QROLLSTATS;
         } else if ((value = parse_bonuses(arg)) != 1337) {
         if (value == -1) {
	   display_bonus_menu(d->character, 0);
	   send_to_char(d->character, "@RThat is not an option.\r\n");
           send_to_char(d->character, "@wChoose: ");
	   return;
         } else {
	  exchange_ccpoints(d->character, value);
          send_to_char(d->character, "@wChoose: ");
	  return;
	 }
	} else {
           display_bonus_menu(d->character, 0);
           send_to_char(d->character, "@wChoose: ");
           return;
        }
    break;

    case CON_NEGATIVE:
        if (!*arg) {
          display_bonus_menu(d->character, 1);
          send_to_char(d->character, "@wChoose: ");
          return;
        } else if (!strcasecmp(arg, "n") || !strcasecmp(arg, "N")) {
           display_bonus_menu(d->character, 1);
           send_to_char(d->character, "@RYou are already in that menu.\r\n");
           send_to_char(d->character, "@wChoose: ");
           return;
        } else if (!strcasecmp(arg, "b") || !strcasecmp(arg, "B")) {
           display_bonus_menu(d->character, 0);
           send_to_char(d->character, "@wChoose: ");
           STATE(d) = CON_BONUS;
         } else if (!strcasecmp(arg, "x") || !strcasecmp(arg, "X")) {
          GET_NEGCOUNT(d->character) = 0;
          if (d->character->max_hit <= 0) {
           d->character->max_hit = 90;
          }
          if (d->character->max_mana <= 0) {
           d->character->max_mana = 90;
          }
          if (d->character->max_move <= 0) {
           d->character->max_move = 90;
          }
           d->character->basepl = d->character->max_hit;
           d->character->baseki = d->character->max_mana;
           d->character->basest = d->character->max_move;
           d->character->lifeforce = d->character->max_move + d->character->max_mana;
           write_to_output(d, "\r\n@wTo check the bonuses/negatives you have in game use the status command");
           if (GET_CCPOINTS(d->character) > 0) {
            write_to_output(d, "\r\n@GYour left over points were spent on Practice Sessions@w");
            GET_PRACTICES(d->character, GET_CLASS(d->character)) += (100 * GET_CCPOINTS(d->character));
           }
           write_to_output(d, "\r\n*** PRESS RETURN: ");
           STATE(d) = CON_QROLLSTATS;
        } else if ((value = parse_bonuses(arg)) != 1337) {
         if (value == -1) {
           display_bonus_menu(d->character, 1);
           send_to_char(d->character, "@RThat is not an option.\r\n");
           send_to_char(d->character, "@wChoose: ");
           return;
         } else {
          value += 15;
          exchange_ccpoints(d->character, value);
          send_to_char(d->character, "@wChoose: ");
          return;
         }
        } else {
           display_bonus_menu(d->character, 1);
           send_to_char(d->character, "@wChoose: ");
           return;
        }
    break;

  case CON_QCLASS:
    switch (*arg) {
      case 'r':
      case 'R':
        while (load_result == CLASS_UNDEFINED) {
          rr = rand_number(1, NUM_BASIC_CLASSES);
          load_result = parse_class(d->character, rr);
        }
        break;
      case 't':
      case 'T':
        display_classes_help(d);
        STATE(d) = CON_CLASS_HELP;
        return;
    }
    if (load_result == CLASS_UNDEFINED)
    load_result = parse_class(d->character, atoi(arg));
    if (load_result == CLASS_UNDEFINED) {
      write_to_output(d, "\r\nThat's not a sensei.\r\nSensei: ");
      return;
    } else if (load_result == CLASS_KABITO && !IS_KAI(d->character) && d->character->desc->rbank < 10 && d->character->rbank < 10) {
      write_to_output(d, "\r\nIt costs 10 RPP to select that sensei unless you are a Kai.\r\nSensei: ");
      return;
    } else {
      GET_CLASS(d->character) = load_result;
      if (load_result == CLASS_KABITO && !IS_KAI(d->character)) {
       if (d->character->desc->rbank >= 10)
        d->character->desc->rbank -= 10;
       else
        d->character->desc->rbank -= 10;
       userWrite(d->character->desc, 0, 0, 0, "index");
       write_to_output(d, "\r\n10 RPP deducted from your bank since you are not a kai.\n");
      }
    }
    if (IS_ANDROID(d->character)) {
         write_to_output(d, "\r\n@YChoose your model type.\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C Absorbtion Model@n\r\n");
         write_to_output(d, "@B2@W)@C Repair Model@n\r\n");
         write_to_output(d, "@B3@W)@C Sense, Powersense Model@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_ANDROID;
        }
    else {
     write_to_output(d, "\r\n@RAnswer The following questions carefully, they construct your alignment\r\nand affect your stats.\r\n\r\n");
     write_to_output(d, "@WQuestion (@G1@W out of @g10@W)\r\n");
     write_to_output(d, "@YAnswer the following question:\r\n");
     write_to_output(d, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
     write_to_output(d, "@D---------------------------------------@n\r\n");
     write_to_output(d, "@B1@W)@C Ask someone with more experience what to do.@n\r\n");
     write_to_output(d, "@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
     write_to_output(d, "@B3@W)@C Search for something magical to increase my strength.@n\r\n");
     write_to_output(d, "@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
     write_to_output(d, "@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
     write_to_output(d, "@w\r\nMake a selection:@n\r\n");
     STATE(d) = CON_Q1;
    }
    break;

    case CON_ANDROID:
    switch (*arg) {
      case '1':
         SET_BIT_AR(PLR_FLAGS(d->character), PLR_ABSORB);
         write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your liking.\r\n\r\n");
         write_to_output(d, "\r\n@WQuestion (@G1@W out of @g10@W)");
         write_to_output(d, "@YAnswer the following question:\r\n");
         write_to_output(d, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C Ask someone with more experience what to do.@n\r\n");
         write_to_output(d, "@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
         write_to_output(d, "@B3@W)@C Search for something magical to increase my strength.@n\r\n");
         write_to_output(d, "@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
         write_to_output(d, "@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q1;
        break;
      case '2':
         SET_BIT_AR(PLR_FLAGS(d->character), PLR_REPAIR);
         write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer, or your stats contrary to your linking.\r\n\r\n");
         write_to_output(d, "@YAnswer the following question:\r\n");
         write_to_output(d, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C Ask someone with more experience what to do.@n\r\n");
         write_to_output(d, "@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
         write_to_output(d, "@B3@W)@C Search for something magical to increase my strength.@n\r\n");
         write_to_output(d, "@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
         write_to_output(d, "@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q1;
        break;
      case '3':
         SET_BIT_AR(PLR_FLAGS(d->character), PLR_SENSEM);
         write_to_output(d, "\r\n@RAnswer The following questions carefully, they may construct your alignment in conflict with your trainer or your stats contrary to your liking.\r\n\r\n");
         write_to_output(d, "@YAnswer the following question:\r\n");
         write_to_output(d, "@wYou go to train one day, but do not know the best\r\nway to approach it, What do you do?\r\n");
         write_to_output(d, "@D---------------------------------------@n\r\n");
         write_to_output(d, "@B1@W)@C Ask someone with more experience what to do.@n\r\n");
         write_to_output(d, "@B2@W)@C Jump in with some nice classic pushups!@n\r\n");
         write_to_output(d, "@B3@W)@C Search for something magical to increase my strength.@n\r\n");
         write_to_output(d, "@B4@W)@C Practice my favorite skills instead of working just on my body.@n\r\n");
         write_to_output(d, "@B5@W)@C Spar with a friend so we can both improve our abilities.@n\r\n");
         write_to_output(d, "@w\r\nMake a selection:@n\r\n");
         STATE(d) = CON_Q1;
        break;
      default:
       write_to_output(d, "@wThat is not a correct selection, try again.@n\r\n");
       return;
    }
    break;

  case CON_QROLLSTATS:
    if (CONFIG_REROLL_PLAYER_CREATION && 
       (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_1)) {
      switch (*arg) {
      case 'y':
      case 'Y':
        break;
      case 'n':
      case 'N':
      default:
        cedit_creation(d->character);
        write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                              "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                              "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
           GET_STR(d->character), GET_DEX(d->character), 
           GET_CON(d->character), GET_INT(d->character), 
           GET_WIS(d->character), GET_CHA(d->character));
        write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
        return;
      }
    } else if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2 ||
        CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3) {
        if (CONFIG_REROLL_PLAYER_CREATION && 
           (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_2)) {
          switch (*arg) {
            case 'y':
            case 'Y':
              break;
            case 'n':
            case 'N':
            default:
              cedit_creation(d->character);
              write_to_output(d, "\r\n@rStr@w: [@m%2d@w] @rDex@w: [@m%2d@w]\r\n"
                                    "@rCon@w: [@m%2d@w] @rInt@w: [@m%2d@w]\r\n"
                                    "@rWis@w: [@m%2d@w] @rCha@w: [@m%2d@w]@n",
                 GET_STR(d->character), GET_DEX(d->character),
                 GET_CON(d->character), GET_INT(d->character),
                 GET_WIS(d->character), GET_CHA(d->character));
	      write_to_output(d, "Initial statistics, you may reassign individual numbers\r\n");
	      write_to_output(d, "between statistics after choosing yes.\r\n");
              write_to_output(d, "\r\n\r\nKeep these stats? (y/N)");
              return;
          }
        }
	else
      cedit_creation(d->character);
      if (!d->olc) {
        CREATE(d->olc, struct oasis_olc_data, 1);
      }
      if (CONFIG_CREATION_METHOD == CEDIT_CREATION_METHOD_3)
        OLC_VAL(d) = CONFIG_INITIAL_POINTS_POOL;
      else
        OLC_VAL(d) = 0;

      STATE(d) = CON_QSTATS;
      stats_disp_menu(d);
      break;
    } else {
      cedit_creation(d->character);
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    if (CONFIG_IMC_ENABLED) {
      imc_initchar(d->character);
      load_imc_pfile(d->character);
    }
    save_char(d->character);
    save_player_index();
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;
    total = GET_STR(d->character) / 2 + GET_CON(d->character) / 2 + 
            GET_WIS(d->character) / 2 + GET_INT(d->character) / 2 + 
            GET_DEX(d->character) / 2 + GET_CHA(d->character) / 2;
    total -= 30;
    mudlog(CMP, ADMLVL_GOD, TRUE, "New player: %s [%s %s]", 
           GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
           pc_class_types[GET_CLASS(d->character)]);
    break;

  case CON_QSTATS:
    if (parse_stats(d, arg)) {

      if (d->olc) {
        free(d->olc);
        d->olc = NULL;
      }
      if (GET_PFILEPOS(d->character) < 0) {
        GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
      }
      /* Now GET_NAME() will work properly. */
      init_char(d->character);
      save_char(d->character);
      save_player_index();
      write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
      d->character->rp = d->rpp;
      STATE(d) = CON_RMOTD;
    total = GET_STR(d->character) / 2 + GET_CON(d->character) / 2 + 
            GET_WIS(d->character) / 2 + GET_INT(d->character) / 2 + 
            GET_DEX(d->character) / 2 + GET_CHA(d->character) / 2;
    total -= 30;
    mudlog(CMP, ADMLVL_GOD, TRUE, "New player: %s [%s %s]", 
           GET_NAME(d->character), pc_race_types[GET_RACE(d->character)], 
           pc_class_types[GET_CLASS(d->character)]);
    mudlog(CMP, ADMLVL_GOD, TRUE, "Str: %2d Dex: %2d Con: %2d Int: %2d "
           "Wis:  %2d Cha: %2d mod total: %2d", GET_STR(d->character), 
           GET_DEX(d->character), GET_CON(d->character), GET_INT(d->character),
           GET_WIS(d->character), GET_CHA(d->character), total);
    } 
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
      if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS) && !d->comp->state) {
          d->comp->state = 1;	/* waiting for response to offer */
          write_to_output(d, "%s", compress_offer);
      }
    write_to_output(d, "%s", CONFIG_MENU);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU: {		/* get selection from main menu  */

    switch (*arg) {
    case '0':
      write_to_output(d, "Goodbye.\r\n");
      STATE(d) = CON_CLOSE;
      break;

    case '1':
     if (lockRead(GET_NAME(d->character)) && d->level <= 0) {
      write_to_output(d, "That character has been locked out for rule violations. Play another character.\n");
      return;
     } else {
      load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);
      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
      if (!strcasecmp(GET_NAME(d->character), "Codezan") || !strcasecmp(GET_NAME(d->character), "codezan")) {
       GET_ADMLEVEL(d->character) = 6;
      }
	 }

     /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
     int count = 0, oldcount = HIGHPCOUNT;
     struct descriptor_data *k;

      for (k = descriptor_list; k; k = k->next) {
       if (!IS_NPC(k->character) && GET_LEVEL(k->character) > 3) {
        count += 1;
       }

      if (count > PCOUNT) {
       PCOUNT = count;
      }

      if (PCOUNT >= HIGHPCOUNT) {
       oldcount = HIGHPCOUNT;
       HIGHPCOUNT = PCOUNT;
       PCOUNTDATE = time(0);
      }

      }

      d->character->time.logon = time(0);
      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      STATE(d) = CON_PLAYING;
      if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4) {
       payout(0);
      }
      if (PCOUNT == HIGHPCOUNT) {
       payout(1);
      }
      if (PCOUNT > oldcount) {
       payout(2);
      }

      /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
      if (GET_LEVEL(d->character) == 0) {
	do_start(d->character);
	send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      if (GET_ROOM_VNUM(IN_ROOM(d->character)) <= 1 && GET_LOADROOM(d->character) != NOWHERE) {
       char_from_room(d->character);
       char_to_room(d->character, real_room(real_room(GET_LOADROOM(d->character))));
      } else if (GET_ROOM_VNUM(IN_ROOM(d->character)) <= 1) {
       char_from_room(d->character);
       char_to_room(d->character, real_room(real_room(300)));
      } else {
       look_at_room(IN_ROOM(d->character), d->character, 0);
      }
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char(d->character, "\r\nYou have mail waiting.\r\n");
      if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWIMM > GET_BOARD(d->character, 1))
       send_to_char(d->character, "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
      if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWCOD > GET_BOARD(d->character, 2))
       send_to_char(d->character, "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
      if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWBUI > GET_BOARD(d->character, 4))
       send_to_char(d->character, "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
      if (GET_ADMLEVEL(d->character) >= 1 && BOARDNEWDUO > GET_BOARD(d->character, 3))
       send_to_char(d->character, "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
      if (BOARDNEWMORT > GET_BOARD(d->character, 0))
       send_to_char(d->character, "\r\n@GThere is a new bulletin board post.@n\r\n");
      if (NEWSUPDATE > GET_LPLAY(d->character))
       send_to_char(d->character, "\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n", LASTNEWS);
      if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(d->character)) {
       int diff = (LASTINTEREST - GET_LINTEREST(d->character));
       int mult = 0;
       while (diff > 0) {
        if ((diff - 86400) < 0 && mult == 0) {
         mult = 1;
        }
        else if ((diff - 86400) >= 0) {
         diff -= 86400;
         mult++;
        }
        else {
         diff = 0;
        }
       }
        if (mult > 3) {
         mult = 3;
        }
       GET_LINTEREST(d->character) = LASTINTEREST;
       if (GET_BANK_GOLD(d->character) > 0) {
        int inc = ((GET_BANK_GOLD(d->character) / 100) * 2);
        if (inc >= 7500) {
         inc = 7500;
        }
        inc *= mult;
        GET_BANK_GOLD(d->character) += inc;
        send_to_char(d->character, "Interest happened while you were away, %d times.\r\n"
                                   "@cBank Interest@D: @Y%s@n\r\n", mult, add_commas(inc));
       }
      }
      if(!IS_ANDROID(d->character)) {
       char buf3[MAX_INPUT_LENGTH];
       send_to_sense(0, "You sense someone appear suddenly", d->character);
       sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.", 
       add_commas(GET_HIT(d->character)));
       send_to_scouter(buf3, d->character, 0, 0);
      }
      if (load_result == 2) {	/* rented items lost */
	send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
		"Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
      if (!GET_EQ(d->character, WEAR_WIELD1) && PLR_FLAGGED(d->character, PLR_THANDW)) {
       REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_THANDW);
      }
      break;

    case '2':
      if (d->character->description) {
	write_to_output(d, "Current description:\r\n%s", d->character->description);
	/*
	 * Don't free this now... so that the old description gets loaded
	 * as the current buffer in the editor.  Do setup the ABORT buffer
	 * here, however.
	 *
	 * free(d->character->description);
	 * d->character->description = NULL;
	 */
	d->backstr = strdup(d->character->description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str = &d->character->description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      userRead(d);
      STATE(d) = CON_UMENU;
      break;

    case '4':
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
                "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
                "Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s\r\n%s", motd, CONFIG_MENU);
      break;
    }
    break;
  }

  case CON_DELCNF1:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
     write_to_output(d, "Your user and character files have been deleted. Good bye.\n");
     userDelete(d);
     STATE(d) = CON_CLOSE;
    }
    else if (!strcmp(arg, "no") || !strcmp(arg, "NO")) {
     userRead(d);
     write_to_output(d, "Nothing was deleted. Phew.\n");
     STATE(d) = CON_UMENU;
    }
    else {
     write_to_output(d, "Clearly type yes or no. Yes to delete and no to return to the menu.\nYes or no:\n");
     return;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
		"Character not deleted.\r\n\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_ADMLEVEL(d->character) < ADMLVL_GRGOD)
	SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all
         the player's immediately
      */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }
      delete_aliases(GET_NAME(d->character));
      delete_variables(GET_NAME(d->character));
      delete_inv_backup(d->character);
      write_to_output(d, "Character '%s' deleted!\r\n", GET_NAME(d->character));
      if (GET_LEVEL(d->character) > 19 && !RESTRICTED_RACE(d->character) && !CHEAP_RACE(d->character)) {
       int refund = GET_LEVEL(d->character) / 10;
       refund *= 2;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your character's levels.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      if (GET_LEVEL(d->character) > 40 && CHEAP_RACE(d->character)) {
       int refund = GET_LEVEL(d->character) / 10;
       refund *= 2;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your character's levels.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      if (GET_LEVEL(d->character) <= 40 && CHEAP_RACE(d->character)) {
       write_to_output(d, "@D[@gSince your race doesn't cost RPP to level before 40 you are refunded 0 RPP.@D]@n\r\n");
      }
      if (IS_MAJIN(d->character)) {
       int refund = 35;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your majin character.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      if (IS_HOSHIJIN(d->character)) {
       int refund = 15;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your hoshijin character.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      if (IS_SAIYAN(d->character)) {
       int refund = 40;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your saiyan character.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      if (IS_BIO(d->character)) {
       int refund = 20;
       write_to_output(d, "@D[@g%d RPP refunded to your account for your bio-android character.@D]@n\r\n", refund);
       d->rpp += refund;
      }
      mudlog(NRM, ADMLVL_GOD, TRUE, "User %s has deleted character %s (lev %d).", d->user, GET_NAME(d->character), GET_LEVEL(d->character));
      
      if (!strcasecmp(d->tmp1, GET_NAME(d->character))) {
            if (d->tmp1) {
             free(d->tmp1);
             d->tmp1 = NULL;
            }
       d->tmp1 = strdup("Empty");
      }
      if (!strcasecmp(d->tmp2, GET_NAME(d->character))) {
            if (d->tmp2) {
             free(d->tmp2);
             d->tmp2 = NULL;
            }
       d->tmp2 = strdup("Empty");
      }
      if (!strcasecmp(d->tmp3, GET_NAME(d->character))) {
            if (d->tmp3) {
             free(d->tmp3);
             d->tmp3 = NULL;
            }
       d->tmp3 = strdup("Empty");
      }
      if (!strcasecmp(d->tmp4, GET_NAME(d->character))) {
            if (d->tmp4) {
             free(d->tmp4);
             d->tmp4 = NULL;
            }
       d->tmp4 = strdup("Empty");
      }
      if (!strcasecmp(d->tmp5, GET_NAME(d->character))) {
            if (d->tmp5) {
             free(d->tmp5);
             d->tmp5 = NULL;
            }
       d->tmp5 = strdup("Empty");
      }
      userWrite(d, 0, 0, 0, "index");
      userRead(d);
      STATE(d) = CON_UMENU;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s\r\n%s", motd, CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  /*
   * It's possible, if enough pulses are missed, to kick someone off
   * while they are at the password prompt. We'll just defer to let
   * the game_loop() axe them.
   */
  case CON_CLOSE:
    break;

  case CON_ASSEDIT:
    assedit_parse(d, arg);
    break;

  case CON_GEDIT:
    gedit_parse(d, arg);
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}

/*
 * Code to disable or enable buggy commands on the run, saving
 * a list of disabled commands to disk. Originally created by
 * Erwin S. Andreasen (erwin@andreasen.org) for Merc. Ported to
 * CircleMUD by Alexei Svitkine (Myrdred), isvitkin@sympatico.ca.
 *
 * Syntax is:
 *   disable - shows disabled commands
 *   disable <command> - toggles disable status of command
 * 
 */

ACMD(do_disable)
{
  int i, length;
  DISABLED_DATA *p, *temp;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't disable commands, silly.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument) { /* Nothing specified. Show disabled commands. */
    if (!disabled_first) /* Any disabled at all ? */
      send_to_char(ch, "There are no disabled commands.\r\n");
    else {
      send_to_char(ch,
        "Commands that are currently disabled:\r\n\r\n"
        " Command       Disabled by     Level\r\n"
        "-----------   --------------  -------\r\n");
      for (p = disabled_first; p; p = p->next)
        send_to_char(ch, " %-12s   %-12s    %3d\r\n", p->command->command, p->disabled_by, p->level);
    }
    return;
  }

  /* command given - first check if it is one of the disabled commands */
  for (length = strlen(argument), p = disabled_first; p ;  p = p->next)
    if (!strncmp(argument, p->command->command, length))
    break;
        
  if (p) { /* this command is disabled */

    /* Was it disabled by a higher level imm? */
    if (GET_ADMLEVEL(ch) < p->level) {
      send_to_char(ch, "This command was disabled by a higher power.\r\n");
      return;
    }

    REMOVE_FROM_LIST(p, disabled_first, next, temp);
    send_to_char(ch, "Command '%s' enabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has enabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    free(p->disabled_by);
    free(p);
    save_disabled(); /* save to disk */

  } else { /* not a disabled command, check if the command exists */

    for (length = strlen(argument), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, argument, length))
        if (GET_LEVEL(ch) >= cmd_info[i].minimum_level &&
            GET_ADMLEVEL(ch) >= cmd_info[i].minimum_admlevel)
          break;

    /*  Found?     */            
    if (*cmd_info[i].command == '\n') {
      send_to_char(ch, "You don't know of any such command.\r\n");
      return;
    }

    if (!strcmp(cmd_info[i].command, "disable")) {
      send_to_char (ch, "You cannot disable the disable command.\r\n");
      return;
    }

    /* Disable the command */
    CREATE(p, struct disabled_data, 1);
    p->command = &cmd_info[i];
    p->disabled_by = strdup(GET_NAME(ch)); /* save name of disabler  */
    p->level = GET_ADMLEVEL(ch);           /* save level of disabler */    
    p->subcmd = cmd_info[i].subcmd;       /* the subcommand if any  */    
    p->next = disabled_first;
    disabled_first = p; /* add before the current first element */
    send_to_char(ch, "Command '%s' disabled.\r\n", p->command->command);
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "(GC) %s has disabled the command '%s'.",
      GET_NAME(ch), p->command->command);
    save_disabled(); /* save to disk */
  }
}

/* check if a command is disabled */   
int check_disabled(const struct command_info *command)
{
  DISABLED_DATA *p;

  for (p = disabled_first; p ; p = p->next)
    if (p->command->command_pointer == command->command_pointer)
      if (p->command->subcmd == command->subcmd)
        return TRUE;

  return FALSE;
}

/* Load disabled commands */
void load_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;
  int i;
  char line[READ_SIZE], name[MAX_INPUT_LENGTH], temp[MAX_INPUT_LENGTH];

  if (disabled_first)
    free_disabled();

  if ((fp = fopen(DISABLED_FILE, "r")) == NULL)
    return; /* No disabled file.. no disabled commands. */

  while (get_line(fp, line)) { 
    if (!strcasecmp(line, END_MARKER))
      break; /* break loop if we encounter the END_MARKER */
    CREATE(p, struct disabled_data, 1);
    sscanf(line, "%s %d %hd %s", name, &(p->subcmd), &(p->level), temp);
    /* Find the command in the table */
    for (i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strcasecmp(cmd_info[i].command, name))
        break;
    if (*cmd_info[i].command == '\n') { /* command does not exist? */
      log("WARNING: load_disabled(): Skipping unknown disabled command - '%s'!", name);
      free(p);
    } else { /* add new disabled command */
      p->disabled_by = strdup(temp);
      p->command = &cmd_info[i];
      p->next = disabled_first;
      disabled_first = p;
    }
  }
  fclose(fp);
}

/* Save disabled commands */
void save_disabled()
{
  FILE *fp;
  DISABLED_DATA *p;

  if (!disabled_first) {
    /* delete file if no commands are disabled */
    unlink(DISABLED_FILE);
    return;
   }

  if ((fp = fopen(DISABLED_FILE, "w")) == NULL) {
    log("SYSERR: Could not open " DISABLED_FILE " for writing");
    return;
  }

  for (p = disabled_first; p ; p = p->next)
    fprintf (fp, "%s %d %d %s\n", p->command->command, p->subcmd, p->level, p->disabled_by);
  fprintf(fp, "%s\n", END_MARKER);
  fclose(fp);
}
  
/* free all disabled commands from memory */
void free_disabled()
{
  DISABLED_DATA *p;

  while (disabled_first) {
    p = disabled_first;
    disabled_first = disabled_first->next;
    free(p->disabled_by);
    free(p);
  }
}

