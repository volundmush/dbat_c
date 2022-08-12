/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "act.wizard.h"
#include "interpreter.h"
#include "utils.h"
#include "config.h"
#include "act.other.h"
#include "maputils.h"
#include "dg_comm.h"
#include "handler.h"
#include "act.item.h"
#include "act.informative.h"
#include "players.h"
#include "weather.h"
#include "assemblies.h"
#include "house.h"
#include "comm.h"
#include "constants.h"
#include "dg_scripts.h"
#include "races.h"
#include "class.h"
#include "spells.h"
#include "improved-edit.h"
#include "objsave.h"
#include "feats.h"
#include "fight.h"
#include "genolc.h"
#include "screen.h"
#include "local_limits.h"
#include "shop.h"
#include "guild.h"
#include "spell_parser.h"

/* local variables */
static int copyover_timer = 0; /* for timed copyovers */

/* local functions */
static void print_lockout(struct char_data *ch);
static void execute_copyover(void);
static int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
static void perform_immort_invis(struct char_data *ch, int level);
static void do_stat_room(struct char_data *ch);
static void do_stat_object(struct char_data *ch, struct obj_data *j);
static void do_stat_character(struct char_data *ch, struct char_data *k);
static void stop_snooping(struct char_data *ch);
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);
static void mob_checkload(struct char_data *ch, mob_vnum mvnum);
static void obj_checkload(struct char_data *ch, obj_vnum ovnum);
static void trg_checkload(struct char_data *ch, trig_vnum tvnum);
static void lockWrite(struct char_data *ch, char *name);

// definitions
ACMD(do_lag)
{

 struct descriptor_data *d;
 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

 two_arguments(argument, arg, arg2);

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: lag (target) (number of seconds)\r\n");
  return;
 }

 int found = FALSE, num = atoi(arg2);

 if (num <= 0 || num > 5) {
  send_to_char(ch, "Keep it between 1 to 5 seconds please.\r\n");
  return;
 }

 for (d = descriptor_list; d; d = d->next) {
  if (!strcasecmp(CAP(GET_NAME(d->character)), CAP(arg))) {
   if (GET_ADMLEVEL(d->character) > GET_ADMLEVEL(ch)) {
    send_to_char(ch, "Sorry, you've been outranked.\r\n");
    return;
   }
    switch (num) {
     case 1:
      WAIT_STATE(d->character, PULSE_1SEC);
      break;
     case 2:
      WAIT_STATE(d->character, PULSE_2SEC);
      break;
     case 3:
      WAIT_STATE(d->character, PULSE_3SEC);
      break;
     case 4:
      WAIT_STATE(d->character, PULSE_4SEC);
      break;
     case 5:
      WAIT_STATE(d->character, PULSE_5SEC);
      break;
    }
    found = TRUE;
  }
 }

 if (found == FALSE) {
  send_to_char(ch, "That player isn't around.\r\n");
  return;
 }

}

/*Update things that need it. Just space for the moment - Iovan */
void update_space(void)
{
       FILE *mapfile;
       int rowcounter, colcounter;
       int vnum_read;

  log("Updated Space Map. ");

  //Load the map vnums from a file into an array
  mapfile = fopen("../lib/surface.map", "r");

  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      fscanf(mapfile, "%d", &vnum_read);
      mapnums[rowcounter][colcounter] = real_room(vnum_read);
    }
  }

  fclose(mapfile);
}

/* Read the news entry */
ACMD(do_news)
{

 if(IS_NPC(ch)) {
   return;
 }

 FILE *fl;
 const char *filename;
 char line[256];
 char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], title[256], lastline[256];
 int entries = 0, lookup = 0, nr, found = FALSE, first = TRUE, exit = FALSE;

 one_argument(argument, arg);
 filename = NEWS_FILE;

  /* Check the file for the entry so we may edit it if need be*/

 if (!*arg) {
  send_to_char(ch, "Syntax: news (number | list)\r\n");
  return;
 }

 lookup = atoi(arg);
 
 if (!(fl = fopen(filename, "r")))
 {
  log("SYSERR: opening news file for reading");
  return;
 }

 if (lookup > 0) {
  while (!feof(fl) && exit == FALSE) {
   get_line(fl, line);
    if(*line == '#') { /* Count the entries */
     if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
        continue;
     } else if (nr != lookup) {
      entries++;
      continue;
     } else { /* One we want to read */
      sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
      sprintf(buf, "@w--------------------------------------------------------------\n@cNum@W: @D(@G%3d@D)                @cTitle@W: @g%-50s@n\n", nr, title);
      found = TRUE;
      while (!feof(fl) && exit == FALSE) {
        get_line(fl, line);
        if (*line != '#') { /* As long as it isn't a new entry*/
         if (first == TRUE) {
          first = FALSE;
          sprintf(buf+strlen(buf), "%s\n@w--------------------------------------------------------------\n", line);
          sprintf(lastline, "%s", line);
         } else if (!strcasecmp(line, lastline)) {
           continue;
         } else {
          sprintf(buf+strlen(buf), "%s\n", line);
          sprintf(lastline, "%s", line);
         }
        } else {
         exit = TRUE;
        }
      } /* End write buffer with entry text */
     } /* End One We wanted to read */
    } /* End Check the entry */
  } /* End of main read while */
  fclose(fl);
 } else if (!strcasecmp(arg, "list")) {
  while (!feof(fl)) {
   get_line(fl, line);
    if(*line == '#') { /* Count the entries */
     entries++;
     if (sscanf(line, "#%d", &nr) != 1) { /* Check the entry */
        continue;
     } else { /* One we want to read */
      if (first == TRUE) {
        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
        sprintf(buf, "@wNews Entries (Newest at the bottom, to read an entry use 'news (number)')\n@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
        first = FALSE;
      } else {
        sscanf(line, "#%d %50[0-9a-zA-Z,.!' ]s\n", &nr, title);
        sprintf(buf+strlen(buf), "@D[@cNum@W: @D(@G%3d@D) @cTitle@W: @g%-50s@D]@n\n", nr, title);
      }
     }
    } /* End Check the entry */
  } /* End of main listwhile */
  fclose(fl);

  if (entries > 0) {
   GET_LPLAY(ch) = time(0);
   WAIT_STATE(ch, PULSE_1SEC);
   page_string(ch->desc, buf, TRUE);
  } else {
   send_to_char(ch, "The news file is empty right now.\r\n");
  }
  *buf = '\0';
  *title = '\0';
  *lastline = '\0';
  return;
 } else {
  fclose(fl);
  send_to_char(ch, "Syntax: news (number | list)\r\n");
  return;
 }

 if (found == TRUE) {
  send_to_char(ch, "%s\r\n", buf);
  GET_LPLAY(ch) = time(0);
  *buf = '\0';
  *title = '\0';
  *lastline = '\0';
  WAIT_STATE(ch, PULSE_1SEC);
  return;
 } else {
  send_to_char(ch, "That news entry does not exist.\r\n");
  return;
 }

}

/* Write the news entry */

ACMD(do_newsedit)
{

 if(GET_ADMLEVEL(ch) < 1 || IS_NPC(ch)) {
   return;
 }

 FILE *fl;
 const char *filename;
 char line[256];
 int entries = 0, lookup = 0, lastentry = 0, nr;

  filename = NEWS_FILE;

 if (!*argument) {
  send_to_char(ch, "Syntax: newsedit (title)\r\n");
  return;
 } else if (strlen(argument) > 50) {
  send_to_char(ch, "Limit of 50 characters for title.\r\n");
  return;
 } else if (strstr(argument, "#")) {
  send_to_char(ch, "# is a forbidden character for news entries as it is used by the file system.\r\n");
  return;
 }

  /* Check the file for the entry so we may edit it if need be*/

  if (!(fl = fopen(filename, "r")))
  {
    log("SYSERR: Couldn't open news file for reading");
    return;
  }

  while (!feof(fl)) {
   get_line(fl, line);
    if(*line == '#') { /* Count the entries */
      entries++;
     if (sscanf(line, "#%d", &nr) != 1) {
      continue;
     } else {
      lastentry = nr;
     }
    }
  } /* End of main read while */
  fclose(fl);

  /* Time to write the entry */
  struct {
    char *cmd;
    char level;
    char **buffer;
    int  size;
    char *filename;
  } fields[] = {
        /* edit the lvls to your own needs */
        { "news",       ADMLVL_IMMORT, &immlist,       2000,  IMMLIST_FILE},
        { "\n",         0,              NULL,           0,      NULL }
  };

         char *tmstr;
         time_t mytime = time(0);
         tmstr = (char *) asctime(localtime(&mytime));
         *(tmstr + strlen(tmstr) - 1) = '\0';

  if (lastentry == 0) {
   if (entries == 0) {  
    lookup = 1;
   } else { /* Uh oh */
    send_to_imm("ERROR: News file entries are disorganized. Report to Iovan for analysis.\r\n");
    return;
   }
  } else {
   lookup = lastentry + 1;
  }
  char *backstr = NULL;
  act("$n begins to edit the news.", TRUE, ch, 0, 0, TO_ROOM);
  send_to_char(ch, "@D----------------------=[@GNews Edit@D]=----------------------@n\n");
  send_to_char(ch, " @RRemember that using # in newsedit is not possible. That\n");
  send_to_char(ch, "character will be eaten because it is required for the news\n");
  send_to_char(ch, "file as a delimiter. Also if you want to create an empty line\n");
  send_to_char(ch, "between paragraphs you will need to enter a single space and\n");
  send_to_char(ch, "not just push enter. Happy editing!@n\n");
  send_to_char(ch, "@D---------------------------------------------------------@n\n");
  send_editor_help(ch->desc);
  skip_spaces(&argument);
  ch->desc->newsbuf = strdup(argument);
  TOP_OF_NEWS = lookup;
  LASTNEWS = lookup;
  string_write(ch->desc, fields[0].buffer, 2000, 0, backstr);
  STATE(ch->desc) = CON_NEWSEDIT;
}

static void print_lockout(struct char_data *ch)
{
  if (IS_NPC(ch))
   return;

  FILE *file;
  char fname[40], filler[50], line[256], buf[MAX_STRING_LENGTH * 4];
  int count = 0, first = TRUE;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  else if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  sprintf(buf, "@b------------------[ @RLOCKOUT @b]------------------@n\n");
  while (!feof(file)) {
    get_line(file, line);
    sscanf(line, "%s\n", filler);
    if (first != TRUE && strstr(buf, filler) == NULL) {
     if (count == 0) {
      sprintf(buf+strlen(buf), "%-23s@D|@n", filler);
      count = 1;
     } else {
      sprintf(buf+strlen(buf), "%-23s\n", filler);
      count = 0;
     }
    } else if (first == TRUE) {
     sprintf(buf+strlen(buf), "%-23s@D|@n", filler);
     first = FALSE;
     count = 1;
    }
    *filler = '\0';
  }
  if (count == 1) {
   sprintf(buf+strlen(buf), "\n");
  }
  sprintf(buf+strlen(buf), "@b------------------[ @RLOCKOUT @b]------------------@n\n");

  page_string(ch->desc, buf, 0);

  fclose(file);
}

ACMD(do_approve)
{

 char arg[MAX_INPUT_LENGTH];
 struct char_data *vict = NULL;

 one_argument(argument, arg);

 if (!*arg){
  send_to_char(ch, "What player do you want to approve as having an acceptable bio?\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
  send_to_char(ch, "That player is not in the game.\r\n");
  return;
 }
 
 if (PLR_FLAGGED(vict, PLR_BIOGR)) {
  send_to_char(ch, "They have already been approved. If this was made in error inform Iovan.\r\n");
  return;
 } else {
  SET_BIT_AR(PLR_FLAGS(vict), PLR_BIOGR);
  send_to_char(ch, "They have now been approved.\r\n");
  return;
 }
}

static void lockWrite(struct char_data *ch, char *name)
{
  FILE *file;
  char fname[40], filler[50], line[256];
  char *names[500] = {""};
  FILE *fl;
  int count = 0, x = 0, found = FALSE;

  /* Read Introduction File */
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  else if (!(file = fopen(fname, "r"))) {
    send_to_char(ch, "The lockout file does not exist.");
    return;
  }
  while (!feof(file) || count < 498) {
    get_line(file, line);
    sscanf(line, "%s\n", filler);
    names[count] = strdup(filler);
    count++;
     *filler = '\0';
  }
  fclose(file);

  /* Write Introduction File */

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
    return;

  if( !(fl = fopen(fname, "w")) ) {
    log("ERROR: could not save Lockout File, %s.", fname);
    return;
  }

  while(x < count) {
    if (x == 0 || strcasecmp(names[x-1], names[x])) {
     if (strcasecmp(names[x], CAP(name))) {
      fprintf(fl, "%s\n", CAP(names[x]));
     }
     else {
      found = TRUE;
     }
    }
    x++;
  }
  if (found == FALSE) {
   fprintf(fl, "%s\n", CAP(name));
   send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, was locked out of the MUD by @c%s@W.@n\r\n", CAP(name), GET_NAME(ch));
   log("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
   log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
  } else {
   send_to_all("@rLOCKOUT@D: @WThe character, @C%s@W, has had lockout removed by @c%s@W.@n\r\n", CAP(name), GET_NAME(ch));
   log("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
   log_imm_action("LOCKOUT: %s sentenced by %s.", CAP(name), GET_NAME(ch));
  }

  fclose(fl);
  return;
}

ACMD(do_reward)
{
 struct char_data *vict = NULL;
 struct descriptor_data *k;
 int amt = 0;

 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];

 two_arguments(argument, arg, arg2);

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: reward (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
  return;
 }

 if(!readUserIndex(arg)) {
  send_to_char(ch, "That is not a recognised user file.\r\n");
  return;
 }

 amt = atoi(arg2);

 if (amt == 0) {
  send_to_char(ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
  return;
 }
 
  for (k = descriptor_list; k; k = k->next) {
   if (IS_NPC(k->character))
    continue;
   if (STATE(k) != CON_PLAYING)
    continue;
   if (!strcasecmp(k->user, arg))
    vict = k->character;
  }

 if (amt > 0) {
  if (vict != NULL) {
   send_to_char(ch, "@WYou award @C%s @D(@G%d@D)@W RP points.@n\r\n", GET_NAME(vict), amt);
   send_to_char(vict, "@D[@YROLEPLAY@D] @WYou have been awarded @D(@G%d@D)@W RP points by @C%s@W.@n\r\n", amt, GET_NAME(ch));
   send_to_imm("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
   vict->rp += amt;
   GET_RP(vict) = vict->rp;
   if (amt <= 29) {
    GET_TRP(vict) += amt;
   }
   vict->desc->rpp += amt;
   userWrite(vict->desc, 0, 0, 0, "index");
  }
  else {
   send_to_char(ch, "@WYou award user @C%s @D(@G%d@D)@W RP points.@n\r\n", arg, amt);
   send_to_imm("ROLEPLAY: User %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
   userWrite(NULL, 0, amt, 0, arg);
  }
 }
 else {
  if (vict != NULL) {
   send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W.@n\r\n", amt, GET_NAME(vict));
   send_to_char(vict, "@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from you.@n\r\n", GET_NAME(ch), amt);
   send_to_imm("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has had %d RP points deducted by %s.", GET_NAME(vict), amt, GET_NAME(ch));
   vict->rp += amt;
   GET_RP(vict) = vict->rp;
   vict->desc->rpp += amt;
   userWrite(vict->desc, 0, 0, 0, "index");
  }
  else {
   send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from user @C%s@W.@n\r\n", amt, arg);
   send_to_imm("ROLEPLAY: User %s has had %d RP points deducted by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has been awarded %d RP points by %s.", arg, amt, GET_NAME(ch));
   userWrite(NULL, 0, amt, 0, arg);
  }
 }

}

ACMD(do_rbank)
{
 struct char_data *vict = NULL;
 struct descriptor_data *k;
 int amt = 0;

 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];

 two_arguments(argument, arg, arg2);

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: rbank (target) (amount)\r\nThis is either a positive number or a negative.\r\n");
  return;
 }

 if(!readUserIndex(arg)) {
  send_to_char(ch, "That is not a recognised user file.\r\n");
  return;
 }

 amt = atoi(arg2);

 if (amt == 0) {
  send_to_char(ch, "That is pointless don't you think? Try an amount higher than 0.\r\n");
  return;
 }
 
  for (k = descriptor_list; k; k = k->next) {
   if (IS_NPC(k->character))
    continue;
   if (STATE(k) != CON_PLAYING)
    continue;
   if (!strcasecmp(k->user, arg))
    vict = k->character;
  }

 if (amt > 0) {
  if (vict != NULL) {
   send_to_char(ch, "@WYou put @D(@G%d@D)@W RP points into @C%s@W's Bank.@n\r\n", amt, GET_NAME(vict));
   send_to_char(vict, "@D[@YROLEPLAY@D] @WYou have had @D(@G%d@D)@W RP points added to your RPP Bank by @C%s@W.@n\r\n", amt, GET_NAME(ch));
   send_to_imm("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, GET_NAME(ch));
   vict->rbank += amt;
   GET_RBANK(vict) = vict->rbank;
   if (amt <= 29) {
    GET_TRP(vict) += amt;
   }
   vict->desc->rbank += amt;
   userWrite(vict->desc, 0, 0, 0, "index");
  }
  else {
   send_to_char(ch, "@WYou put @D(@G%d@D)@W RP points into @C%s@W's Bank.@n\r\n", amt, arg);
   send_to_imm("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has had %d RP points put into their RPP Bank by %s.", arg, amt, GET_NAME(ch));
   userWrite(NULL, 0, 0, amt, arg);
  }
 }
 else {
  if (vict != NULL) {
   send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W's Bank.@n\r\n", amt, GET_NAME(vict));
   send_to_char(vict, "@D[@YROLEPLAY@D] @C%s@W deducts @D(@G%d@D)@W RP points from your RPP Bank.@n\r\n", GET_NAME(ch), amt);
   send_to_imm("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", GET_NAME(vict), amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", GET_NAME(vict), amt, GET_NAME(ch));
   vict->rbank += amt;
   GET_RBANK(vict) = vict->rbank;
   vict->desc->rbank += amt;
   userWrite(vict->desc, 0, 0, 0, "index");
  }
  else {
   send_to_char(ch, "@WYou deduct @D(@G%d@D)@W RP points from @C%s@W's Bank.@n\r\n", amt, arg);
   send_to_imm("ROLEPLAY: %s has had %d RP points deducted from their RPP Bank by %s.", arg, amt, GET_NAME(ch));
   log_imm_action("ROLEPLAY: %s has had %d RP points put deducted from RPP Bank by %s.", arg, amt, GET_NAME(ch));
   userWrite(NULL, 0, 0, amt, arg);
  }
 }

}

ACMD(do_permission)
{
 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];

 two_arguments(argument, arg, arg2);

 if (!*arg) {
  send_to_char(ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
  return;
 }

 if (!*arg2 && !strcasecmp("unrestrict", arg)) {
  send_to_char(ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
  return;
 }
 if (!strcasecmp("unrestrict", arg)) {
  if (!strcasecmp("saiyan", arg2)) {
   send_to_char(ch, "You have unrestricted saiyans for the very next character creation.\r\n");
   send_to_imm("PERMISSION: %s unrestricted saiyans.", GET_NAME(ch));
   SAIYAN_ALLOWED = TRUE;
  }
  else if (!strcasecmp("majin", arg2)) {
   send_to_char(ch, "You have unrestricted majins for the very next character creation.\r\n");
   send_to_imm("PERMISSION: %s unrestricted majins.", GET_NAME(ch));
   MAJIN_ALLOWED = TRUE;
  }
  else {
   send_to_char(ch, "You want to unrestrict which race? @Gsaiyan @nor @Gmajin@n?\r\n");
   return;
  }
 }
 else if (!strcasecmp("restrict", arg)) {
  send_to_char(ch, "You have restricted character creation to standard race slection.\r\n");
  send_to_imm("PERMISSION: %s restricted races again.", GET_NAME(ch));
  MAJIN_ALLOWED = FALSE;
 }
 else {
  send_to_char(ch, "You want to @Grestrict@n or @Gunrestrict@n?\r\n");
  return;
 }
}

ACMD(do_transobj)
{

 struct obj_data *obj;
 struct char_data *vict;
 struct descriptor_data *d;
 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];
 
 two_arguments(argument, arg, arg2);

 if (!IS_NPC(ch) && GET_ADMLEVEL(ch) < 1) {
  send_to_char(ch, "Huh!?");
  return;
 }

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: transo (object) (target)\r\n");
  return;
 }

 if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
  send_to_char(ch, "You want to send what?\r\n");
  return;
 }
 else if (!strcasecmp("all", arg2)) {
  int num = GET_OBJ_VNUM(obj);
  struct obj_data *obj2 = NULL;

  act("You send $p to everyone in the game.", TRUE, ch, obj, 0, TO_CHAR);

  for (d = descriptor_list; d; d = d->next) {
   if (IS_NPC(d->character))
    continue;
   else if (!IS_PLAYING(d))
    continue;
   else if (d->character == ch)
    continue;
   else {
    act("$N sends $p across the universe to you.", TRUE, d->character, obj, ch, TO_CHAR);
    obj2 = read_object(num, VIRTUAL);
    obj_to_char(obj2, d->character);    
   }
  }
 }
 else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
  send_to_char(ch, "That player is not in the game.\r\n");
  return;
 }
 else {
  act("You send $p to $N.", TRUE, ch, obj, vict, TO_CHAR);
  act("$n sends $p across the universe to you.", TRUE, ch, obj, vict, TO_VICT);
  obj_from_char(obj);
  obj_to_char(obj, vict);
  return;
 }
}

void search_replace(char *string, const char *find, const char *replace)
{
        char final[MAX_STRING_LENGTH], temp[2];
        size_t start, end, i;

        while (strstr(string, find) != NULL) {

                final[0] = '\0';
                start = strstr(string, find) - string;
                end = start + strlen(find);

                temp[1] = '\0';

                strncat(final, string, start);

                strcat(final, replace);

                for (i = end; string[i] != '\0'; i++) {
                        temp[0] = string[i];
                        strcat(final, temp);
                }

                sprintf(string, final);

        }
        return;
}

ACMD(do_interest)
{
 if (GET_ADMLEVEL(ch) < 5) {
  send_to_char(ch, "Huh!?\r\n");
  return;
 }
 else {
  if (INTERESTTIME > 0) {
   char *tmstr;
   tmstr = (char *) asctime(localtime(&INTERESTTIME));
   *(tmstr + strlen(tmstr) - 1) = '\0';
   send_to_char(ch, "INTEREST TIME: [%s]\r\n", tmstr);
   return;
  }
  send_to_char(ch, "Interest time has been initiated!\r\n");
  INTERESTTIME = time(0) + 86400;
  LASTINTEREST = time(0) + 86400;
  return;
 }
}

/* do_finddoor, finds the door(s) that a key goes to */ 
ACMD(do_finddoor) 
{ 
  int d, vnum = NOTHING, num = 0; 
  size_t len, nlen; 
  room_rnum i; 
  char arg[MAX_INPUT_LENGTH]; 
  char buf[MAX_STRING_LENGTH] = {0}; 
  struct char_data *tmp_char; 
  struct obj_data *obj; 

  one_argument(argument, arg); 

  if (!*arg) { 
    send_to_char(ch, "Format: finddoor <obj/vnum>\r\n"); 
  } else if (is_number(arg)) { 
    vnum = atoi(arg); 
    obj = &obj_proto[real_object(vnum)]; 
  } else { 
    generic_find(arg, 
         FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_WORLD | FIND_OBJ_EQUIP, 
         ch, &tmp_char, &obj); 
    if (!obj) 
      send_to_char(ch, "What key do you want to find a door for?\r\n"); 
    else 
      vnum = GET_OBJ_VNUM(obj); 
  } 
  if (vnum != NOTHING) { 
      len = snprintf(buf, sizeof(buf), "Doors unlocked by key [%d] %s are:\r\n", 
                      vnum, GET_OBJ_SHORT(obj)); 
      for (i = 0; i <= top_of_world; i++) { 
        for (d = 0; d < NUM_OF_DIRS; d++) { 
          if (world[i].dir_option[d] && world[i].dir_option[d]->key && 
              world[i].dir_option[d]->key == vnum) { 
            nlen = snprintf(buf + len, sizeof(buf) - len, 
                            "[%3d] Room %d, %s (%s)\r\n", 
                            ++num, world[i].number, 
                            dirs[d], world[i].dir_option[d]->keyword); 
            if (len + nlen >= sizeof(buf) || nlen < 0) 
              break; 
            len += nlen; 
          } 
        } /* for all directions */ 
      } /* for all rooms */ 
      if (num > 0) 
        page_string(ch->desc, buf, 1); 
      else 
        send_to_char(ch, "No doors were found for key [%d] %s.\r\n", 
                         vnum, GET_OBJ_SHORT(obj)); 
  } 
}

ACMD(do_recall)
{
 if (GET_ADMLEVEL(ch) < 1) {
  send_to_char(ch, "You are not an immortal!\r\n");
 }
 else {
  send_to_char(ch, "You disappear in a burst of light!\r\n");
  act("$n disappears in a burst of light!", FALSE, ch, 0, 0, TO_ROOM);
  if (real_room(2) != NOWHERE) {
  char_from_room(ch);
  char_to_room(ch, real_room(2));
  look_at_room(IN_ROOM(ch), ch, 0);
  GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
  }
 }

}

ACMD(do_hell)
{
 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (!*arg) {
  send_to_char(ch, "Syntax: lockout (character)\n"
                   "        lockout list\r\n");
  return;
 }

 if (!strcasecmp(arg, "Iovan") || !strcasecmp(arg, "iovan") || !strcasecmp(arg, "Fahl") || !strcasecmp(arg, "fahl") || !strcasecmp(arg, "Xyron") || !strcasecmp(arg, "xyron") || !strcasecmp(arg, "Samael") || !strcasecmp(arg, "samael")) {
  send_to_char(ch, "What are you smoking? You can't lockout senior imms.\r\n");
  return;
 }

 if (!strcasecmp(arg, "list")) {
  print_lockout(ch);
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
   lockWrite(ch, arg);
   return;
 } else {
  struct descriptor_data *d = vict->desc;
  Crash_rentsave(vict, 0);
  extract_char(vict);
  lockWrite(ch, GET_NAME(vict));
  if (d && STATE(d) != CON_PLAYING) {
   STATE(d) = CON_CLOSE;
   vict->desc->character = NULL;
   vict->desc = NULL;
  }
  return;
 }
 return;
}

ACMD(do_echo)
{
  skip_spaces(&argument);
  bool NoName = FALSE;

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 4];
    char name[128];
    int found = FALSE, trunc = 0;
    struct char_data *vict = NULL, *next_v = NULL, *tch = NULL;

    if (strlen(argument) > 1000) {
     trunc = strlen(argument) - 1000;
     argument[strlen(argument) - trunc] = '\0';
     sprintf(argument, "%s\n@D(@gMessage truncated to 1000 characters@D)@n\n", argument);
    }

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
     next_v = vict->next_in_room;
     if (vict == ch)
      continue;
     if (found == FALSE) {
      sprintf(name, "*%s", GET_NAME(vict));
      if (strstr(argument, CAP(name))) {
       found = TRUE;
       tch = vict;
      }
      if (found == FALSE && !IS_NPC(vict)) {
       if (readIntro(ch, vict) == 1) {
        sprintf(name, "*%s", get_i_name(ch, vict));
        if (strstr(argument, CAP(name))) {
         found = TRUE;
         tch = vict;
        }
       }
      }
     }
    }

    if (subcmd == SCMD_SMOTE) {
       if (!strstr(argument, "#")) {
        NoName = TRUE;
       }
      strlcpy(buf, argument, sizeof(buf));
      search_replace(buf, "#", "$n");
      search_replace(buf, "&1", "'@C");
      search_replace(buf, "&2", "@w'");
      if (found == TRUE) {
       search_replace(buf, name, "$N");
      }
      else if (strstr(buf, "*")) {
       search_replace(buf, "*", "");
      }
    }
    else if (subcmd == SCMD_EMOTE) {
      snprintf(buf, sizeof(buf), "$n %s", argument);
      search_replace(buf, "#", "$n");
      search_replace(buf, "&1", "'@C");
      search_replace(buf, "&2", "@w'");
      if (found == TRUE) {
       search_replace(buf, name, "$N");
      }
    }
    else {
     snprintf(buf, sizeof(buf), "%s", argument);
    }
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(ch, "%s", CONFIG_OK);
    }
    if (NoName == TRUE) {
     char blom[MAX_INPUT_LENGTH];
     sprintf(blom, "@D(@GOOC@W: @gSmote by user %s@D)@n", IS_NPC(ch) ? GET_NAME(ch) : (ch->desc->user == NULL ? "ERROR REPORT" : ch->desc->user));
     act(blom, FALSE, ch, 0, 0, TO_ROOM);
    }
    if (found == FALSE) {
     act(buf, FALSE, ch, 0, 0, TO_CHAR);
     act(buf, FALSE, ch, 0, 0, TO_ROOM);
    }
    else {

     act(buf, FALSE, ch, 0, tch, TO_CHAR);
     act(buf, FALSE, ch, 0, tch, TO_NOTVICT);
     search_replace(buf, "$N", "you");
     act(buf, FALSE, ch, 0, tch, TO_VICT);
    }
  }
}

ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];
  struct room_data *rm;

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL) {
      if ((location = IN_ROOM(target_mob)) == NOWHERE) {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE) {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* a location has been found -- if you're >= GRGOD, no restrictions. */
  if (GET_ADMLEVEL(ch) >= ADMLVL_VICE)
    return (location);

  rm = &world[location];

  if ((!can_edit_zone(ch, rm->zone) && GET_ADMLEVEL(ch) < ADMLVL_GOD)
       && ZONE_FLAGGED(rm->zone, ZONE_QUEST)) {
       send_to_char(ch, "This target is in a quest zone.\r\n");
       return (NOWHERE);
       }

  if ((GET_ADMLEVEL(ch) < ADMLVL_VICE) && ZONE_FLAGGED(rm->zone, ZONE_NOIMMORT)){
       send_to_char(ch, "This target is in a zone closed to all.\r\n");
       return (NOWHERE);
      }

  if (ROOM_FLAGGED(location, ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else
    return (location);

  return (NOWHERE);
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_rnum location;

  if ((location = find_target_room(ch, argument)) == NOWHERE)
    return;
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "They are inside a healing tank!\r\n");
   return;
  }

  snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, location);

  snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(IN_ROOM(ch), ch, 0);
  enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
}

ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (strcasecmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((GET_ADMLEVEL(ch) < GET_ADMLEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char(ch, "Go transfer someone your own size.\r\n");
	return;
      }
      if (PLR_FLAGGED(victim, PLR_HEALT)) {
       send_to_char(ch, "They are inside a healing tank!\r\n");
       return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(IN_ROOM(victim), victim, 0);
      enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
    }
  } else {			/* Trans All */
    if (!ADM_FLAGGED(ch, ADM_TRANSALL)) {
      send_to_char(ch, "I think not.\r\n");
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
        look_at_room(IN_ROOM(victim), victim, 0);
        enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
      }
    send_to_char(ch, "%s", CONFIG_OK);
  }
}

ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE) {
    if (PLR_FLAGGED(victim, PLR_HEALT)) {
     send_to_char(ch, "They are inside a healing tank!\r\n");
     return;
    }
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(IN_ROOM(victim), victim, 0);
    enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
  }
}

ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj") && !is_abbrev(buf, "mat") && !is_abbrev(buf, "wtype") && !is_abbrev(buf, "atype"))) {
    send_to_char(ch, "Usage: vnum { atype | material | mob | obj | wtype } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");

  if (is_abbrev(buf, "mat"))
    if (!vnum_material(buf2, ch))
      send_to_char(ch, "No materials by that name.\r\n");

  if (is_abbrev(buf, "wtype"))
    if (!vnum_weapontype(buf2, ch))
      send_to_char(ch, "No weapon types by that name.\r\n");

  if (is_abbrev(buf, "atype"))
    if (!vnum_armortype(buf2, ch))
      send_to_char(ch, "No armor types by that name.\r\n");

}

#define ZOCMD zone_table[zrnum].cmd[subcmd]
 
void list_zone_commands_room(struct char_data *ch, room_vnum rvnum)
{
  extern struct index_data **trig_index;
  zone_rnum zrnum = real_zone_by_thing(rvnum);
  room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE) {
    send_to_char(ch, "No zone information available.\r\n");
    return;
  }

  send_to_char(ch, "Zone commands in this room:@y\r\n");
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        break;
    }
    if (cmd_room == rrnum) {
      count++;
      /* start listing */
      switch (ZOCMD.command) {
        case 'M':
          send_to_char(ch, "%sLoad %s@y [@c%d@y], MaxMud : %d, MaxR : %d, Chance : %d\r\n",
                  ZOCMD.if_flag ? " then " : "",
                  mob_proto[ZOCMD.arg1].short_descr,
                  mob_index[ZOCMD.arg1].vnum, ZOCMD.arg2,
                  ZOCMD.arg4, ZOCMD.arg5
                  );
          break;
        case 'G':
          send_to_char(ch, "%sGive it %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      ZOCMD.arg2, ZOCMD.arg5
    	      );
          break;
        case 'O':
          send_to_char(ch, "%sLoad %s@y [@c%d@y], Max : %d, MaxR : %d, Chance : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      ZOCMD.arg2, ZOCMD.arg4, ZOCMD.arg5
    	      );
          break;
        case 'E':
          send_to_char(ch, "%sEquip with %s@y [@c%d@y], %s, Max : %d, Chance : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      equipment_types[ZOCMD.arg3],
    	      ZOCMD.arg2, ZOCMD.arg5
    	      );
          break;
        case 'P':
          send_to_char(ch, "%sPut %s@y [@c%d@y] in %s@y [@c%d@y], Max : %d, Chance : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      obj_proto[ZOCMD.arg3].short_description,
    	      obj_index[ZOCMD.arg3].vnum,
    	      ZOCMD.arg2, ZOCMD.arg5
    	      );
          break;
        case 'R':
          send_to_char(ch, "%sRemove %s@y [@c%d@y] from room.\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg2].short_description,
    	      obj_index[ZOCMD.arg2].vnum
    	      );
          break;
        case 'D':
          send_to_char(ch, "%sSet door %s as %s.\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      dirs[ZOCMD.arg2],
    	      ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open"
    	      );
          break;
        case 'T':
          send_to_char(ch, "%sAttach trigger @c%s@y [@c%d@y] to %s\r\n",
            ZOCMD.if_flag ? " then " : "",
            trig_index[ZOCMD.arg2]->proto->name,
            trig_index[ZOCMD.arg2]->vnum,
            ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
              ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????"))));
          break;
        case 'V':
          send_to_char(ch, "%sAssign global %s:%d to %s = %s\r\n",
            ZOCMD.if_flag ? " then " : "",
            ZOCMD.sarg1, ZOCMD.arg2,
            ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
              ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????"))),
            ZOCMD.sarg2);
          break;
        default:
          send_to_char(ch, "<Unknown Command>\r\n");
          break;
      }
    } 
    subcmd++;  
  }
  send_to_char(ch, "@n");
  if (!count) 
    send_to_char(ch, "None!\r\n");

}
#undef ZOCMD

static void do_stat_room(struct char_data *ch)
{
  char buf2[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  struct room_data *rm = &world[IN_ROOM(ch)];
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  send_to_char(ch, "Room name: @c%s@n\r\n", rm->name);

  sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
  send_to_char(ch, "Zone: [%3d], VNum: [@g%5d@n], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
	  zone_table[rm->zone].number, rm->number, IN_ROOM(ch),
          (long) rm->number + ROOM_ID_BASE, buf2);

  sprintbitarray(rm->room_flags, room_bits, RF_ARRAY_MAX, buf2);
  send_to_char(ch, "Room Damage: %d, Room Effect: %d\r\n", rm->dmg, rm->geffect);
  send_to_char(ch, "SpecProc: %s, Flags: %s\r\n", rm->func == NULL ? "None" : "Exists", buf2);

  send_to_char(ch, "Description:\r\n%s", rm->description ? rm->description : "  None.\r\n");

  if (rm->ex_description) {
    send_to_char(ch, "Extra descs:");
    for (desc = rm->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [@c%s@n]", desc->keyword);
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch, "Chars present:");
  column = 14;	/* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s @y%s@n(%s)", found++ ? "," : "", GET_NAME(k),
		!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }

  if (rm->contents) {
    send_to_char(ch, "Contents:@g");
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;

      column += send_to_char(ch, "%s %s", found++ ? "," : "", j->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "@n");
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    char buf1[128];

    if (!rm->dir_option[i])
      continue;

    if (rm->dir_option[i]->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " @cNONE@n");
    else
      snprintf(buf1, sizeof(buf1), "@c%5d@n", GET_ROOM_VNUM(rm->dir_option[i]->to_room));

    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(ch, "Exit @c%-5s@n:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n  DC Lock: [%2d], DC Hide: [%2d], DC Skill: [%4s], DC Move: [%2d]\r\n%s",
        dirs[i], buf1,
        rm->dir_option[i]->key == NOTHING ? -1 : rm->dir_option[i]->key,
	rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2,
	rm->dir_option[i]->dclock, rm->dir_option[i]->dchide,
	rm->dir_option[i]->dcskill == 0 ? "None" : spell_info[rm->dir_option[i]->dcskill].name, rm->dir_option[i]->dcmove,
	rm->dir_option[i]->general_description ? rm->dir_option[i]->general_description : "  No exit description.\r\n");
  }

  /* check the room for a script */
  do_sstat_room(ch, rm);
 
  list_zone_commands_room(ch, rm->number);
}

static void do_stat_object(struct char_data *ch, struct obj_data *j)
{
  int i, found;
  obj_vnum vnum;
  struct obj_data *j2;
  struct char_data *sitter;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];

  vnum = GET_OBJ_VNUM(j);
  if (GET_LAST_LOAD(j) > 0) {
  char *tmstr;
  tmstr = (char *) asctime(localtime(&GET_LAST_LOAD(j)));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  send_to_char(ch, "LOADED DROPPED: [%s]\r\n", tmstr);
  }
  if (GET_OBJ_VNUM(j) == 65) {
   send_to_char(ch, "Healing Tank Charge Level: [%d]\r\n", HCHARGE(j));
  }
  send_to_char(ch, "Name: '%s', Keywords: %s, Size: %s\r\n",
               j->short_description ? j->short_description : "<None>", j->name,
               size_names[GET_OBJ_SIZE(j)]);

  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  send_to_char(ch, "VNum: [@g%5d@n], RNum: [%5d], Idnum: [%5d], Type: %s, SpecProc: %s\r\n",
	vnum, GET_OBJ_RNUM(j), GET_ID(j), buf, GET_OBJ_SPEC(j) ? "Exists" : "None");

  send_to_char(ch, "Generation time: @g%s@nUnique ID: @g%"I64T"@n\r\n",
    ctime(&j->generation), j->unique_id);

  send_to_char(ch, "Object Hit Points: [ @g%3d@n/@g%3d@n]\r\n",
   GET_OBJ_VAL(j, VAL_ALL_HEALTH), GET_OBJ_VAL(j, VAL_ALL_MAXHEALTH));

  send_to_char(ch, "Object loaded in room: @y%d@n\r\n",
   OBJ_LOADROOM(j));

  send_to_char(ch, "Object Material: @y%s@n\r\n",
   material_names[GET_OBJ_MATERIAL(j)]);

  if (SITTING(j)) {
   sitter = SITTING(j);
   send_to_char(ch, "HOLDING: %s\r\n", GET_NAME(sitter));
  }

  if (j->ex_description) {
    send_to_char(ch, "Extra descs:");
    for (desc = j->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [@g%s@n]", desc->keyword);
    send_to_char(ch, "\r\n");
  }

  sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Can be worn on: %s\r\n", buf);

  sprintbitarray(GET_OBJ_PERM(j), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "Set char bits : %s\r\n", buf);

  sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
  send_to_char(ch, "Extra flags   : %s\r\n", buf);

  send_to_char(ch, "Weight: %"I64T", Value: %d, Cost/day: %d, Timer: %d, Min Level: %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));

  send_to_char(ch, "In room: %d (%s), ", GET_ROOM_VNUM(IN_ROOM(j)),
	IN_ROOM(j) == NOWHERE ? "Nowhere" : world[IN_ROOM(j)].name);

  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  send_to_char(ch, "In object: %s, ", j->in_obj ? j->in_obj->short_description : "None");
  send_to_char(ch, "Carried by: %s, ", j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  send_to_char(ch, "Worn by: %s\r\n", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, VAL_LIGHT_HOURS) == -1)
      send_to_char(ch, "Hours left: Infinite\r\n");
    else
      send_to_char(ch, "Hours left: [%d]\r\n", GET_OBJ_VAL(j, VAL_LIGHT_HOURS));
    break;
  case ITEM_SCROLL:
    send_to_char(ch, "Spell: (Level %d) %s\r\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL),
	    skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL1)));
    break;
  case ITEM_POTION:
    send_to_char(ch, "Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, VAL_POTION_LEVEL),
	    skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL1)), 
            skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL2)),
	    skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char(ch, "Spell: %s at level %d, %d (of %d) charges remaining\r\n",
	    skill_name(GET_OBJ_VAL(j, VAL_STAFF_SPELL)), GET_OBJ_VAL(j, VAL_STAFF_LEVEL),
	    GET_OBJ_VAL(j, VAL_STAFF_CHARGES), GET_OBJ_VAL(j, VAL_STAFF_MAXCHARGES));
    break;
  case ITEM_WEAPON:
    send_to_char(ch, "Weapon Type: %s, Todam: %dd%d, Message type: %d\r\n",
	    weapon_type[GET_OBJ_VAL(j, VAL_WEAPON_SKILL)], 
            GET_OBJ_VAL(j, VAL_WEAPON_DAMDICE),
            GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE),
            GET_OBJ_VAL(j, VAL_WEAPON_DAMTYPE));
    send_to_char(ch, "Average damage per round %.1f\r\n", ((GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE) + 1) / 2.0) * GET_OBJ_VAL (j, VAL_WEAPON_DAMDICE));
    send_to_char(ch, "Crit type: %s, Crit range: %d-20\r\n",
            crit_type[GET_OBJ_VAL(j, 6)], 20 - GET_OBJ_VAL(j, 8));
    break;
  case ITEM_ARMOR:
    send_to_char(ch, "Armor Type: %s, AC-apply: [%d]\r\n", armor_type[GET_OBJ_VAL(j, VAL_ARMOR_SKILL)], GET_OBJ_VAL(j, VAL_ARMOR_APPLYAC));
    send_to_char(ch, "Max dex bonus: %d, Armor penalty: %d, Spell failure: %d\r\n",
                 GET_OBJ_VAL(j, VAL_ARMOR_MAXDEXMOD), GET_OBJ_VAL(j, VAL_ARMOR_CHECK), GET_OBJ_VAL(j, VAL_ARMOR_SPELLFAIL));
    break;
  case ITEM_TRAP:
    send_to_char(ch, "Spell: %d, - Hitpoints: %d\r\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL), GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_FLAGS), container_bits, buf, sizeof(buf));
    send_to_char(ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
	    GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf, GET_OBJ_VAL(j, VAL_CONTAINER_KEY),
	    YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID), drinks, buf, sizeof(buf));
    send_to_char(ch, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n",
	    GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(j, VAL_DRINKCON_HOWFULL), YESNO(GET_OBJ_VAL(j, VAL_DRINKCON_POISON)), buf);
    break;
  case ITEM_NOTE:
    send_to_char(ch, "Tongue: %d\r\n", GET_OBJ_VAL(j, VAL_NOTE_LANGUAGE));
    break;
  case ITEM_KEY:
    /* Nothing */
    break;
  case ITEM_FOOD:
    send_to_char(ch, "Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, VAL_FOOD_FOODVAL), YESNO(GET_OBJ_VAL(j, VAL_FOOD_POISON)));
    break;
  case ITEM_MONEY:
    send_to_char(ch, "Coins: %d\r\n", GET_OBJ_VAL(j, VAL_MONEY_SIZE));
    break;
  default:
    send_to_char(ch, "Values 0-12: [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\r\n",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3),
	    GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5),
	    GET_OBJ_VAL(j, 6), GET_OBJ_VAL(j, 7),
	    GET_OBJ_VAL(j, 8), GET_OBJ_VAL(j, 9),
	    GET_OBJ_VAL(j, 10), GET_OBJ_VAL(j, 11));
    break;
  }

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    int column;

    send_to_char(ch, "\r\nContents:@g");
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", j2->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j2->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "@n");
  }

  found = FALSE;
  send_to_char(ch, "Affections:");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf, sizeof(buf));
      send_to_char(ch, "%s %+d to %s", found++ ? "," : "", j->affected[i].modifier, buf);
      switch (j->affected[i].location) {
      case APPLY_FEAT:
        send_to_char(ch, " (%s)", feat_list[j->affected[i].specific].name);
        break;
      case APPLY_SKILL:
        send_to_char(ch, " (%s)", spell_info[j->affected[i].specific].name);
        break;
      }
    }
  if (!found)
    send_to_char(ch, " None");

  send_to_char(ch, "\r\n");

  /* check the object for a script */
  do_sstat_object(ch, j);
}

static void do_stat_character(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i, i2, column, found = FALSE;
  struct obj_data *j;
  struct obj_data *chair;
  struct follow_type *fol;
  struct affected_type *aff;


  if (IS_NPC(k)) {
   char *tmstr;
   tmstr = (char *) asctime(localtime(&GET_LPLAY(k)));
   *(tmstr + strlen(tmstr) - 1) = '\0';
   send_to_char(ch, "LOADED AT: [%s]\r\n", tmstr);
  }
  sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
  send_to_char(ch, "%s %s '%s'  IDNum: [%5d], In room [%5d], Loadroom : [%5d]\r\n",
	  buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)), IS_NPC(k) ? MOB_LOADROOM(k) : GET_LOADROOM(k));

  send_to_char(ch, "DROOM: [%5d]\r\n", GET_DROOM(k));
  if (IS_MOB(k)) {
    if (k->master_id > -1)
      sprintf(buf, ", Master: %s", get_name_by_id(k->master_id));
    else
      buf[0] = 0;
    send_to_char(ch, "Keyword: %s, VNum: [%5d], RNum: [%5d]%s\r\n", k->name,
                 GET_MOB_VNUM(k), GET_MOB_RNUM(k), buf);
  } else

  send_to_char(ch, "Title: %s\r\n", k->title ? k->title : "<None>");

  send_to_char(ch, "L-Des: %s@n", k->long_descr ? k->long_descr : "<None>\r\n");
  if (CONFIG_ALLOW_MULTICLASS) {
    strncpy(buf, class_desc_str(k, 1, 0), sizeof(buf));
  } else {
    sprinttype(k->chclass, pc_class_types, buf, sizeof(buf));
  }
  sprinttype(k->race, pc_race_types, buf2, sizeof(buf2));
  send_to_char(ch, "Class: %s, Race: %s, Lev: [@y%2d(%dHD+%dcl+%d)@n], XP: [@y%"I64T"@n]\r\n",
                   buf, buf2, GET_LEVEL(k), GET_HITDICE(k),
                   GET_CLASS_LEVEL(k), GET_LEVEL_ADJ(k), GET_EXP(k));

  if (!IS_NPC(k)) {
    char buf1[64], cmbuf2[64];

    strlcpy(buf1, asctime(localtime(&(k->time.created))), sizeof(buf1));
    strlcpy(cmbuf2, asctime(localtime(&(k->time.logon))), sizeof(cmbuf2));
    buf1[10] = cmbuf2[10] = '\0';

    send_to_char(ch, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
	    buf1, cmbuf2, (int)k->time.played / 3600,
	    (int)((k->time.played % 3600) / 60), age(k)->year);

    if (k->desc != NULL) {
     send_to_char(ch, "@YOwned by User@D: [@C%s@D]@n\r\n", GET_USER(k));
    } else {
     send_to_char(ch, "@YOwned by User@D: [@C%s@D]@n\r\n", GET_LOG_USER(k));
    }
    if (!IS_NPC(k)) {
     send_to_char(ch, "@RCharacter Deaths@D: @r%d@n\r\n", GET_DCOUNT(k));
    }

    send_to_char(ch, "Hometown: [%d], Align: [%4d], Ethic: [%4d]", GET_HOME(k),
                 GET_ALIGNMENT(k), GET_ETHIC_ALIGNMENT(k));

    /*. Display OLC zone for immorts .*/
    if (GET_ADMLEVEL(k) >= ADMLVL_BUILDER) {
      if (GET_OLC_ZONE(k)==AEDIT_PERMISSION)
        send_to_char(ch, ", OLC[@cActions@n]");
      else if (GET_OLC_ZONE(k)==HEDIT_PERMISSION)
        send_to_char(ch, ", OLC[@cHedit@n]");
      else if (GET_OLC_ZONE(k)==NOWHERE)
        send_to_char(ch, ", OLC[@cOFF@n]");
      else
        send_to_char(ch, ", OLC: [@c%d@n]", GET_OLC_ZONE(k));
    }
    send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "Str: [@c%d@n]  Int: [@c%d@n]  Wis: [@c%d@n]  "
	  "Dex: [@c%d@n]  Con: [@c%d@n]  Cha: [@c%d@n]\r\n",
	  GET_STR(k), GET_INT(k), GET_WIS(k), GET_DEX(k), GET_CON(k), GET_CHA(k));

  send_to_char(ch, "PL :[@g%12s@n]  KI :[@g%12s@n]  ST :[@g%12s@n]\r\n",
	  add_commas(GET_HIT(k)), add_commas(GET_MANA(k)), add_commas(GET_MOVE(k)));
  send_to_char(ch, "MPL:[@g%12s@n]  MKI:[@g%12s@n]  MST:[@g%12s@n]\r\n",
          add_commas(GET_MAX_HIT(k)), add_commas(GET_MAX_MANA(k)), add_commas(GET_MAX_MOVE(k)));
  send_to_char(ch, "BPL:[@g%12s@n]  BKI:[@g%12s@n]  BST:[@g%12s@n]\r\n",
          add_commas(GET_BASE_PL(k)), add_commas(GET_BASE_KI(k)), add_commas(GET_BASE_ST(k)));
  send_to_char(ch, "LF :[@g%12s@n]  MLF:[@g%12s@n]  LFP:[@g%3d@n]\r\n",
          add_commas(GET_LIFEFORCE(k)), add_commas(GET_LIFEMAX(k)), GET_LIFEPERC(k));

  if (GET_ADMLEVEL(k))
    send_to_char(ch, "Admin Level: [@y%d - %s@n]\r\n", GET_ADMLEVEL(k), admin_level_names[GET_ADMLEVEL(k)]);

  send_to_char(ch, "Coins: [%9d], Bank: [%9d] (Total: %d)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

  send_to_char(ch, "Armor: [%d ], Damage: [%2d], Saving throws: [%d/%d/%d]\r\n",
	  GET_ARMOR(k), GET_DAMAGE_MOD(k), GET_SAVE_MOD(k, 0),
          GET_SAVE_MOD(k, 1), GET_SAVE_MOD(k, 2));

  sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
  send_to_char(ch, "Pos: %s, Fighting: %s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");

  if (k->desc) {
    sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
    send_to_char(ch, ", Connected: %s", buf);
  }

  if (IS_NPC(k)) {
    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    send_to_char(ch, ", Default position: %s\r\n", buf);
    sprintbitarray(MOB_FLAGS(k), action_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "NPC flags: @c%s@n\r\n", buf);
  } else {
    send_to_char(ch, ", Idle Timer (in tics) [%d]\r\n", k->timer);

    sprintbitarray(PLR_FLAGS(k), player_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "PLR: @c%s@n\r\n", buf);

    sprintbitarray(PRF_FLAGS(k), preference_bits, PR_ARRAY_MAX, buf);
    send_to_char(ch, "PRF: @g%s@n\r\n", buf);
  }

  if (IS_MOB(k)) {
    send_to_char(ch, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(ch, "Average damage per round %.1f (%.1f [BHD] + %d [STR MOD] + %d [DMG MOD])\r\n", (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice) + ability_mod_value(GET_STR(k)) + GET_DAMAGE_MOD(k), (((k->mob_specials.damsizedice) + 1) / 2.0) * (k->mob_specials.damnodice), ability_mod_value(GET_STR(k)), GET_DAMAGE_MOD(k));
  }

  int counts = 0, total = 0;
  for (i = 0, j = k->carrying; j; j = j->next_content, i++) {
   counts += check_insidebag(j, 0.5);
   counts++;
  }
  total = counts;
  total += i;
  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i)) {
      i2++;
      total += check_insidebag(GET_EQ(k, i), 0.5) + 1;
    }
  send_to_char(ch, "Carried: weight: %d, Total Items (includes bagged items): %d, EQ: %d\r\n", IS_CARRYING_W(k), total, i2);

  

  if (!IS_NPC(k))
    send_to_char(ch, "Hunger: %d, Thirst: %d, Drunk: %d\r\n", GET_COND(k, HUNGER), GET_COND(k, THIRST), GET_COND(k, DRUNK));

  column = send_to_char(ch, "Master is: %s, Followers are:", k->master ? GET_NAME(k->master) : "<none>");
  if (!k->followers)
    send_to_char(ch, " <none>\r\n");
  else {
    for (fol = k->followers; fol; fol = fol->next) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
      if (column >= 62) {
        send_to_char(ch, "%s\r\n", fol->next ? "," : "");
        found = FALSE;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(ch, "\r\n");
  }

  if (SITS(k)) {
   chair = SITS(k);
   send_to_char(ch, "Is on: %s@n\r\n", chair->short_description);
  }

  /* Showing the bitvector */
  sprintbitarray(AFF_FLAGS(k), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "AFF: @y%s@n\r\n", buf);

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      send_to_char(ch, "SPL: (%3dhr) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

      if (aff->modifier)
	send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
	if (aff->modifier)
	  send_to_char(ch, ", ");

	strcpy(buf, affected_bits[aff->bitvector]);
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }

  /* Routine to show what spells a char is affectedv by */
  if (k->affectedv) {
    for (aff = k->affectedv; aff; aff = aff->next) {
      send_to_char(ch, "SPL: (%3d rounds) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

      if (aff->modifier)
        send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
        if (aff->modifier)
          send_to_char(ch, ", ");

        strcpy(buf, affected_bits[aff->bitvector]);
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }

 /* check mobiles for a script */
 if (IS_NPC(k)) {
   do_sstat_character(ch, k);
   if (SCRIPT_MEM(k)) {
     struct script_memory *mem = SCRIPT_MEM(k);
     send_to_char(ch, "Script memory:\r\n  Remember             Command\r\n");
     while (mem) {
       struct char_data *mc = find_char(mem->id);
       if (!mc)
         send_to_char(ch, "  ** Corrupted!\r\n");
       else {
         if (mem->cmd)
           send_to_char(ch, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
         else
           send_to_char(ch, "  %-20.20s <default>\r\n",GET_NAME(mc));
       }
     mem = mem->next;
     }
   }
 } else {
  int x, track = 0;

  send_to_char(ch, "Bonuses/Negatives:\r\n");

  for (x = 0; x < 30; x++) {
   if (x < 15) {
    if (GET_BONUS(k, x) > 0) {
     send_to_char(ch, "@c%s@n\n", list_bonus[x]);
     track += 1;
    }
   } else {
    if (GET_BONUS(k, x) > 0) {
     send_to_char(ch, "@r%s@n\n", list_bonus[x]);
     track += 1;
    }
   }
  }
  if (track <= 0) {
   send_to_char(ch, "@wNone.@n\r\n");
  }
  send_to_char(ch, "To see player variables use varstat now.\r\n");
 }
}

ACMD(do_varstat)
{
 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg); 

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
  send_to_char(ch, "That player is not in the game.\r\n");
  return;
 } else if (IS_NPC(vict)) {
  send_to_char(ch, "Just use stat for an NPC\r\n");
  return;
 } else {
   /* Display their global variables */
   if (vict->script && vict->script->global_vars) {
     struct trig_var_data *tv;
     char uname[MAX_INPUT_LENGTH];
     void find_uid_name(char *uid, char *name, size_t nlen);

     send_to_char(ch, "%s's Global Variables:\r\n", GET_NAME(vict));

     /* currently, variable context for players is always 0, so it is */
     /* not displayed here. in the future, this might change */
     for (tv = vict->script->global_vars; tv; tv = tv->next) {
       if (*(tv->value) == UID_CHAR) {
         find_uid_name(tv->value, uname, sizeof(uname));
         send_to_char(ch, "    %10s:  [UID]: %s\r\n", tv->name, uname);
       } else
         send_to_char(ch, "    %10s:  %s\r\n", tv->name, tv->value);
     }
   }
 }

}

ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Stats on who or what or where?\r\n");
    return;
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      if (load_char(buf2, victim) >= 0) {
	char_to_room(victim, 0);
	if (GET_ADMLEVEL(victim) > GET_ADMLEVEL(ch))
	  send_to_char(ch, "Sorry, you can't do that.\r\n");
	else
	  do_stat_character(ch, victim);
	extract_char_final(victim);
      } else {
	send_to_char(ch, "There is no such player.\r\n");
	free_char(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char(ch, "No such object around.\r\n");
    }
  } else if (is_abbrev(buf1, "zone")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which zone?\r\n");
      return;
    } else {
      print_zone(ch, atoi(buf2));
      return;
    }
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}

ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!strcasecmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!strcasecmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!strcasecmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!strcasecmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}

void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen
   *  to be snooping (or snooped) and get advanced/demoted will
   *  not be snooping/snooped someone of a higher/lower level (and
   *  thus, not entitled to be snooping.
   */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
     (GET_ADMLEVEL(ch->desc->snooping->character) >= GET_ADMLEVEL(ch))) {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
     (GET_ADMLEVEL(ch) >= GET_ADMLEVEL(ch->desc->snoop_by->character))) {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

static void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(ch, "You stop snooping.\r\n");
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_ADMLEVEL(tch) >= GET_ADMLEVEL(ch)) {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}

ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if (!(IS_NPC(victim) || ADM_FLAGGED(ch, ADM_SWITCHMORTAL)))
    send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (GET_ADMLEVEL(ch) < ADMLVL_VICE && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else {
    send_to_char(ch, "%s", CONFIG_OK);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char(ch, "You return to your original body.\r\n");

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], buf3[MAX_INPUT_LENGTH];
  int i=0, n=1;

  one_argument(two_arguments(argument, buf, buf2), buf3);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: load { obj | mob } <vnum> (amt)\r\n");
    return;
  }
  if (!is_number(buf2) || !is_number(buf3)) {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }

  if (atoi(buf3) > 0 ) {
    if (atoi(buf3) >= 100) {
     n = 100;
    }
    else if (atoi(buf3) < 100) {
     n = atoi(buf3);
    }
  } else {
    n = 1;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob=NULL;
    mob_rnum r_num;

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, IN_ROOM(ch));

      act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
    }
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    for (i=0; i < n; i++) {
    obj = read_object(r_num, REAL);
    add_unique_id(obj);
    if (GET_ADMLEVEL(ch) > 0) {
     send_to_imm("LOAD: %s has loaded a %s", GET_NAME(ch), obj->short_description);
     log_imm_action("LOAD: %s has loaded a %s", GET_NAME(ch), obj->short_description);
    }
    if (CONFIG_LOAD_INVENTORY)
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, IN_ROOM(ch));
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    load_otrigger(obj);
    }
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: vstat { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  one_argument(argument, buf);

  /* argument supplied. destroy single object or char */
  if (*buf) {
    if ((vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)) != NULL) {
      if (!IS_NPC(vict) && (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))) {
	send_to_char(ch, "Fuuuuuuuuu!\r\n");
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);
      if (!IS_NPC(vict)) {
       send_to_all("@R%s@r purges @R%s's@r sorry ass right off the MUD!@n\r\n", GET_NAME(ch), GET_NAME(vict));
      }


      if (!IS_NPC(vict)) {
	mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
        log_imm_action("PURGED: %s burned %s's sorry ass off the MUD!", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", CONFIG_OK);
  } else {			/* no argument. clean out the room */
    int i;

    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
      if (!IS_NPC(vict))
        continue;
		
	  delete_inv_backup(vict);
	  
      /* Dump inventory. */
      while (vict->carrying)
        extract_obj(vict->carrying);

      /* Dump equipment. */
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(vict, i))
          extract_obj(GET_EQ(vict, i));

      /* Dump character. */
      extract_char(vict);
    }

    /* Clear the ground. */
    while (world[IN_ROOM(ch)].contents)
      extract_obj(world[IN_ROOM(ch)].contents);
  }
}

static const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "Your syslog is currently %s.\r\n",
	logtypes[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
    return;
  }
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
  if (tp & 1) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
  if (tp & 2) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);

  send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}

#define EXE_FILE "bin/circle" /* maybe use argv[0] but it's not reliable */

/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover) 
{ 
#ifdef CIRCLE_WINDOWS 
  send_to_char(ch, "Copyover is not available for Windows.\r\n"); 
#else 
  char arg[MAX_INPUT_LENGTH]; 
  int secs; 

  one_argument(argument, arg); 
  if (!*arg) { 
   execute_copyover();
  } else if (is_abbrev(arg, "cancel") || is_abbrev(arg, "stop")) { 
    if (!copyover_timer) { 
      send_to_char(ch, "A timed copyover has not been started!\r\n"); 
   } else { 
      copyover_timer=0; 
      game_info("Copyover cancelled"); 
    } 
  } else if (is_abbrev(arg, "help")) { 
      send_to_char(ch, "COPYOVER\r\n\r\n"); 
      send_to_char(ch, "Usage: @ycopyover@n           - Perform an immediate copyover\r\n" 
                       "       @ycopyover <seconds>@n - Start a timed copyover\r\n" 
                       "       @ycopyover cancel@n    - Stop a timed copyover\r\n\r\n"); 
      send_to_char(ch, "A timed copyover will produce an automatic warning when it starts, and then\r\n"); 
      send_to_char(ch, "every minute.  During the last minute, there will be a warning every 15 seconds.\r\n"); 
  } else { 
    secs = atoi(arg); 
    if (!secs || secs < 0) { 
      send_to_char(ch, "Type @ycopyover help@n for usage info."); 
    } else { 
      copyover_timer = secs; 
      log("-- Timed Copyover started by %s - %d seconds until copyover --", GET_NAME(ch), secs); 
      if (secs >= 60) { 
        if (secs%60) { 
          game_info("A copyover will be performed in %d minutes and %d seconds.", (copyover_timer/60), (copyover_timer%60)); 
        } else { 
          game_info("A copyover will be performed in %d minute%s.", (copyover_timer/60), (copyover_timer/60) > 1 ? "s" : ""); 
        } 
      } else { 
        game_info("A copyover will be performed in %d seconds.", copyover_timer); 
      } 
    } 
  } 
#endif 
} 

static void execute_copyover(void)
{
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf [100], buf2[100];

  fp = fopen (COPYOVER_FILE, "w");

  if (!fp) {
    send_to_imm("Copyover file not writeable, aborted.\n\r");
    return;
  }

  /* Consider changing all saved areas here, if you use OLC */
  save_all();
  save_mud_time(&time_info);
  sprintf (buf, "\t\x1B[1;31m \007\007\007The universe stops for a moment as space and time fold.\x1B[0;0m\r\n");
  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d ; d = d_next) {
    struct char_data * och = d->character;
    d_next = d->next; /* We delete from the list , so need to save this */
    if (!d->character || d->connected > CON_PLAYING) {
      write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r", d->comp);
      close (d); /* throw'em out */
    } else {
      if (GET_ROOM_VNUM(IN_ROOM(och)) > 1) {
       fprintf (fp, "%d %s %s %d %s\n", d->descriptor, GET_NAME(och), d->host, GET_ROOM_VNUM(IN_ROOM(och)), d->user);
      } else if (GET_ROOM_VNUM(IN_ROOM(och)) <= 1 && GET_ROOM_VNUM(GET_WAS_IN(och)) > 1) {
       fprintf (fp, "%d %s %s %d %s\n", d->descriptor, GET_NAME(och), d->host, GET_ROOM_VNUM(GET_WAS_IN(och)), d->user);
      } else {
       fprintf (fp, "%d %s %s 300 %s\n", d->descriptor, GET_NAME(och), d->host, d->user);
      }
      log("printing descriptor name and host of connected players");
      /* save och */
      Crash_rentsave(och, 0);
      save_char(och);
        if (d->comp->state == 2) {
            d->comp->state = 3; /* Code to use Z_FINISH for deflate */
        }
      write_to_descriptor (d->descriptor, buf, d->comp);
      d->comp->state = 0;
        if (d->comp->stream) {
            deflateEnd(d->comp->stream);
            free(d->comp->stream);
            free(d->comp->buff_out);
            free(d->comp->buff_in);
        }
    }
  }

  fprintf (fp, "-1\n");
  fclose (fp);

  /* Close reserve and other always-open files and release other resources
     since we are now using ASCII pfiles, closing the player_fl would crash
     the game, since it's no longer around, so I commented it out. I'll
     leave the code here, for historical reasons -spl
     fclose(player_fl); */

  /* exec - descriptors are inherited */

  sprintf (buf, "%d", port);
  sprintf (buf2, "-C%d", mother_desc);
  chdir ("..");
  execl (EXE_FILE, "circle", buf2, buf, (char *) NULL);
  /* Failed - sucessful exec will not return */

  perror ("do_copyover: execl");
  send_to_imm("Copyover FAILED!\n\r");

  exit (1); /* too much trouble to try to recover! */
}

void copyover_check(void) 
{ 
  if (!copyover_timer) return; 
  copyover_timer--; 
  if (!copyover_timer) {
   execute_copyover();
  }
  if (copyover_timer > 59) { 
    if (copyover_timer%60 == 0) { 
      game_info("A copyover will be performed in %d minute%s.", (copyover_timer/60), (copyover_timer/60) > 1 ? "s" : ""); 
    } 
  }
    else {
    if (copyover_timer%10 == 0 && copyover_timer > 29) {
      game_info("A copyover will be performed in %d seconds.", (copyover_timer));
    }
    if (copyover_timer%5 == 0 && copyover_timer <= 29) { 
      game_info("A copyover will be performed in %d seconds.", (copyover_timer)); 
    } 
  } 
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level) {
    send_to_char(ch, "[ 1 - 100 | demote ]\r\n");
    return;
  }
  else if ((newlevel = atoi(level)) <= 0) {
   if (!strcasecmp("demote", level)) {
   victim->level = 1;
   GET_MAX_HIT(victim) = 150;
   GET_MAX_MANA(victim) = 150;
   GET_MAX_MOVE(victim) = 150;
   GET_BASE_PL(victim) = 150;
   GET_BASE_KI(victim) = 150;
   GET_BASE_ST(victim) = 150;
   send_to_char(ch, "They have now been demoted!\r\n");
   send_to_char(victim, "You were demoted to level 1!\r\n");
   return;
   }
   else {
    send_to_char(ch, "That's not a level!\r\n");
    return;
   }
  }
  if (newlevel > 100) {
    send_to_char(ch, "100 is the highest possible level.\r\n");
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    send_to_char(ch, "You cannot demote a player.\r\n");
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you, like a giant hand, light comes down\r\n"
	"from above, grabbing your body, which begins to pulse with colored\r\n"
        "lights from inside.\r\n\r\n"
	"Your head seems to be filled with demons from another plane as your\r\n"
        "body dissolves to the elements of time and space itself.\r\n\r\n"
	"Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp_regardless(victim,
	 level_exp(victim, newlevel) - GET_EXP(victim));
  save_char(victim);
}

ACMD(do_handout)
{
  struct descriptor_data *j;

 if (GET_ADMLEVEL(ch) < 3) {
  send_to_char(ch, "You can't do that.\r\n");
  return;
 }

  for (j = descriptor_list; j; j = j->next) {
    if (!IS_PLAYING(j) || ch == j->character || GET_ADMLEVEL(j->character) > 0)
     continue;
    if (IS_NPC(j->character))
     continue;
    else {
     GET_PRACTICES(j->character, GET_CLASS(j->character)) += 10;
    }
  }
  
  send_to_all("@g%s@G hands out 10 practice sessions to everyone!@n\r\n", GET_NAME(ch));
  log("%s gave a handout of 10 PS to everyone.", GET_NAME(ch));
  log_imm_action("HANDOUT: %s has handed out 10 PS to everyone.", GET_NAME(ch));
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct descriptor_data *j;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
  else if (is_abbrev(buf, "all")) 
   { 
     send_to_imm("[Log: %s restored all.]", GET_NAME(ch));
     log_imm_action("RESTORE: %s has restored all players.", GET_NAME(ch));
     for (j = descriptor_list; j; j = j->next) 
    { 
      if (!IS_PLAYING(j) || !(vict = j->character)) 
       continue; 
      if (GET_HIT(vict) < gear_pl(vict)) { 
       GET_HIT(vict)  = gear_pl(vict); 
      }
      GET_MANA(vict) = GET_MAX_MANA(vict); 
      GET_MOVE(vict) = GET_MAX_MOVE(vict); 
 
      update_pos(vict); 
      act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR | TO_SLEEP); 
      if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) > (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict))) {
       GET_HIT(vict) = (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict));
       send_to_char(vict, "@mYou are healed to your suppression limit.@n\r\n");
      }
    } 
    send_to_char(ch, "Okay.\r\n");
  }
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
    send_to_char(ch, "They don't need your help.\r\n");
  else {
    GET_HIT(vict) = gear_pl(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);
    GET_KI(vict) = GET_MAX_KI(vict);
    REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_BLIND);
    GET_LIMBCOND(vict, 1) = 100;
    GET_LIMBCOND(vict, 2) = 100;
    GET_LIMBCOND(vict, 3) = 100;
    GET_LIMBCOND(vict, 4) = 100;
    SET_BIT_AR(PLR_FLAGS(vict), PLR_HEAD);
    if (!IS_NPC(vict) && GET_ADMLEVEL(ch) >= ADMLVL_VICE) {
      if (GET_ADMLEVEL(vict) >= ADMLVL_IMMORT)
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(vict, i, 100);
      
      if (GET_ADMLEVEL(vict) >= ADMLVL_GRGOD) {
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      }
    }
    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "%s", CONFIG_OK);
    send_to_imm("[Log: %s restored %s.]", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("RESTORE: %s has restored %s.", GET_NAME(ch), GET_NAME(vict));
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
      if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) > (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict))) {
       GET_HIT(vict) = (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict));
       send_to_char(vict, "@mYou are healed to your suppression limit.@n\r\n");
      }
  }
}

void perform_immort_vis(struct char_data *ch)
{
  GET_INVIS_LEV(ch) = 0;
}

static void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_ADMLEVEL(tch) >= GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_ADMLEVEL(tch) < GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_ADMLEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_ADMLEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}

ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
	send_to_char(pt->character, "%s\r\n", argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}

ACMD(do_ginfo)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
        send_to_char(pt->character, "@D[@GINFO@D] @g%s@n\r\n", argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "@D[@GINFO@D] @g%s@n\r\n", argument);
  }
}

ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = strdup(argument);

  send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_ADMLEVEL(d->character) >= GET_ADMLEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char(ch, "They're already being disconnected.\r\n");
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}

ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > 101) {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  if (*arg) {
  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    send_to_all("@RWIZLOCK@D: @WThe game has been completely opened by @C%s@W.@n", GET_NAME(ch));
    log("WIZLOCK: The game has been completely opened by %s.", GET_NAME(ch));
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    send_to_all("@RWIZLOCK@D: @WThe game is %s closed to new players by @C%s@W.@n", when, GET_NAME(ch));
    log("WIZLOCK: The game is %s closed to new players by %s.", when, GET_NAME(ch));
    break;
  case 101:
    send_to_char(ch, "The game is %s closed to non-imms.\r\n", when);
    send_to_all("@RWIZLOCK@D: @WThe game is %s closed to non-imms by @C%s@W.@n", when, GET_NAME(ch));
    log("WIZLOCK: The game is %s closed to non-imms by %s.", when, GET_NAME(ch));
    break;
  default:
    send_to_char(ch, "Only level %d+ may enter the game %s.\r\n", circle_restrict, when);
    send_to_all("@RWIZLOCK@D: @WLevel %d+ only can enter the game %s, thanks to @C%s@W.@n", circle_restrict, when, GET_NAME(ch));
    log("WIZLOCK: Level %d+ only can enter the game %s, thanks to %s.", circle_restrict, when, GET_NAME(ch));
    break;
  }
 }
  if (!*arg) {
  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
 }
}

ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}

ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "For whom do you wish to search?\r\n");
    return;
  }
  CREATE(vict, struct char_data, 1);
  clear_char(vict);
  CREATE(vict->player_specials, struct player_special_data, 1);
  if (load_char(arg, vict) <  0) {
    send_to_char(ch, "There is no such player.\r\n");
    free_char(vict);
    return;
  }
  if ((GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) && (GET_ADMLEVEL(ch) < ADMLVL_IMPL)) {
    send_to_char(ch, "You are not sufficiently godly for that!\r\n");
    return;
  }

  send_to_char(ch, "[%5d] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
    GET_IDNUM(vict), (int) GET_LEVEL(vict),
    race_abbrevs[(int) GET_RACE(vict)], class_abbrevs[(int) GET_CLASS(vict)],
    GET_NAME(vict), vict->player_specials->host && *vict->player_specials->host
    ? vict->player_specials->host : "(NOHOST)",
    ctime(&vict->time.logon));
  free_char(vict);
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

  half_chop(argument, arg, to_force);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if (!ADM_FLAGGED(ch, ADM_FORCEMASS) || (strcasecmp("all", arg) && strcasecmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))
      send_to_char(ch, "No, no, no!\r\n");
    else {
      send_to_char(ch, "%s", CONFIG_OK);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!strcasecmp("room", arg)) {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}

ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32], *msg;
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = ADMLVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), ADMLVL_IMMORT);
      if (level > GET_ADMLEVEL(ch)) {
	send_to_char(ch, "You can't wizline above your own level.\r\n");
	return;
      }
    } else if (emote)
      argument++;
    break;

  case '@':
    send_to_char(ch, "God channel status:\r\n");
    for (any = 0, d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(d->character) < ADMLVL_IMMORT)
        continue;
      if (!CAN_SEE(ch, d->character))
        continue;

      send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
		PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
		PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
		PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
    }
    return;

  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char(ch, "You are offline!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the gods like that!\r\n");
    return;
  }
  if (level > ADMLVL_IMMORT) {
    snprintf(buf1, sizeof(buf1), "@c%s@D: <@C%d@D> @G%s%s@n\r\n", GET_NAME(ch), level, emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: <@C%d@D> @G%s%s@n\r\n", level, emote ? "<--- " : "", argument);
  } else {
    snprintf(buf1, sizeof(buf1), "@c%s@D: @G%s%s@n\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone@D: @G%s%s@n\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if (IS_PLAYING(d) && (GET_ADMLEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if (CAN_SEE(d->character, ch)) {
        msg = strdup(buf1);
	send_to_char(d->character, "%s", buf1);
      } else {
        msg = strdup(buf2);
	send_to_char(d->character, "%s", buf2);
    }
      add_history(d->character, msg, HIST_WIZNET);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);

  if (*arg == '*') {
    if (GET_ADMLEVEL(ch) < ADMLVL_VICE) {
       send_to_char(ch, "You do not have permission to reset the entire world.\r\n");
       return;
    } else {
    for (i = 0; i <= top_of_zone_table; i++) {
     if (i < 200) {
      reset_zone(i);
     }
    }
    send_to_char(ch, "Reset world.\r\n");
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset all MUD zones.", GET_NAME(ch));
    log_imm_action("RESET: %s has reset all MUD zones.", GET_NAME(ch));
    return; 
     }
  } else if (*arg == '.' || !*arg)
    i = world[IN_ROOM(ch)].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i <= top_of_zone_table && (can_edit_zone(ch, i) || GET_ADMLEVEL(ch) > ADMLVL_IMMORT)) {
    reset_zone(i);
    send_to_char(ch, "Reset zone #%d: %s.\r\n", zone_table[i].number, zone_table[i].name);
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset zone %d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
    log_imm_action("RESET: %s has reset zone #%d: %s.", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
  } else
    send_to_char(ch, "You do not have permission to reset this zone. Try %d.\r\n", GET_OLC_ZONE(ch));
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char(ch, "Rerolling is not possible at this time, bug Iovan about it...\r\n");
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) &&  !PLR_FLAGGED(vict, PLR_KILLER)) {
	send_to_char(ch, "Your victim is not flagged.\r\n");
	return;
      }
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THIEF);
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_KILLER);
      send_to_char(ch, "Pardoned.\r\n");
      send_to_char(vict, "You have been pardoned by the Gods!\r\n");
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Squelch %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Mute turned %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_all("@D[@RMUTE@D] @C%s@W has had mute turned @r%s@W by @C%s@W.\r\n", GET_NAME(vict), ONOFF(result), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
	return;
      }
      if (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict)) {
        send_to_char(ch, "Pfft...\r\n");
        return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Your victim is already pretty cold.\r\n");
	return;
      }
      SET_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_ADMLEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_ADMLEVEL(ch)) {
	send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
		GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	return;
      }
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected || AFF_FLAGS(vict) || vict->affectedv) {
        while (vict->affected)
          affect_remove(vict, vict->affected);
          for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
          AFF_FLAGS(ch)[taeller] = 0;
        while (vict->affectedv)
          affectv_remove(vict, vict->affectedv);
          for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
          AFF_FLAGS(ch)[taeller] = 0;
	send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
	send_to_char(ch, "All spells removed.\r\n");
      } else {
	send_to_char(ch, "Your victim does not have any affections!\r\n");
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but this
       *  function handles 'reroll', 'pardon', 'freeze', etc.
       */
      break;
    }
    save_char(vict);
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
static size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;
  
  if (listall) {
    int i, j, k, l, m, n, o;
    extern int top_of_trigt;
    extern struct index_data **trig_index;
    int count_shops(shop_vnum low, shop_vnum high);
    int count_guilds(guild_vnum low, guild_vnum high);

    tmp = snprintf(bufptr, left,
	"%3d %-30.30s By: %-10.10s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name, zone_table[zone].builders,
	zone_table[zone].age, zone_table[zone].lifespan,
	zone_table[zone].reset_mode,
	zone_table[zone].bot, zone_table[zone].top);
        i = j = k = l = m = n = o = 0;
        
        for (i = 0; i < top_of_world; i++)
          if (world[i].number >= zone_table[zone].bot && world[i].number <= zone_table[zone].top)
            j++;

        for (i = 0; i < top_of_objt; i++)
          if (obj_index[i].vnum >= zone_table[zone].bot && obj_index[i].vnum <= zone_table[zone].top)
            k++;
        
        for (i = 0; i < top_of_mobt; i++)
          if (mob_index[i].vnum >= zone_table[zone].bot && mob_index[i].vnum <= zone_table[zone].top)
            l++;

        m = count_shops(zone_table[zone].bot, zone_table[zone].top);

        for (i = 0; i < top_of_trigt; i++)
          if (trig_index[i]->vnum >= zone_table[zone].bot && trig_index[i]->vnum <= zone_table[zone].top)
            n++;

        o = count_guilds(zone_table[zone].bot, zone_table[zone].top);

	tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Rooms:    %2d\r\n"
                        "         Objects:  %2d\r\n"
                        "         Mobiles:  %2d\r\n"
                        "         Shops:    %2d\r\n"
                        "         Triggers: %2d\r\n"
                        "         Guilds:   %2d\r\n", 
                          j, k, l, m, n, o);
        
    return tmp;
  } 
  
  return snprintf(bufptr, left,
	"%3d %-*s By: %-10.10s Range: %5d-%5d\r\n", zone_table[zone].number, 
        count_color_chars(zone_table[zone].name)+30, zone_table[zone].name, 
        zone_table[zone].builders, zone_table[zone].bot, zone_table[zone].top);
}

ACMD(do_show)
{
  int i, j, k, l, con;                /* i, j, k to specifics? */
  size_t len, nlen;
  zone_rnum zrn;
  zone_vnum zvn;
  int low, high;
  int8_t self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj;
  struct descriptor_data *d;
  struct affected_type *aff;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], *strp,
	arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  extern int top_of_trigt;

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		ADMLVL_IMMORT },		/* 1 */
    { "player",		ADMLVL_GOD },
    { "rent",		ADMLVL_GRGOD },
    { "stats",		ADMLVL_IMMORT },
    { "errors",		ADMLVL_IMPL },			/* 5 */
    { "death",		ADMLVL_GOD },
    { "godrooms",	ADMLVL_IMMORT },
    { "shops",		ADMLVL_IMMORT },
    { "houses",		ADMLVL_GOD },
    { "snoop",		ADMLVL_GRGOD },			/* 10 */
    { "assemblies",     ADMLVL_IMMORT },
    { "guilds",         ADMLVL_GOD },
    { "levels",         ADMLVL_GRGOD },
    { "uniques",        ADMLVL_GRGOD },
    { "affect",         ADMLVL_GRGOD },			/* 15 */
    { "affectv",        ADMLVL_GRGOD },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Game Info options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_ADMLEVEL(ch))
	send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));	/* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_ADMLEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l) {
  /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone, 1);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, sizeof(buf), zrn, 1);
      else {
	send_to_char(ch, "That is not a valid zone.\r\n");
	return;
      }
    } else
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++) {
	nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn, 0);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show player */
  case 2:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    if (load_char(value, vict) < 0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }
    send_to_char(ch, "Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict),
      genders[(int) GET_SEX(vict)], GET_LEVEL(vict), class_abbrevs[(int)
      GET_CLASS(vict)], race_abbrevs[(int) GET_RACE(vict)]);
    send_to_char(ch, "Au: %-8d  Bal: %-8d  Exp: %"I64T"  Align: %-5d  Ethic: %-5d\r\n",
                 GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
                 GET_ALIGNMENT(vict), GET_ETHIC_ALIGNMENT(vict));
    if (CONFIG_ALLOW_MULTICLASS)
      send_to_char(ch, "Class ranks: %s\r\n", class_desc_str(vict, 1, 0));

    /* ctime() uses static buffer: do not combine. */
    send_to_char(ch, "Started: %-20.16s  ", ctime(&vict->time.created));
    send_to_char(ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
      ctime(&vict->time.logon),
      (int) (vict->time.played / 3600),
      (int) (vict->time.played / 60 % 60));
    free_char(vict);
    break;

  /* show rent */
  case 3:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(ch, value);
    break;

  /* show stats */
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    send_to_char(ch,
	"             @D---   @CCore Stats   @D---\r\n"
	"  @Y%5d@W players in game  @y%5d@W connected\r\n"
	"  @Y%5d@W registered\r\n"
	"  @Y%5d@W mobiles          @y%5d@W prototypes\r\n"
	"  @Y%5d@W objects          @y%5d@W prototypes\r\n"
	"  @Y%5d@W rooms            @y%5d@W zones\r\n"
        "  @Y%5d@W triggers\r\n"
	"  @Y%5d@W large bufs\r\n"
	"  @Y%5d@W buf switches     @y%5d@W overflows\r\n"
        "             @D--- @CMiscellaneous  @D---\r\n"
        "  @Y%5s@W Mob ki attacks this boot\r\n"
        "  @Y%5s@W Asssassins Generated@n\r\n"
        "  @Y%5d@W Wish Selfishness Meter@n\r\n",
	i, con,
	top_of_p_table + 1,
	j, top_of_mobt + 1,
	k, top_of_objt + 1,
	top_of_world + 1, top_of_zone_table + 1,
	top_of_trigt + 1,
	buf_largecount,
	buf_switches, buf_overflows,
        add_commas(mob_specials_used),
        add_commas(number_of_assassins),
        SELFISHMETER
	);
    break;

  /* show errors */
  case 5:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++) {
      	if (!W_EXIT(i,j))
      	  continue;
        if (W_EXIT(i,j)->to_room == 0) {
            nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+40, world[i].name, QNRM, dirs[j]);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
        if (W_EXIT(i,j)->to_room == NOWHERE && !W_EXIT(i,j)->general_description) {
            nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-*s%s (%s)\r\n", ++k, GET_ROOM_VNUM(i), count_color_chars(world[i].name)+ 40, world[i].name, QNRM, dirs[j]);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show death */
  case 6:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show godrooms */
  case 7:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show shops */
  case 8:
    show_shops(ch, value);
    break;

  /* show houses */
  case 9:
    hcontrol_list_houses(ch);
    break;

  /* show snoop */
  case 10:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(ch) < GET_ADMLEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      i++;
      send_to_char(ch, "%-10s - snooped by %s.\r\n", GET_NAME(d->snooping->character), GET_NAME(d->character));
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;
  /* show assembly */
  case 11:
    assemblyListToChar(ch);
    break;
  /* show guilds */
  case 12:
    show_guild(ch, value);
    break;

  /* show level tables */
  case 13:
    send_to_char(ch, "This is not used currently.\r\n");
    break;

  case 14:
    if (value != NULL && *value) {
      if (sscanf(value, "%d-%d", &low, &high) != 2) {
        if (sscanf(value, "%d", &low) != 1) {
          send_to_char(ch, "Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
          return;
        } else {
          high = low;
        }
      }
    } else {
      low = -1;
      high = 9999999;
    }
    strp = sprintuniques(low, high);
    page_string(ch->desc, strp, TRUE);
    free(strp);
    break;

  case 15:
  case 16:
    if (!*value) {
      low = 1;
      vict = (l == 15) ? affect_list : affectv_list;
    } else {
      low = 0;
      if (! (vict = get_char_world_vis(ch, value, NULL))) {
        send_to_char(ch, "Cannot find that character.\r\n");
        return;
      }
    }
    k = MAX_STRING_LENGTH;
    CREATE(strp, char, k);
    strp[0] = j = 0;
    if (!vict) {
      send_to_char(ch, "None.\r\n");
      return;
    }
    do {
      if ((k - j) < (MAX_INPUT_LENGTH * 8)) {
        k *= 2;
        RECREATE(strp, char, k);
      }
      j += snprintf(strp + j, k - j, "Name: %s\r\n", GET_NAME(vict));
      if (l == 15)
        aff = vict->affected;
      else
        aff = vict->affectedv;
      for (; aff; aff = aff->next) {
        j += snprintf(strp + j, k - j, "SPL: (%3d%s) @c%-21s@n ", aff->duration + 1,
                      (l == 15) ? "hr" : "rd", skill_name(aff->type));

        if (aff->modifier)
          j += snprintf(strp + j, k - j, "%+d to %s", aff->modifier,
                        apply_types[(int) aff->location]);

        if (aff->bitvector) {
          if (aff->modifier)
            j += snprintf(strp + j, k - j, ", ");

          strcpy(field, affected_bits[aff->bitvector]);
          j += snprintf(strp + j, k - j, "sets %s", field);
        }
        j += snprintf(strp + j, k - j, "\r\n");
      }
      if (l == 15)
        vict = vict->next_affect;
      else
        vict = vict->next_affectv;
    } while (low && vict);
    page_string(ch->desc, strp, TRUE);
    free(strp);
    break;

  /* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}

/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT_AR(flagset, flags); \
	else if (off) REMOVE_BIT_AR(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
static const struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "brief",		ADMLVL_GOD, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	ADMLVL_GOD, 	PC, 	BINARY },  /* 1 */
   { "title",		ADMLVL_GOD, 	PC, 	MISC },
   { "nosummon", 	ADMLVL_GRGOD, 	PC, 	BINARY },
   { "maxpl",		ADMLVL_BUILDER, BOTH, 	NUMBER },
   { "maxki",   	ADMLVL_BUILDER, BOTH, 	NUMBER },  /* 5 */
   { "maxst",   	ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "pl", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "ki",		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "sta",		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "align",		ADMLVL_BUILDER, BOTH, 	NUMBER },  /* 10 */
   { "str",		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "stradd",		ADMLVL_IMPL, 	BOTH, 	NUMBER },
   { "int", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "wis", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "dex", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },  /* 15 */
   { "con", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "cha",		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "ac", 		ADMLVL_IMPL, 	BOTH, 	NUMBER },
   { "zenni",		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "bank",		ADMLVL_BUILDER, PC, 	NUMBER },  /* 20 */
   { "exp", 		ADMLVL_BUILDER,	BOTH, 	NUMBER },
   { "accuracy", 	ADMLVL_VICE, 	BOTH, 	NUMBER },
   { "damage", 		ADMLVL_VICE, 	BOTH, 	NUMBER },
   { "invis",		ADMLVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	ADMLVL_VICE, 	PC, 	BINARY },  /* 25 */
   { "frozen",		ADMLVL_FREEZE, 	PC, 	BINARY },
   { "practices", 	ADMLVL_BUILDER,	PC, 	NUMBER },
   { "lessons", 	ADMLVL_IMPL, 	PC, 	NUMBER },
   { "drunk",		ADMLVL_GOD, 	BOTH, 	MISC },
   { "hunger",		ADMLVL_BUILDER, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		ADMLVL_BUILDER, 	BOTH, 	MISC },
   { "killer",		ADMLVL_GOD, 	PC, 	BINARY },
   { "thief",		ADMLVL_GOD, 	PC, 	BINARY },
   { "level",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "room",		ADMLVL_IMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	ADMLVL_VICE, 	PC, 	BINARY },
   { "siteok",		ADMLVL_VICE, 	PC, 	BINARY },
   { "deleted", 	ADMLVL_IMPL, 	PC, 	BINARY },
   { "class",		ADMLVL_VICE, 	BOTH, 	MISC },
   { "nowizlist", 	ADMLVL_GOD, 	PC, 	BINARY },  /* 40 */
   { "quest",		ADMLVL_GOD, 	PC, 	BINARY },
   { "loadroom", 	ADMLVL_VICE, 	PC, 	MISC },
   { "color",		ADMLVL_GOD, 	PC, 	BINARY },
   { "idnum",		ADMLVL_IMPL, 	PC, 	NUMBER },
   { "passwd",		ADMLVL_IMPL, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	ADMLVL_GOD, 	PC, 	BINARY },
   { "sex", 		ADMLVL_VICE, 	BOTH, 	MISC },
   { "age",		ADMLVL_VICE,	BOTH,	NUMBER },
   { "height",		ADMLVL_GOD,	BOTH,	NUMBER },
   { "weight",		ADMLVL_GOD,	BOTH,	NUMBER },  /* 50 */
   { "olc",		ADMLVL_GRGOD,	PC,	MISC },
   { "race",		ADMLVL_VICE,	PC,	MISC },
   { "trains",		ADMLVL_VICE,	PC,	NUMBER },
   { "feats",		ADMLVL_VICE,	PC,	NUMBER },
   { "ethic",		ADMLVL_GOD, 	BOTH, 	NUMBER },  /* 55 */
   { "unused1",	 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "unused2", 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "adminlevel",	ADMLVL_GRGOD,	PC,	NUMBER },
   { "hairl",           ADMLVL_VICE,   PC,     NUMBER },
   { "hairs",           ADMLVL_VICE,   PC,     NUMBER },
   { "hairc",           ADMLVL_VICE,   PC,     NUMBER },
   { "skin",            ADMLVL_VICE,   PC,     NUMBER },
   { "eye",             ADMLVL_VICE,   PC,     NUMBER },
   { "basepl",          ADMLVL_BUILDER,   BOTH,   NUMBER },
   { "baseki",          ADMLVL_BUILDER,   BOTH,   NUMBER },
   { "basest",          ADMLVL_BUILDER,   BOTH,   NUMBER },
   { "droom",           ADMLVL_GRGOD,   BOTH,   NUMBER },
   { "absorbs",         ADMLVL_GRGOD,   BOTH,   NUMBER },
   { "ugp",             ADMLVL_GRGOD,   BOTH,   NUMBER },
   { "aura",            ADMLVL_IMMORT,   BOTH,  NUMBER },
   { "trp",             ADMLVL_GRGOD,   BOTH,   NUMBER },
   { "boost",           ADMLVL_GRGOD,   BOTH,   NUMBER },
   { "multi",           ADMLVL_IMPL,    PC,     BINARY },
   { "deaths",          ADMLVL_BUILDER, BOTH,   NUMBER },
   { "user",            ADMLVL_IMPL,    PC,     MISC },
   { "phase",           ADMLVL_IMMORT,  PC,     NUMBER },
   { "racial",          ADMLVL_IMMORT,  PC,     NUMBER },
   { "slots",           ADMLVL_IMMORT,  PC,     NUMBER },
   { "feature",         ADMLVL_IMMORT,  PC,     BINARY },
   { "tclass",          ADMLVL_VICE,  PC,     NUMBER },
   { "clones",          ADMLVL_IMPL,   PC,     NUMBER },
   { "\n", 0, BOTH, MISC }
  };

static int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, on = 0, off = 0;
  int64_t value = 0;
  room_rnum rnum;
  room_vnum rvnum;

  /* Check to make sure all the levels are correct */
  if (GET_ADMLEVEL(ch) != ADMLVL_IMPL) {
    if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict) && vict != ch) {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_ADMLEVEL(ch) < set_fields[mode].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    char *ptr;

    value = strtoll(val_arg, &ptr, 10);
  /*    value = atoi(val_arg); */
    send_to_char(ch, "%s's %s set to %"I64T".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  } else
    send_to_char(ch, "%s", CONFIG_OK);

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->max_hit = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set maxpl for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set maxpl for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 5:
    vict->max_mana = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set maxki for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set maxki for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 6:
    vict->max_move = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set maxsta for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set maxsta for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 7:
    vict->hit = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set pl for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set pl for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 8:
    vict->mana = value;
    affect_total(vict);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set ki for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set ki for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 9:
    vict->move = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set st for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set st for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set align for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set align for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 11:
    RANGE(0, 100);
    vict->real_abils.str = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set str for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set str for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 12:
    send_to_char(ch, "Setting str_add does nothing now.\r\n");
    /* vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
       break; */
  case 13:
    RANGE(0, 100);
    vict->real_abils.intel = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set intel for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set intel for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 14:
    RANGE(0, 100);
    vict->real_abils.wis = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set wis for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set wis for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 15:
    RANGE(0, 100);
    vict->real_abils.dex = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set dex for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set dex for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 16:
    RANGE(0, 100);
    vict->real_abils.con = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set con for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set con for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 17:
    RANGE(0, 100);
    vict->real_abils.cha = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set speed for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set speed for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 18:
    vict->armor = RANGE(-100, 500);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set armor index for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set armor index for %s.", GET_NAME(ch), GET_NAME(vict));
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set zenni for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set zenni for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set bank for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set bank for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 21:
    vict->exp = RANGE(0, 50000000);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set exp for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set exp for %s.", GET_NAME(ch), GET_NAME(vict));
    break;
  case 22:
    send_to_char(ch, "This does nothing at the moment.\r\n");
    break;
  case 23:
    vict->damage_mod = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_ADMLEVEL(vict));
    break;
  case 25:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on) {
      send_to_char(ch, "Better not -- could be a long winter!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    if (GET_CLASS_LEVEL(vict)) {
      GET_PRACTICES(vict, GET_CLASS(vict)) = RANGE(0, 10000);
     mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set PS for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set PS for %s.", GET_NAME(ch), GET_NAME(vict));
    }
    else {
      GET_RACE_PRACTICES(vict) = RANGE(0, 10000);
      mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set PS for %s.", GET_NAME(ch), GET_NAME(vict));
     log_imm_action("SET: %s has set PS for %s.", GET_NAME(ch), GET_NAME(vict));
    }
    break;
  case 29:
  case 30:
  case 31:
    if (!strcasecmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = -1; /* warning: magic number here */
      send_to_char(ch, "%s's %s now off.\r\n", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = value; /* and here too */
      send_to_char(ch, "%s's %s set to %"I64T".\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    } else {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (!IS_NPC(vict) && value > 100) {
      send_to_char(ch, "You can't do that.\r\n");
      return (0);
    }
    value = MAX(0, value);
    vict->level = value;
    break;
  case 35:
    if ((rnum = real_room(value)) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, rnum);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = search_block(val_arg, class_names, FALSE)) < 0) {
      send_to_char(ch, "That is not a class.\r\n");
      return (0);
    }
    value = GET_CLASS_RANKS(vict, GET_CLASS(vict));
    GET_CLASS(vict) = i;
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!strcasecmp(val_arg, "off")) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
      GET_LOADROOM(vict) = NOWHERE;
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
        SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = rvnum;
	send_to_char(ch, "%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
      } else {
	send_to_char(ch, "That room does not exist!\r\n");
	return (0);
      }
    } else {
      send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
      return (0);
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR);
    break;
  case 44:
    if (GET_IDNUM(ch) == 0 || IS_NPC(vict))
      return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (GET_IDNUM(ch) > 1) {
      send_to_char(ch, "Please don't use this command, yet.\r\n");
      return (0);
    }
    if (GET_ADMLEVEL(ch) < 10) {
      send_to_char(ch, "NO.\r\n");
      return (0);
    }
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0) {
      send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 20000) {	/* Arbitrary limits. */
      send_to_char(ch, "Ages 2 to 20000 accepted.\r\n");
      return (0);
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->time.birth = time(0) - (value * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
      GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
    else if (is_abbrev(val_arg, "hedit"))
      GET_OLC_ZONE(vict) = HEDIT_PERMISSION;
    else if (is_abbrev(val_arg, "off"))
      GET_OLC_ZONE(vict) = NOWHERE;
    else if (!is_number(val_arg)) {
      send_to_char(ch, "Value must be either 'socials', 'actions', 'hedit', 'off' or a zone number.\r\n");
      return (0);
    } else
      GET_OLC_ZONE(vict) = atoi(val_arg);
    break;

  case 52:
    if ((i = search_block(val_arg, race_names, FALSE )) < 0) {
      send_to_char(ch, "That is not a race.\r\n");
      return (0);
    }
    GET_RACE(vict) = i;
    racial_body_parts(vict);
    break;

  case 53:
    GET_TRAINS(vict) = RANGE(0, 500);
    break;

  case 54:
    GET_FEAT_POINTS(vict) = RANGE(0, 500);
    break;

  case 55:
    GET_ETHIC_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;

  case 56:
    vict->max_ki = RANGE(1, 5000);
    affect_total(vict);
    break;

  case 57:
    vict->ki = RANGE(0, vict->max_ki);
    affect_total(vict);
    break;

  case 58:
    if (GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch) && vict != ch) {
      send_to_char(ch, "Permission denied.\r\n");
      return (0);
    }
    if (value < ADMLVL_NONE || value > GET_ADMLEVEL(ch)) {
      send_to_char(ch, "You can't set it to that.\r\n");
      return (0);
    }
    if (GET_ADMLEVEL(vict) == value)
      return (1);
    admin_set(vict, value);
    break;

  case 59:
   if (value < 0 || value >= 6) {
      send_to_char(ch, "You can't set it to that.\r\n");
    return (0);
   }
    GET_HAIRL(vict) = value;
    return (1);
   break; 

  case 60:
   if (value < 0 || value >= 13) {
      send_to_char(ch, "You can't set it to that.\r\n");
    return (0);
   }
    GET_HAIRS(vict) = value;
    return (1);
   break;

  case 61:
   if (value < 0 || value >= 15) {
      send_to_char(ch, "You can't set it to that.\r\n");
    return (0);
   }
    GET_HAIRC(vict) = value;
    return (1);
   break;

  case 62:
   if (value < 0 || value >= 12) {
      send_to_char(ch, "You can't set it to that.\r\n");
    return (0);
   }
    GET_SKIN(vict) = value;
    return (1);
   break;

  case 63:
   if (value < 0 || value >= 13) {
      send_to_char(ch, "You can't set it to that.\r\n");
    return (0);
   }
    GET_EYE(vict) = value;
    return (1);
   break;

  case 64:
    GET_BASE_PL(vict) = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set basepl for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set basepl for %s.", GET_NAME(ch), GET_NAME(vict));
    break;

  case 65:
    GET_BASE_KI(vict) = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set baseki for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set baseki for %s.", GET_NAME(ch), GET_NAME(vict));
    break;

  case 66:
    GET_BASE_ST(vict) = value;
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set basest for %s.", GET_NAME(ch), GET_NAME(vict));
    log_imm_action("SET: %s has set basest for %s.", GET_NAME(ch), GET_NAME(vict));
    break;

  case 67:
   GET_DROOM(vict) = RANGE(0, 20000);
   break;

  case 68:
   GET_ABSORBS(vict) = RANGE(0, 3);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set absorbs for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set absorbs for %s.", GET_NAME(ch), GET_NAME(vict));
   break;

  case 69:
   GET_UP(vict) += RANGE(1, 1000);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set upgrade points for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set upgrade points for %s.", GET_NAME(ch), GET_NAME(vict));
   break;

  case 70:
   GET_AURA(vict) = RANGE(0, 8);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set aura for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set aura for %s.", GET_NAME(ch), GET_NAME(vict));
   break;
  case 71:
   send_to_char(ch, "Use the reward command.\r\n");
   break;
  case 72:
   GET_BOOSTS(vict) = RANGE(-1000, 1000);
   break;
  case 73:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_MULTP);
    break;
  case 74:
   GET_DCOUNT(vict) = RANGE(-1000, 1000);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set death count for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set death count for %s.", GET_NAME(ch), GET_NAME(vict));
   break;
  case 75:
    send_to_char(ch, "No.");
    break;
  case 76:
   if (vict->desc != NULL) {
    star_phase(vict, RANGE(0, 2));
   } else {
    send_to_char(ch, "They aren't even in the game!\r\n");
   }
   break;
  case 77:
   RACIAL_PREF(vict) = RANGE(1, 3);
   break;
  
  case 78:
   GET_SLOTS(vict) = RANGE(1, 1000);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set skill slots for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set skill slots for %s.", GET_NAME(ch), GET_NAME(vict));
   break;

  case 79:
   GET_FEATURE(vict) = '\0';
   break;

  case 80:
   GET_TRANSCLASS(vict) = RANGE(1, 3);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "SET: %s has set transformation class for %s.", GET_NAME(ch), GET_NAME(vict));
   log_imm_action("SET: %s has set transformation class for %s.", GET_NAME(ch), GET_NAME(vict));
   break;

  case 81:
   GET_CLONES(vict) = RANGE(1, 3);
   send_to_char(ch, "Done.\r\n");
   break;

  default:
    send_to_char(ch, "Can't set that!\r\n");
    return (0);
  }

  return (1);
}


ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!strcasecmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!strcasecmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such player.\r\n");
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such creature.\r\n");
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_ADMLEVEL(cbuf) >= GET_ADMLEVEL(ch)) {
	free_char(cbuf);
	send_to_char(ch, "Sorry, you can't do that.\r\n");
	return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strcmp(field, set_fields[mode].cmd))
      break;
  if (*(set_fields[mode].cmd) == '\n') 
    for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++) 
      if (!strncmp(field, set_fields[mode].cmd, len)) 
        break; 

  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict);
    if (is_file) {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_saveall)
{
 if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER)
    send_to_char (ch, "You are not holy enough to use this privelege.\n\r");
 else {
    save_all();
    House_save_all();
    send_to_char(ch, "World and house files saved.\n\r");
 }
}

#define PLIST_FORMAT \
  "players [minlev[-maxlev]] [-n name] [-d days] [-h hours] [-m]" 

ACMD(do_plist) 
{ 
  int i, len = 0, count = 0; 
  char mode, buf[MAX_STRING_LENGTH], name_search[MAX_NAME_LENGTH], time_str[MAX_STRING_LENGTH]; 
  struct time_info_data time_away; 
  int low = 0, high = 100, low_day = 0, high_day = 10000, low_hr = 0, high_hr = 24; 
 
  skip_spaces(&argument); 
  strcpy(buf, argument);        /* strcpy: OK (sizeof: argument == buf) */ 
  name_search[0] = '\0'; 
 
  while (*buf) { 
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH]; 
 
    half_chop(buf, arg, buf1); 
    if (isdigit(*arg)) { 
      if (sscanf(arg, "%d-%d", &low, &high) == 1) 
        high = low; 
      strcpy(buf, buf1);        /* strcpy: OK (sizeof: buf1 == buf) */ 
    } else if (*arg == '-') { 
      mode = *(arg + 1);        /* just in case; we destroy arg in the switch */ 
      switch (mode) { 
      case 'l': 
        half_chop(buf1, arg, buf); 
        sscanf(arg, "%d-%d", &low, &high); 
        break; 
      case 'n': 
        half_chop(buf1, name_search, buf); 
        break; 
      case 'i': 
        strcpy(buf, buf1); 
        low = 1; 
        break; 
      case 'm': 
        strcpy(buf, buf1);  
        high = 100; 
        break; 
      case 'd': 
        half_chop(buf1, arg, buf); 
        if (sscanf(arg, "%d-%d", &low_day, &high_day) == 1) 
          high_day = low_day; 
        break; 
      case 'h': 
        half_chop(buf1, arg, buf); 
        if (sscanf(arg, "%d-%d", &low_hr, &high_hr) == 1) 
          high_hr = low_hr; 
        break; 
      default: 
        send_to_char(ch, "%s\r\n", PLIST_FORMAT); 
        return;    
      } 
    } else { 
      send_to_char(ch, "%s\r\n", PLIST_FORMAT); 
      return; 
    } 
  } 
 
  len = 0;    
  len += snprintf(buf + len, sizeof(buf) - len, "@W[ Id] (Lv) Name         Last@n\r\n" 
                  "%s-----------------------------------------------%s\r\n", CCCYN(ch, C_NRM), 
                  CCNRM(ch, C_NRM)); 
 
  for (i = 0; i <= top_of_p_table; i++) { 
    if (IS_SET(player_table[i].flags, PINDEX_DELETED)) {
     len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] <DELETED> --Will be removed next boot.\r\n", player_table[i].id); 
      continue;
    }

    if (player_table[i].level < low || player_table[i].level > high) 
      continue; 
 
    time_away = *real_time_passed(time(0), player_table[i].last); 
 
    if (*name_search && strcasecmp(name_search, player_table[i].name))
      continue; 
 
    if (time_away.day > high_day || time_away.day < low_day) 
      continue; 
    if (time_away.hours > high_hr || time_away.hours < low_hr) 
      continue; 
 
    strcpy(time_str, asctime(localtime(&player_table[i].last))); 
    time_str[strlen(time_str) - 1] = '\0'; 
 
    len += snprintf(buf + len, sizeof(buf) - len, "[%3ld] (%2d) %-12s %s\r\n", 
                    player_table[i].id, player_table[i].level, 
                    CAP(strdup(player_table[i].name)), time_str); 
    count++; 
  } 
  snprintf(buf + len, sizeof(buf) - len, "%s-----------------------------------------------%s\r\n" 
           "%d players listed.\r\n", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM), count); 
  page_string(ch->desc, buf, TRUE); 
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;
  send_to_room(IN_ROOM(ch), "Everything is quite peaceful now.\r\n");

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;  
      if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) 
        continue;
      stop_fighting(vict);  
      GET_POS(vict) = POS_SITTING; 
    }
    stop_fighting(ch);
    GET_POS(ch) = POS_STANDING;  
}

ACMD(do_wizupdate)
{
       run_autowiz();
       send_to_char(ch, "Wizlists updated.\n\r");
}

ACMD(do_raise)
{
  struct char_data *vict = NULL;
  char name[MAX_INPUT_LENGTH];

  one_argument(argument, name);

  if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER && !IS_NPC(ch)) {
   return;
  }

  if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "There is no such player.\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "But they aren't even dead!\r\n");
    return;
  }

  send_to_char(ch, "@wYou return %s from the @Bspirit@w world, to the world of the living!@n\r\n", GET_NAME(vict));
  send_to_char(vict, "@wYour @Bspirit@w has been returned to the world of the living by %s!@n\r\n", GET_NAME(ch));

  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ETHEREAL);
  REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_SPIRIT);

  send_to_imm("Log: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));
  log_imm_action("RAISE: %s has raised %s from the dead.", GET_NAME(ch), GET_NAME(vict));

  int statpunish = FALSE;  

  if (GET_ADMLEVEL(ch) <= 0) {
   statpunish = TRUE;
  }

  if (GET_HIT(vict) < 1)
    GET_HIT(vict) = gear_pl(vict);
  GET_MANA(vict) = GET_MAX_MANA(vict);
  GET_MOVE(vict) = GET_MAX_MOVE(vict);
  GET_LIMBCOND(vict, 1) = 100;
  GET_LIMBCOND(vict, 2) = 100;
  GET_LIMBCOND(vict, 3) = 100;
  GET_LIMBCOND(vict, 4) = 100;
  SET_BIT_AR(PLR_FLAGS(vict), PLR_HEAD);
  REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_PDEATH);
  char_from_room(vict);
    if (GET_DROOM(vict) != NOWHERE && GET_DROOM(vict) != 0 && GET_DROOM(vict) != 1) {
     char_to_room(vict, real_room(GET_DROOM(vict)));
    }
    else if (IS_ROSHI(vict)) {
     char_to_room(vict, real_room(1130));
    }
    else if (IS_KABITO(vict)) {
     char_to_room(vict, real_room(12098));
    }
    else if (IS_NAIL(vict)) {
     char_to_room(vict, real_room(11683));
    }
    else if (IS_BARDOCK(vict)) {
     char_to_room(vict, real_room(2268));
    }
    else if (IS_KRANE(vict)) {
     char_to_room(vict, real_room(13009));
    }
    else if (IS_TAPION(vict)) {
     char_to_room(vict, real_room(8231));
    }
    else if (IS_PICCOLO(vict)) {
     char_to_room(vict, real_room(1659));
    }
    else if (IS_ANDSIX(vict)) {
     char_to_room(vict, real_room(1713));
    }
    else if (IS_DABURA(vict)) {
     char_to_room(vict, real_room(6486));
    }
    else if (IS_FRIEZA(vict)) {
     char_to_room(vict, real_room(4282));
    }
    else if (IS_GINYU(vict)) {
     char_to_room(vict, real_room(4289));
    }
    else if (IS_JINTO(vict)) {
     char_to_room(vict, real_room(3499));
    }
    else if (IS_TSUNA(vict)) {
     char_to_room(vict, real_room(15000));
    }
    else if (IS_KURZAK(vict)) {
     char_to_room(vict, real_room(16100));
    }
    else {
     char_to_room(vict, real_room(300));
     send_to_imm("ERROR: Player %s without acceptable sensei.\r\n", GET_NAME(vict));
    }
  look_at_room(IN_ROOM(vict), vict, 0);
     int losschance = axion_dice(0);
     if (GET_LEVEL(vict) > 9 && statpunish == TRUE) {
        send_to_char(vict, "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength, constitution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        int str = -8, intel = -8, wis = -8, spd = -8, con = -8, agl = -8;
        int dur = 100;
        if (GET_DCOUNT(vict) >= 8 && GET_DCOUNT(vict) < 10) {
         dur = 90;
        } else if (GET_DCOUNT(vict) >= 5 && GET_DCOUNT(vict) < 8) {
         dur = 75;
        } else if (GET_DCOUNT(vict) >= 3 && GET_DCOUNT(vict) < 5) {
         dur = 60;
        } else if (GET_DCOUNT(vict) >= 1 && GET_DCOUNT(vict) < 3) {
         dur = 40;
        }
        if (vict->real_abils.intel <= 16) {
         intel = -4;
        }
        if (vict->real_abils.cha <= 16) {
         spd = -4;
        }
        if (vict->real_abils.dex <= 16) {
         agl = -4;
        }
        if (vict->real_abils.wis <= 16) {
         wis = -4;
        }
        if (vict->real_abils.con <= 16) {
         con = -4;
        }
        assign_affect(vict, AFF_WEAKENED_STATE, SKILL_WARP, dur, str, con, intel, agl, wis, spd);
      if (losschance >= 100) {
         int psloss = rand_number(100, 300);
         GET_PRACTICES(vict, GET_CLASS(vict)) -= psloss;
         send_to_char(vict, "@R...and a loss of @r%d@R PS!@n", psloss);
        if (GET_PRACTICES(vict, GET_CLASS(vict)) < 0) {
         GET_PRACTICES(vict, GET_CLASS(vict)) = 0;
        }
      }
     }
  GET_DTIME(ch) = 0;

  act("$n's body forms in a pool of @Bblue light@n.", TRUE, vict, 0, 0, TO_ROOM);
}

ACMD(do_chown)
{
 struct char_data *victim;
 struct obj_data *obj;
 char buf2[80];
 char buf3[80];
 int i, k = 0;

 two_arguments(argument, buf2, buf3);

 if (!*buf2)
   send_to_char(ch, "Syntax: chown <object> <character>.\r\n");
 else if (!(victim = get_char_vis(ch, buf3, NULL, FIND_CHAR_WORLD)))
   send_to_char(ch, "No one by that name here.\r\n");
 else if (victim == ch)
   send_to_char(ch, "Are you sure you're feeling ok?\r\n");
 else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
   send_to_char(ch, "That's really not such a good idea.\r\n");
 else if (!*buf3)
   send_to_char(ch, "Syntax: chown <object> <character>.\r\n");
 else {
   for (i = 0; i < NUM_WEARS; i++) {
     if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
        isname(buf2, GET_EQ(victim, i)->name)) {
       obj_to_char(unequip_char(victim, i), victim);
       k = 1;
     }
   }

 if (!(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
   if (!k && !(obj = get_obj_in_list_vis(victim, buf2, NULL, victim->carrying))) {
     send_to_char(ch, "%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
     return;
   }
 }

 act("@n$n makes a magical gesture and $p@n flies from $N to $m.", FALSE, ch, obj, victim, TO_NOTVICT);
 act("@n$n makes a magical gesture and $p@n flies away from you to $m.", FALSE, ch, obj, victim, TO_VICT);
 act("@nYou make a magical gesture and $p@n flies away from $N to you.", FALSE, ch, obj, victim, TO_CHAR);

 obj_from_char(obj);
 obj_to_char(obj, ch);
 save_char(ch);
 save_char(victim);
 }
}

ACMD(do_zpurge)
{
   struct obj_data *obj, *next_obj;
   struct char_data *mob, *next_mob;
   int i, stored = -1, zone, found = FALSE;
   int room;
   char arg[MAX_INPUT_LENGTH];

   one_argument(argument, arg);

   if (!*arg) {
     zone = zone_table[world[IN_ROOM(ch)].zone].number;
   } else {
     zone = atoi(arg);
   }

   for (i = 0; i <= top_of_zone_table && !found; i++) {
     if (zone_table[i].number == zone) {
       stored = i;
       found = TRUE;
     }
   }

   if (!found || !can_edit_zone(ch, zone)) {
     send_to_char(ch, "You cannot purge that zone. Try %d.\r\n", GET_OLC_ZONE(ch));
     return;
   }

   for (room = genolc_zone_bottom(stored); room <= zone_table[stored].top; room++) {
     if ((i = real_room(room)) != NOWHERE) {
       for (mob = world[i].people; mob; mob = next_mob) {
         next_mob = mob->next_in_room;
         if (IS_NPC(mob)) {
           extract_char(mob);
         }
        }

       for (obj = world[i].contents; obj; obj = next_obj) {
         next_obj = obj->next_content;
         extract_obj(obj);
       }
     }
   }

   send_to_char(ch, "All mobiles and objects in zone %d purged.\r\n", zone);
   mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged zone %d.", GET_NAME(ch), zone);
} 

/******************************************************************************/
/*                         Zone Checker Code below                            */
/******************************************************************************/

/*mob limits*/
#define MAX_DAMAGE_MOD_ALLOWED   MAX(GET_LEVEL(mob)/5, 2)
#define MAX_GOLD_ALLOWED         GET_LEVEL(mob)*3000
#define MAX_EXP_ALLOWED          GET_LEVEL(mob)*GET_LEVEL(mob) * 120
#define MAX_LEVEL_ALLOWED        (100)
#define GET_OBJ_AVG_DAM(obj)     (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1))
/* arbitrary limit for per round dam */
#define MAX_MOB_DAM_ALLOWED      500

#define ZCMD2 zone_table[zone].cmd[cmd_no]  /*from DB.C*/

/*item limits*/
#define MAX_DAM_ALLOWED            50    /* for weapons  - avg. dam*/
#define MAX_AFFECTS_ALLOWED        3

/* Armor class limits*/
#define TOTAL_WEAR_CHECKS  (NUM_ITEM_WEARS-2)  /*minus Wield and Take*/
static const struct zcheck_armor {
  bitvector_t bitvector;          /* from Structs.h                       */
  int ac_allowed;                 /* Max. AC allowed for this body part  */
  char *message;                  /* phrase for error message            */
} zarmor[] = {
  {ITEM_WEAR_FINGER,	10,	"Ring"},
  {ITEM_WEAR_NECK,	10,	"Necklace"},
  {ITEM_WEAR_BODY,	10,	"Body armor"},
  {ITEM_WEAR_HEAD,	10,	"Head gear"},
  {ITEM_WEAR_LEGS,	10,	"Legwear"},
  {ITEM_WEAR_FEET,	10,	"Footwear"},
  {ITEM_WEAR_HANDS,	10,	"Glove"},
  {ITEM_WEAR_ARMS,	10,	"Armwear"},
  {ITEM_WEAR_SHIELD,	10,	"Shield"},
  {ITEM_WEAR_ABOUT,	10,	"Cloak"},
  {ITEM_WEAR_WAIST,	10,	"Belt"},
  {ITEM_WEAR_WRIST,	10,	"Wristwear"},
  {ITEM_WEAR_HOLD,	10,	"Held item"},
  {ITEM_WEAR_PACK,	10,	"Backpack item"},
  {ITEM_WEAR_EAR,	10,	"Earring item"},
  {ITEM_WEAR_SH,	10,	"Shoulder item"},
  {ITEM_WEAR_EYE,	10,	"Eye item"}
};

/*These are strictly boolean*/
#define CAN_WEAR_WEAPONS         0     /* toggle - can weapon also be armor? */
#define MAX_APPLIES_LIMIT        1     /* toggle - is there a limit at all?  */
#define CHECK_ITEM_RENT          0     /* do we check for rent cost == 0 ?   */
#define CHECK_ITEM_COST          0     /* do we check for item cost == 0 ?   */
/*
  Applies limits
  !! Very Important:  Keep these in the same order as in Structs.h
  To ignore an apply, set max_aff to -99.
  These will be ignored if MAX_APPLIES_LIMIT = 0
*/
static const struct zcheck_affs {
  int aff_type;    /*from Structs.h*/
  int min_aff;     /*min. allowed value*/
  int max_aff;     /*max. allowed value*/
  char *message;   /*phrase for error message*/
} zaffs[] = {
  {APPLY_NONE,		0,	-99,	"unused0"},
  {APPLY_STR,		-6,	6,	"strength"},
  {APPLY_DEX,		-6,	6,	"dexterity"},
  {APPLY_INT,		-6,	6,	"intelligence"},
  {APPLY_WIS,		-6,	6,	"wisdom"},
  {APPLY_CON,		-6,	6,	"constitution"},
  {APPLY_CHA,		-6,	6,	"charisma"},
  {APPLY_CLASS,		0,	0,	"class"},
  {APPLY_LEVEL,		0,	0,	"level"},
  {APPLY_AGE,		-10,	10,	"age"},
  {APPLY_CHAR_WEIGHT,	-50,	50,	"character weight"},
  {APPLY_CHAR_HEIGHT,	-50,	50,	"character height"},
  {APPLY_MANA,		-50,	50,	"mana"},
  {APPLY_HIT,		-50,	50,	"hit points"},
  {APPLY_MOVE,		-50,	50,	"movement"},
  {APPLY_GOLD,		0,	0,	"gold"},
  {APPLY_EXP,		0,	0,	"experience"},
  {APPLY_AC,		-10,	10,	"magical AC"},
  {APPLY_ACCURACY,	0,	-99,	"accuracy"},
  {APPLY_DAMAGE,	0,	-99,	"damage"},
  {APPLY_REGEN ,	0,	0,	"regen"},
  {APPLY_TRAIN ,	0,	0,	"train"},
  {APPLY_LIFEMAX,	0,	0,	"lifemax"},
  {APPLY_UNUSED3,	0,	0,	"unused"},
  {APPLY_UNUSED4,	0,	0,	"unused"},
  {APPLY_RACE,		0,	0,	"race"},
  {APPLY_TURN_LEVEL,	-6,	6,	"turn level"},
  {APPLY_SPELL_LVL_0,	0,	0,	"spell level 0"},
  {APPLY_SPELL_LVL_1,	0,	0,	"spell level 1"},
  {APPLY_SPELL_LVL_2,	0,	0,	"spell level 2"},
  {APPLY_SPELL_LVL_3,	0,	0,	"spell level 3"},
  {APPLY_SPELL_LVL_4,	0,	0,	"spell level 4"},
  {APPLY_SPELL_LVL_5,	0,	0,	"spell level 5"},
  {APPLY_SPELL_LVL_6,	0,	0,	"spell level 6"},
  {APPLY_SPELL_LVL_7,	0,	0,	"spell level 7"},
  {APPLY_SPELL_LVL_8,	0,	0,	"spell level 8"},
  {APPLY_SPELL_LVL_9,	0,	0,	"spell level 9"},
  {APPLY_KI,		0,	0,	"ki"},
  {APPLY_FORTITUDE,	-4,	4,	"fortitude"},
  {APPLY_REFLEX,	-4,	4,	"reflex"},
  {APPLY_WILL,		-4,	4,	"will"},
  {APPLY_SKILL,		-10,	10,	"skill"},
  {APPLY_FEAT,		-10,	10,	"feat"},
  {APPLY_ALLSAVES,	-4,	4,	"all 3 save types"},
  {APPLY_RESISTANCE,	-4,	4,	"resistance"}
};

/* These are ABS() values. */
#define MAX_APPLY_ACCURCY_MOD_TOTAL	5
#define MAX_APPLY_DAMAGE_MOD_TOTAL	5

/*room limits*/
/* Off limit zones are any zones a player should NOT be able to walk to (ex. Limbo) */
static const int offlimit_zones[] = {0,12,13,14,-1};  /*what zones can no room connect to (virtual num) */
#define MIN_ROOM_DESC_LENGTH   80       /* at least one line - set to 0 to not care. */
#define MAX_COLOUMN_WIDTH      80       /* at most 80 chars per line */


ACMD (do_zcheck)
{ 
  zone_rnum zrnum;
  struct obj_data *obj;
  struct char_data *mob = NULL;
  room_vnum exroom=0;
  int ac=0;
  int affs=0, tohit, todam, value;
  int i = 0, j = 0, k = 0, l = 0, m = 0, found = 0; /* found is used as a 'send now' flag*/
  char buf[MAX_STRING_LENGTH];
  float avg_dam;
  size_t len = 0;
  struct extra_descr_data *ext, *ext2;
  one_argument(argument, buf);

  if (buf == NULL || !*buf || !strcmp(buf, "."))
    zrnum = world[IN_ROOM(ch)].zone;
  else 
    zrnum = real_zone(atoi(buf));
 
  if (zrnum == NOWHERE) {
    send_to_char(ch, "Check what zone ?\r\n");
    return;
  } else
    send_to_char(ch, "Checking zone %d!\r\n", zone_table[zrnum].number);

 /************** Check mobs *****************/

  send_to_char(ch, "Checking Mobs for limits...\r\n");
  /*check mobs first*/
  for (i=0; i<top_of_mobt;i++) {                   
      if (real_zone_by_thing(mob_index[i].vnum) == zrnum) {  /*is mob in this zone?*/
        mob = &mob_proto[i];
        if (!strcmp(mob->name, "mob unfinished") && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Alias hasn't been set.\r\n");

        if (!strcmp(mob->short_descr, "the unfinished mob") && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Short description hasn't been set.\r\n");

        if (!strncmp(mob->long_descr, "An unfinished mob stands here.", 30) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Long description hasn't been set.\r\n");

        if (mob->description && *mob->description) {
          if (!strncmp(mob->description, "It looks unfinished.", 20) && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Description hasn't been set.\r\n");
          else if (strncmp(mob->description, "   ", 3) && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Description hasn't been formatted. (/fi)\r\n");
        }
       
        if (GET_LEVEL(mob)>MAX_LEVEL_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Is level %d (limit: 1-%d)\r\n",
                          GET_LEVEL(mob), MAX_LEVEL_ALLOWED);

        if (GET_DAMAGE_MOD(mob)>MAX_DAMAGE_MOD_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Damage mod of %d is too high (limit: %d)\r\n",
                          GET_DAMAGE_MOD(mob), MAX_DAMAGE_MOD_ALLOWED);

        /* avg. dam including damroll per round of combat */
        avg_dam = (((mob->mob_specials.damsizedice / 2.0) * mob->mob_specials.damnodice)+GET_DAMAGE_MOD(mob));
        if (avg_dam>MAX_MOB_DAM_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- average damage of %4.1f is too high (limit: %d)\r\n",
                          avg_dam, MAX_MOB_DAM_ALLOWED);

        if (mob->mob_specials.damsizedice == 1 &&
            mob->mob_specials.damnodice == 1 &&
            GET_LEVEL(mob) == 0 &&
            (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Needs to be fixed - %sAutogenerate!%s\r\n", CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
       
        if (MOB_FLAGGED(mob, MOB_AGGRESSIVE) &&
            MOB_FLAGGED(mob, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
          "- Both aggresive and agressive to align.\r\n");
       
        if ((GET_GOLD(mob) > MAX_GOLD_ALLOWED) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Set to %d Gold (limit : %d).\r\n",
                                  GET_GOLD(mob), 
                                  MAX_GOLD_ALLOWED);

        if (GET_EXP(mob)>MAX_EXP_ALLOWED && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Has %"I64T" experience (limit: %d)\r\n",
                              GET_EXP(mob), MAX_EXP_ALLOWED);
        if (AFF_FLAGGED(mob, AFF_GROUP | AFF_CHARM | AFF_POISON) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- Has illegal affection bits set (%s %s %s)\r\n",
                              AFF_FLAGGED(mob, AFF_GROUP) ? "GROUP" : "",
                              AFF_FLAGGED(mob, AFF_CHARM) ? "CHARM" : "",
                              AFF_FLAGGED(mob, AFF_POISON) ? "POISON" : "");
         

        /*if (!MOB_FLAGGED(mob, MOB_SENTINEL) && !MOB_FLAGGED(mob, MOB_STAY_ZONE) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                            "- Neither SENTINEL nor STAY_ZONE bits set.\r\n");*/

        if (MOB_FLAGGED(mob, MOB_SPEC) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                            "- SPEC flag needs to be removed.\r\n");

          /*****ADDITIONAL MOB CHECKS HERE*****/
          if (found) {
            send_to_char(ch,
                    "%s[%5d]%s %-30s: %s\r\n%s",
                    CCCYN(ch, C_NRM), GET_MOB_VNUM(mob),
                    CCYEL(ch, C_NRM), GET_NAME(mob),
                    CCNRM(ch, C_NRM), buf);
          }
          /* reset buffers and found flag */
          strcpy(buf, "");
          found = 0;
          len = 0;
        }   /*mob is in zone*/
    }  /*check mobs*/

 /************** Check objects *****************/
  send_to_char(ch, "\r\nChecking Objects for limits...\r\n");
  for (i=0; i<top_of_objt; i++) {
    if (real_zone_by_thing(obj_index[i].vnum) == zrnum) { /*is object in this zone?*/
      obj = &obj_proto[i];
      switch (GET_OBJ_TYPE(obj)) {       
        case ITEM_MONEY:
          if ((value = GET_OBJ_VAL(obj, 1))>MAX_GOLD_ALLOWED && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Is worth %d (money limit %d coins).\r\n",
                                 value, MAX_GOLD_ALLOWED);
          break;             
        case ITEM_WEAPON:
          if (GET_OBJ_VAL(obj, 3) >= NUM_ATTACK_TYPES && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- has out of range attack type %d.\r\n",
                                 GET_OBJ_VAL(obj, 3));
         
          if (GET_OBJ_AVG_DAM(obj)>MAX_DAM_ALLOWED && (found=1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Damroll is %2.1f (limit %d)\r\n",
                                 GET_OBJ_AVG_DAM(obj), MAX_DAM_ALLOWED);
          break;
        case ITEM_ARMOR:
          ac=GET_OBJ_VAL(obj,0);
          for (j=0; j<TOTAL_WEAR_CHECKS;j++) {
            if (CAN_WEAR(obj,zarmor[j].bitvector) && (ac>zarmor[j].ac_allowed) && (found=1))
              len += snprintf(buf + len, sizeof(buf) - len,
                                   "- Has AC %d (%s limit is %d)\r\n",
                                   ac, zarmor[j].message, zarmor[j].ac_allowed);
          }
          break;             
     
      }  /*switch on Item_Type*/

      if (!CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
        if ((GET_OBJ_COST(obj) || (GET_OBJ_WEIGHT(obj) && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) ||
           GET_OBJ_RENT(obj)) && (found = 1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- is NO_TAKE, but has cost (%d) weight (%"I64T") or rent (%d) set.\r\n",
                          GET_OBJ_COST(obj), GET_OBJ_WEIGHT(obj), GET_OBJ_RENT(obj));
      } else {
        if (GET_OBJ_COST(obj) == 0 && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 cost (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) == 0 && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has 0 weight (min. 1).\r\n");

        if (GET_OBJ_WEIGHT(obj) > MAX_OBJ_WEIGHT && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "  Weight is too high: %"I64T" (limit  %d).\r\n",
                          GET_OBJ_WEIGHT(obj), MAX_OBJ_WEIGHT);


        if (GET_OBJ_COST(obj) > MAX_OBJ_COST && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d cost (max %d).\r\n",
                          GET_OBJ_COST(obj), MAX_OBJ_COST);
      }

      if (GET_OBJ_LEVEL(obj) > ADMLVL_IMMORT-1 && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has min level set to %d (max %d).\r\n",
                          GET_OBJ_LEVEL(obj), ADMLVL_IMMORT-1);

      if (obj->action_description && *obj->action_description &&
          GET_OBJ_TYPE(obj) != ITEM_STAFF &&
          GET_OBJ_TYPE(obj) != ITEM_WAND &&
          GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
          GET_OBJ_TYPE(obj) != ITEM_NOTE && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has action_description set, but is inappropriate type.\r\n");
         
      /*first check for over-all affections*/
      for (affs=0, j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj->affected[j].modifier) affs++;

      if (affs>MAX_AFFECTS_ALLOWED && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                          "- has %d affects (limit %d).\r\n",
                             affs, MAX_AFFECTS_ALLOWED);
       
      /*check for out of range affections. */
      for (j=0;j<MAX_OBJ_AFFECT;j++)
        if (zaffs[(int)obj->affected[j].location].max_aff != -99 && /* only care if a range is set */
            (obj->affected[j].modifier > zaffs[(int)obj->affected[j].location].max_aff ||
             obj->affected[j].modifier < zaffs[(int)obj->affected[j].location].min_aff ||
             zaffs[(int)obj->affected[j].location].min_aff == zaffs[(int)obj->affected[j].location].max_aff) && (found=1))
          len += snprintf(buf + len, sizeof(buf) - len,
                          "- apply to %s is %d (limit %d - %d).\r\n",
                               zaffs[(int)obj->affected[j].location].message,
                               obj->affected[j].modifier,
                               zaffs[(int)obj->affected[j].location].min_aff,
                               zaffs[(int)obj->affected[j].location].max_aff);

     /* special handling of +hit and +dam because of +hit_n_dam */
     for (todam=0, tohit=0, j=0;j<MAX_OBJ_AFFECT;j++) {
       if (obj->affected[j].location == APPLY_DAMAGE)
         todam += obj->affected[j].modifier;
     }
     if (abs(todam) > MAX_APPLY_DAMAGE_MOD_TOTAL && (found=1))
       len += snprintf(buf + len, sizeof(buf) - len,
                       "- total damage mod %d out of range (limit +/-%d.\r\n",
                       todam, MAX_APPLY_DAMAGE_MOD_TOTAL);
     if (abs(tohit) > MAX_APPLY_ACCURCY_MOD_TOTAL && (found=1))
       len += snprintf(buf + len, sizeof(buf) - len,
                       "- total accurcy mod %d out of range (limit +/-%d).\r\n",
                       tohit, MAX_APPLY_ACCURCY_MOD_TOTAL);
       
     
     for (ext2 = NULL, ext = obj->ex_description; ext; ext = ext->next)
       if (strncmp(ext->description, "   ", 3))
         ext2 = ext;
     
     if (ext2 && (found = 1))
       len += snprintf(buf + len, sizeof(buf) - len,
                       "- has unformatted extra description\r\n");
     /*****ADDITIONAL OBJ CHECKS HERE*****/
     if (found) {
        send_to_char(ch, "[%5d] %-30s: \r\n%s", GET_OBJ_VNUM(obj), obj->short_description, buf);
      }
      strcpy(buf, "");
      len = 0;
      found = 0;
    }   /*object is in zone*/
  } /*check objects*/

  /************** Check rooms *****************/
  send_to_char(ch, "\r\nChecking Rooms for limits...\r\n");
  for (i=0; i<top_of_world;i++) {
    if (world[i].zone==zrnum) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
        /*check for exit, but ignore off limits if you're in an offlimit zone*/
        if (!world[i].dir_option[j])
          continue;
        exroom=world[i].dir_option[j]->to_room;
        if (exroom==NOWHERE)
          continue;
        if (world[exroom].zone == zrnum)
          continue;
        if (world[exroom].zone == world[i].zone)
          continue;

        for (k=0;offlimit_zones[k] != -1;k++) {
          if (world[exroom].zone == real_zone(offlimit_zones[k]) && (found = 1))
            len += snprintf(buf + len, sizeof(buf) - len,
                            "- Exit %s cannot connect to %d (zone off limits).\r\n",
                            dirs[j], world[exroom].number);
        } /* for (k.. */
      } /* cycle directions */         

     if (ROOM_FLAGGED(i, ROOM_ATRIUM | ROOM_HOUSE | ROOM_HOUSE_CRASH | ROOM_OLC | ROOM_BFS_MARK))
         len += snprintf(buf + len, sizeof(buf) - len,
         "- Has illegal affection bits set (%s %s %s %s %s)\r\n",
                            ROOM_FLAGGED(i, ROOM_ATRIUM) ? "ATRIUM" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE) ? "HOUSE" : "",
                            ROOM_FLAGGED(i, ROOM_HOUSE_CRASH) ? "HCRSH" : "",
                            ROOM_FLAGGED(i, ROOM_OLC) ? "OLC" : "",
                            ROOM_FLAGGED(i, ROOM_BFS_MARK) ? "*" : "");

      if ((MIN_ROOM_DESC_LENGTH) && strlen(world[i].description)<MIN_ROOM_DESC_LENGTH && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description is too short. (%4.4"SZT" of min. %d characters).\r\n",
                             strlen(world[i].description), MIN_ROOM_DESC_LENGTH);

      if (strncmp(world[i].description, "   ", 3) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not formatted with indent (/fi in the editor).\r\n");

      /* strcspan = size of text in first arg before any character in second arg */
      if ((strcspn(world[i].description, "\r\n")>MAX_COLOUMN_WIDTH) && (found=1))
        len += snprintf(buf + len, sizeof(buf) - len,
                        "- Room description not wrapped at %d chars (/fi in the editor).\r\n",
                             MAX_COLOUMN_WIDTH);
           
     for (ext2 = NULL, ext = world[i].ex_description; ext; ext = ext->next)
       if (strncmp(ext->description, "   ", 3))
         ext2 = ext;

     if (ext2 && (found = 1))
       len += snprintf(buf + len, sizeof(buf) - len,
                       "- has unformatted extra description\r\n");

      if (found) {
        send_to_char(ch, "[%5d] %-30s: \r\n%s",
                       world[i].number, world[i].name ? world[i].name : "An unnamed room", buf);
        strcpy(buf, "");
        len = 0;
        found = 0;
      }
    } /*is room in this zone?*/
  } /*checking rooms*/ 

  for (i=0; i<top_of_world;i++) {
    if (world[i].zone==zrnum) {
      m++;
      for (j = 0, k = 0; j < NUM_OF_DIRS; j++)
        if (!world[i].dir_option[j])
          k++;
       
      if (k == NUM_OF_DIRS)
        l++;
    }
  }
  if (l * 3 > m)
    send_to_char(ch, "More than 1/3 of the rooms are not linked.\r\n");
 
}
/**********************************************************************************/

static void mob_checkload(struct char_data *ch, mob_vnum mvnum)
{
  int cmd_no, count = 0;
  zone_rnum zone;
  mob_rnum mrnum = real_mobile(mvnum);
 
  if (mrnum == NOBODY) {
      send_to_char(ch, "That mob does not exist.\r\n");
      return;
  }

  send_to_char(ch, "Checking load info for the mob [%d] %s...\r\n",
                    mvnum, mob_proto[mrnum].short_descr);

  for (zone=0; zone <= top_of_zone_table; zone++) {   
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      if (ZCMD2.command != 'M')
        continue;

      /* read a mobile */
      if (ZCMD2.arg1 == mrnum) {
        send_to_char(ch, "  [%5d] %s (%d MAX)\r\n",
                         world[ZCMD2.arg3].number,
                         world[ZCMD2.arg3].name,
                         ZCMD2.arg2);
        count += 1;
      }
    }
  }
  if (count > 0)
   send_to_char(ch, "@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count));
}

static void obj_checkload(struct char_data *ch, obj_vnum ovnum)
{
  int cmd_no, count = 0;
  zone_rnum zone;
  obj_rnum ornum = real_object(ovnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0;
  mob_rnum lastmob_r = 0;

  if (ornum ==NOTHING) {
    send_to_char(ch, "That object does not exist.\r\n");
    return;
  }
 
  send_to_char(ch, "Checking load info for the obj [%d] %s...\r\n",
                   ovnum, obj_proto[ornum].short_description);

  for (zone=0; zone <= top_of_zone_table; zone++) {   
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastmob_r = ZCMD2.arg1;
          break;
        case 'O':                   /* read an object */
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          if (ZCMD2.arg1 == ornum) {                       
            send_to_char(ch, "  [%5d] %s (%d Max)\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             ZCMD2.arg2);
           count += 1;
          }
          break;
        case 'P':                   /* object to object */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(ch, "  [%5d] %s (Put in another object [%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             ZCMD2.arg2);
           count += 1;
          }
          break;
        case 'G':                   /* obj_to_char */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(ch, "  [%5d] %s (Given to %s [%d][%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             mob_proto[lastmob_r].short_descr,
                             mob_index[lastmob_r].vnum,
                             ZCMD2.arg2);
           count += 1;
          }
          break;
        case 'E':                   /* object to equipment list */
          if (ZCMD2.arg1 == ornum) {
            send_to_char(ch, "  [%5d] %s (Equipped to %s [%d][%d Max])\r\n",
                             lastroom_v,
                             world[lastroom_r].name,
                             mob_proto[lastmob_r].short_descr,
                             mob_index[lastmob_r].vnum,
                             ZCMD2.arg2);
            count += 1;
           }
            break;
          case 'R': /* rem obj from room */
            lastroom_v = world[ZCMD2.arg1].number;
            lastroom_r = ZCMD2.arg1;
            if (ZCMD2.arg2 == ornum) {
              send_to_char(ch, "  [%5d] %s (Removed from room)\r\n",
                               lastroom_v,
                               world[lastroom_r].name);                   
             count += 1;
            }
            break;
      }/* switch */
    } /*for cmd_no......*/
  }  /*for zone...*/

  if (count > 0)
   send_to_char(ch, "@D[@nTotal counted: %s.@D]@n\r\n", add_commas(count));
}

static void trg_checkload(struct char_data *ch, trig_vnum tvnum)
{
  int cmd_no, found = 0;
  zone_rnum zone;
  trig_rnum trnum = real_trigger(tvnum);
  room_vnum lastroom_v = 0;
  room_rnum lastroom_r = 0, k;
  mob_rnum lastmob_r = 0, i;
  obj_rnum lastobj_r = 0, j;
  struct trig_proto_list *tpl;
 
  if (trnum == NOTHING) {
    send_to_char(ch, "That trigger does not exist.\r\n");
    return;
  }
 
  send_to_char(ch, "Checking load info for the %s trigger [%d] '%s':\r\n",
                    trig_index[trnum]->proto->attach_type == MOB_TRIGGER ? "mobile" :
                    (trig_index[trnum]->proto->attach_type == OBJ_TRIGGER ? "object" : "room"),                   
                    tvnum, trig_index[trnum]->proto->name);

  for (zone=0; zone <= top_of_zone_table; zone++) {   
    for (cmd_no = 0; ZCMD2.command != 'S'; cmd_no++) {
      switch (ZCMD2.command) {
        case 'M':
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastmob_r = ZCMD2.arg1;
          break;
        case 'O':                   /* read an object */
          lastroom_v = world[ZCMD2.arg3].number;
          lastroom_r = ZCMD2.arg3;
          lastobj_r = ZCMD2.arg1;
          break;
        case 'P':                   /* object to object */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'G':                   /* obj_to_char */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'E':                   /* object to equipment list */
          lastobj_r = ZCMD2.arg1;
          break;
        case 'R':                   /* rem obj from room */
          lastroom_v = 0;
          lastroom_r = 0;
          lastobj_r = 0;
          lastmob_r = 0;
        case 'T':                   /* trigger to something */
          if (ZCMD2.arg2 != trnum)
            break;
          if (ZCMD2.arg1 == MOB_TRIGGER) {
            send_to_char(ch, "mob [%5d] %-60s (zedit room %5d)\r\n",
                               mob_index[lastmob_r].vnum,
                               mob_proto[lastmob_r].short_descr,
                               lastroom_v);   
            found = 1;
          } else if (ZCMD2.arg1 == OBJ_TRIGGER) {
            send_to_char(ch, "obj [%5d] %-60s  (zedit room %d)\r\n",
                               obj_index[lastobj_r].vnum,
                               obj_proto[lastobj_r].short_description,
                               lastroom_v); 
            found = 1;
          } else if (ZCMD2.arg1==WLD_TRIGGER) {
            send_to_char(ch, "room [%5d] %-60s (zedit)\r\n",
                               lastroom_v,
                               world[lastroom_r].name);                   
            found = 1;
          }
        break;
      } /* switch */
    } /*for cmd_no......*/
  }  /*for zone...*/
 
  for (i = 0; i < top_of_mobt; i++) {
    if (!mob_proto[i].proto_script)
      continue;
   
    for (tpl = mob_proto[i].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "mob [%5d] %s\r\n",
                         mob_index[i].vnum,
                         mob_proto[i].short_descr);
        found = 1;
      }
 
  }
   
  for (j = 0; j < top_of_objt; j++) {
    if (!obj_proto[j].proto_script)
      continue;
   
    for (tpl = obj_proto[j].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "obj [%5d] %s\r\n",
                         obj_index[j].vnum,
                         obj_proto[j].short_description);
        found = 1;
      }
 
  }
 
  for (k = 0;k < top_of_world; k++) {
    if (!world[k].proto_script)
      continue;

    for (tpl = world[k].proto_script;tpl;tpl = tpl->next)
      if (tpl->vnum == tvnum) {
        send_to_char(ch, "room[%5d] %s\r\n",
                         world[k].number,
                         world[k].name);
        found = 1;
      }
  }

  if (!found)
    send_to_char(ch, "This trigger is not attached to anything.\r\n");
}

ACMD(do_checkloadstatus)
{ 
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf1, buf2);

  if ((!*buf1) || (!*buf2) || (!isdigit(*buf2))) {
    send_to_char(ch, "Checkload <M | O | T> <vnum>\r\n");
    return;
  }   

  if (LOWER(*buf1) == 'm') {         
    mob_checkload(ch, atoi(buf2));
    return;
  }
 
  if (LOWER(*buf1) == 'o') {         
    obj_checkload(ch, atoi(buf2));
    return;
  }

  if (LOWER(*buf1) == 't') {
    trg_checkload(ch, atoi(buf2));
    return;
  }
}
/**************************************************************************************/
/*                         Zone Checker code above                                    */
/**************************************************************************************/ 

/* do_findkey, finds where the key to a door loads to using do_checkloadstatus() */
ACMD(do_findkey)
{
  int dir, key;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  any_one_arg(argument, arg); /* Because "in" is a valid direction */

  if (!*arg) {
    send_to_char(ch, "Format: findkey <dir>\r\n");
  } else if ((dir = search_block(arg, dirs, FALSE)) >= 0 ||
      (dir = search_block(arg, abbr_dirs, FALSE)) >= 0 ) {
    if (!EXIT(ch, dir)) {
      send_to_char(ch, "There's no exit in that direction!\r\n");
    } else if ((key = EXIT(ch, dir)->key) == NOTHING || key == 0) {
      send_to_char(ch, "There's no key for that exit.\r\n");
    } else {
      sprintf(buf, "obj %d", key);
      do_checkloadstatus(ch, buf, 0, 0);
    }
  } else {
    send_to_char(ch, "What direction is that?!?\r\n");
  }
} 

ACMD(do_spells)
{
  int i, qend;

  send_to_char(ch, "The following spells are in the game:\r\n");

  for (qend = 0, i = 0; i < MAX_SPELLS; i++) {
    if (spell_info[i].name == unused_spellname)       /* This is valid. */
      continue;
    send_to_char(ch, "%18s", spell_info[i].name);
    if (qend++ % 4 == 3)
      send_to_char(ch, "\r\n");
  }
  if (qend % 4 != 0)
    send_to_char(ch, "\r\n");
  return;

}

ACMD(do_boom)
{
  /* Only IDNUM = 1 can cause a boom! Ideally for testing something that might
   * cause a crash. Currently left over from testing changes to send_to_outdoor. */
  if (GET_IDNUM(ch) != 1) {
    send_to_char(ch, "Sorry, only the Founder may use the boom command.\r\n");
    return;
  }

  send_to_outdoor("%s shakes the world with a mighty boom!\r\n", GET_NAME(ch));
}
