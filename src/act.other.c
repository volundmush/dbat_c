/**************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/
#include "act.other.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "dg_comm.h"
#include "combat.h"
#include "config.h"
#include "act.misc.h"
#include "weather.h"
#include "act.item.h"
#include "act.wizard.h"
#include "act.informative.h"
#include "act.movement.h"
#include "obj_edit.h"
#include "graph.h"
#include "alias.h"
#include "spells.h"
#include "interpreter.h"
#include "fight.h"
#include "races.h"
#include "class.h"
#include "constants.h"
#include "shop.h"
#include "feats.h"
#include "guild.h"
#include "dg_scripts.h"
#include "objsave.h"
#include "mail.h"
#include "clan.h"

/* local functions */
static int has_scanner(struct char_data *ch);
static void boost_obj(struct obj_data *obj, struct char_data *ch, int type);
static void handle_revert(struct char_data *ch, uint64_t add, double mult);
static void handle_transform(struct char_data *ch, int64_t add, double mult, double drain);
static int perform_group(struct char_data *ch, struct char_data *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl);
static void print_group(struct char_data *ch);
static void check_eq(struct char_data *ch);
static int spell_in_book(struct obj_data *obj, int spellnum);
static int spell_in_scroll(struct obj_data *obj, int spellnum);
static int spell_in_domain(struct char_data *ch, int spellnum);
static void show_clan_info(struct char_data *ch);

// definitions
void log_imm_action(char *messg, ...)
{

   FILE *fl;
   const char *filename;
   struct stat fbuf;

   filename = REQUEST_FILE;

     if (stat(filename, &fbuf) < 0) {
      perror("SYSERR: Can't stat() file");
      /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
      return;
     }
     if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
      return;
     }
     if (!(fl = fopen(filename, "a"))) {
      perror("SYSERR: log_imm_action");
       /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

      return;
     }
    time_t ct = time(0);
    char *time_s = asctime(localtime(&ct));

    va_list args;

    va_start(args, messg);
    time_s[strlen(time_s) - 1] = '\0';

    fprintf(fl, "%-15.15s :: ", time_s + 4);
    vfprintf(fl, messg, args);
    fprintf(fl, "\n");
    va_end(args);

    fclose(fl);

}

void log_custom(struct descriptor_data *d, struct obj_data *obj)
{
   FILE *fl;
   const char *filename;
   struct stat fbuf;

   filename = CUSTOM_FILE;

     if (stat(filename, &fbuf) < 0) {
      perror("SYSERR: Can't stat() file");
      /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
      return;
     }
     if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
      return;
     }
     if (!(fl = fopen(filename, "a"))) {
      perror("SYSERR: log_custom");
       /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

      return;
     }

    fprintf(fl, "@D[@cUser@W: @R%-20s @cName@W: @C%-20s @cCustom@W: @Y%s@D]\n", GET_USER(d->character), GET_NAME(d->character), obj->short_description);
    fclose(fl);
}

/* Used by do_rpp for soft-cap */
void bring_to_cap(struct char_data *ch)
{
 int skippl = FALSE, skipki = FALSE, skipst = FALSE, mult = 1;

 if (!soft_cap(ch, 0)) {
  skippl = TRUE;
 } if (!soft_cap(ch, 1)) {
  skipki = TRUE;
 } if (!soft_cap(ch, 2)) {
  skipst = TRUE;
 }
/* Rillao: transloc, add new transes here */
  if (IS_BIO(ch)) {
   if (PLR_FLAGGED(ch, PLR_TRANS1)) {
    mult = 2;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
    mult = 3;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    mult = 3.5;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
    mult = 4;
   }
  } else if (IS_MAJIN(ch)) {
   if (PLR_FLAGGED(ch, PLR_TRANS1)) {
    mult = 2;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
    mult = 3;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    mult = 4.5;
   }
  } else if (IS_TRUFFLE(ch)) {
   if (PLR_FLAGGED(ch, PLR_TRANS1)) {
    mult = 3;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
    mult = 4;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    mult = 5;
   }
  }
if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
 if (skippl == FALSE) {
   int64_t base = GET_BASE_PL(ch), diff;
   if ((base < GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 2000000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 2000000;
   }
   if ((base < GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 1000000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 1000000;
   }
   if ((base < GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 300000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 300000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 100000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 100000;
   }
   if ((base < GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 40000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 40000;
   }
   if ((base < GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 25000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 25000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 500;
   }
 } /* End powerlevel section */

 if (skipki == FALSE) {
   int64_t base = GET_BASE_KI(ch), diff;
   if ((base < GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 2000000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 2000000;
   }
   if ((base < GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 1000000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 1000000;
   }
   if ((base < GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 300000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 300000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 100000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 100000;
   }
   if ((base < GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 40000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 40000;
   }
   if ((base < GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 25000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 25000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 500;
   }
 } /* End ki section */

 if (skipst == FALSE) {
   int64_t base = GET_BASE_ST(ch), diff;
   if ((base < GET_LEVEL(ch) * 2000000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 2000000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 2000000;
   }
   if ((base < GET_LEVEL(ch) * 1000000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 1000000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 1000000;
   }
   if ((base < GET_LEVEL(ch) * 300000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 300000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 300000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 100000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 100000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 100000;
   }
   if ((base < GET_LEVEL(ch) * 40000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 40000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 40000;
   }
   if ((base < GET_LEVEL(ch) * 25000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 25000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 25000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 500;
   }
 } /* End stamina section */
}
 else {
  if (skippl == FALSE) {
   int64_t base = GET_BASE_PL(ch), diff;
   if ((base < GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 1500000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 1500000;
   }
   if ((base < GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 800000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 800000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 200000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 200000;
   }
   if ((base < GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 80000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 80000;
   }
   if ((base < GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 20000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 20000;
   }
   if ((base < GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 15000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 15000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_PL(ch);
    GET_MAX_HIT(ch) += diff * mult;
    GET_BASE_PL(ch) = GET_LEVEL(ch) * 500;
   }
 } /* End Powerlevel Section */

 if (skipki == FALSE) {
   int64_t base = GET_BASE_KI(ch), diff;
   if ((base < GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 1500000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 1500000;
   }
   if ((base < GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 800000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 800000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 200000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 200000;
   }
   if ((base < GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 80000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 80000;
   }
   if ((base < GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 20000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 20000;
   }
   if ((base < GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 15000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 15000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_KI(ch);
    GET_MAX_MANA(ch) += diff * mult;
    GET_BASE_KI(ch) = GET_LEVEL(ch) * 500;
   }
 } /* End Ki Section */

 if (skipst == FALSE) {
   int64_t base = GET_BASE_ST(ch), diff;
   if ((base < GET_LEVEL(ch) * 1500000) && GET_LEVEL(ch) > 90 && GET_LEVEL(ch) <= 99) {
    diff = (GET_LEVEL(ch) * 1500000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 1500000;
   }
   if ((base < GET_LEVEL(ch) * 800000) && GET_LEVEL(ch) > 80 && GET_LEVEL(ch) <= 90) {
    diff = (GET_LEVEL(ch) * 800000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 800000;
   }
   if ((base < GET_LEVEL(ch) * 250000) && GET_LEVEL(ch) > 70 && GET_LEVEL(ch) <= 80) {
    diff = (GET_LEVEL(ch) * 250000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 250000;
   }
   if ((base < GET_LEVEL(ch) * 200000) && GET_LEVEL(ch) > 60 && GET_LEVEL(ch) <= 70) {
    diff = (GET_LEVEL(ch) * 200000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 200000;
   }
   if ((base < GET_LEVEL(ch) * 80000) && GET_LEVEL(ch) > 50 && GET_LEVEL(ch) <= 60) {
    diff = (GET_LEVEL(ch) * 80000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 80000;
   }
   if ((base < GET_LEVEL(ch) * 20000) && GET_LEVEL(ch) > 40 && GET_LEVEL(ch) <= 50) {
    diff = (GET_LEVEL(ch) * 20000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 20000;
   }
   if ((base < GET_LEVEL(ch) * 15000) && GET_LEVEL(ch) > 30 && GET_LEVEL(ch) <= 40) {
    diff = (GET_LEVEL(ch) * 15000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 15000;
   }
   if ((base < GET_LEVEL(ch) * 5000) && GET_LEVEL(ch) > 20 && GET_LEVEL(ch) <= 30) {
    diff = (GET_LEVEL(ch) * 5000) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 5000;
   }
   if ((base < GET_LEVEL(ch) * 1500) && GET_LEVEL(ch) > 10 && GET_LEVEL(ch) <= 20) {
    diff = (GET_LEVEL(ch) * 1500) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 1500;
   }
   if ((base < GET_LEVEL(ch) * 500) && GET_LEVEL(ch) <= 10) {
    diff = (GET_LEVEL(ch) * 500) - GET_BASE_ST(ch);
    GET_MAX_MOVE(ch) += diff * mult;
    GET_BASE_ST(ch) = GET_LEVEL(ch) * 500;
   }
  } /* End Stamina Section */
 }

}

/* Let's Reward Those Roleplayers! - Iovan*/
ACMD(do_rpp)
{

 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 int tnlcost = 1, revcost = 1, selection = 0, pay = 0, bpay = 0;
 int max_choice = 15; /* Controls the maximum number of menu choices */
 struct obj_data *obj;
 
 half_chop(argument, arg, arg2);
 
 if (IS_NPC(ch)) {
  return;
 }
 
 revcost = revcost * (GET_LEVEL(ch) / 15);
 tnlcost = 1 + (tnlcost * (GET_LEVEL(ch) / 40));
 
 if (revcost < 1) {
  revcost = 1;
 } if (tnlcost < 1) {
  tnlcost = 1;
 }

 if (PLR_FLAGGED(ch, PLR_PDEATH)) {
  revcost *= 6;
 }
 
 if (!*arg) { /* Display menu */
  send_to_char(ch, "@C                             Rewards Menu\n");
  send_to_char(ch, "@b  ------------------------------------------------------------------\n");
  send_to_char(ch, "  @C1@D)@R Disabled            @D[@G -- RPP @D]  @C2@D)@R Disabled              @D[@G -- RPP @D]@n\n");
  send_to_char(ch, "  @C3@D)@c Custom Equipment    @D[@G 30 RPP @D]  @C4@D)@c Alignment Change      @D[@G 20 RPP @D]\n");
  send_to_char(ch, "  @C5@D)@c 7,500 zenni         @D[@G  1 RPP @D]  @C6@D)@c +2 To A Stat          @D[@G  2 RPP @D]\n");
  send_to_char(ch, "  @C7@D)@c +750 PS             @D[@G  4 RPP @D]  @C8@D)@c Revival               @D[@G%3d RPP @D]\n", revcost);
  send_to_char(ch, "  @C9@D)@c 50%s TNL Exp         @D[@G%3d RPP @D] @C10@D)@c Aura Change           @D[@G  2 RPP @D]\n", "%", tnlcost);
  send_to_char(ch, " @C11@D)@c Reach Softcap       @D[@G  6 RPP @D] @C12@D)@c RPP Store             @D[@G ?? RPP @D]\n");
  send_to_char(ch, " @C13@D)@c Extra Feature       @D[@G  1 RPP @D] @C14@D)@c Restring Equipment    @D[@G  1 RPP @D]\n");
  send_to_char(ch, " @C15@D)@c Extra Skillslot     @D[@G  5 RPP @D] @C16@D)@R Disabled              @D[@G -- RPP @D]@n\n");
  send_to_char(ch, "@b  ------------------------------------------------------------------@n\n");
  send_to_char(ch, "@D                           [@YYour RPP@D:@G %3d@D]@n\n", GET_RP(ch));
 // send_to_char(ch, "@D                           [@YRPP Bank@D:@G %3d@D]@n\n", GET_RBANK(ch));
  send_to_char(ch, "\nSyntax: rpp (num)\n");
  return;
 }

   /* What choice did they make? */
   
   selection = atoi(arg);
   
   if (selection <= 0 || selection > max_choice) {
    send_to_char(ch, "You must choose a number from the menu. Enter the command again with no arguments for the menu.\r\n");
	return;
   }
  
   /* Instant Reward Section*/
  if (selection > 2) {
   
   if (selection == 3) { /* Custom Equipment Construction */
    if (GET_RP(ch) < 30) {
     send_to_char(ch, "You need at least 30 RPP to initiate a custom equipment build.\r\n");
     return;
    } else {
     STATE(ch->desc) = CON_POBJ;
     ch->desc->obj_name = strdup("Generic Armor Vest");
     ch->desc->obj_short = strdup("@cGeneric @DArmor @WVest@n");
     ch->desc->obj_long = strdup("@wA @cgeneric @Darmor @Wvest@w is lying here@n");
     ch->desc->obj_type = 1;
     ch->desc->obj_weapon = 0;
     disp_custom_menu(ch->desc);
     ch->desc->obj_editflag = EDIT_CUSTOM;
     ch->desc->obj_editval = EDIT_CUSTOM_MAIN;
     return;
    }
   }
   if (selection == 4) {     /* Simple Align Change*/
    pay = 20;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
	} else {
	 if (!*arg2) {
	  send_to_char(ch, "What do you want to change your alignment to? (evil, sorta-evil, neutral, sorta-good, good)");
	  return;
	 }
	 if (!strcasecmp(arg2, "evil")) {
          send_to_char(ch, "You change your alignment to Evil.\r\n");
          GET_ALIGNMENT(ch) = -750;
	 } else if (!strcasecmp(arg2, "sorta-evil")) {
          send_to_char(ch, "You change your alignment to Sorta Evil.\r\n");
          GET_ALIGNMENT(ch) = -50;
	 } else if (!strcasecmp(arg2, "neutral")) {
          send_to_char(ch, "You change your alignment to Neutral.\r\n");
          GET_ALIGNMENT(ch) = 0;
 	 } else if (!strcasecmp(arg2, "sorta-good")) {
          send_to_char(ch, "You change your alignment to Sorta Good.\r\n");
          GET_ALIGNMENT(ch) = 51;
	 } else if (!strcasecmp(arg2, "good")) {
          send_to_char(ch, "You change your alignment to Good.\r\n");
          GET_ALIGNMENT(ch) = 300;
	 } else {
          send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
          return;
     }
	
    } /* Can pay for it */
   } /* End Simple Align Change */
   
   if (selection == 5) { /*Simple Zenni Reward*/
    pay = 1;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
	} else {
     GET_BANK_GOLD(ch) += 7500;
     send_to_char(ch, "Your bank zenni has been increased by 7,500\r\n");
    } /* Can pay for it */
   } /* End Simple Zenni Reward */
   
   if (selection == 6) {     /* Simple Stat Change*/
    pay = 2;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
	} else {
	 if (!*arg2) {
	  send_to_char(ch, "What stat? (str, con, int, wis, spd, agl)");
	  return;
	 }
	 if (!strcasecmp(arg2, "str")) {
       if (GET_BONUS(ch, BONUS_WIMP) > 0 && ch->real_abils.str >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_STR(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your strength by 2.\r\n");
       ch->real_abils.str += 2;
     } else if (!strcasecmp(arg2, "con")) {
       if (GET_BONUS(ch, BONUS_FRAIL) > 0 && ch->real_abils.con >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_CON(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your constitution by 2.\r\n");
       ch->real_abils.con += 2;
     } else if (!strcasecmp(arg2, "int")) {
       if (GET_BONUS(ch, BONUS_DULL) > 0 && ch->real_abils.intel >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_INT(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your intelligence by 2.\r\n");
       ch->real_abils.intel += 2;
     } else if (!strcasecmp(arg2, "wis")) {
       if (GET_BONUS(ch, BONUS_FOOLISH) > 0 && ch->real_abils.wis >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_WIS(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your wisdom by 2.\r\n");
       ch->real_abils.wis += 2;
     } else if (!strcasecmp(arg2, "spd")) {
       if (GET_BONUS(ch, BONUS_SLOW) > 0 && ch->real_abils.cha >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_CHA(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your speed by 2.\r\n");
       ch->real_abils.cha += 2;
     } else if (!strcasecmp(arg2, "agl")) {
       if (GET_BONUS(ch, BONUS_CLUMSY) > 0 && ch->real_abils.dex >= 45) {
        send_to_char(ch, "You can't because that stat maxes at 45 due to a trait negative.\r\n");
        return;
       } else if (GET_DEX(ch) >= 100) {
        send_to_char(ch, "100 is the maximum for any stat.\r\n");
        return;
       }
       send_to_char(ch, "You increase your speed by 2.\r\n");
       ch->real_abils.dex += 2;
	 } else {
       send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
       return;
     }
	
	} /* Can pay for it */
   } /* End Simple Align Change */   

   if (selection == 7) { /*Simple PS Reward*/
    pay = 4;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
	} else {
     GET_PRACTICES(ch, GET_CLASS(ch)) += 750;
     send_to_char(ch, "Your practices have been increased by 750\r\n");
    } /* Can pay for it */
   } /* End Simple Zenni Reward */

   if (selection == 8) { /* Simple Revival Reward */
    pay = revcost;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
    } else if (!AFF_FLAGGED(ch, AFF_SPIRIT)) {
     send_to_char(ch, "You aren't even dead!");
     return;
	} else {
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ETHEREAL);
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SPIRIT);
     GET_HIT(ch) = gear_pl(ch);
     GET_MANA(ch) = GET_MAX_MANA(ch);
     GET_MOVE(ch) = GET_MAX_MOVE(ch);
     GET_LIMBCOND(ch, 1) = 100;
     GET_LIMBCOND(ch, 2) = 100;
     GET_LIMBCOND(ch, 3) = 100;
     GET_LIMBCOND(ch, 4) = 100;
     SET_BIT_AR(PLR_FLAGS(ch), PLR_HEAD);
     REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_PDEATH);
     char_from_room(ch);
     if (GET_DROOM(ch) != NOWHERE && GET_DROOM(ch) != 0 && GET_DROOM(ch) != 1) {
      char_to_room(ch, real_room(GET_DROOM(ch)));
     } else if (IS_ROSHI(ch)) {
      char_to_room(ch, real_room(1130));
     } else if (IS_KABITO(ch)) {
      char_to_room(ch, real_room(12098));
     } else if (IS_NAIL(ch)) {
      char_to_room(ch, real_room(11683));
     } else if (IS_BARDOCK(ch)) {
      char_to_room(ch, real_room(2268));
     } else if (IS_KRANE(ch)) {
      char_to_room(ch, real_room(13009));
     } else if (IS_TAPION(ch)) {
      char_to_room(ch, real_room(8231));
     } else if (IS_PICCOLO(ch)) {
      char_to_room(ch, real_room(1659));
     } else if (IS_ANDSIX(ch)) {
      char_to_room(ch, real_room(1713));
     } else if (IS_DABURA(ch)) {
      char_to_room(ch, real_room(6486));
     } else if (IS_FRIEZA(ch)) {
      char_to_room(ch, real_room(4282));
     } else if (IS_GINYU(ch)) {
      char_to_room(ch, real_room(4289));
     } else if (IS_JINTO(ch)) {
      char_to_room(ch, real_room(3499));
     } else if (IS_TSUNA(ch)) {
      char_to_room(ch, real_room(15000));
     } else if (IS_KURZAK(ch)) {
      char_to_room(ch, real_room(16100));
     } else {
      char_to_room(ch, real_room(300));
      send_to_imm("ERROR: Player %s without acceptable sensei.\r\n", GET_NAME(ch));
     }
     look_at_room(IN_ROOM(ch), ch, 0);
     GET_DTIME(ch) = 0;

     act("$n's body forms in a pool of @Bblue light@n.", TRUE, ch, 0, 0, TO_ROOM);
     send_to_char(ch, "You have been revived.\r\n");
    if (GET_LEVEL(ch) > 9) {
    int losschance = axion_dice(0);
     if (GET_LEVEL(ch) > 9) {
        send_to_char(ch, "@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength, constitution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        int str = -8, intel = -8, wis = -8, spd = -8, con = -8, agl = -8;
        if (ch->real_abils.str <= 16) {
         str = -4;
        }
        if (ch->real_abils.intel <= 16) {
         intel = -4;
        }
        if (ch->real_abils.cha <= 16) {
         spd = -4;
        }
        if (ch->real_abils.dex <= 16) {
         agl = -4;
        }
        if (ch->real_abils.wis <= 16) {
         wis = -4;
        }
        if (ch->real_abils.con <= 16) {
         con = -4;
        }
        assign_affect(ch, AFF_WEAKENED_STATE, SKILL_WARP, 100, str, con, intel, agl, wis, spd);
      } if (losschance >= 100) {
        int psloss = rand_number(100, 300);
        GET_PRACTICES(ch, GET_CLASS(ch)) -= psloss;
        send_to_char(ch, "@R...and a loss of @r%d@R PS!@n", psloss);
        if (GET_PRACTICES(ch, GET_CLASS(ch)) < 0) {
         GET_PRACTICES(ch, GET_CLASS(ch)) = 0;
        }
      }
     }
    } /* Can pay for it */
   } /* End Simple Revival Reward */

   if (selection == 9) { /*Simple Exp Reward*/
    pay = tnlcost;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
    } else if (GET_LEVEL(ch) >= 100) {
     send_to_char(ch, "You can not buy experience anymore at your level. I think you know why.\r\n");
     return;
    } else if (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) < 0) {
     send_to_char(ch, "You can not buy experience anymore UNTIL you level.\r\n");
     return;
	} else {
     GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * .52;
     send_to_char(ch, "You gained 50%s of the entire experience needed for your next level.\r\n", "%");
    } /* Can pay for it */
   } /* End Simple Exp Reward */

   if (selection == 10) {     /* Simple Aura Change*/
    pay = 2;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
	} else {
	 if (!*arg2) {
	  send_to_char(ch, "Change your aura to what? (white, blue, red, green, pink, purple, yellow, black, orange)");
	  return;
	 }
	 if (!strcasecmp(arg2, "white")) {
      GET_AURA(ch) = 0;
      send_to_char(ch, "You change your aura to white.\r\n");
	 } else if (!strcasecmp(arg2, "blue")) {
      GET_AURA(ch) = 1;
      send_to_char(ch, "You change your aura to blue.\r\n");
	 } else if (!strcasecmp(arg2, "red")) {
      GET_AURA(ch) = 2;
      send_to_char(ch, "You change your aura to red.\r\n");
	 } else if (!strcasecmp(arg2, "green")) {
      GET_AURA(ch) = 3;
      send_to_char(ch, "You change your aura to green.\r\n");
	 } else if (!strcasecmp(arg2, "pink")) {
      GET_AURA(ch) = 4;
      send_to_char(ch, "You change your aura to pink.\r\n");
	 } else if (!strcasecmp(arg2, "purple")) {
      GET_AURA(ch) = 5;
      send_to_char(ch, "You change your aura to purple.\r\n");
	 } else if (!strcasecmp(arg2, "yellow")) {
      GET_AURA(ch) = 6;
      send_to_char(ch, "You change your aura to yellow.\r\n");
	 } else if (!strcasecmp(arg2, "black")) {
      GET_AURA(ch) = 7;
      send_to_char(ch, "You change your aura to black.\r\n");
	 } else if (!strcasecmp(arg2, "orange")) {
      GET_AURA(ch) = 8;
      send_to_char(ch, "You change your aura to orange.\r\n");
	 } else {
       send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
       return;
     }
	
	} /* Can pay for it */
   } /* End Simple Aura Change */

   if (selection == 11) {     /* Simple Soft-cap Reward*/
    pay = 6;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
	 return;
    } else if (GET_LEVEL(ch) >= 100) {
         send_to_char(ch, "You can't use this at level 100.\r\n");
         return;
    } else if (IS_ARLIAN(ch)) {
         send_to_char(ch, "This is not available to bugs.\r\n");
         return;
    } else {
     if (!soft_cap(ch, 0) && !soft_cap(ch, 1) && !soft_cap(ch, 2)) {
       send_to_char(ch, "You are already above your softcap for this level.\r\n");
       return;
     }
     bring_to_cap(ch);
    } /* Can pay for it */
   } /* End Simple Soft-cap Reward */
   
   if (selection == 12) {
    if (!*arg2) {
     disp_rpp_store(ch);
     return;
    } else if (atoi(arg2) <= 0) {
     send_to_char(ch, "That is not a choice in the RPP store!\r\n");
     return;
    } else {
     int choice = atoi(arg2);
     handle_rpp_store(ch, choice);
     return;
    }
   }

   if (selection == 13) { /* Extra Feature Reward*/
    rpp_feature(ch, arg2);
    return;
   } /* End Extra Feature Reward */

   if (selection == 14) { /* Restring equipment reward */
    pay = 1;
    if (GET_RP(ch) < pay) {
     send_to_char(ch, "You need at least 1 RPP to initiate an equipment restring.\r\n");
     return;
    } else if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
     send_to_char(ch, "You don't have a that equipment to restring in your inventory.\r\n");
     send_to_char(ch, "Syntax: rpp 14 (obj name)\r\n");
     return;
    } else if (OBJ_FLAGGED(obj, ITEM_CUSTOM)) {
     send_to_char(ch, "You can not restring a custom piece. Why? Cause I say so. :P\r\n");
     return;
    } else {
     STATE(ch->desc) = CON_POBJ;
     char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH], thelong[MAX_INPUT_LENGTH];

     *thename = '\0';
     *theshort = '\0';
     *thelong = '\0';

     sprintf(thename, "%s", obj->name);
     sprintf(theshort, "%s", obj->short_description);
     sprintf(thelong, "%s", obj->description);

     ch->desc->obj_name = strdup(thename);
     ch->desc->obj_was = strdup(theshort);
     ch->desc->obj_short = strdup(theshort);
     ch->desc->obj_long = strdup(thelong);
     ch->desc->obj_point = obj;
     ch->desc->obj_type = 1;
     ch->desc->obj_weapon = 0;
     disp_restring_menu(ch->desc);
     ch->desc->obj_editflag = EDIT_RESTRING;
     ch->desc->obj_editval = EDIT_RESTRING_MAIN;
     return;
    }
   } /* End equipment restring reward */
  
  if (selection == 15) { /* Skillslot Reward */
    pay = 5;
    if (GET_RP(ch) < pay) {
	 send_to_char(ch, "You do not have enough RPP in your bank for that selection.\r\n");
	 return;
    } else if (GET_BONUS(ch, BONUS_GMEMORY) && GET_SLOTS(ch) >= 65) {
         send_to_char(ch, "You are already at your skillslot cap.\r\n");
         return;
    } else if (!GET_BONUS(ch, BONUS_GMEMORY) && (GET_SLOTS(ch) >= 60)) {
         send_to_char(ch, "You are already at your skillslot cap.\r\n");
         return;
    } else {
     GET_SLOTS(ch) += 1;
    } /* Can pay for it */
  } /*End Skillslot Reward */

  if (selection == 16) { /* DB Spawn Reward */
    pay = 5000;
    if (GET_RP(ch) < pay) {
         send_to_char(ch, "You do not have enough RPP in your bank for that selection.\r\n");
         return;
    } else {
     int found = FALSE;
     struct obj_data *k = NULL;
     for (k = object_list; k; k = k->next) {
      if (OBJ_FLAGGED(k, ITEM_FORGED)) {
       continue;
      }
      if (GET_OBJ_VNUM(k) == 20) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 21) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 22) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 23) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 24) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 25) {
       found = TRUE;
      } else if (GET_OBJ_VNUM(k) == 26) {
       found = TRUE;
      }
     }
     if (found == FALSE) {
      send_to_char(ch, "You have reduced the Dragon Ball wait by a whole real life day!\r\n");
      send_to_all("%s has just reduced the Dragon Ball wait by a whole real life day!\r\n", GET_NAME(ch));
      dballtime -= 86400;
      if (dballtime <= 0) {
       dballtime = 1;
      }    
     } else if (SELFISHMETER >= 10) {
      send_to_char(ch, "Sorry, it seems there there are several powers interfering with the Dragon Balls.\r\n");
      return;
     } else {
      send_to_char(ch, "Sorry, but there is already a set of Dragon Balls in existence.\r\n");
      return;
     }
    } /* Can pay for it */
  }
  
  } /* End Instant Reward Section */

   /* Resuest Only Section */
  if (selection <= 2) {
   FILE *fl;
   const char *filename;
   struct stat fbuf;

   filename = REQUEST_FILE;
   
   if (selection == 1) {
    pay = 6500;
    if (GET_RP(ch) < pay) {
        send_to_char(ch, "Nice try but you don't have enough RPP for that.\r\n");
        return;
    } else {
      send_to_char(ch, "You now have an Excel House Capsule!\r\n");
      struct obj_data *hobj = read_object(6, VIRTUAL);
      obj_to_char(hobj, ch);
      GET_RP(ch) -= pay;
      ch->desc->rpp = GET_RP(ch);
      userWrite(ch->desc, 0, 0, 0, "index");
      save_char(ch);
      send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
      send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), pay);
      return;
    }    
   } else if (selection == 2) {
    return;
    pay = 200;
   }
   
   if (GET_RP(ch) < pay) {
    send_to_char(ch, "Nice try but you don't have enough RPP for that.\r\n");
	return;
   }
   
     if (stat(filename, &fbuf) < 0) {
      perror("SYSERR: Can't stat() file");
      /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
      return;
     }
     if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
      send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
      return;
     }
     if (!(fl = fopen(filename, "a"))) {
      perror("SYSERR: do_reward_request");
       /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

      send_to_char(ch, "Could not open the file.  Sorry.\r\n");
      return;
     }
   if (selection == 1) {
      fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", GET_NAME(ch), "House");
      send_to_imm("RPP Request: %s paid for house", GET_NAME(ch));
      BOARDNEWCOD = time(0);
      save_mud_time(&time_info);
   } else if (selection == 2) {
      fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", GET_NAME(ch), "Custom Skill");
      send_to_imm("RPP Request: %s paid for Custom Skill, uhoh spaggettios", GET_NAME(ch));
      BOARDNEWCOD = time(0);
      save_mud_time(&time_info);
   }
   GET_RP(ch) -= pay;
   ch->desc->rpp = GET_RP(ch);
   userWrite(ch->desc, 0, 0, 0, "index");
   save_char(ch);
   send_to_char(ch, "@R%d@W RPP paid for your selection. An immortal will address the request soon enough. Be patient.@n\r\n", pay);

   fclose(fl);
  } /* End Request Only Section */

  /* Pay for purchases here */
  if (selection >= 4 && selection < 12 && pay > 0) {
     GET_RP(ch) -= pay;
     ch->desc->rpp = GET_RP(ch);
     userWrite(ch->desc, 0, 0, 0, "index");
     save_char(ch);
      send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
      send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), pay);
  }
  
    if (selection > 12 && pay > 0) {
     GET_RP(ch) -= pay;
     ch->desc->rpp = GET_RP(ch);
     userWrite(ch->desc, 0, 0, 0, "index");
     save_char(ch);
      send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", bpay);
      send_to_imm("RPP Purchase: %s %d", GET_NAME(ch), bpay);
  }
    
}

ACMD(do_commune)
{

 if (IS_NPC(ch))
  return;
  
 if (!know_skill(ch, SKILL_COMMUNE)) {
  return;
 }
  
 if (GET_MOVE(ch) >= GET_MAX_MOVE(ch)) {
  send_to_char(ch, "Your stamina is already at full.\r\n");
  return;
 }
 
 int prob = GET_SKILL(ch, SKILL_COMMUNE), perc = axion_dice(0);
 int64_t cost = GET_MAX_MOVE(ch) * .05;
 
 if (GET_MANA(ch) < cost) {
  send_to_char(ch, "You do not have enough ki to commune with the Eldritch Star.\r\n");
  return;
 }
 if (prob < perc) {
  GET_MANA(ch) -= cost;
        reveal_hiding(ch, 0);
  act("@cYou close your eyes and try to commune with the Eldritch Star. You are unable to concentrate though.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@W$n closes $s eyes for a moment. Then $e reopens them and frowns.@n", TRUE, ch, 0, 0, TO_ROOM);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
 else {
  GET_MANA(ch) -= cost;
  GET_MOVE(ch) += cost;
  if (GET_MOVE(ch) > GET_MAX_MOVE(ch)) {
   GET_MOVE(ch) = GET_MAX_MOVE(ch);
  }
        reveal_hiding(ch, 0);
  act("@cYou close your eyes and commune with the Eldritch Star spiritually. You feel your stamina replenish some.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@W$n closes $s eyes for a moment. Then $e reopens them and smiles.@n", TRUE, ch, 0, 0, TO_ROOM);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
}

ACMD(do_willpower)
{

 int fail = FALSE;

 if (IS_NPC(ch))
  return;

 if (MAJINIZED(ch) <= 0) {
  send_to_char(ch, "You are not majinized and have no need to reclaim full control of your own will.\r\n");
  return;
 }
 else {
  if (GET_PRACTICES(ch, GET_CLASS(ch)) < 100 && GET_LEVEL(ch) < 100) {
   send_to_char(ch, "You do not have enough PS to focus your attempt to break free.\r\n");
   fail = TRUE;
  }
  if (GET_PRACTICES(ch, GET_CLASS(ch)) < 200 && GET_LEVEL(ch) >= 100) {
   send_to_char(ch, "You do not have enough PS to focus your attempt to break free.\r\n");
   fail = TRUE;
  }
  if (GET_EXP(ch) < level_exp(ch, GET_LEVEL(ch) + 1) && GET_LEVEL(ch) < 100) {
   send_to_char(ch, "You need a full level's worth of experience stored up to try and break free.\r\n");
   fail = TRUE;
  }
  if (fail == TRUE) {
   return;
  }
  else {
   GET_EXP(ch) = 0;
   GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
   if (GET_LEVEL(ch) >= 100) {
      GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
   }
   if (rand_number(10, 100) - GET_INT(ch) > 60) {
          reveal_hiding(ch, 0);
    act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments you give up, having failed to overcome the majinization!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments $n seems to give up and the commotion dies down.@n", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
   GET_EXP(ch) = 0;
   GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
   if (GET_LEVEL(ch) >= 100) {
      GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
   }
          reveal_hiding(ch, 0);
    act("@WYou focus all your knowledge and will on breaking free. Dark purple energy swirls around your body and the M on your forehead burns brightly. After a few moments the ground splits beneath you and while letting out a piercing scream the M disappears from your forehead! You are free while still keeping the boost you had recieved from the majinization!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n focuses hard with $s eyes closed. Dark purple energy swirls around $s body and the M on $s head burns brightly. After a few moments the ground beneath $n splits and $e lets out a piercing scream. The M on $s forehead disappears!@n", TRUE, ch, 0, 0, TO_ROOM);
    MAJINIZED(ch) = 3;
    return;
   }
  }
 }
}

ACMD(do_grapple)
{

 /*if (IS_NPC(ch))
  return;*/

 if (!know_skill(ch, SKILL_GRAPPLE)) {
  return;
 }

 if (PLR_FLAGGED(ch, PLR_THANDW)) {
  send_to_char(ch, "Your are too busy wielding your weapon with two hands!\r\n");
  return;
 }

 if (ABSORBING(ch)) {
  send_to_char(ch, "You are currently absorbing from someone!\r\n");
  return;
 }
 if (ABSORBBY(ch)) {
  send_to_char(ch, "You are currently being absorbed by someone! Try 'escape'!\r\n");
  return;
 }

 if (GRAPPLING(ch) != NULL) {
  act("@RYou stop grappling with @r$N@R!@n", TRUE, ch, 0, GRAPPLING(ch), TO_CHAR);
  act("@r$n@R stops grappling with @rYOU!!@n", TRUE, ch, 0, GRAPPLING(ch), TO_VICT);
  act("@r$n@R stops grappling with @r$N@R!@n", TRUE, ch, 0, GRAPPLING(ch), TO_NOTVICT);
  GRAPTYPE(GRAPPLING(ch)) = -1;
  GRAPPLED(GRAPPLING(ch)) = NULL;
  GRAPPLING(ch) = NULL;
  GRAPTYPE(ch) = -1;
  return;
 }

 if (GRAPPLED(ch) != NULL) {
  send_to_char(ch, "You are currently a victim of grappling! Try 'escape' to break free!\r\n");
  return;
 }

 if (!HAS_ARMS(ch)) {
  send_to_char(ch, "You have no available arms!\r\n");
  return;
 }

 struct char_data *vict;
 char arg[200], arg2[200];

 two_arguments(argument, arg, arg2);

 if (!*arg || !*arg2) {
  send_to_char(ch, "Syntax: grapple (target) (hold | choke | grab)\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  if (!FIGHTING(ch)) {
   send_to_char(ch, "That target isn't here.\r\n");
   return;
  } else {
   vict = FIGHTING(ch);
  }
 }

 if (!can_kill(ch, vict, NULL, 0)) {
  return;
 }

 if (AFF_FLAGGED(vict, AFF_KNOCKED)) {
  send_to_char(ch, "They are unconcious. What would be the point?\r\n");
  return;
 }

 if (GRAPPLED(vict)) {
  send_to_char(ch, "They are currently in someone else's grasp!\r\n");
  return;
 }

 if (ABSORBBY(vict)) {
  send_to_char(ch, "They are currently in someone else's grasp!\r\n");
  return;
 }

 if (ABSORBING(vict)) {
  send_to_char(ch, "They are currently absorbing from someone!\r\n");
  return;
 }


 int pass = FALSE;

 if (!strcasecmp("hold", arg2) || !strcasecmp("choke", arg2) || !strcasecmp("grab", arg2) || !strcasecmp("wrap", arg2)) {
  pass = TRUE;
  int perc = GET_SKILL(ch, SKILL_GRAPPLE), prob = axion_dice(0), cost = GET_MAX_MOVE(ch) / 100;

  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina to grapple!\r\n");
   return;
  }

  if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) && GET_MOVE(vict) >= 1 && GET_POS(vict) != POS_SLEEPING) {
   if (!AFF_FLAGGED(ch, AFF_ZANZOKEN) || (AFF_FLAGGED(ch, AFF_ZANZOKEN) && GET_SPEEDI(ch) + rand_number(1, 5) < GET_SPEEDI(vict) + rand_number(1, 5))) {
           reveal_hiding(ch, 0);
     act("@C$N@c disappears, avoiding your grapple attempt before reappearing!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou disappear, avoiding @C$n's@c grapple attempt before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, avoiding @C$n's@c grapple attempt before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     if (AFF_FLAGGED(ch, AFF_ZANZOKEN)) {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
     }
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     GET_MOVE(ch) -= cost;
     WAIT_STATE(ch, PULSE_4SEC);
     return;
   }
   else {
           reveal_hiding(ch, 0);
     act("@C$N@c disappears, trying to avoid your grapple but your zanzoken is faster!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou zanzoken to avoid the grapple attempt but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, trying to avoid @C$n's@c grapple attempt but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
   }
  }

  if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2) {
   perc += 5;
  } else if (GET_SPEEDI(ch) > GET_SPEEDI(vict)) {
   perc += 2;
  } else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict)) {
   perc -= 5;
  } else if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
   perc -= 2;
  }

  if ((GET_HIT(ch) * 0.02) * GET_STR(ch) < (GET_HIT(vict) * 0.01) * GET_STR(vict)) {
         reveal_hiding(ch, 0);
   act("@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if ((GET_HIT(ch) * 0.01) * GET_STR(ch) < (GET_HIT(vict) * 0.01) * GET_STR(vict) && rand_number(1, 4) == 1) {
         reveal_hiding(ch, 0);
   act("@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if (perc < prob) {
         reveal_hiding(ch, 0);
   act("@RYou try to grapple with @r$N@R, but $E manages to avoid it!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R tries to grapple with YOU, but you manage to avoid it!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R tries to grapple with @r$N@R, but $E manages to avoid it!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if (!HAS_ARMS(vict) && !strcasecmp("grab", arg2)) {
    send_to_char(ch, "They don't even have an arm to grab onto!\r\n");
    return;
  } else if (!strcasecmp("hold", arg2)) {
         reveal_hiding(ch, 0);
   act("@RYou rush at @r$N@R and manage to get $M in a hold from behind!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R rushes at YOU and manages to get you in a hold from behind!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R rushes at @r$N@R and manages to get $M in a hold from behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);

   /* Let's grapple! */
   GRAPPLING(ch) = vict;
   GRAPTYPE(ch) = 1;
   GRAPPLED(vict) = ch;
   GRAPTYPE(vict) = 1;
   /* Let's grapple! */

   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if (!strcasecmp("choke", arg2)) {
         reveal_hiding(ch, 0);
   act("@RYou rush at @r$N@R and manage to grab $S throat with both hands!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R rushes at YOU and manages to grab your throat with both hands!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R rushes at @r$N@R and manages to grab $S throat with both hands!@n", TRUE, ch, 0, vict, TO_NOTVICT);

   /* Let's grapple! */
   GRAPPLING(ch) = vict;
   GRAPTYPE(ch) = 2;
   GRAPPLED(vict) = ch;
   GRAPTYPE(vict) = 2;
   /* Let's grapple! */

   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if (!strcasecmp("wrap", arg2)) {
   if (!IS_MAJIN(ch)) {
    send_to_char(ch, "Your body is not flexible enough to wrap around a target!\r\n");
    return;
   }

   act("@MMoving quickly you stretch your body out and wrap it around the length of @c$N's@M body! You tighten your body until you begin crushing @c$N@M!", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@M quickly stretches out $s body and wraps it around @RYOU@M! You feel $s body begin to crush your own!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@C$n@M quickly stretches out $s body and wraps it around @c$N@M! It appears that @c$N's@M body is being crushed slowly!@n", TRUE, ch, 0, vict, TO_NOTVICT);

   /* Let's grapple! */
   GRAPPLING(ch) = vict;
   GRAPTYPE(ch) = 4;
   GRAPPLED(vict) = ch;
   GRAPTYPE(vict) = 4;
   /* Let's grapple! */

   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  } else if (!strcasecmp("grab", arg2)) {
         reveal_hiding(ch, 0);
   act("@RYou rush at @r$N@R and manage to lock your arm onto $S!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@r$n@R rushes at YOU and manages to lock $s arm onto your's!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@r$n@R rushes at @r$N@R and manages to lock $s arm onto @r$N's@R!@n", TRUE, ch, 0, vict, TO_NOTVICT);

   /* Let's grapple! */
   GRAPPLING(ch) = vict;
   GRAPTYPE(ch) = 3;
   GRAPPLED(vict) = ch;
   GRAPTYPE(vict) = 3;
   /* Let's grapple! */

   if (!PLR_FLAGGED(vict, PLR_THANDW)) {
    REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THANDW);
   }

   GET_MOVE(ch) -= cost;
   improve_skill(ch, SKILL_GRAPPLE, 1);
   WAIT_STATE(ch, PULSE_4SEC);
   return;
  }
 }
 else { /* You need to learn proper syntax! */
  send_to_char(ch, "Syntax: grapple (target) (hold | choke | grab | wrap)\r\n");
  return;
 }
}

ACMD(do_trip)
{

 char arg[200];
 struct char_data *vict = NULL;
 
 one_argument(argument, arg);

 if (!check_skill(ch, SKILL_TRIP) && !IS_NPC(ch)) {
  return;
 }

 int cost = GET_MAX_HIT(ch) / 200;

 if (cost > GET_MOVE(ch)) {
  send_to_char(ch, "You don't have enough stamina.\r\n");
  return;
 }

 int perc = init_skill(ch, SKILL_TRIP), prob = rand_number(1, 114);

 if (perc == 0) {
  perc = GET_LEVEL(ch) + rand_number(1, 10);
 }

 vict = NULL; vict = NULL;
 if (!*arg || !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  if (FIGHTING(ch) && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch)) {
   vict = FIGHTING(ch);
  } else {
   send_to_char(ch, "That target isn't here.\r\n");
   return;
  }
 }

 if (!can_kill(ch, vict, NULL, 0)) {
  return;
 }

 if (vict != NULL) {
  if (AFF_FLAGGED(vict, AFF_FLYING)) {
   send_to_char(ch, "They are flying and are not on their feet!\r\n");
   return;
  }
  if (GET_POS(vict) == POS_SITTING) {
   send_to_char(ch, "They are not on their feet!\r\n");
   return;
  }
  if (PLR_FLAGGED(vict, PLR_HEALT)) {
   send_to_char(ch, "They are inside a healing tank!\r\n");
   return;
  }

  if (GET_SPEEDI(ch) > GET_SPEEDI(vict) * 2) {
   perc += 5;
  } else if (GET_SPEEDI(ch) > GET_SPEEDI(vict)) {
   perc += 2;
  } else if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict)) {
   perc -= 5;
  } else if (GET_SPEEDI(ch) < GET_SPEEDI(vict)) {
   perc -= 2;
  }

  if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) || AFF_FLAGGED(vict, AFF_ZANZOKEN)) && GET_MOVE(vict) >= 1 && GET_POS(vict) != POS_SLEEPING) {
   if (!AFF_FLAGGED(ch, AFF_ZANZOKEN) || (AFF_FLAGGED(ch, AFF_ZANZOKEN) && GET_SPEEDI(ch) + rand_number(1, 5) < GET_SPEEDI(vict) + rand_number(1, 5))) {
          reveal_hiding(ch, 0);
     act("@C$N@c disappears, avoiding your trip before reappearing!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou disappear, avoiding @C$n's@c trip before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, avoiding @C$n's@c trip before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     if (AFF_FLAGGED(ch, AFF_ZANZOKEN)) {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
     }
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     GET_MOVE(ch) -= cost;
     WAIT_STATE(ch, PULSE_4SEC);
     return;
   }
   else {
           reveal_hiding(ch, 0);
     act("@C$N@c disappears, trying to avoid your trip but your zanzoken is faster!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou zanzoken to avoid the trip but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, trying to avoid @C$n's@c trip but @C$n's@c zanzoken is faster!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
   }
  }

  if (perc < prob) { /* Fail! */
          reveal_hiding(ch, 0);
   act("@mYou move to trip $N@m, but you screw up and $E keeps $S footing!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@m$n@m moves to trip YOU, but $e screws up and you manage to keep your footing!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@m$n@m moves to trip $N@m, but $e screws up and $N@m manages to keep $S footing!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   improve_skill(ch, SKILL_TRIP, 0);
   GET_MOVE(ch) -= cost;
   WAIT_STATE(ch, PULSE_4SEC);
   if (FIGHTING(ch) == NULL) {
    set_fighting(ch, vict);
   } else if (FIGHTING(ch) != vict) {
    set_fighting(ch, vict);
   }
   if (FIGHTING(vict) == NULL) {
    set_fighting(vict, ch);
   } else if (FIGHTING(vict) != ch) {
    set_fighting(vict, ch);
   }
   return;
  } else { /* Success! */
         reveal_hiding(ch, 0);
   act("@mYou move to trip $N@m, and manage to knock $M off $S feet!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@m$n@m moves to trip YOU, and manages to knock you off your feet!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@m$n@m moves to trip $N@m, and manages to knock $N@m off $S feet!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   improve_skill(ch, SKILL_TRIP, 0);
   GET_MOVE(ch) -= cost;
   GET_POS(vict) = POS_SITTING;
   WAIT_STATE(ch, PULSE_4SEC);
   if (FIGHTING(ch) == NULL) {
    set_fighting(ch, vict);
   }
   if (FIGHTING(vict) == NULL) {
    set_fighting(vict, ch);
   }
   return;
  }
 } else {
  send_to_char(ch, "ERROR: Report to Iovan.\r\n");
  return;
 }

}

ACMD(do_train)
{

 if (IS_NPC(ch)) {
  return;
 }

 if (IS_CARRYING_W(ch) > CAN_CARRY_W(ch)) {
  send_to_char(ch, "You are weighted down too much!\r\n");
  return;
 }

 int plus = 0;
 int64_t total = 0, weight = 0, bonus = 0, cost = 0;
 char arg[200];

 one_argument(argument, arg);

 weight = gear_weight(ch) - IS_CARRYING_W(ch);

 int strcap = 5000, spdcap = 5000, intcap = 5000, wiscap = 5000, concap = 5000, aglcap = 5000;

 strcap += 500 * ch->real_abils.str;
 intcap += 500 * ch->real_abils.intel;
 wiscap += 500 * ch->real_abils.wis;
 spdcap += 500 * ch->real_abils.cha;
 concap += 500 * ch->real_abils.con;
 aglcap += 500 * ch->real_abils.dex;

 if (IS_HUMAN(ch)) {
  intcap = intcap * 0.75;
  wiscap = wiscap * 0.75;
 } else if (IS_KANASSAN(ch)) {
  intcap = intcap * 0.4;
  wiscap = wiscap * 0.4;
  aglcap = aglcap * 0.4;
 } else if (IS_HALFBREED(ch)) {
  intcap = intcap * 0.75;
  strcap = strcap * 0.75;
 } else if (IS_TRUFFLE(ch)) {
  strcap = strcap * 1.5;
  concap = concap * 1.5;
 }

 if (!*arg) {
  send_to_char(ch, "@D-------------[ @GTraining Status @D]-------------@n\r\n");
  send_to_char(ch, "  @mStrength Progress    @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSTR(ch)), ch->real_abils.str >= 80 ? "@rCAPPED" : add_commas(strcap));
  send_to_char(ch, "  @mSpeed Progress       @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINSPD(ch)), ch->real_abils.cha >= 80 ? "@rCAPPED" : add_commas(spdcap));
  send_to_char(ch, "  @mConstitution Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINCON(ch)), ch->real_abils.con >= 80 ? "@rCAPPED" : add_commas(concap));
  send_to_char(ch, "  @mIntelligence Progress@D: @R%6s/%6s@n\r\n", add_commas(GET_TRAININT(ch)), ch->real_abils.intel >= 80 ? "@rCAPPED" : add_commas(intcap));
  send_to_char(ch, "  @mWisdom Progress      @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINWIS(ch)), ch->real_abils.wis >= 80 ? "@rCAPPED" : add_commas(wiscap));
  send_to_char(ch, "  @mAgility Progress     @D: @R%6s/%6s@n\r\n", add_commas(GET_TRAINAGL(ch)), ch->real_abils.dex >= 80 ? "@rCAPPED" : add_commas(aglcap));
  send_to_char(ch, "@D  -----------------------------------------  @n\r\n");
  send_to_char(ch, "  @CCurrent Weight Worn  @D: @c%s@n\r\n", add_commas(weight));
  send_to_char(ch, "@D---------------------------------------------@n\r\n");
  send_to_char(ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
  return;
 }

 if (weight < (GET_LEVEL(ch) * 100) && GET_LEVEL(ch) <= 19) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 } else if (weight < (GET_LEVEL(ch) * 110) && GET_LEVEL(ch) <= 45) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 } else if (weight < (GET_LEVEL(ch) * 125) && GET_LEVEL(ch) > 45) {
  send_to_char(ch, "With so little weight on you like that it would be a joke to try and train.\r\n");
  return;
 }


 /* Figure up the weight bonus */
 total = weight * (ROOM_GRAVITY(IN_ROOM(ch)) + 1);
 total += (ROOM_GRAVITY(IN_ROOM(ch)) + 1) * (ROOM_GRAVITY(IN_ROOM(ch)) + 1);
 if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 6100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 6135) {
  total += total * 0.15;
 }

 int sensei = -1;

   if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1131 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GRoshi begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_ROSHI;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1131 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1131 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 12102 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GKibito begins to instruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_KABITO;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 12182 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 12182 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1714 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@Sixteen begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_ANDSIX;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1714 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1714 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4283 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GFrieza begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_FRIEZA;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4283 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4283 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 13012 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GKrane begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_KRANE;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 13012 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 13012 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2267 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GBardock begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_BARDOCK;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2267 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2267 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1662 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GPiccolo begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_PICCOLO;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1662 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 1662 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 11684 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GNail begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_NAIL;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 11684 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 11684 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4290 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GGinyu begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_GINYU;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4290 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 4290 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 8233 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GTapion begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_TAPION;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 8233 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 8233 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 6487 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GDabura begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_DABURA;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 6487 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 6487 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 3499 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GJinto begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_JINTO;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 3499 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 3499 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 15009 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GTsuna begins to intruct you in training technique@n\r\n");
    total += total * 0.85;
    sensei = CLASS_TSUNA;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 15009 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 15009 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 16100 && GET_GOLD(ch) >= 8 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 1) {
    send_to_char(ch, "@GKurzak begins to intruct you in training technique.@n\r\n");
    total += total * 0.85;
    sensei = CLASS_KURZAK;
    if (GET_LEVEL(ch) >= 100)
     total *= 15000;
    else if (GET_LEVEL(ch) >= 80)
     total *= 1500;
    else if (GET_LEVEL(ch) >= 40)
     total *= 600;
    else if (GET_LEVEL(ch) >= 20)
     total *= 300;
    else if (GET_LEVEL(ch) >= 10)
     total *= 150;
    GET_GOLD(ch) -= 8;
    GET_PRACTICES(ch, GET_CLASS(ch)) -= 1;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 16100 && GET_GOLD(ch) < 8) {
    send_to_char(ch, "@YYou do not have enough zenni (5) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 16100 && GET_PRACTICES(ch, GET_CLASS(ch)) < 1) {
    send_to_char(ch, "@YYou do not have enough PS (1) in order to train with your sensei. Train on your own elsewhere.@n\r\n");
  }
 
 if (total > GET_MAX_HIT(ch) * 2) {
  bonus = 5;
 } else if (total > GET_MAX_HIT(ch)) {
  bonus = 4;
 } else if (total > (GET_MAX_HIT(ch) /  2)) {
  bonus = 3;
 } else if (total > (GET_MAX_HIT(ch) /  4)) {
  bonus = 2;
 } else if (total > (GET_MAX_HIT(ch) /  8)) {
  bonus = 1;
 }

 if (sensei < 0) 
  cost = ((total / 20) + (GET_MAX_MOVE(ch) / 50));
 else
  cost = ((total / 25) + (GET_MAX_MOVE(ch) / 60));


  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   cost -= cost * 0.25;
  }

  if (GET_RELAXCOUNT(ch) >= 464) {
   cost *= 10;
  } else if (GET_RELAXCOUNT(ch) >= 232) {
   cost *= 5;
  } else if (GET_RELAXCOUNT(ch) >= 116) {
   cost *= 2;
  }

 if (!strcasecmp("str", arg)) {
  if (ch->real_abils.str == 80) {
   send_to_char(ch, "Your base strength is maxed!\r\n");
   return;
  }
  if (ch->real_abils.str == 25 && GET_BONUS(ch, BONUS_WIMP) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your strength beyond 25.\r\n");
    return;
  }
  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);

  switch (rand_number(1, 3)) {
   case 1:
    act("@WYou throw a flurry of punches into the air at an invisible opponent.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n throws a flurry of punches into the air.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou leap into the air and throw a wild kick at an invisible opponent@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n leaps into the air and throws a wild kick at nothing.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou leap high into the air and unleash a flurry of punches and kicks at an invisible opponent@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n leaps high into the air and unleashes a flurry of punches and kicks at nothing.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_BRAWNY)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  if (sensei > -1) {
   plus += plus * 0.2;
  }
  switch (bonus) {
   case 1:
    GET_TRAINSTR(ch) += 5 + plus;
    GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAINSTR(ch) += 10 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAINSTR(ch) += 25 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAINSTR(ch) += 50 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAINSTR(ch) += 100 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAINSTR(ch) += 1;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAINSTR(ch) >= strcap) {
   GET_TRAINSTR(ch) -= strcap;
   send_to_char(ch, "You feel your strength improve!@n\r\n");
   ch->real_abils.str += 1;
   if (IS_PICCOLO(ch) && IS_NAMEK(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
   save_char(ch);
  }
 } else if (!strcasecmp("spd", arg)) {
  if (ch->real_abils.cha == 80) {
   send_to_char(ch, "Your base speed is maxed!\r\n");
   return;
  }
  if (ch->real_abils.cha == 25 && GET_BONUS(ch, BONUS_SLOW) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your speed beyond 25.\r\n");
    return;
  }
  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);
  switch (rand_number(1, 3)) {
   case 1:
    act("@WYou dash quickly around the surrounding area as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n dashes quickly around the surrounding area as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou dodge to the side as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n dodges to the side as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou dash backwards as fast as you can!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n dashes backwards as fast as $e can!@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_QUICK)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  switch (bonus) {
   case 1:
    GET_TRAINSPD(ch) += 5 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAINSPD(ch) += 10 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAINSPD(ch) += 25 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAINSPD(ch) += 50 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAINSPD(ch) += 100 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAINSPD(ch) += 1;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAINSPD(ch) >= spdcap) {
   GET_TRAINSPD(ch) -= spdcap;
   send_to_char(ch, "You feel your speed improve!@n\r\n");
    ch->real_abils.cha += 1;
   if (IS_PICCOLO(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
   save_char(ch);
  }
 } else if (!strcasecmp("con", arg)) {
  if (ch->real_abils.con == 80) {
   send_to_char(ch, "Your base constitution is maxed!\r\n");
   return;
  }
  if (ch->real_abils.con == 25 && GET_BONUS(ch, BONUS_WIMP) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your constitution beyond 25.\r\n");
    return;
  }
  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);
  switch (rand_number(1, 3)) {
   case 1:
    act("@WYou leap into the air and then slam into the ground with your feet outstretched!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n leaps into the air and then slams into the ground with $s feet outstretched!?@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou leap into the air and then slam into the ground with your fists!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n leaps into the air and then slams into the ground with $s fists!?@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou leap into the air and then slam into the ground with your body!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n leaps into the air and then slams into the ground with $s body!?@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_STURDY)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  switch (bonus) {
   case 1:
    GET_TRAINCON(ch) += 5 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAINCON(ch) += 10 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAINCON(ch) += 25 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAINCON(ch) += 50 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAINCON(ch) += 100 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAINCON(ch) += 1;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAINCON(ch) >= concap) {
   GET_TRAINCON(ch) -= concap;
   send_to_char(ch, "You feel your constitution improve!@n\r\n");
    ch->real_abils.con += 1;
   if (IS_PICCOLO(ch) && IS_NAMEK(ch) &&  level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
   save_char(ch);
  }
 } else if (!strcasecmp("agl", arg)) {
  if (ch->real_abils.dex == 80) {
   send_to_char(ch, "Your base agility is maxed!\r\n");
   return;
  }
  if (ch->real_abils.dex == 25 && GET_BONUS(ch, BONUS_CLUMSY) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your agility beyond 25.\r\n");
    return;
  }
  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);
  switch (rand_number(1, 3)) {
   case 1:
    act("@WYou do a series of backflips through the air, landing gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n does a series of backflips through the air, landing gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou flip forward and launch off your hands into the air. You land gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n flips forward and launches off $s hands into the air. Then $e lands gracefully on one foot a moment later.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou flip to the side off one hand and then land on your feet.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n flips to the side off one hand and then lands on $s feet.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MOVE(ch) / 50)) * 100) / GET_MAX_MOVE(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_AGILE)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  switch (bonus) {
   case 1:
    GET_TRAINAGL(ch) += 5 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAINAGL(ch) += 10 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAINAGL(ch) += 25 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAINAGL(ch) += 50 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAINAGL(ch) += 100 + plus;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAINAGL(ch) += 1;
     GET_MOVE(ch) -= cost;
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAINAGL(ch) >= aglcap) {
   GET_TRAINAGL(ch) -= aglcap;
   send_to_char(ch, "You feel your agility improve!@n\r\n");
    ch->real_abils.dex += 1;
   if (IS_PICCOLO(ch) && IS_NAMEK(ch) &&  level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
   save_char(ch);
  }
 } else if (!strcasecmp("int", arg)) {
  if (ch->real_abils.intel == 80) {
   send_to_char(ch, "Your base intelligence is maxed!\r\n");
   return;
  }
  if (ch->real_abils.intel == 25 && GET_BONUS(ch, BONUS_DULL) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your intelligence beyond 25.\r\n");
    return;
  }
  if (GET_MANA(ch) < ((total / 20) + (GET_MAX_MANA(ch) / 50))) {
   send_to_char(ch, "You do not have enough ki with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);
  switch (rand_number(1, 3)) {
   case 1:
    act("@WConcentrating you fly high into the air as fast as you can before settling slowly back to the ground.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n flies high into the air as fast as $e can before settling slowly back to the ground.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou focus your ki at your outstretched hand and send a mild shockwave in that direction!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n focuses $s ki at $s outstretched hand and sends a mild shockwave in that direction!@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou concentrate on your ki and force torrents of it to rush out from your body randomly!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n seems to concentrate before torrents of ki randomly blasts out from $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MANA(ch) / 50)) * 100) / GET_MAX_MANA(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_SCHOLARLY)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  switch (bonus) {
   case 1:
    GET_TRAININT(ch) += 5 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAININT(ch) += 10 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAININT(ch) += 25 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAININT(ch) += 50 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAININT(ch) += 100 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAININT(ch) += 1;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAININT(ch) >= intcap) {
   GET_TRAININT(ch) -= intcap;
   send_to_char(ch, "You feel your intelligence improve!@n\r\n");
    ch->real_abils.intel += 1;
   if (IS_PICCOLO(ch) && IS_NAMEK(ch) &&  level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
   save_char(ch);
  }
 } else if (!strcasecmp("wis", arg)) {
  if (ch->real_abils.wis == 80) {
   send_to_char(ch, "Your base wisdom is maxed!\r\n");
   return;
  }
  if (ch->real_abils.wis == 25 && GET_BONUS(ch, BONUS_FOOLISH) > 0) {
    send_to_char(ch, "You're not able to withstand increasing your wisdom beyond 25.\r\n");
    return;
  }
  if (GET_MANA(ch) < ((total / 20) + (GET_MAX_MANA(ch) / 50))) {
   send_to_char(ch, "You do not have enough ki with the current weight worn and gravity!\r\n");
   return;
  }
  /* what training message is displayed? */
        reveal_hiding(ch, 0);
  switch (rand_number(1, 3)) {
   case 1:
    act("@WYou close your eyes and wage a mental battle against an imaginary opponent.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n closes $s eyes for a moment and an expression of intensity forms on it.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 2:
    act("@WYou look around and contemplate battle tactics for an imaginary scenario.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n looks around and appears to be imagining things that aren't there.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
   case 3:
    act("@WYou invent a battle plan for a battle that doesn't exist!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n seems to have thought of something.@n", TRUE, ch, 0, 0, TO_ROOM);
    break;
  }
  plus = (((total / 20) + (GET_MAX_MANA(ch) / 50)) * 100) / GET_MAX_MANA(ch);
  if (GET_LEVEL(ch) > 80) {
   plus += 50;
  } else if (GET_LEVEL(ch) > 60) {
   plus += 25;
  }
  if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   plus *= 4;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   plus *= 3;
  }
  if (GET_BONUS(ch, BONUS_HARDWORKER)) {
   plus += plus * 0.25;
  }
  if (GET_BONUS(ch, BONUS_SAGE)) {
   plus += plus * 0.75;
  }
  if (GET_BONUS(ch, BONUS_LONER)) {
   plus += plus * 0.05;
  }
  switch (bonus) {
   case 1:
    GET_TRAINWIS(ch) += 5 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel slight improvement. @D[@G+%d@D]@n\r\n", (plus + 5));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 2:
    GET_TRAINWIS(ch) += 10 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel some improvement. @D[@G+%d@D]@n\r\n", (plus + 10));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 3:
    GET_TRAINWIS(ch) += 25 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel good improvement. @D[@G+%d@D]@n\r\n", (plus + 25));
    WAIT_STATE(ch, PULSE_3SEC);
    break;
   case 4:
    GET_TRAINWIS(ch) += 50 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel great improvement! @D[@G+%d@D]@n\r\n", (plus + 50));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   case 5:
    GET_TRAINWIS(ch) += 100 + plus;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You feel awesome improvement! @D[@G+%d@D]@n\r\n", (plus + 100));
    WAIT_STATE(ch, PULSE_5SEC);
    break;
   default:
    GET_TRAINWIS(ch) += 1;
    if (sensei < 0)
     GET_MANA(ch) -= (total / 20) + (GET_MAX_MANA(ch) / 50);
    else
     GET_MANA(ch) -= (total / 25) + (GET_MAX_MANA(ch) / 60);
    send_to_char(ch, "You barely feel any improvement. @D[@G+1@D]@n\r\n");
    WAIT_STATE(ch, PULSE_3SEC);
    break;
  }
  if (GET_TRAINWIS(ch) >= wiscap) {
   GET_TRAINWIS(ch) -= wiscap;
   send_to_char(ch, "You feel your wisdom improve!@n\r\n");
    ch->real_abils.wis += 1;
   if (IS_PICCOLO(ch) && IS_NAMEK(ch) &&  level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0) {
    GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.25;
    send_to_char(ch, "You gained quite a bit of experience from that!\r\n");
   }
    save_char(ch);
  }
 } else {
  send_to_char(ch, "Syntax: train (str | spd | agl | wis | int | con)\r\n");
  return;
 }
}

ACMD(do_rip)
{
 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (IS_NPC(ch))
  return;

 if (!*arg) {
  send_to_char(ch, "Rip the tail off who?\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "That target isn't here.\r\n");
  return;
 }

 if (!PLR_FLAGGED(vict, PLR_TAIL) && !PLR_FLAGGED(vict, PLR_STAIL)) {
  send_to_char(ch, "They do not have a tail to rip off!\r\n");
  return;
 }

 if (ch != vict && GET_POS(ch) > POS_SLEEPING) {
  if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 20) {
   send_to_char(ch, "You are too tired to manage to grab their tail!\r\n");
   return;
  } else if (GET_SPEEDI(ch) > GET_SPEEDI(vict)) {
   GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 20;
   if (GET_HIT(ch) > GET_HIT(vict) * 2) {
          reveal_hiding(ch, 0);
    act("@rYou rush at @R$N@r and grab $S tail! With a powerful tug you pull it off!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@R$n@r rushes at YOU and grabs your tail! With a powerful tug $e pulls it off!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$n@R rushes at @R$N@r and grab $S tail! With a powerful tug $e pulls it off!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_TAIL);
    REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_STAIL);
    oozaru_drop(vict);
    return;
   } else {
          reveal_hiding(ch, 0);
    act("@rYou rush at @R$N@r and grab $S tail! You are too weak to pull it off though!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@R$n@r rushes at YOU and grabs your tail! $e is too weak to pull it off though!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$n@R rushes at @R$N@r and grab $S tail! $e is too weak to pull it off though!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    return;
   }
  } else {
   GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 20;
         reveal_hiding(ch, 0);
   act("@rYou rush at @R$N@r and try to grab $S tail, but fail!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@R$n@r rushes at YOU and tries to grab your tail, but fails!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@R$n@R rushes at @R$N@r and tries to grab $S tail, but fails!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   return;
  }
 } else if (ch == vict) {
         reveal_hiding(ch, 0);
   act("@rYou grab your own tail and yank it off!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@R$n@r grabs $s own tail and yanks it off!@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TAIL);
   REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_STAIL);
    oozaru_drop(vict);
 } else {
  if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 20) {
   send_to_char(ch, "You are too tired to manage to grab their tail!\r\n");
   return;
  }
   GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 20;
          reveal_hiding(ch, 0);
    act("@rYou reach and grab @R$N's@r tail! With a powerful tug you pull it off!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@RYou feel your tail pulled off!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$n@R reaches and grabs @R$N's@r tail! With a powerful tug $e pulls it off!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_TAIL);
    REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_STAIL);
    oozaru_drop(vict);
    return;
 }
}

ACMD(do_infuse)
{

 if (!know_skill(ch, SKILL_INFUSE)) {
  return;
 }

 if (AFF_FLAGGED(ch, AFF_INFUSE)) {
  act("You stop infusing ki into your attacks.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n stops infusing ki into $s attacks.", TRUE, ch, 0, 0, TO_ROOM);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INFUSE);
  return;
 }

 if (GET_MANA(ch) < GET_MAX_MANA(ch) / 100) {
  send_to_char(ch, "You don't have enough ki to infuse into your attacks!\r\n");
  return;
 }
       reveal_hiding(ch, 0);
 act("You start infusing ki into your attacks.", TRUE, ch, 0, 0, TO_CHAR);
 act("$n starts infusing ki into $s attacks.", TRUE, ch, 0, 0, TO_ROOM);
 SET_BIT_AR(AFF_FLAGS(ch), AFF_INFUSE);
 GET_MANA(ch) -= GET_MAX_MANA(ch) / 100;
}

ACMD(do_paralyze)
{
 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (!know_skill(ch, SKILL_PARALYZE)) {
   return;
 }

 if (!*arg) {
  send_to_char(ch, "Who are you wanting to paralyze?\r\n");
  return;
 }

 if (!limb_ok(ch, 0)) {
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "That target isn't here.\r\n");
  return;
 }
 if (!can_kill(ch, vict, NULL, 0)) {
  return;
 } 
 if (AFF_FLAGGED(vict, AFF_PARA)) {
  send_to_char(ch, "They are already partially paralyzed!\r\n");
  return;
 }

 if (GET_MANA(ch) < GET_HIT(vict) / 10 + (GET_MAX_MANA(ch) / 20)) {
  send_to_char(ch, "You realize you can't paralyze them. You don't have enough ki to restrain them!\r\n");
  return;
 }

 int prob = GET_SKILL(ch, SKILL_PARALYZE), perc = axion_dice(0);

 if (GET_SPEEDI(ch) * 2 < GET_SPEEDI(vict)) {
  prob -= 10;
 }
 if (GET_SPEEDI(ch) + (GET_SPEEDI(ch) / 2) < GET_SPEEDI(vict)) {
  prob -= 5;
 }

 if (GET_BONUS(vict, BONUS_INSOMNIAC)) {
  GET_MANA(ch) -= GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20);
  act("@RYou focus ki and point both your arms at @r$N@R. However $N seems to shake off your paralysis attack!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@r$n @Rfocuses ki and points both $s arms at YOU! Your insomnia makes you immune to $s feeble paralysis attempt.@n", TRUE, ch, 0, vict, TO_VICT);
  act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $N seems to shake off $s paralysis attack!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  return;
 } else if (prob < perc) {
        reveal_hiding(ch, 0);
  act("@RYou focus ki and point both your arms at @r$N@R. However $E manages to avoid your attempt to paralyze $M!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@r$n @Rfocuses ki and points both $s arms at YOU! You manage to avoid $s technique though...@n", TRUE, ch, 0, vict, TO_VICT);
  act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. However $E manages to avoid @r$n's@R attempted technique...@n", TRUE, ch, 0, vict, TO_NOTVICT);
  GET_MANA(ch) -= GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20);
  improve_skill(ch, SKILL_PARALYZE, 0);
 } else {
        reveal_hiding(ch, 0);
  act("@RYou focus ki and point both your arms at @r$N@R. Your ki flows into $S body and partially paralyzes $M!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@r$n @Rfocuses ki and points both $s arms at YOU! You are caught in $s paralysis technique and now can barely move!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@r$n @Rfocuses ki and points both $s arms at @r$N@R. @r$n's@R ki flows into @r$N@R body and partially paralyzes $M!@n", TRUE, ch, 0, vict, TO_NOTVICT);
     int duration = GET_INT(ch) / 15;
     assign_affect(vict, AFF_PARA, SKILL_PARALYZE, duration, 0, 0, 0, 0, 0, 0);
  GET_MANA(ch) -= GET_HIT(vict) / 6 + (GET_MAX_MANA(ch) / 20);
  improve_skill(ch, SKILL_PARALYZE, 0);
 }
}

ACMD(do_taisha)
{

 if (IS_NPC(ch))
  return;
 
 if (!know_skill(ch, SKILL_TAISHA)) {
  return;
 }

 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AURA)) {
  send_to_char(ch, "This area already has an aura of regeneration around it.\r\n");
  return;
 }

 if (ROOM_GRAVITY(IN_ROOM(ch)) > 0) {
  send_to_char(ch, "This area's gravity is too hostile to an aura.\r\n");
  return;
 }

 if (GET_MANA(ch) < GET_MAX_MANA(ch) / 3) {
  send_to_char(ch, "You don't have enough ki.\r\n");
  return;
 }

 int prob = GET_SKILL(ch, SKILL_TAISHA), perc = axion_dice(0);

 if (prob < perc) {
        reveal_hiding(ch, 0);
  act("@WYou hold up your hands while channeling ki. Your technique fails to produce an aura though....@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n@W holds up $s hands while channeling ki. $s technique fails to produce an aura though....", TRUE, ch, 0, 0, TO_ROOM);
  GET_MANA(ch) -= GET_MAX_MANA(ch) / 3;
  improve_skill(ch, SKILL_TAISHA, 1);
  return;
 }
 else {
        reveal_hiding(ch, 0);
  act("@WYou hold up your hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@g$n holds up $s hands while channeling ki. Suddenly a @wburst@W of calming @Cblue@W light covers the surrounding area!@n", TRUE, ch, 0, 0, TO_ROOM);
  GET_MANA(ch) -= GET_MAX_MANA(ch) / 3;
  improve_skill(ch, SKILL_TAISHA, 1);
  SET_BIT_AR(ROOM_FLAGS(IN_ROOM(ch)), ROOM_AURA);
  return;
 }  
}

ACMD(do_kura)
{
 if (!know_skill(ch, SKILL_KURA)) {
  return;
 }

 if (GET_MANA(ch) >= GET_MAX_MANA(ch)) {
  send_to_char(ch, "Your ki is already maxed out!\r\n");
  return;
 }
 
 char arg[MAX_INPUT_LENGTH];
 one_argument(argument, arg);

 if (!*arg) {
  send_to_char(ch, "Syntax: kuraiiro (1-100).\r\n");
  return;
 }

 int num = atoi(arg);
 int skill = GET_SKILL(ch, SKILL_KURA);
 int64_t cost = 0, bonus = 0;

 if (num > skill) {
  send_to_char(ch, "The number can not be greater than your skill.\r\n");
  return;
 }
 if (num <= 0) {
  send_to_char(ch, "The number can not be less than 1.\r\n");
  return;
 }

 cost = (GET_MAX_MANA(ch) / 100) * num;
 bonus = cost;

 if (GET_MOVE(ch) < cost) {
  send_to_char(ch, "You do not have enough stamina for that high a number.\r\n");
  return;
 }

 if (skill <= axion_dice(0)) {
  GET_MOVE(ch) -= cost;
        reveal_hiding(ch, 0);
  act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies but you fail!", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@w crouches down and screams as $s eyes turn red and $e attempts to tap into dark energies but fails!", TRUE, ch, 0, 0, TO_ROOM);
  improve_skill(ch, SKILL_KURA, 0);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
 else {
  GET_MOVE(ch) -= cost;
  GET_MANA(ch) += bonus;
  if (GET_MANA(ch) > GET_MAX_MANA(ch)) {
   GET_MANA(ch) = GET_MAX_MANA(ch);
  }
        reveal_hiding(ch, 0);
  act("You crouch down and scream as your eyes turn red. You attempt to tap into your dark energies and succeed as a rush of energy explodes around you!", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@w crouches down and screams as $s eyes turn red. Suddenly $e manages to tap into dark energies and a rush of energy explodes around $m!", TRUE, ch, 0, 0, TO_ROOM);
  improve_skill(ch, SKILL_KURA, 0);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }

}

ACMD(do_candy)
{

	struct char_data *vict;
	struct obj_data *obj;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!IS_MAJIN(ch)) {
		send_to_char(ch, "You are not a majin, how can you do that?\r\n");
		return;
	}
	//if (FIGHTING(ch)) {
		//send_to_char(ch, "You are too busy fighting!\r\n");
		//return;
	//}

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
		send_to_char(ch, "Turn who into candy?\r\n");
		return;
	}
	if (!can_kill(ch, vict, NULL, 0)) {
		return;
	}
	if (!IS_NPC(vict)) {
		send_to_char(ch, "You can't turn them into candy.\r\n");
		return;
	}
	if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 2) {
		send_to_char(ch, "They are too powerful.\r\n");
		return;
	}
	if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) * .25 && GET_LEVEL(ch) < 100) {
		send_to_char(ch, "They are too weak.\r\n");
		return;
	}
	if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) * .09 && GET_LEVEL(ch) == 100) {
		send_to_char(ch, "They are too weak.\r\n");
		return;
	}
	if (GET_MANA(ch) < GET_MAX_MANA(ch) / 15) {
		send_to_char(ch, "You do not have enough ki.\r\n");
		return;
	}
	if (rand_number(1, 6) == 6) {
		GET_MANA(ch) -= GET_MAX_MANA(ch) / 15;
		reveal_hiding(ch, 0);
		act("@cYou aim your forelock at @R$N@c and fire a beam of energy but it is dodged!@n", TRUE, ch, 0, vict, TO_CHAR);
		act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy but the beam is dodged!@n", TRUE, ch, 0, vict, TO_NOTVICT);
		if (!FIGHTING(ch)) {
			set_fighting(ch, vict);
		}
		if (!FIGHTING(vict)) {
			set_fighting(vict, ch);
		}
		WAIT_STATE(ch, PULSE_3SEC);
		return;
	}
	else {
		GET_MANA(ch) -= GET_MAX_MANA(ch) / 15;
		reveal_hiding(ch, 0);
		act("@cYou aim your forelock at @R$N@c and fire a beam of energy that envelopes $S entire body and changes $M into candy!@n", TRUE, ch, 0, vict, TO_CHAR);
		act("@C$n@c aims $s forelock at @R$N@c and fires a beam of energy that envelopes $S entire body and changes $M into candy!@n ", TRUE, ch, 0, vict, TO_NOTVICT);
		if (GET_MAX_HIT(vict) >= GET_MAX_HIT(ch) * 1.5) {
			send_to_char(ch, "You grab the candy as it falls.\r\n");
			obj = read_object(95, VIRTUAL);
			obj_to_char(obj, ch);
		}
		else if (GET_MAX_HIT(vict) >= GET_MAX_HIT(ch)) {
			send_to_char(ch, "You grab the candy as it falls.\r\n");
			obj = read_object(94, VIRTUAL);
			obj_to_char(obj, ch);
		}
		else if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) && (((GET_MAX_HIT(vict) >= GET_MAX_HIT(ch) * .5) && GET_LEVEL(ch) < 100) || ((GET_MAX_HIT(vict) >= GET_MAX_HIT(ch) * .1) && GET_LEVEL(ch) >= 100))) {
			send_to_char(ch, "You grab the candy as it falls.\r\n");
			obj = read_object(93, VIRTUAL);
			obj_to_char(obj, ch);
		}
		else if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) && (((GET_MAX_HIT(vict) < GET_MAX_HIT(ch) * .5) && GET_LEVEL(ch) < 100) || ((GET_MAX_HIT(vict) < GET_MAX_HIT(ch) * .1) && GET_LEVEL(ch) >= 100))) {
			send_to_char(ch, "You grab the candy as it falls.\r\n");
			obj = read_object(53, VIRTUAL);
			obj_to_char(obj, ch);
		}
		SET_BIT_AR(MOB_FLAGS(vict), MOB_HUSK);
		die(vict, ch);
	}


}

ACMD(do_future)
{
 char arg[MAX_INPUT_LENGTH];
 struct char_data *vict = NULL;
 one_argument(argument, arg);

 if (IS_NPC(ch) || !IS_KANASSAN(ch)) {
  send_to_char(ch, "You are incapable of this ability.\r\n");
  return;
 }

 if (!*arg) {
  send_to_char(ch, "Bestow advance future sight on who?\r\n");
  return;
 }
 
 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "Bestow advance future sight on who?\r\n");
  return;
 }

 if (AFF_FLAGGED(vict, AFF_FUTURE)) {
  send_to_char(ch, "They already can see the future.\r\n");
  return;
 }

 if (IS_NPC(vict)) {
  send_to_char(ch, "You can't target them, there would be no point.\r\n");
  return;
 }

 if (GET_MANA(ch) < GET_MAX_MANA(ch) / 40) {
  send_to_char(ch, "You do not have enough ki.\r\n");
  return;
 }

 if (GET_PRACTICES(ch, GET_CLASS(ch)) < 100) {
  send_to_char(ch, "You do not have enough PS to activate or pass on this ability.\r\n");
  return;
 }

 if (vict != ch) {
  if (vict->real_abils.cha + 5 > 25 && GET_BONUS(vict, BONUS_SLOW) > 0) {
   send_to_char(ch, "They can't handle having their speed increased beyond 25.\r\n");
   return;
  }
  if (vict->real_abils.intel + 2 > 25 && GET_BONUS(vict, BONUS_DULL) > 0) {
   send_to_char(ch, "They can't handle having their intelligence increased beyond 25.\r\n");
   return;
  }
  GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
  GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
        reveal_hiding(ch, 0);
  act("@CYou focus your energy into your fingers before stabbing your claws into $N and bestowing the power of Future Sight upon $M. Shortly after $E passes out.@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n focuses $s energy into $s fingers before stabbing $s claws into YOUR neck and bestowing the power of Future Sight upon you! Soon after you pass out!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n focuses $s energy into $s fingers before stabbing $s claws into $N's neck and bestowing the power of Future Sight upon $M! Soon after $E passes out!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  SET_BIT_AR(AFF_FLAGS(vict), AFF_FUTURE);
  vict->real_abils.cha += 5;
  vict->real_abils.intel += 2;
  GET_POS(vict) = POS_SLEEPING;
  save_char(vict);
 }
 else {
  if (ch->real_abils.cha + 5 > 25 && GET_BONUS(ch, BONUS_SLOW) > 0) {
   send_to_char(ch, "You can't handle having your speed increased beyond 25.\r\n");
   return;
  }
  if (ch->real_abils.intel + 2 > 25 && GET_BONUS(ch, BONUS_DULL) > 0) {
   send_to_char(ch, "You can't handle having your intelligence increased beyond 25.\r\n");
   return;
  }
  GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
  GET_PRACTICES(ch, GET_CLASS(ch)) -= 100;
        reveal_hiding(ch, 0);
  act("@CYou focus your energy into your mind and awaken your latent Future Sight powers!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n focuses $s energy while closing $s eyes for a moment.@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n focuses $s energy while closing $s eyes for a moment.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  SET_BIT_AR(AFF_FLAGS(ch), AFF_FUTURE);
  ch->real_abils.cha += 5;
  ch->real_abils.intel += 2;
  GET_POS(vict) = POS_SLEEPING;
  save_char(ch);
 }
  

}

ACMD(do_drag)
{
 struct char_data *vict = NULL;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (IS_NPC(ch))
  return;

 if (DRAGGING(ch)) {
  vict = DRAGGING(ch);
  DRAGGING(ch) = NULL;
  DRAGGED(vict) = NULL;
  act("@wYou stop dragging @C$N@W.@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W stops dragging @c$N@W.@n", TRUE, ch, 0, vict, TO_ROOM);
  return;
 }

 if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
 }

 if (CARRYING(ch)) {
  send_to_char(ch, "You are busy carrying someone at the moment.\r\n");
  return;
 }

 if (!*arg) {
  send_to_char(ch, "Who do you want to drag?\r\n");
  return;
 }

 if (FIGHTING(ch)) {
  send_to_char(ch, "You are a bit busy fighting right now!\r\n");
  return;
 }

 if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM || SECT(IN_ROOM(ch)) == SECT_WATER_SWIM) {
  send_to_char(ch, "You decide to not be a tugboat instead.\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "Drag who?\r\n");
  return;
 }

 if (vict == ch) {
  send_to_char(ch, "You can't drag yourself.\r\n");
  return;
 }

 if (DRAGGED(vict)) {
  send_to_char(ch, "They are already being dragged!\r\n");
  return;
 }

 if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
  send_to_char(ch, "They are not to be touched!\r\n");
  return;
 }


 if (GET_POS(vict) != POS_SLEEPING) {
        reveal_hiding(ch, 0);
  act("@wYou try to grab and pull @C$N@W with you, but $E resists!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W tries to grab and pull you! However you resist!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W tries to grab and pull @c$N@W but $E resists!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(vict) && !FIGHTING(vict)) {
   set_fighting(vict, ch);
  }
  return;
 }
 else if (GET_PC_WEIGHT(vict) + IS_CARRYING_W(vict) > CAN_CARRY_W(ch)) {
        reveal_hiding(ch, 0);
  act("@wYou try to grab and pull @C$N@W with you, but $E is too heavy!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W tries to grab and pull @c$N@W but $E is too heavy!@n", TRUE, ch, 0, vict, TO_ROOM);
  return;
 }
 else {
        reveal_hiding(ch, 0);
  act("@wYou grab and start dragging @C$N@W.@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W grabs and starts dragging @c$N@W.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  DRAGGING(ch) = vict;
  DRAGGED(vict) = ch;
  if (!AFF_FLAGGED(vict, AFF_KNOCKED) && !AFF_FLAGGED(vict, AFF_SLEEP) && rand_number(1, 3)) {
   send_to_char(vict, "You feel your sleeping body being moved.\r\n");
   if (IS_NPC(vict) && !FIGHTING(vict)) {
    set_fighting(vict, ch);
   }
  }
 }
}

ACMD(do_stop)
{

 if (IS_NPC(ch))
  return;

 if (!FIGHTING(ch)) {
  send_to_char(ch, "You are not even fighting!\r\n");
  return;
 }
 else {
  act("@CYou move out of your fighting posture.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@c$n@C moves out of $s fighting posture.@n", TRUE, ch, 0, 0, TO_ROOM);
  stop_fighting(ch);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
}

ACMD(do_suppress)
{
 char arg[MAX_INPUT_LENGTH];
 one_argument(argument, arg);

 if (IS_NPC(ch))
  return;

 if (IS_ANDROID(ch)) {
  send_to_char(ch, "You are unable to suppress your powerlevel.\r\n");
  return;
 }
 if (GET_BONUS(ch, BONUS_ARROGANT) > 0) {
  send_to_char(ch, "You are far too arrogant to hide your strength.\r\n");
  return;
 }
 if (PLR_FLAGGED(ch, PLR_POWERUP)) {
  send_to_char(ch, "You are currently powering up, can't suppress.\r\n");
  return;
 }
 if (GET_KAIOKEN(ch)) {
  send_to_char(ch, "You are currently concentrating on kaioken!\r\n");
  return;
 }

 if (IS_NONPTRANS(ch) && !IS_ICER(ch) && IS_TRANSFORMED(ch)) {
  send_to_char(ch, "You must revert before you try and suppress.\r\n");
  return;
 }

 if (!*arg) {
  send_to_char(ch, "Suppress to what percent?\r\nSyntax: suppress (1 - 99 | release)\r\n");
  return;
 }

 if (!strcasecmp(arg, "release")) {
  if (GET_SUPPRESS(ch)) {
        reveal_hiding(ch, 0);
   act("@GYou stop suppressing your current powerlevel!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@G$n smiles as a rush of power erupts around $s body briefly.@n", TRUE, ch, 0, 0, TO_ROOM);
   GET_HIT(ch) += GET_SUPP(ch);
   GET_SUPP(ch) = 0;
   if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
    GET_HIT(ch) = gear_pl(ch);
   }
   GET_SUPPRESS(ch) = 0;
   return;
  } else {
   send_to_char(ch, "You are not suppressing!\r\n");
   return;
  }
 }

 int num = atoi(arg);

 if (num > 99 || num <= 0) {
  send_to_char(ch, "Out of suppression range.\r\nSyntax: suppress (1 - 99 | release)\r\n");
  return;
 }

 int64_t max = gear_pl(ch);
 int64_t amt = ((max * 0.01) * num);
 
 if (GET_HIT(ch) < amt && GET_SUPPRESS(ch) == 0) {
  send_to_char(ch, "You are already below %d percent of your max!\r\n", num);
  return;
 } else if (GET_HIT(ch) < amt && GET_SUPPRESS(ch) != 0) {
  if (GET_SUPP(ch) + GET_HIT(ch) < amt) {
   send_to_char(ch, "You do not have enough suppressed to raise your powerlevel by that much.\r\n");
   return;
  } else {
         reveal_hiding(ch, 0);
   act("@GYou alter your suppression level!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@G$n seems to concentrate for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
   int64_t diff = amt - GET_HIT(ch);
   GET_SUPP(ch) -= diff;
   GET_HIT(ch) += diff;
   GET_SUPPRESS(ch) = num;
   return;
  }
 } else if (GET_HIT(ch) > amt && GET_SUPPRESS(ch) != 0) {
        reveal_hiding(ch, 0);
  act("@GYou alter your suppression level!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@G$n seems to concentrate for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
  int64_t diff = GET_HIT(ch) - amt;
  GET_SUPP(ch) += diff;
  GET_HIT(ch) -= diff;
  GET_SUPPRESS(ch) = num;
  return;
 }

 if (GET_HIT(ch) == amt) {
  send_to_char(ch, "You are already at %d percent of your max!\r\n", num);
  return;
 }
       reveal_hiding(ch, 0);
 act("@GYou suppress your current powerlevel!@n", TRUE, ch, 0, 0, TO_CHAR);
 act("@G$n seems to concentrate for a moment.@n", TRUE, ch, 0, 0, TO_ROOM);
 int64_t diff = GET_HIT(ch) - amt;
 GET_SUPP(ch) = diff;
 GET_HIT(ch) = amt;
 GET_SUPPRESS(ch) = num;
 return;
}

ACMD(do_hass)
{

 int perc = 0, prob = 0;

 if (!check_skill(ch, SKILL_HASSHUKEN)) {
   return;
 }
 if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 30) {
   send_to_char(ch, "You do not have enough stamina.\r\n");
   return;
 }
 
 perc = init_skill(ch, SKILL_HASSHUKEN);
 prob = axion_dice(0);

 if (perc < prob) {
         reveal_hiding(ch, 0);
  act("@WYou try to move your arms at incredible speeds but screw up and waste some of your stamina.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W tries to move $s arms at incredible speeds but screws up and wastes some of $s stamina.@n", TRUE, ch, 0, 0, TO_ROOM);
  GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 30;
  improve_skill(ch, SKILL_HASSHUKEN, 0);
  return;
 }
 else {
        reveal_hiding(ch, 0);
  act("@WYou concentrate and start to move your arms at incredible speeds.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W concentrates and starts to move $s arms at incredible speeds.@n", TRUE, ch, 0, 0, TO_ROOM);
  int duration = perc / 15;
  assign_affect(ch, AFF_HASS, SKILL_HASSHUKEN, duration, 0, 0, 0, 0, 0, 0);
  GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 30;
  improve_skill(ch, SKILL_HASSHUKEN, 0);
  return;
 }
}

ACMD(do_implant)
{
 
 struct obj_data *limb = NULL, *obj = NULL, *next_obj;
 struct char_data *vict = NULL;
 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];
 int found = FALSE;

 two_arguments(argument, arg, arg2);

  if (!limb_ok(ch, 0)) {
   return;
  }

 if (!*arg) {
  send_to_char(ch, "Syntax: implant (rarm | larm | rleg | lleg) (target)\r\n");
  return;
 }
 
 if (!*arg2) {
  vict = ch;
 }
 else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "That person isn't here.\r\n");
   return;
 }

 for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
   if (found == FALSE && GET_OBJ_VNUM(obj) == 66 && (!OBJ_FLAGGED(obj, ITEM_BROKEN) && !OBJ_FLAGGED(obj, ITEM_FORGED))) {
    found = TRUE;
    limb = obj;
   }
 }
 if (found == FALSE) {
  send_to_char(ch, "You do not have a cybernetic limb to implant.\r\n");
  return;
 }

 if (!(strcmp(arg, "rarm"))) {
  if (GET_LIMBCOND(vict, 1) >= 1) {
   if (vict != ch) {
   send_to_char(ch, "They already have a right arm!\r\n");
   }
   if (vict == ch) {
   send_to_char(ch, "You already have a right arm!\r\n");
   }
   return;
  }
  else {
   if (vict != ch) {
          reveal_hiding(ch, 0);
    act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_CHAR);
    act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_VICT);
    act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, vict, TO_NOTVICT);
   }
   if (vict == ch) {
         reveal_hiding(ch, 0);
   act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, 0, TO_CHAR);
   act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right arm!@n", TRUE, ch, limb, 0, TO_ROOM);
   }
   SET_BIT_AR(PLR_FLAGS(vict), PLR_CRARM);
   obj_from_char(limb);
   extract_obj(limb);
   return;
  }
 }
 else if (!(strcmp(arg, "larm"))) {
  if (GET_LIMBCOND(vict, 2) >= 1) {
   if (vict != ch) {
   send_to_char(ch, "They already have a left arm!\r\n");
   }
   if (vict == ch) {
   send_to_char(ch, "You already have a left arm!\r\n");
   }
   return;
  }
  else {
   if (vict && vict != ch) {
          reveal_hiding(ch, 0);
    act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_CHAR);
    act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_VICT);
    act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, vict, TO_NOTVICT);
   }
   if (vict == ch) {
         reveal_hiding(ch, 0);
   act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, 0, TO_CHAR);
   act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left arm!@n", TRUE, ch, limb, 0, TO_ROOM);
   }
   SET_BIT_AR(PLR_FLAGS(vict), PLR_CLARM);
   obj_from_char(limb);
   extract_obj(limb);
   return;
  }
 }
 else if (!(strcmp(arg, "rleg"))) {
  if (GET_LIMBCOND(vict, 3) >= 1) {
   if (vict != ch) {
   send_to_char(ch, "They already have a right leg!\r\n");
   }
   if (vict == ch) {
   send_to_char(ch, "You already have a right leg!\r\n");
   }
   return;
  }
  else {
   if (vict && vict != ch) {
          reveal_hiding(ch, 0);
    act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_CHAR);
    act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_VICT);
    act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, vict, TO_NOTVICT);
   }
   if (vict == ch) {
         reveal_hiding(ch, 0);
   act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, 0, TO_CHAR);
   act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new right leg!@n", TRUE, ch, limb, 0, TO_ROOM);
   }
   SET_BIT_AR(PLR_FLAGS(vict), PLR_CRLEG);
   obj_from_char(limb);
   extract_obj(limb);
   return;
  }
 }
 else if (!(strcmp(arg, "lleg"))) {
  if (GET_LIMBCOND(vict, 4) >= 1) {
   if (vict != ch) {
   send_to_char(ch, "They already have a left leg!\r\n");
   }
   if (vict == ch) {
   send_to_char(ch, "You already have a left leg!\r\n");
   }
   return;
  }
  else {
   if (vict && vict != ch) {
          reveal_hiding(ch, 0);
    act("@WYou place the $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_CHAR);
    act("@C$n@W places a $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_VICT);
    act("@C$n@W places a $p@W up to @c$N@W's body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, vict, TO_NOTVICT);
   }
   if (!vict || vict == ch) {
         reveal_hiding(ch, 0);
   act("@WYou place the $p@W up to your body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, 0, TO_CHAR);
   act("@C$n@W places the $p@W up to $s body. It automaticly adjusts itself, becoming a new left leg!@n", TRUE, ch, limb, 0, TO_ROOM);
   }
   SET_BIT_AR(PLR_FLAGS(vict), PLR_CLLEG);
   obj_from_char(limb);
   extract_obj(limb);
   return;
  }
 }
 else {
  send_to_char(ch, "Syntax: implant (rarm | larm | rleg | rleg)\r\n");
  return;
 }
}

ACMD(do_pose)
{

 if (!know_skill(ch, SKILL_POSE)) {
  return;
 }

 if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 40) {
  send_to_char(ch, "You do not have enough stamina to pull off such an exciting pose!\r\n");
  return;
 }

 if (PLR_FLAGGED(ch, PLR_POSE)) {
  send_to_char(ch, "You are already feeling good and confident from a previous pose.\r\n");
  return;
 }

 if (FIGHTING(ch)) {
  send_to_char(ch, "You are too busy to pose right now!\r\n");
  return;
 }

 int prob = GET_SKILL(ch, SKILL_POSE);
 int perc = rand_number(1, 70);

  if (ch->real_abils.str + 8 > 25 && GET_BONUS(ch, BONUS_WIMP) > 0) {
   send_to_char(ch, "You can't handle having your strength increased beyond 25.\r\n");
   return;
  }
  if (ch->real_abils.dex + 8 > 25 && GET_BONUS(ch, BONUS_CLUMSY) > 0) {
   send_to_char(ch, "You can't handle having your agility increased beyond 25.\r\n");
   return;
  }

 if (prob < perc) {
        reveal_hiding(ch, 0);
  act("@WYou attempt to strike an awe inspiring pose, but end up falling on your face!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W attempts to strike an awe inspiring pose, but ends up falling on $s face!@n", TRUE, ch, 0, 0, TO_ROOM);
  GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 40;
  improve_skill(ch, SKILL_POSE, 0);
  return;
 }
 else {
        reveal_hiding(ch, 0);
  switch (rand_number(1, 4)) {
   case 1:
   act("@WYou turn around with your back to everyone. You bend forward dramatically and put your head between your legs!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@W turns around with $s back to you. $e bends forward dramatically and puts $s head between $s legs. Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
   break;
   case 2:
   act("@WYou turn to the side while flexing your muscles and extend your arms up at an angle dramatically!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@W turns to the side while flexing $s muscles and extending $s arms up at an angle dramatically!@n", TRUE, ch, 0, 0, TO_ROOM);  
   break;
   case 3:
   act("@WYou extend one leg outward while you bend forward, balancing on a single leg!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@W extends one leg outward while $e bends forward, balancing on a single leg!@n", TRUE, ch, 0, 0, TO_ROOM);  
   break;
   case 4:
   act("@WYou drop down to one knee while angling your arms up to either side and slanting your hands down like wings!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@W drops down to one knee while angling $s arms up to either side and slanting $s hands down like wings!@n", TRUE, ch, 0, 0, TO_ROOM);  
   break;
  }
  send_to_char(ch, "@WYou feel your confidence increase! @G+3 Str @Wand@G +3 Agl!@n\r\n");
  ch->real_abils.str += 8;
  ch->real_abils.dex += 8;
  save_char(ch);
  int64_t before = GET_LIFEMAX(ch);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_POSE);
  GET_LIFEFORCE(ch) += GET_LIFEMAX(ch) - before;
  if (GET_LIFEFORCE(ch) > GET_LIFEMAX(ch)) {
   GET_LIFEFORCE(ch) = GET_LIFEMAX(ch);
  }
  GET_MOVE(ch) -= GET_MAX_MOVE(ch) / 40;
  improve_skill(ch, SKILL_POSE, 0);
  return;
 }

}

/* do_fury for halfbreeds to release their raaaage, rawrg! */

ACMD(do_fury) {

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!IS_HALFBREED(ch) || IS_NPC(ch)) {
   send_to_char(ch, "You are furious, but you'll get over it.\r\n");
   return;
  }

  if (PLR_FLAGGED(ch, PLR_FURY)) {
   send_to_char(ch, "You are already furious, your next attack will devestate, hurry use it!\r\n");
   return;
  }
  
  if (GET_FURY(ch) < 100) {
   send_to_char(ch, "You do not have enough anger to release your fury upon your foes!\r\n");
   return;
  }

  if (!*arg) {
   if (GET_HIT(ch) < gear_pl(ch)) {
    if (GET_LIFEFORCE(ch) >= GET_LIFEMAX(ch) * 0.2) {
     GET_HIT(ch) = gear_pl(ch);
     GET_LIFEFORCE(ch) -= GET_LIFEMAX(ch) * 0.2;
    } else {
     GET_HIT(ch) += GET_LIFEFORCE(ch);
     GET_LIFEFORCE(ch) = -1;
     if (GET_HIT(ch) > gear_pl(ch)) {
      GET_HIT(ch) = gear_pl(ch);
     }
    }
   }
   GET_FURY(ch) = 0;
  } else if (!strcasecmp(arg, "attack")) {
   GET_FURY(ch) = 50;
  } else {
   send_to_char(ch, "Syntax: fury (attack) <--- this will not use up your LF to restore PL.\n        fury <--- fury by itself will do both LF to PL restore and attack boost.\r\n");
   return;
  }

  reveal_hiding(ch, 0);
  act("You release your fury! Your very next attack is guaranteed to rip your foes a new one!", TRUE, ch, 0, 0, TO_CHAR);
  act("$n screams furiously as a look of anger appears on $s face!", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_FURY);
}

/* End of do_fury for halfbreeds to release their raaage, rawrg! */

void hint_system(struct char_data *ch, int num)
{
 const char *hints[22] = {"Remember to save often.", /* 0 */
                   "Remember to eat or drink if you want to stay alive.", /* 1 */
                   "It is a good idea to save up PS for learning skills instead of just practicing them.", /* 2 */
                   "A good way to save up money is with the bank.", /* 3 */
                   "If you want to stay alive in this rough world you will need to be mindful of your surroundings.", /* 4 */
                   "Knowing when to rest and recover can be the difference between life and death.", /* 5 */
                   "Not every battle can be won. Great warriors know how to pick their fights.", /* 6 */
                   "It is a good idea to experiment with skills fully before deciding their worth.", /* 7 */
                   "Having a well balanced repertoire of skills can help you out of any situation.", /* 8 */
                   "You can become hidden from your enemies on who and ooc with the whohide command.", /* 9 */
                   "You can value an item at a shopkeeper with the value command.", /* 10 */
                   "There are ways to earn money through jobs, try looking for a job. Bum.", /* 11 */
                   "You never know what may be hidden nearby. You should always check out anything you can.", /* 12 */
                   "You should check for a help file on any subject you can, you never know how the info may 'help' you.", /* 13 */
                   "Until you are capable of taking care of yourself for long periods of time you should stick near your sensei.", /* 14 */
                   "You shouldn't travel to other planets until you have a stable supply of money.", /* 15 */
                   "There is a vast galaxy out there that you may not be able to reach by public ship.", /* 16 */
                   "Score is used to view the various statistics about your character.", /* 17 */
                   "Status is used to view what is influencing your character and its characteristics.", /* 18 */
                   "You will need a scouter in order to use the Scouter Network (SNET).", /* 19 */
                   "The DBAT forum is a great resource for finding out information and for conversing\r\nwith fellow players. http://advent-truth.com/forum", /* 20 */
                   "Found a bug or have a suggestion? Log into our forums and post in the relevant section."
                   };
 if (num == 0) {
  num = rand_number(0, 21);
 }
 
 if (!IS_ANDROID(ch) && !IS_NAMEK(ch)) {
  send_to_char(ch, "@D[@GHint@D] @G%s@n\r\n", hints[num]);
 } 
 else {
  if (num == 1) {
   num = 0;
  }
  send_to_char(ch, "@D[@GHint@D] @G%s@n\r\n", hints[num]);
 }
  send_to_char(ch, "@D(@gYou can turn off hints with the command 'hints'@D)@n\r\n");
}

ACMD(do_think)
{
 skip_spaces(&argument);

 if (IS_NPC(ch)) {
  return;
 }
 if (IN_ARENA(ch)) {
  send_to_char(ch, "Lol, no.\r\n");
  return;
 }
 if (GET_SKILL(ch, SKILL_TELEPATHY)) {
  send_to_char(ch, "You can just use telepathy.\r\n");
  return;
 }
 if (!MINDLINK(ch)) {
  send_to_char(ch, "No one has linked with your mind.\r\n");
  return;
 }
 if (!*argument) {
  send_to_char(ch, "Syntax: think (message)\r\n");
  return;
 }
 else {
  struct char_data *tch;
  tch = MINDLINK(ch);
  send_to_char(ch, "@c%s@w reads your thoughts, '@C%s@w'@n\r\n", GET_NAME(tch), argument);
  send_to_char(tch, "@c%s@w thinks, '@C%s@w'@n\r\n", GET_NAME(ch), argument);
  send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),  GET_ADMLEVEL(tch) > 0 ? GET_NAME(tch) : GET_USER(tch), argument);
  return;
 }
}

ACMD(do_telepathy)
{
 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];

 half_chop(argument, arg, arg2);
 
 if (!know_skill(ch, SKILL_TELEPATHY)) {
  return;
 }

 if (IN_ARENA(ch)) {
  send_to_char(ch, "Lol, no.\r\n");
  return;
 }

 if (!*arg) {
  send_to_char(ch, "Syntax: telepathy [ read ] (target)\r\n"
                   "        telepathy [ link ] (target)\r\n"
                   "        telepathy [  far ] (target)\r\n"
                   "        telepathy (target) (message)\r\n");
  return;
 }

 else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 40) {
  send_to_char(ch, "You do not have enough ki to focus your mental abilities.\r\n");
  return;
 }
 if (!(strcmp(arg, "far"))) {
  if (IS_NPC(ch)) {
   return;
  } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
   send_to_char(ch, "Look through who's eyes?\r\n");
   return;
  } else if (vict == ch) {
   send_to_char(ch, "Oh that makes a lot of sense...\r\n");
   return;
  } else if (IS_NPC(vict)) {
   send_to_char(ch, "You can't touch the mind of such a thing.\r\n");
   return;
  } else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) {
   send_to_char(ch, "Their mental power oustrips your's by unfathomable measurements!\r\n");
   return;
  } else if (AFF_FLAGGED(ch, AFF_SHOCKED)) {
   send_to_char(ch, "Your mind has been shocked by telepathic feedback! You are not able to use telepathy right now.\r\n");
   return;
  } else if (IS_ANDROID(vict)) {
   send_to_char(ch, "You can't touch the mind of such an artificial being.\r\n");
   return;
  } else if (GET_SKILL(vict, SKILL_TELEPATHY) + GET_INT(vict) > GET_SKILL(ch, SKILL_TELEPATHY) + GET_INT(ch)) {
   send_to_char(ch, "They throw off your attempt with their own telepathic abilities!\r\n");
   return;
  } else if (IN_ROOM(ch) == IN_ROOM(vict)) {
   send_to_char(ch, "They are in the same room as you!\r\n");
   return;
  } else if (AFF_FLAGGED(vict, AFF_BLIND)) {
   send_to_char(ch, "They are blind!\r\n");
   return;
  } else if (PLR_FLAGGED(vict, PLR_EYEC)) {
   send_to_char(ch, "Their eyes are closed!\r\n");
   return;
  } else {
   look_at_room(IN_ROOM(vict), ch, 0);
   send_to_char(ch, "You see all this through their eyes!\r\n");
   if (GET_INT(vict) > GET_INT(ch)) {
    send_to_char(ch, "You feel like someone was using your mind for something...\r\n");
   }
   GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
   return;  
  }
 }
 if (!(strcmp(arg, "link"))) {
  if (IS_NPC(ch)) {
   return;
  }
  if (MINDLINK(ch)) {
   act("@CYou remove the link your mind had with @w$N.@n", TRUE, ch, 0, MINDLINK(ch), TO_CHAR);
   act("@w$n@C removes the link $s mind had with yours.@n", TRUE, ch, 0, MINDLINK(ch), TO_VICT);
   MINDLINK(MINDLINK(ch)) = NULL;
   MINDLINK(ch) = NULL;
   LINKER(ch) = 0;
   return;
  } else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD))) {
   send_to_char(ch, "Link with the mind of who?\r\n");
   return;
  } else if (vict == ch) {
   send_to_char(ch, "Oh that makes a lot of sense...\r\n");
   return;
  } else if (IS_NPC(vict)) {
   send_to_char(ch, "You can't touch the mind of such a thing.\r\n");
   return;
  } else if (IS_ANDROID(vict)) {
   send_to_char(ch, "You can't touch the mind of such an artificial being.\r\n");
   return;
  } else if (GET_SKILL(vict, SKILL_TELEPATHY)) {
   send_to_char(ch, "Kinda pointless when you are both telepathic huh?\r\n");
   return;
  } else if (MINDLINK(vict)) {
   send_to_char(ch, "Someone else is already telepathically linked with them.\r\n");
   return;
  } else if (GET_SKILL(ch, SKILL_TELEPATHY) < axion_dice(GET_INT(vict) * 0.1)) {
   act("@R$n@r tried to link $s mind with yours, but you manage to force a break in the link!@n", FALSE, ch, 0, vict, TO_VICT);
   act("@R$N@r manages to sense the intrusion and with $S intelligence push you out!@n", FALSE, ch, 0, vict, TO_CHAR);
   return;
  } else {
   act("@CYou link your mind with @w$N.@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@w$n@C links $s mind with yours. You can speak your thoughts to $m with 'think'.@n", TRUE, ch, 0, vict, TO_VICT);
   send_to_char(vict, "@wIf this is undesirable, Try: meditate break@n\r\n");
   MINDLINK(vict) = ch;
   MINDLINK(ch) = vict;
   LINKER(ch) = 1;
   return;
  }
 }
 else if (!(strcmp(arg, "read"))) {
  if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "Read the mind of who?\r\n");
   return;
  }  
  else if (vict == ch) {
   send_to_char(ch, "Oh that makes a lot of sense...\r\n");
   return;
  }
  else if (IS_ANDROID(vict)) {
   send_to_char(ch, "You can't touch the mind of such an artificial being.\r\n");
   return;
  }
  else {
   if (axion_dice(0) > GET_SKILL(ch, SKILL_TELEPATHY)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
    act("@wYou attempt to read $N's@w mind, but fail to see it clearly.@n", TRUE, ch, 0, vict, TO_CHAR);
    if (rand_number(1, 15) >= 14 && !AFF_FLAGGED(ch, AFF_SHOCKED)) {
     act("@MYour mind has been shocked!@n", TRUE, ch, 0, 0, TO_CHAR);
     SET_BIT_AR(AFF_FLAGS(ch), AFF_SHOCKED);
    } else {
     improve_skill(ch, SKILL_TELEPATHY, 0);
    }
    return;
   }
   else if (GET_SKILL(vict, SKILL_TELEPATHY) >= GET_SKILL(ch, SKILL_TELEPATHY) && rand_number(1, 2) == 2) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
    act("@wYou fail to read @c$N's@w mind and they seemed to have noticed the attempt!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@w attempts to read your mind, but you resist and force $m out!@n", TRUE, ch, 0, vict, TO_VICT);
    improve_skill(ch, SKILL_TELEPATHY, 0);
    return;
   }
   else {
    send_to_char(ch, "@wYou peer into their mind:\r\n");
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
    send_to_char(ch, "@GName      @D: @W%s@n\r\n", GET_NAME(vict));
    send_to_char(ch, "@GRace      @D: @W%s@n\r\n", pc_race_types[(int)GET_RACE(vict)]);
    send_to_char(ch, "@GSensei    @D: @W%s@n\r\n", pc_class_types[(int)GET_CLASS(vict)]);
    send_to_char(ch, "@GStr       @D: @W%d@n\r\n", GET_STR(vict));
    send_to_char(ch, "@GCon       @D: @W%d@n\r\n", GET_CON(vict));
    send_to_char(ch, "@GInt       @D: @W%d@n\r\n", GET_INT(vict));
    send_to_char(ch, "@GWis       @D: @W%d@n\r\n", GET_WIS(vict));
    send_to_char(ch, "@GSpd       @D: @W%d@n\r\n", GET_CHA(vict));
	send_to_char(ch, "@GAgi       @D: @W%d@n\r\n", GET_DEX(vict));
    send_to_char(ch, "@GZenni     @D: @W%s@n\r\n", add_commas(GET_GOLD(vict)));
    send_to_char(ch, "@GBank Zenni@D: @W%s@n\r\n", add_commas(GET_BANK_GOLD(vict)));
    if (GET_ALIGNMENT(vict) >= 1000) {
     send_to_char(ch, "@GAlignment @D: @wSaint         @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > 750) {
     send_to_char(ch, "@GAlignment @D: @wExtremely Good@n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > 500) {
     send_to_char(ch, "@GAlignment @D: @wReally Good   @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > 250) {
     send_to_char(ch, "@GAlignment @D: @wGood          @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > 100) {
     send_to_char(ch, "@GAlignment @D: @wPretty Good   @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > 50) {
     send_to_char(ch, "@GAlignment @D: @wSorta Good    @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > -50) {
     send_to_char(ch, "@GAlignment @D: @wNeutral       @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > -100) {
     send_to_char(ch, "@GAlignment @D: @wSorta Evil    @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) > -500) {
     send_to_char(ch, "@GAlignment @D: @wPretty Evil   @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) >= -750) {
     send_to_char(ch, "@GAlignment @D: @wEvil          @n\r\n");
    }
    else if (GET_ALIGNMENT(vict) < -750) {
     send_to_char(ch, "@GAlignment @D: @wExtremely Evil@n\r\n");
    }
    else if (GET_ALIGNMENT(vict) <= -1000) {
     send_to_char(ch, "@GAlignment @D: @wDevil         @n\r\n");
    }
    else {
     send_to_char(ch, "@GAlignment @D: @wUnknown       @n\r\n");
    }
   } // End of read success
  } // End of vict is there
  improve_skill(ch, SKILL_TELEPATHY, 0);
  return;
 } // End of read argument

 else {
  if (MINDLINK(ch)) {
   vict = MINDLINK(ch);
  }
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
   send_to_char(ch, "Send your thoughts to who?\r\n");
   return;
  }  
  if (vict == ch) {
   send_to_char(ch, "Oh that makes a lot of sense...\r\n");
   return;
  }
  else if (IS_ANDROID(vict)) {
   send_to_char(ch, "You can't touch the mind of such an artificial being.\r\n");
   return;
  }
  else {
   if (!MINDLINK(ch)) {
    send_to_char(ch, "@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", GET_NAME(vict), arg2);
    send_to_char(vict, "@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", GET_NAME(ch), arg2);
    send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),  GET_ADMLEVEL(vict) > 0 ? GET_NAME(vict) : GET_USER(vict), arg2);
   }
   else {
    send_to_char(ch, "@WYou tell @c%s@W telepathically, @w'@C%s@w'@n\r\n", GET_NAME(vict), argument);
    send_to_char(vict, "@c%s@W talks to you telepathically, @w'@C%s@w'@n\r\n", GET_NAME(ch), argument);
    send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", GET_ADMLEVEL(ch) > 0 ? GET_NAME(ch) : GET_USER(ch),  GET_ADMLEVEL(vict) > 0 ? GET_NAME(vict) : GET_USER(vict), argument);
   }
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 40;
  } // End of vict is here
  return;
 } // End of send argument

}

ACMD(do_potential)
{
 int boost = 0;

 struct char_data *vict;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (!know_skill(ch, SKILL_POTENTIAL)) {
  return;
 }

 if (!*arg) {
  send_to_char(ch, "Who's potential do you want to release?\r\n");
  send_to_char(ch, "Potential Releases: %d\r\n", GET_BOOSTS(ch));
  return;
 }
 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "That target isn't here.\r\n");
  return;
 }

 if (IS_NPC(vict)) {
  send_to_char(ch, "Why would you waste your time releasing their potential?\r\n");
  return;
 }
 if (vict == ch) {
  send_to_char(ch, "You can't release your own potential.\r\n");
  return;
 }
 if (GET_BOOSTS(ch) == 0) {
   send_to_char(ch, "You have no potential releases to perform.\r\n");
   return;
 }
 if (PLR_FLAGGED(vict, PLR_PR)) {
  send_to_char(ch, "Their potential has already been released\r\n");
  return;
 }
 if (IS_ANDROID(vict)) {
  send_to_char(ch, "They are a machine and have no potential to release.\r\n");
  return;
 }
 if (MAJINIZED(vict) > 0) {
  send_to_char(ch, "They are already majinized and have no potential to release.\r\n");
  return;
 }
 if (IS_MAJIN(vict)) {
  send_to_char(ch, "They have no potential to release...\r\n");
  return;
 }
 /* Rillao: transloc, add new transes here */
 else {
  boost = GET_SKILL(ch, SKILL_POTENTIAL) / 2;
           double mult = 1;
           if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS1)) {
            mult = 3;
           }
           else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS2)) {
            mult = 4;
           }
           else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS3)) {
            mult = 5;
           }
           else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 1) {
            mult = 2;
           }
           else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 2) {
            mult = 3;
           }
           else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS1)) {
            mult = 2;
           }
           else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS2)) {
            mult = 3;
           }
           else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS3)) {
            mult = 3.5;
           }
           else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS4)) {
            mult = 4;
           }
  SET_BIT_AR(PLR_FLAGS(vict), PLR_PR);
  GET_MAX_HIT(vict) += ((GET_BASE_PL(vict) / 100) * boost) * mult;
  GET_BASE_PL(vict) += (GET_BASE_PL(vict) / 100) * boost;
  if (IS_HALFBREED(vict)) {
     GET_MAX_MANA(vict) += ((GET_BASE_KI(vict) / 100) * boost) * mult;
     GET_BASE_KI(vict) += (GET_BASE_KI(vict) / 100) * boost;
     GET_MAX_MOVE(vict) += ((GET_BASE_ST(vict) / 100) * boost) * mult;
     GET_BASE_ST(vict) += (GET_BASE_ST(vict) / 100) * boost;
  }
        reveal_hiding(ch, 0);
  act("You place your hand on top of $N's head. After a moment of concentrating you release their hidden potential.", TRUE, ch, 0, vict, TO_CHAR);
  act("$n places $s hand on top of your head. After a moment you feel a rush of power as your hidden potential is released!", TRUE, ch, 0, vict, TO_VICT);
  act("$n places $s hand on $N's head. After a moment a rush of power explodes off of $N's body!", TRUE, ch, 0, vict, TO_NOTVICT);
  improve_skill(ch, SKILL_POTENTIAL, 0);
  improve_skill(ch, SKILL_POTENTIAL, 0);
  improve_skill(ch, SKILL_POTENTIAL, 0);
  improve_skill(ch, SKILL_POTENTIAL, 0);
  GET_BOOSTS(ch) -= 1;
  return;
 }
}

ACMD(do_majinize)
{

	struct char_data *vict;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!IS_MAJIN(ch))
	{
		send_to_char(ch, "You are not a majin and can not majinize anyone.\r\n");
		return;
	}

	if (!*arg)
	{
		send_to_char(ch, "Who do you want to majinize?\r\n");
		return;
	}

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(ch, "That target isn't here.\r\n");
		return;
	}

	if (IS_NPC(vict))
	{
		send_to_char(ch, "Why would you waste your time majinizing them?\r\n");
		return;
	}
	if (vict == ch)
	{
		send_to_char(ch, "You can't majinize yourself.\r\n");
		return;
	}
	if (PLR_FLAGGED(vict, PLR_PR))
	{
		send_to_char(ch, "You can't majinize them their potential has been released!\r\n");
		return;
	}
	int alignmentTotal = GET_ALIGNMENT(ch) - GET_ALIGNMENT(vict);
	if (MAJINIZED(vict) > 0 && MAJINIZED(vict) != GET_ID(ch))
	{
		send_to_char(ch, "They are already majinized before by someone else.\r\n");
		return;
	}
	else if ((vict->master != ch))
	{
		send_to_char(ch, "They must be following you in order for you to majinize them.\r\n");
		return;
	}

	else if (!((alignmentTotal >= -1500) && (alignmentTotal <= 1500)))
	{
		send_to_char(ch, "Their alignment is so opposed to your's that they resist your attempts to enslave them!\r\n");
		return;
	}
	else if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 4)
	{
		send_to_char(ch, "Their powerlevel is so much higher than yours they resist your attempts to enslave them!\r\n");
		return;
	}
	/* Rillao: transloc, add new transes here */
	else if (MAJINIZED(vict) > 0 && MAJINIZED(vict) == GET_ID(ch))
	{
		reveal_hiding(ch, 0);
		act("You remove $N's majinization, freeing them from your influence, but also weakening them.", TRUE, ch, 0, vict, TO_CHAR);
		act("$n removes your majinization, freeing you from their influence, and weakening you!", TRUE, ch, 0, vict, TO_VICT);
		act("$n waves a hand at $N, and instantly the glowing M on $S forehead disappears!", TRUE, ch, 0, vict, TO_NOTVICT);
		MAJINIZED(vict) = 0;
		GET_BOOSTS(ch) += 1;
		double mult = 1;
		if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 3;
		}
		else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 4;
		}
		else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 5;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 2;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 3;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 3.5;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS4))
		{
			mult = 4;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 2;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 3;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 4.5;
		}
		else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 1)
		{
			mult = 2;
		}
		else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 2)
		{
			mult = 3;
		}
		if (GET_MAJINIZED(vict) == 0)
		{
			GET_MAJINIZED(vict) = (GET_BASE_PL(vict) * .4) * mult;
		}
		GET_MAX_HIT(vict) -= GET_MAJINIZED(vict) * mult;
		if (GET_MAJINIZED(vict) == 0)
		{
			GET_MAJINIZED(vict) = (GET_BASE_PL(vict) * .4);
		}
		GET_BASE_PL(vict) -= GET_MAJINIZED(vict);
		return;
	}
	else if (GET_BOOSTS(ch) == 0)
	{
		send_to_char(ch, "You are incapable of majinizing%s.\r\n", GET_LEVEL(ch) < 100 ? " right now" : " anymore");
		if (GET_LEVEL(ch) < 25)
		{
			send_to_char(ch, "Your next available majinize will be at level 25\r\n");
		}
		else if (GET_LEVEL(ch) < 50)
		{
			send_to_char(ch, "Your next available majinize will be at level 50\r\n");
		}
		else if (GET_LEVEL(ch) < 75)
		{
			send_to_char(ch, "Your next available majinize will be at level 75\r\n");
		}
		else if (GET_LEVEL(ch) < 100)
		{
			send_to_char(ch, "Your next available majinize will be at level 100\r\n");
		}
		return;
	}
	else
	{
		reveal_hiding(ch, 0);
		act("You focus your power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", TRUE, ch, 0, vict, TO_CHAR);
		act("$n focuses power into you, influencing your mind and increasing your strength! After the struggle in your mind ends a glowing purple M forms on your forehead.", TRUE, ch, 0, vict, TO_VICT);
		act("$n focuses power into $N, influencing their mind and increasing their strength! After the struggle ends in $S mind a glowing purple M forms on $S forehead.", TRUE, ch, 0, vict, TO_NOTVICT);
		MAJINIZED(vict) = GET_ID(ch);
		GET_BOOSTS(ch) -= 1;
		double mult = 1;
		if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 3;
		}
		else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 4;
		}
		else if (IS_TRUFFLE(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 5;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 2;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 3;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 3.5;
		}
		else if (IS_BIO(vict) && PLR_FLAGGED(vict, PLR_TRANS4))
		{
			mult = 4;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS1))
		{
			mult = 2;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS2))
		{
			mult = 3;
		}
		else if (IS_MAJIN(vict) && PLR_FLAGGED(vict, PLR_TRANS3))
		{
			mult = 4.5;
		}
		else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 1)
		{
			mult = 2;
		}
		else if (IS_HOSHIJIN(vict) && GET_PHASE(vict) == 2)
		{
			mult = 3;
		}
		GET_MAJINIZED(vict) = GET_BASE_PL(vict) * .4;
		GET_MAX_HIT(vict) += (GET_BASE_PL(vict) * .4) * mult;
		GET_BASE_PL(vict) += GET_BASE_PL(vict) * .4;
		return;
	}

}

ACMD(do_spit)
{
 int cost = 0;
 struct char_data *vict;
 struct affected_type af;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (!know_skill(ch, SKILL_SPIT)) {
  return;
 }
 if (FIGHTING(ch)) {
  send_to_char(ch, "You can't manage to spit in this fight!\r\n");
  return;
 }
 
 if (!*arg) {
  send_to_char(ch, "Yes but who do you want to petrify?\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "That target isn't here.\r\n");
  return;
 }
  if (!can_kill(ch, vict, NULL, 0)) {
   return;
  }
 if (AFF_FLAGGED(vict, AFF_PARALYZE)) {
  act("$N has already been turned to stone.", TRUE, ch, 0, vict, TO_CHAR);
  return;
 }
 if (FIGHTING(vict)) {
  send_to_char(vict, "You can't manage to spit on them, they are moving around too much!\r\n");
  return;
 }

 cost = ((GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_SPIT) / 4)) + GET_MAX_MANA(ch) / 100);

 if (GET_MANA(ch) < cost) {
  send_to_char(ch, "You do not have enough ki to petrifiy with your spit!\r\n");
  return;
 }

 if (GET_SKILL(ch, SKILL_SPIT) < axion_dice(0)) {
  GET_MANA(ch) -= cost;
        reveal_hiding(ch, 0);
  act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, but it falls short of hitting @c$N@W!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W seems to focus ki before hawking a loogie at you! Fortunatly the loogie falls short.@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! Fortunatly for @c$N@W the loogie falls short.@n", TRUE, ch, 0, vict, TO_NOTVICT);
  improve_skill(ch, SKILL_SPIT, 1);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
 else if (AFF_FLAGGED(vict, AFF_ZANZOKEN) && GET_MOVE(vict) >= 1 && GET_POS(vict) != POS_SLEEPING) {
  GET_MANA(ch) -= cost;
           reveal_hiding(ch, 0);
     act("@C$N@c disappears, avoiding your spit before reappearing!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou disappear, avoiding @C$n's@c @rstone spit@c before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, avoiding @C$n's@c @rstone spit@c before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     WAIT_STATE(ch, PULSE_2SEC);
     improve_skill(ch, SKILL_SPIT, 1);
     return;
 }
 else {
  af.type = SPELL_PARALYZE;
  af.duration = rand_number(1, 2);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_PARALYZE;
  affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);

  GET_MANA(ch) -= cost;
        reveal_hiding(ch, 0);
  act("@WGathering spit you concentrate ki into a wicked loogie and let it loose, and it smacks into @c$N@W turning $M into stone!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@C$n@W seems to focus ki before hawking a loogie at you! It manages to hit and you instantly turn to stone!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@C$n@W seems to focus ki before hawking a loogie at @c$N@W! It manages to hit and $E instantly turns to stone!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  improve_skill(ch, SKILL_SPIT, 1);
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }

}

/* This handles increasing the stats of a created object based on the skill *
 * and/or stats of the user.                                                */
static void boost_obj(struct obj_data *obj, struct char_data *ch, int type)
{

 if (!obj || !ch)
  return;

 int boost = 0;

 if (GET_LEVEL(ch) >= 100) {
  boost = 100;
 } else if (GET_LEVEL(ch) >= 90) {
  boost = 90;
 } else if (GET_LEVEL(ch) >= 80) {
  boost = 80;
 } else if (GET_LEVEL(ch) >= 70) {
  boost = 70;
 } else if (GET_LEVEL(ch) >= 60) {
  boost = 60;
 } else if (GET_LEVEL(ch) >= 50) {
  boost = 50;
 } else if (GET_LEVEL(ch) >= 40) {
  boost = 40;
 } else if (GET_LEVEL(ch) >= 30) {
  boost = 30;
 }

 switch (type) { /* Main switch of boost_obj */
  case 0: /* This object is a piece of worn equipment, not a weapon. */
   if (boost != 0) { /* Change it if it qualifies */
    GET_OBJ_LEVEL(obj) = boost;
    obj->affected[0].location = 17;
    obj->affected[0].modifier += (boost * GET_LEVEL(ch));
    if (GET_OBJ_VNUM(obj) == 91) {
     obj->affected[1].location = 1;
     obj->affected[1].modifier = (boost / 20);
    } else {
     obj->affected[1].location = 3;
     obj->affected[1].modifier = (boost / 20);
    }
   }
   break;
  case 1: /* This object is a weapon. */
   switch (boost) {
    case 30:
     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_WEAPLVL2);
     break;
    case 40:
    case 50:
     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_WEAPLVL3);
     break;
    case 60:
    case 70:
    case 80:
    case 90:
     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_WEAPLVL4);
     break;
    case 100:
     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_WEAPLVL5);
     break;
    default:
     SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_WEAPLVL1);
     break;
   }
   if (boost != 0) {
    GET_OBJ_LEVEL(obj) = boost;
    obj->affected[0].location = 1;
    obj->affected[0].modifier = (boost / 20);
   }
   break;
 } /* End of main switch */

}

ACMD(do_form)
{
 int skill = 0, senzu = FALSE, bag = FALSE, light = FALSE, sword = FALSE, mattress = FALSE, gi = FALSE, pants = FALSE, kachin = FALSE, boost = FALSE, shuriken = FALSE;
 int clothes = FALSE, wrist = FALSE, boots = FALSE, level = 0;
 double discount = 1.0;
 int64_t cost = 0;
 struct obj_data *obj;
 char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], clam[MAX_INPUT_LENGTH];

 half_chop(argument, arg, clam);

 half_chop(clam, arg2, arg3);

 if (!know_skill(ch, SKILL_CREATE)) {
  return;
 }

  if (GET_COOLDOWN(ch) > 0) {
   send_to_char(ch, "You must wait a short period before concentrating again.\r\n");
   return;
  }

 skill = GET_SKILL(ch, SKILL_CREATE);

 if (skill >= 100) {
  boost = TRUE;
 }
 if (skill >= 90) {
  kachin = TRUE;
 }
 if (skill >= 80) {
  senzu = TRUE;  
 }
 if (skill >= 70) {
  shuriken = TRUE;
 }
 if (skill >= 60) {
  clothes = TRUE;
 }
 if (skill >= 50) {
  sword = TRUE;
  gi = TRUE;
  pants = TRUE;
  wrist = TRUE;
  boots = TRUE;
 }
 if (skill >= 40) {
  mattress = TRUE;
 }
 if (skill >= 30) {
  bag = TRUE;
 }
 if (skill >= 20) {
  light = TRUE;
 }

 if (GET_SKILL(ch, SKILL_CONCENTRATION)) {
  if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 100) {
   discount = 0.5;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 90) {
   discount = 0.6;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 80) {
   discount = 0.65;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 70) {
   discount = 0.7;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 60) {
   discount = 0.75;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 50) {
   discount = 0.8;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 40) {
   discount = 0.85;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 30) {
   discount = 0.9;
  } else if (GET_SKILL(ch, SKILL_CONCENTRATION) >= 20) {
   discount = 0.95;
  }
 }


 if (!*arg) {
  send_to_char(ch, "What do you want to create?\r\n"
                   "@GCreation @WMenu@n\r\n"
                   "@D---------------@n\r\n"
                   "@wcreate food\r\n"
                   "create water\r\n"
                   "%s%s%s%s%s%s%s%s%s%s%s%s%s\r\n", light ? "create light\r\n" : "", bag ? "create bag\r\n" : "", mattress ? "create mattress\r\n" : "", 
sword ? "create weapon (sword | club | dagger | spear | gun )\r\n" : "", pants ? "create pants\r\n" : "", gi ? "create gi\r\n" : "", 
wrist ? "create wristband\r\n" : "", boots ? "create boots\r\n" : "", clothes ? "create clothesbeam (target)\r\n" : "", shuriken ? "create shuriken\r\n" : "", senzu ? "create senzu\r\n" : "", 
kachin ? "create kachin\r\n" : "",  boost ? "create elixir\r\n" : "");
  return;
 }  
         reveal_hiding(ch, 0);
 if (!(strcmp(arg, "food"))) {
  cost = GET_MAX_MANA(ch) / (skill / 2);
  cost *= discount;
   
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  } else {
   if (!*arg2) {
    send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
    send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
   } else if (*arg2) {
    if (!strcasecmp(arg2, "highest") && skill >= 100) {
     level = 4;
    } else if (!strcasecmp(arg2, "high") && skill >= 75) {
     level = 3;
    } else if (!strcasecmp(arg2, "mid") && skill >= 50) {
     level = 2;
    } else {
     level = 1;
    }
   }
   if (level == 4) {
    obj = read_object(1512, VIRTUAL);
    add_unique_id(obj);
   } else if (level == 3) {
    obj = read_object(1511, VIRTUAL);
    add_unique_id(obj);
   } else if (level == 2) {
    obj = read_object(1510, VIRTUAL);
    add_unique_id(obj);
   } else {
    obj = read_object(70, VIRTUAL);
    add_unique_id(obj);
   }
   obj_to_char(obj, ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "water"))) {
  cost = GET_MAX_MANA(ch) / (skill * 2);
  cost *= discount;
   
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   if (!*arg2) {
    send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
    send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
   } else if (*arg2) {
    if (!strcasecmp(arg2, "highest") && skill >= 100) {
     level = 4;
    } else if (!strcasecmp(arg2, "high") && skill >= 75) {
     level = 3;
    } else if (!strcasecmp(arg2, "mid") && skill >= 50) {
     level = 2;
    } else {
     level = 1;
    }
   }
   if (level == 4) {
    obj = read_object(1515, VIRTUAL);
    add_unique_id(obj);
   } else if (level == 3) {
    obj = read_object(1514, VIRTUAL);
    add_unique_id(obj);
   } else if (level == 2) {
    obj = read_object(1513, VIRTUAL);
    add_unique_id(obj);
   } else {
    obj = read_object(71, VIRTUAL);
    add_unique_id(obj);
   }
   obj_to_char(obj, ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "bag"))) {
  cost = GET_MAX_MANA(ch) / (skill * 2);
  cost *= discount;
   
  if (bag == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(319, VIRTUAL);
   add_unique_id(obj);
   obj_to_char(obj, ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "mattress"))) {
  cost = GET_MAX_MANA(ch) / skill;
  cost *= discount;

  if (mattress == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(16, VIRTUAL);
    add_unique_id(obj);
   obj_to_char(obj, ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "weapon"))) {
  cost = GET_MAX_MANA(ch) / 5;
  cost *= discount;

  if (sword == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   if (!*arg2) {
    send_to_char(ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
    return;
   }
   if (!*arg3) {
    send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | higher | highest)\r\n");
    send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
   } else if (*arg3) {
    if (!strcasecmp(arg3, "highest") && skill >= 100) {
     level = 5;
    } else if (!strcasecmp(arg3, "higher") && skill >= 75) {
     level = 4;
    } else if (!strcasecmp(arg3, "high") && skill >= 50) {
     level = 3;
    } else if (!strcasecmp(arg3, "mid") && skill >= 30) {
     level = 2;
    } else {
     level = 1;
    }
   }
   if (!strcasecmp(arg2, "sword")) {
    if (level == 5) {
     obj = read_object(1519, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 4) {
     obj = read_object(1518, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 3) {
     obj = read_object(1517, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 2) {
     obj = read_object(1516, VIRTUAL);
     add_unique_id(obj);
    } else {
     obj = read_object(90, VIRTUAL);
     add_unique_id(obj);
    }
   } else if (!strcasecmp(arg2, "dagger")) {
    if (level == 5) {
     obj = read_object(1540, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 4) {
     obj = read_object(1539, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 3) {
     obj = read_object(1538, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 2) {
     obj = read_object(1537, VIRTUAL);
     add_unique_id(obj);
    } else {
     obj = read_object(1536, VIRTUAL);
     add_unique_id(obj);
    }
   } else if (!strcasecmp(arg2, "club")) {
    if (level == 5) {
     obj = read_object(1545, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 4) {
     obj = read_object(1544, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 3) {
     obj = read_object(1543, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 2) {
     obj = read_object(1542, VIRTUAL);
     add_unique_id(obj);
    } else {
     obj = read_object(1541, VIRTUAL);
     add_unique_id(obj);
    }
   } else if (!strcasecmp(arg2, "spear")) {
    if (level == 5) {
     obj = read_object(1550, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 4) {
     obj = read_object(1549, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 3) {
     obj = read_object(1548, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 2) {
     obj = read_object(1547, VIRTUAL);
     add_unique_id(obj);
    } else {
     obj = read_object(1546, VIRTUAL);
     add_unique_id(obj);
    }
   } else if (!strcasecmp(arg2, "gun")) {
    if (level == 5) {
     obj = read_object(1555, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 4) {
     obj = read_object(1554, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 3) {
     obj = read_object(1553, VIRTUAL);
     add_unique_id(obj);
    } else if (level == 2) {
     obj = read_object(1552, VIRTUAL);
     add_unique_id(obj);
    } else {
     obj = read_object(1551, VIRTUAL);
     add_unique_id(obj);
    }
   } else {
    send_to_char(ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
    return;
   }
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "clothesbeam"))) {
  cost = GET_MAX_MANA(ch) / 2;
  cost *= discount;

  if (clothes == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  if (!*arg2) {
   send_to_char(ch, "Who do you want to hit with clothesbeam?\r\nSyntax: create clothesbeam (target)\r\n");
   return;
  }

  struct char_data *vict = NULL;

  if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "Clothesbeam who?\r\nSyntax: create clothesbeam (target)\r\n");
   return;
  }

  if (vict->master != ch) {
   send_to_char(ch, "They must be following you first.\r\n");
   return;
  } else {
   obj = read_object(92, VIRTUAL); /* gi */
   add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, vict);
   GET_OBJ_SIZE(obj) = get_size(vict);
   obj = read_object(91, VIRTUAL); /* pants */
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, vict);
   GET_OBJ_SIZE(obj) = get_size(vict);
   obj = read_object(1528, VIRTUAL); /* wrist */
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, vict);
   GET_OBJ_SIZE(obj) = get_size(vict);
   obj = read_object(1528, VIRTUAL); /* wrist */
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, vict);
   GET_OBJ_SIZE(obj) = get_size(vict);
   obj = read_object(1532, VIRTUAL); /* boots */
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, vict);
   GET_OBJ_SIZE(obj) = get_size(vict);
   do_wear(vict, "all", 0, 0);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "gi"))) {
  cost = GET_MAX_MANA(ch) / 5;
  cost *= discount;
  if (gi == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(92, VIRTUAL);
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 } else if (!(strcmp(arg, "shuriken"))) {
  cost = GET_MAX_MANA(ch) / 4;
  cost *= discount;

  if (shuriken == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(19053, VIRTUAL);
    add_unique_id(obj);
   obj_to_char(obj, ch);
   SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NORENT);
   SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "pants"))) {
  cost = GET_MAX_MANA(ch) / 5;
  cost *= discount;

  if (pants == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(91, VIRTUAL);
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "wristband"))) {
  cost = GET_MAX_MANA(ch) / 5;
  cost *= discount;

  if (wrist == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(1528, VIRTUAL);
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "boots"))) {
  cost = GET_MAX_MANA(ch) / 5;
  cost *= discount;

  if (boots == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(1532, VIRTUAL);
    add_unique_id(obj);
   boost_obj(obj, ch, 0);
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "light"))) {
  cost = GET_MAX_MANA(ch) / (skill * 2);
  cost *= discount;
   
  if (light == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(72, VIRTUAL);
    add_unique_id(obj);
   obj_to_char(obj, ch);
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "kachin"))) {
  cost = GET_MAX_MANA(ch) - 1;
  cost *= discount;

  if (kachin == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  else {
   obj = read_object(87, VIRTUAL);
    add_unique_id(obj);
   obj_to_room(obj, IN_ROOM(ch));
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   return;
  }
 }
 else if (!(strcmp(arg, "elixir"))) {
  cost = GET_MAX_MANA(ch) - 1;
  cost *= discount;

  if (boost == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
   return;
  }
  if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
   send_to_char(ch, "You need to be at full powerlevel to create %s\r\n", arg);
   return;
  }
  else if (GET_PRACTICES(ch, GET_CLASS(ch)) < 10) {
   send_to_char(ch, "You do not have enough PS to create %s, you need at least 10.\r\n", arg);
   return;
  }
  else {
   obj = read_object(86, VIRTUAL);
    add_unique_id(obj);
   obj_to_room(obj, IN_ROOM(ch));
   GET_OBJ_SIZE(obj) = get_size(ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   GET_HIT(ch) = 1;
   GET_PRACTICES(ch, GET_CLASS(ch)) -= 10;   
   return;
  }
 }
 else if (!(strcmp(arg, "senzu"))) {
  cost = GET_MAX_MANA(ch);
  int64_t cost2 = gear_pl(ch) - 1;
   
  if (senzu == FALSE) {
   send_to_char(ch, "What do you want to create?\r\n");
   return;
  }
  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to create %s, you need full ki.\r\n", arg);
   return;
  }
  else if (GET_HIT(ch) <= cost2) {
   send_to_char(ch, "You do not have enough powerlevel to create %s, you need to be at full.\r\n", arg);
   return;
  }
  else if (GET_MOVE(ch) < GET_MAX_MOVE(ch)) {
   send_to_char(ch, "You do not have enough stamina to create %s, you need to be at full.\r\n", arg);
   return;
  }
  else if (GET_PRACTICES(ch, GET_CLASS(ch)) < 50) {
   send_to_char(ch, "You do not have enough PS to create %s, you need at least 50.\r\n", arg);
   return;
  }
  else {
   obj = read_object(1, VIRTUAL);
    add_unique_id(obj);
   obj_to_char(obj, ch);
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
   act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   GET_HIT(ch) -= cost2;
   GET_MOVE(ch) = 1;
   GET_PRACTICES(ch, GET_CLASS(ch)) -= 50;
   return;
  }
 }
 else {
  send_to_char(ch, "Create what?\r\n");
  return;
 }

}

ACMD(do_recharge)
{

 if (IS_NPC(ch) || !IS_ANDROID(ch)) {
  send_to_char(ch, "Only androids can use recharge\r\n");
  return;
 }

  if (GET_COOLDOWN(ch) > 0) {
	send_to_char(ch, "You must wait a short period before your nanites can convert your ki.\r\n");
	return;
}
 
 if (!PLR_FLAGGED(ch, PLR_REPAIR)) {
  send_to_char(ch, "You are not a repair model android.\r\n");
  return;
 }
 else {
  int64_t cost = 0;

  cost = GET_MAX_MOVE(ch) / 20;

  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki to recharge your stamina.\r\n");
   return;
  }
  else if (GET_MOVE(ch) >= GET_MAX_MOVE(ch)) {
   send_to_char(ch, "Your energy reserves are already full.\r\n");
   return;
  }
  else {
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You focus your ki into your energy reserves, recharging them some.", TRUE, ch, 0, 0, TO_CHAR);
   act("$n stops and glows green briefly.", TRUE, ch, 0, 0, TO_ROOM);
   GET_MANA(ch) -= cost;
   if (GET_MOVE(ch) + (cost * 2) < GET_MAX_MOVE(ch)) {
    GET_MOVE(ch) += cost * 2;
   }
   else {
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    send_to_char(ch, "You are fully recharged now.\r\n");
   }
   WAIT_STATE(ch, PULSE_2SEC);
  }
 }
}

ACMD(do_srepair)
{
 int i;

 if (!IS_ANDROID(ch)) {
  send_to_char(ch, "Only androids can use repair, maybe you want 'fix' instead?\r\n");
  return;
 }

  if (GET_COOLDOWN(ch) > 0) {
	send_to_char(ch, "You must wait a short period before your nanites can repair you.\r\n");
	return;
}
 
 if (!IS_NPC(ch) && !PLR_FLAGGED(ch, PLR_REPAIR)) {
  send_to_char(ch, "You are not a repair model android.\r\n");
  return;
 }
 else {
  int64_t cost = 0, heal = 0;
  
  cost = GET_MAX_HIT(ch) / 40;

  if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You do not have enough stamina to repair yourself.\r\n");
   return;
  }
  else if (GET_HIT(ch) >= gear_pl(ch)) {
   send_to_char(ch, "You are already at full functionality and do not require repairs.\r\n");
   return;
  }
  else {
   reveal_hiding(ch, 0);  GET_COOLDOWN(ch) = 10;
   act("You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.", TRUE, ch, 0, 0, TO_CHAR);
   act("$n stops a moment as small glowing particles move across $s body.", TRUE, ch, 0, 0, TO_ROOM);

   int repaired = FALSE;
   if (!IS_NPC(ch)) {
    for (i = 0; i < NUM_WEARS; i++) {
     if (GET_EQ(ch, i)) {
      if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH) < 100) {
       GET_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH) += 20;
       if (GET_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH) > 100) {
        GET_OBJ_VAL(GET_EQ(ch, i), VAL_ALL_HEALTH) = 100;
       }
       if (OBJ_FLAGGED(GET_EQ(ch, i), ITEM_BROKEN)) {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(GET_EQ(ch, i)), ITEM_BROKEN);
       }
       repaired = TRUE;
      }
     }
    }
   }

   if (repaired == TRUE) {
    send_to_char(ch, "@GYour nano-robots also repair all of your equipment a little bit.@n\r\n");
   }

   GET_MOVE(ch) -= cost;
    heal = cost * 2;
   if (GET_BONUS(ch, BONUS_HEALER) > 0) {
    heal += heal * .25;
   }
   if (GET_HIT(ch) + heal < gear_pl(ch)) {
    GET_HIT(ch) += heal;
   } else {
    GET_HIT(ch) = gear_pl(ch);
    send_to_char(ch, "You are fully repaired now.\r\n");
   }
   if (!IS_NPC(ch) && rand_number(1, 3) == 2 && GET_MANA(ch) < GET_MAX_MANA(ch)) {
    send_to_char(ch, "@GThe repairs have managed to relink power reserves and boost your current energy level.@n\r\n");
    GET_MANA(ch) += cost;
    if (GET_MANA(ch) > GET_MAX_MANA(ch)) {
     GET_MANA(ch) = GET_MAX_MANA(ch);
    }
   }
   WAIT_STATE(ch, PULSE_2SEC);
  }
 }
}

ACMD(do_upgrade)
{
 int count = 0, bonus = 0, cost = 0;
 char arg[MAX_INPUT_LENGTH];
 char arg2[MAX_INPUT_LENGTH];
 
 two_arguments(argument, arg, arg2);

 if (IS_NPC(ch) || !IS_ANDROID(ch)) {
  send_to_char(ch, "You are not an android!\r\n");
  return;
 }

 if (!*arg) {
  if (!PLR_FLAGGED(ch, PLR_ABSORB)) {
  send_to_char(ch, "@c--------@D[@rUpgrade Menu@D]@c--------\r\n"
                   "@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n"
                   "@cUpgrade @CKi        @D: @Y40 @WPoints\r\n"
                   "@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n"
                   "@D            -----------\r\n");
  }
  send_to_char(ch,  "@cAugment @RPowerlevel\r\n"
                   "@cAugment @CKi\r\n"
                   "@cAugment @GStamina\r\n"
                   "@WCurrent Upgrade Points @D[@y%s@D]@n\r\n", add_commas(GET_UP(ch)));
  return;
 }

 if (!strcasecmp("augment", arg)) {
   struct obj_data *obj = NULL;
   int64_t gain = 0;
  if (GET_LEVEL(ch) < 80) {
   send_to_char(ch, "You need to be at least level 80 to use these kits.\r\n");
   return;
  }
  if (!(obj = get_obj_in_list_vis(ch, "Augmentation", NULL, ch->carrying))) {
   send_to_char(ch, "You don't have a Circuit Augmentation Kit.\r\n");
   return;
  } else {
   switch (GET_LEVEL(ch)) { /* R: Original was GET_LEVEL(ch) */
    case 80:
    gain = GET_MAX_HIT(ch) * 0.005;
    break;
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
     gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 2000; */
     break;
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
    case 98:
    case 99:
     gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 4000; */
     break;
    case 100:
     gain = GET_MAX_HIT(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 100000 */
     if (gain > 10000000) {
       gain = 10000000;
       }
     break;
   }
   if (!strcasecmp("powerlevel", arg2)) {
    obj_from_char(obj);
    extract_obj(obj);
    act("@WYou install the circuits and upgrade your maximum powerlevel.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
    GET_MAX_HIT(ch) += gain;
    GET_BASE_PL(ch) += gain;
    send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
    return;
   } else if (!strcasecmp("ki", arg2)) {
    obj_from_char(obj);
    extract_obj(obj);
    act("@WYou install the circuits and upgrade your maximum ki.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
    GET_MAX_MANA(ch) += gain;
    GET_BASE_KI(ch) += gain;
    send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
    return;
   } else if (!strcasecmp("stamina", arg2)) {
    obj_from_char(obj);
    extract_obj(obj);
    act("@WYou install the circuits and upgrade your maximum stamina.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
    GET_MAX_MOVE(ch) += gain;
    GET_BASE_ST(ch) += gain;
    send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
    return;
   } else {
     send_to_char(ch, "What do you want to augment? Powerlevel, ki, or stamina?\r\n");
    return;
   }
  }
 }

 if (PLR_FLAGGED(ch, PLR_ABSORB)) {
  send_to_char(ch, "You are an absorb model and can only upgrade with augmentation kits.\r\n");
  return;
 }

 if (!soft_cap(ch, 0)) {
  send_to_char(ch, "@mYou are unable to spend anymore UGP right now (Softcap)@n\r\n");
  return;
 }

 if (!*arg2 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
  send_to_char(ch, "How many times do you want to increase %s?", arg);
  return;
 }

 if (atoi(arg2) <= 0 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
  send_to_char(ch, "It needs to be between 1-1000\r\n");
  return;
 } 

 if (atoi(arg2) > 1000 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
  send_to_char(ch, "It needs to be between 1-1000\r\n");
  return;
 }

 if (!strcasecmp("powerlevel", arg)) {
  count = atoi(arg2);
  while (count > 0) {
   if (GET_LEVEL(ch) >= 90) {
    bonus += GET_LEVEL(ch) * 5000;
   }
   else if (GET_LEVEL(ch) >= 80) {
    bonus += GET_LEVEL(ch) * 2500;
   }
   else if (GET_LEVEL(ch) >= 70) {
    bonus += GET_LEVEL(ch) * 2000;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1300;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1200;
   }
   else if (GET_LEVEL(ch) >= 50) {
    bonus += GET_LEVEL(ch) * 500;
   }
   else if (GET_LEVEL(ch) >= 25) {
    bonus += GET_LEVEL(ch) * 250;
   }
   else {
    bonus += GET_LEVEL(ch) * 150;
   }
   cost += 75;
   count--;
  }
  if (cost > GET_UP(ch)) {
   send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(GET_UP(ch)));
   return;
  }
  else if (!soft_cap(ch, bonus)) {
   send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
   return;
  }
  else {
   GET_UP(ch) -= cost;
   send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
   GET_MAX_HIT(ch) += bonus;
   GET_BASE_PL(ch) += bonus;
  }
 }
 else if (!strcasecmp("ki", arg)) {
  count = atoi(arg2);
  while (count > 0) {
   if (GET_LEVEL(ch) >= 90) {
    bonus += GET_LEVEL(ch) * 3650;
   }
   else if (GET_LEVEL(ch) >= 80) {
    bonus += GET_LEVEL(ch) * 2450;
   }
   else if (GET_LEVEL(ch) >= 70) {
    bonus += GET_LEVEL(ch) * 1800;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1250;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1150;
   }
   else if (GET_LEVEL(ch) >= 50) {
    bonus += GET_LEVEL(ch) * 400;
   }
   else if (GET_LEVEL(ch) >= 25) {
    bonus += GET_LEVEL(ch) * 200;
   }
   else {
    bonus += GET_LEVEL(ch) * 120;
   }
   cost += 40;
   count--;
  }
  if (cost > GET_UP(ch)) {
   send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(GET_UP(ch)));
   return;
  }
  else if (!soft_cap(ch, bonus)) {
   send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
   return;
  }
  else {
   GET_UP(ch) -= cost;
   send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
   GET_MAX_MANA(ch) += bonus;
   GET_BASE_KI(ch) += bonus;
  }
 }
 else if (!strcasecmp("stamina", arg)) {
  count = atoi(arg2);
  while (count > 0) {
   if (GET_LEVEL(ch) >= 90) {
    bonus += GET_LEVEL(ch) * 3650;
   }
   else if (GET_LEVEL(ch) >= 80) {
    bonus += GET_LEVEL(ch) * 2450;
   }
   else if (GET_LEVEL(ch) >= 70) {
    bonus += GET_LEVEL(ch) * 1800;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1250;
   }
   else if (GET_LEVEL(ch) >= 60) {
    bonus += GET_LEVEL(ch) * 1150;
   }
   else if (GET_LEVEL(ch) >= 50) {
    bonus += GET_LEVEL(ch) * 500;
   }
   else if (GET_LEVEL(ch) >= 25) {
    bonus += GET_LEVEL(ch) * 200;
   }
   else {
    bonus += GET_LEVEL(ch) * 120;
   }
   cost += 50;
   count--;
  }
  if (cost > GET_UP(ch)) {
   send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(GET_UP(ch)));
   return;
  }
  else if (!soft_cap(ch, bonus)) {
   send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
   return;
  }
  else {
   GET_UP(ch) -= cost;
   send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
   GET_MAX_MOVE(ch) += bonus;
   GET_BASE_ST(ch) += bonus;
  }
 }
 else {
  send_to_char(ch, "That is not a valid upgrade option.\r\n");
  return;
 }

}

ACMD(do_ingest)
{
 
 if (IS_MAJIN(ch)) {
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];
  
  one_argument(argument, arg);

 if (!*arg) {
  send_to_char(ch, "Who do you want to ingest?\r\n");
  return;
 }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "Ingest who?\r\n");
   return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
   return;
  }
  if (ABSORBBY(vict)) {
    send_to_char(ch, "%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
    return;
  }
  if (GET_ABSORBS(ch) > 3) {
    send_to_char(ch, "You already have already ingested 4 people.\r\n");
    return;
  }
  if (GET_LEVEL(ch) < 25) {
   send_to_char(ch, "You can't ingest yet.\r\n");
   return;
  }
  if (GET_LEVEL(ch) < 100 && GET_LEVEL(ch) >= 75 && GET_ABSORBS(ch) == 3) {
   send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
   return;
  }
  if (GET_LEVEL(ch) < 75 && GET_LEVEL(ch) >= 50 && GET_ABSORBS(ch) == 2) {
   send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
   return;
  }
  if (GET_LEVEL(ch) < 50 && GET_LEVEL(ch) >= 25 && GET_ABSORBS(ch) == 1) {
   send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
   return;
  }
  
  if (GET_MAX_HIT(vict) >= GET_BASE_PL(ch) * 3) {
   send_to_char(ch, "You are too weak to ingest them into your body!\r\n");
   return;
  }
  if (AFF_FLAGGED(vict, AFF_SANCTUARY)) {
   send_to_char(ch, "You can't ingest them, they have a barrier!\r\n");
   return;
  }
   reveal_hiding(ch, 0);
  if (AFF_FLAGGED(vict, AFF_ZANZOKEN) && GET_MOVE(vict) >= 1 && GET_POS(vict) != POS_SLEEPING) {
     act("@C$N@c disappears, avoiding your attempted ingestion!@n", FALSE, ch, 0, vict, TO_CHAR);
     act("@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
     act("@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
     REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_ZANZOKEN);
     WAIT_STATE(ch, PULSE_3SEC);
     return;
  }
  if (GET_SPEEDI(ch) + rand_number(1, 5) < GET_SPEEDI(ch) + rand_number(1, 5)) {
   act("@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W flings a piece of goo at you, you manage to avoid it though!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   WAIT_STATE(ch, PULSE_3SEC);   
   return;
  }
  else {
    act("@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_ABSORBS(ch) += 1;
    int64_t pl = GET_BASE_PL(vict) / 6;
    int64_t stam = GET_BASE_ST(vict) / 6;
    int64_t ki = GET_BASE_KI(vict) / 6;
    GET_MAX_HIT(ch) += pl;
    GET_BASE_PL(ch) += pl;
    GET_MAX_MANA(ch) += ki;
    GET_BASE_KI(ch) += ki;
    GET_MAX_MOVE(ch) += stam;
    GET_BASE_ST(ch) += stam;
    if (!IS_NPC(vict) && !IS_NPC(ch)) {
     send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(ch), GET_NAME(vict), GET_ROOM_VNUM(IN_ROOM(vict)));
     SET_BIT_AR(PLR_FLAGS(vict), PLR_ABSORBED);
    }
    send_to_char(ch, "@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
    if (rand_number(1, 3) == 3) {
     send_to_char(ch, "You get %s's eye color.\r\n", GET_NAME(vict));
     GET_EYE(ch) = GET_EYE(vict);      
    }
    else if (rand_number(1, 3) == 3) {
     send_to_char(ch, "%s changes your height.\r\n", GET_NAME(vict));
     if (GET_PC_HEIGHT(ch) > GET_PC_HEIGHT(vict)) {
      GET_HEIGHT(ch) -= ((GET_PC_HEIGHT(ch) - GET_PC_HEIGHT(vict)) / 2);
     }
     else if (GET_PC_HEIGHT(ch) < GET_PC_HEIGHT(vict)) {
      GET_HEIGHT(ch) += ((GET_PC_HEIGHT(vict) - GET_PC_HEIGHT(ch)) / 2);
     }
     else {
      GET_HEIGHT(ch) = GET_PC_HEIGHT(vict);
     }
    }
    else if (rand_number(1, 3) == 3) {
     send_to_char(ch, "%s changes your weight.\r\n", GET_NAME(vict));
     if (GET_PC_WEIGHT(ch) > GET_PC_WEIGHT(vict)) {
      GET_WEIGHT(ch) -= ((GET_PC_WEIGHT(ch) - GET_PC_WEIGHT(vict)) / 2);
     }
     else if (GET_PC_WEIGHT(ch) < GET_PC_WEIGHT(vict)) {
      GET_WEIGHT(ch) += ((GET_PC_WEIGHT(vict) - GET_PC_WEIGHT(ch)) / 2);
     }
     else {
      GET_WEIGHT(ch) = GET_PC_WEIGHT(vict);
     }
    }
    else {
     send_to_char(ch, "Your forelock length changes because of %s.\r\n", GET_NAME(vict));
     GET_HAIRL(ch) = GET_HAIRL(vict);
    }
    handle_ingest_learn(ch, vict);
    die(vict, NULL);
    return;
  }
 } // End of ingest

 else {
  send_to_char(ch, "You are not a majin, you can not ingest.\r\n");
  return;
 } // Error
 
}

ACMD(do_absorb)
{
  struct char_data *vict = NULL;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

 two_arguments(argument, arg, arg2);

 if (!check_skill(ch, SKILL_ABSORB)) {
  return;
 }

 if (IS_ANDROID(ch)) {
  if (!limb_ok(ch, 0)) {
   return;
  }
 }
 if (!IS_NPC(ch)) {
  if (IS_BIO(ch) && !PLR_FLAGGED(ch, PLR_TAIL)) {
   send_to_char(ch, "You have no tail!\r\n");
   return;
  }
 }

 if (!IS_ANDROID(ch) && !IS_BIO(ch)) {
  send_to_char(ch, "You shouldn't have this skill, you are incapable of absorbing.\r\n");
  send_to_imm("ERROR: Absorb skill on %s when they are not a bio or android.", GET_NAME(ch));
  return;
 }

 if (FIGHTING(ch) && !IS_ANDROID(ch)) {
  send_to_char(ch, "You are too busy fighting!\r\n");
  return;
 }

 if (GRAPPLED(ch)) {
  send_to_char(ch, "You are currently being grappled with! Try 'escape'!\r\n");
  return;
 }
 if (GRAPPLING(ch)) {
  send_to_char(ch, "You are currently grappling with someone!\r\n");
  return;
 }

 if (ABSORBING(ch)) {
  act("@WYou stop absorbing from @c$N@W!@n", TRUE, ch, 0, ABSORBING(ch), TO_CHAR);
  act("$n stops absorbing from you!", TRUE, ch, 0, ABSORBING(ch), TO_VICT);
  act("$n stops absorbing from $N!", TRUE, ch, 0, ABSORBING(ch), TO_NOTVICT);
  if (IS_NPC(ABSORBING(ch)) && !FIGHTING(ABSORBING(ch))) {
  set_fighting(ABSORBING(ch), ch);
  }
  ABSORBBY(ABSORBING(ch)) = NULL;
  ABSORBING(ch) = NULL;
 }

 if (!*arg && IS_ANDROID(ch)) {
  send_to_char(ch, "Who do you want to absorb?\r\n");
  return;
 }

 if (IS_ANDROID(ch)) {
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "Absorb %s?\r\n", IS_ANDROID(ch) ? "from who" : "who");
   return;
  }
 }
 if (IS_BIO(ch)) {
  if (!*arg) {
   send_to_char(ch, "Syntax: absorb (swallow | extract) (target)\r\n");
   return;
  }
  else if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "Syntax: absorb (swallow | extract) (target)\r\n");
   return;
  }
 }
  if (AFF_FLAGGED(vict, AFF_SANCTUARY)) {
   send_to_char(ch, "You can't absorb them, they have a barrier!\r\n");
   return;
  }

 if (IS_ANDROID(ch)) {
   if (!IS_NPC(ch)) {
    if (!PLR_FLAGGED(ch, PLR_ABSORB)) {
     send_to_char(ch, "You are not an absorbtion model.\r\n");
     return;
    }
   }
   if (!can_kill(ch, vict, NULL, 0)) {
    return;
   }
   if (ABSORBBY(vict)) {
    send_to_char(ch, "%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
    return;
   }
   if (GET_MAX_HIT(vict) > GET_MAX_HIT(ch) * 2) {
    send_to_char(ch, "They are too strong for you to absorb from.\r\n");
    return;
   }
   if (GET_MAX_HIT(vict) * 20 < GET_MAX_HIT(ch)) {
    send_to_char(ch, "They are too weak for you to bother absorbing from.\r\n");
    return;
   }
   if (GET_MOVE(vict) < (GET_MAX_MOVE(vict) / 20) && GET_MANA(vict) < (GET_MAX_MANA(vict) / 20)) {
    send_to_char(ch, "They have nothing to absorb right now, they are drained...\r\n");
    return;
   }
   reveal_hiding(ch, 0);
   if (init_skill(ch, SKILL_ABSORB) < axion_dice(0)) {
    act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_ABSORB, 1);
    if (IS_NPC(vict) && IS_HUMANOID(vict) && rand_number(1, 3) == 3) {
      if (FIGHTING(ch) == NULL) {
       set_fighting(ch, vict);
      }
      if (FIGHTING(vict) == NULL) {
       set_fighting(vict, ch);
      }
    }
    WAIT_STATE(ch, PULSE_3SEC);
    return;
   }
   else {
    act("@WYou rush at @c$N@W and try to absorb from them, and manage to grab on!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W rushes at you and tries to grab you, and manages to grab on!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@w rushes at @c$N@W and tries to grab $M, and manages to grab on!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_ABSORB, 1);
    ABSORBING(ch) = vict;
    ABSORBBY(vict) = ch;
    WAIT_STATE(ch, PULSE_3SEC);
    return;
   }
 } // End of android absorb

 else if (IS_BIO(ch) && !(strcmp(arg, "swallow"))) {
  if (ABSORBBY(vict)) {
    send_to_char(ch, "%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
    return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
   return;
  }
  if (GET_ABSORBS(ch) < 1) {
    send_to_char(ch, "You already have already absorbed 3 people.\r\n");
    return;
  }
  if (GET_MAX_HIT(vict) >= GET_BASE_PL(ch) * 3) {
   send_to_char(ch, "You are too weak to absorb them into your cellular structure!\r\n");
   return;
  }
   reveal_hiding(ch, 0);
  if (GET_SKILL(ch, SKILL_ABSORB) < axion_dice(0)) {
   act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_ABSORB, 1);
   if (!FIGHTING(ch)) {
    set_fighting(ch, vict);
   } 
   if (!FIGHTING(vict)) {
    set_fighting(vict, ch);
   }
   WAIT_STATE(ch, PULSE_3SEC);   
  }
  else {
    act("@WYou rush at @c$N@W and your tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing $m!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W rushes at you and $s tail engulfs you! $e quickly sucks your squirming body into $s tail, absorbing you!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@w rushes at @c$N@W and $s tail engulfs $M! You quickly suck $S squirming body into your tail, absorbing @c$N@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_ABSORBS(ch) -= 1;
    GET_MAX_HIT(ch) += GET_BASE_PL(vict) / 5;
    GET_BASE_PL(ch) += GET_BASE_PL(vict) / 5;
    GET_MAX_MANA(ch) += GET_BASE_KI(vict) / 5;
    GET_BASE_KI(ch) += GET_BASE_KI(vict) / 5;
    GET_MAX_MOVE(ch) += GET_BASE_ST(vict) / 5;
    GET_BASE_ST(ch) += GET_BASE_ST(vict) / 5;
    if (!IS_NPC(vict) && !IS_NPC(ch)) {
     send_to_imm("[PK] %s killed %s at room [%d]\r\n", GET_NAME(ch), GET_NAME(vict), GET_ROOM_VNUM(IN_ROOM(vict)));
     SET_BIT_AR(PLR_FLAGS(vict), PLR_ABSORBED);
    }
    int64_t stam = GET_BASE_ST(vict) / 5;
    int64_t ki = GET_BASE_KI(vict) / 5;
    int64_t pl = GET_BASE_PL(vict) / 5;
    send_to_char(ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
    improve_skill(ch, SKILL_ABSORB, 1);
    die(vict, NULL);
  }
 } // End of bio absorb
 else if (IS_BIO(ch) && !(strcmp(arg, "extract"))) {
  int failthresh = rand_number(1, 125);
  if (GET_LEVEL(vict) > 99) {
   failthresh += (GET_LEVEL(vict) - 95) * 2;
  }
  if (ABSORBBY(vict)) {
    send_to_char(ch, "%s is already absorbing from them!", GET_NAME(ABSORBBY(vict)));
    return;
  }
  if (!can_kill(ch, vict, NULL, 0)) {
   return;
  }
  if (GET_MAX_HIT(vict) >= GET_MAX_HIT(ch)) {
   send_to_char(ch, "You are too weak to absorb them into your cellular structure!\r\n");
   return;
  }
  if (GET_MAX_HIT(vict) < GET_MAX_HIT(ch) / 5) {
   send_to_char(ch, "They would be worthless to you at your strength!\r\n");
   return;
  }
  if (!IS_NPC(vict)) {
   send_to_char(ch, "You can't absorb their bio extract, you need to swallow them with your tail!\r\n");
   return;
  }
  if (!soft_cap(ch, 0)) {
    send_to_char(ch, "You can not handle any more bio extract at your current level.\r\n");
    return;
  }
  if (GET_SKILL(ch, SKILL_ABSORB) < failthresh) {
   act("@WYou rush at @c$N@W and try to absorb from them, but $E manages to avoid you!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W rushes at you and tries to grab you, but you manage to avoid $m!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@C$n@w rushes at @c$N@W and tries to grab $M, but @c$N@W manages to avoid @c$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   improve_skill(ch, SKILL_ABSORB, 0);
   if (!FIGHTING(ch)) {
    set_fighting(ch, vict);
   }
   if (!FIGHTING(vict)) {
    set_fighting(vict, ch);
   }
   WAIT_STATE(ch, PULSE_4SEC);
  }
  /* Rillao: transloc, add new transes here */
  else {
   act("@WYou rush at @c$N@W and stab them with your tail! You quickly suck out all the bio extract you need and leave the empty husk behind!", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@w rushes at @c$N@W and stabs $M with $s tail! $e quickly sucks out all the bio extract and leaves the empty husk of @c$N@W behind!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   int64_t stam = GET_BASE_ST(vict) / 5000;
   int64_t ki = GET_BASE_KI(vict) / 5000;
   int64_t pl = GET_BASE_PL(vict) / 5000;
   stam += rand_number(GET_LEVEL(ch), GET_LEVEL(ch) * 2);
   pl += rand_number(GET_LEVEL(ch), GET_LEVEL(ch) * 2);
   ki += rand_number(GET_LEVEL(ch), GET_LEVEL(ch) * 2);
   if (stam > 1500000) {
     stam = 1500000;
   }
   if (pl > 1500000) {
    pl = 1500000;
   }
   if (ki > 1500000) {
    ki = 1500000;
   }
   if (PLR_FLAGGED(ch, PLR_TRANS1)) {
    GET_MAX_HIT(ch) += pl * 2;
    GET_MAX_MOVE(ch) += stam * 2;
    GET_MAX_MANA(ch) += ki * 2;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS2)) {
    GET_MAX_HIT(ch) += pl * 3;
    GET_MAX_MOVE(ch) += stam * 3;
    GET_MAX_MANA(ch) += ki * 3;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS3)) {
    GET_MAX_HIT(ch) += pl * 3.5;
    GET_MAX_MOVE(ch) += stam * 3.5;
    GET_MAX_MANA(ch) += ki * 3.5;
   }
   else if (PLR_FLAGGED(ch, PLR_TRANS4)) {
    GET_MAX_HIT(ch) += pl * 4;
    GET_MAX_MOVE(ch) += stam * 4;
    GET_MAX_MANA(ch) += ki * 4;
   }
   else {
    GET_MAX_HIT(ch) += pl;
    GET_MAX_MOVE(ch) += stam;
    GET_MAX_MANA(ch) += ki;
   }
   GET_BASE_PL(ch) += pl;
   GET_BASE_ST(ch) += stam;
   GET_BASE_KI(ch) += ki;
   GET_LIFEFORCE(ch) += GET_LIFEMAX(ch) * 0.05;
    if (GET_LIFEFORCE(ch) > GET_LIFEMAX(ch)) {
     GET_LIFEFORCE(ch) = GET_LIFEMAX(ch);
    }
   send_to_char(ch, "@D[@gABSORB@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
   improve_skill(ch, SKILL_ABSORB, 0);
   WAIT_STATE(ch, PULSE_4SEC);
   SET_BIT_AR(MOB_FLAGS(vict), MOB_HUSK);
   die(vict, ch);
  }
 }

 else {
  if (!IS_BIO(ch) && !IS_ANDROID(ch)) {
  send_to_char(ch, "You have the absorb skill but are incapable of absorbing. This error has been reported.\r\n");
  send_to_imm("ERROR: Absorb attempted by %s even though they are not bio or android.", GET_NAME(ch));
  }
  else {
  send_to_char(ch, "Syntax: absorb (extract | swallow) (target)\r\n");
  }
  return;
 } // Error
}

ACMD(do_escape)
{

 if (!ABSORBBY(ch) && !GRAPPLED(ch)) {
  send_to_char(ch, "You are not in anyone's grasp!\r\n");
  return;
 }
 int num = GET_STR(ch);

 if (ABSORBBY(ch)) {
  int skill = GET_SKILL(ABSORBBY(ch), SKILL_ABSORB);
  if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 10) {
   num += rand_number(10, 15);
  } else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 5) {
   num += rand_number(6, 10);
  } else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch)) * 2) {
   num += rand_number(4, 8);
  } else if (GET_HIT(ch) > GET_HIT(ABSORBBY(ch))) {
   num += rand_number(2, 5);
  } else if (GET_HIT(ch) * 10 <= GET_HIT(ABSORBBY(ch))) {
   skill -= rand_number(10, 15);
  } else if (GET_HIT(ch) * 5 <= GET_HIT(ABSORBBY(ch))) {
   skill -= rand_number(6, 10);
  } else if (GET_HIT(ch) * 2 <= GET_HIT(ABSORBBY(ch))) {
   skill -= rand_number(4, 8);
  } else if (GET_HIT(ch) < GET_HIT(ABSORBBY(ch))) {
   skill -= rand_number(2, 5);
  }
 if (num > skill) {
  act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_NOTVICT);
  act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_VICT);
  act("@c$N@W manages to break loose of your hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_CHAR);
  if (FIGHTING(ch) == NULL) {
   set_fighting(ch, ABSORBBY(ch));
  }
  if (FIGHTING(ABSORBBY(ch)) == NULL) {
    set_fighting(ABSORBBY(ch), ch);
  }
  ABSORBING(ABSORBBY(ch)) = NULL;
  ABSORBBY(ch) = NULL;
 }
 else {
  act("@c$N@W struggles to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_NOTVICT);
  act("@WYou struggle to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_VICT);
  act("@c$N@W struggles to break loose of your hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_CHAR);
  if (rand_number(1, 3) == 3) {
   int64_t dmg = GET_MAX_HIT(ch) * 0.025;
   hurt(0, 0, ch, ABSORBBY(ch), NULL, dmg, 0);
   if (GET_POS(ABSORBBY(ch)) == POS_SLEEPING) {
    act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_NOTVICT);
    act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_VICT);
    act("@c$N@W manages to break loose of your hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_CHAR);
    ABSORBING(ABSORBBY(ch)) = NULL;
    ABSORBBY(ch) = NULL;
   }
  }
  WAIT_STATE(ch, PULSE_2SEC);
 }
 }
 if (GRAPPLED(ch)) {
  int skill = GET_SKILL(GRAPPLED(ch), SKILL_GRAPPLE);
  if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 10) {
   num += rand_number(10, 15);
  } else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 5) {
   num += rand_number(6, 10);
  } else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch)) * 2) {
   num += rand_number(4, 8);
  } else if (GET_HIT(ch) > GET_HIT(GRAPPLED(ch))) {
   num += rand_number(2, 5);
  } else if (GET_HIT(ch) * 10 <= GET_HIT(GRAPPLED(ch))) {
   skill -= rand_number(10, 15);
  } else if (GET_HIT(ch) * 5 <= GET_HIT(GRAPPLED(ch))) {
   skill -= rand_number(6, 10);
  } else if (GET_HIT(ch) * 2 <= GET_HIT(GRAPPLED(ch))) {
   skill -= rand_number(4, 8);
  } else if (GET_HIT(ch) < GET_HIT(GRAPPLED(ch))) {
   skill -= rand_number(2, 5);
  }

 if (num > skill) {
  if (GRAPTYPE(GRAPPLED(ch)) == 4) {
   act("@c$N@M flexes with all $S might and causes your body to explode outward into gooey chunks!@n", TRUE, GRAPPLED(ch), 0, ch, TO_CHAR);
   act("@MYou flex with all your might and cause @C$n's@M body to explode outward into gooey chunks!@n", TRUE, GRAPPLED(ch), 0, ch, TO_VICT);
   act("@c$N@M flexes with all $S might and causes @C$n's@M body to explode outward into gooey chunks!@n", TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);

   act("@MYou reform your body mere moments later.@n", TRUE, GRAPPLED(ch), 0, 0, TO_CHAR);
   act("@C$n@M reforms $s body mere moments later.", TRUE, GRAPPLED(ch), 0, 0, TO_ROOM);
  } else {
   act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);
   act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_VICT);
   act("@c$N@W manages to break loose of your hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_CHAR);
  }
  if (FIGHTING(ch) == NULL) {
   set_fighting(ch, GRAPPLED(ch));
  }
  if (FIGHTING(GRAPPLED(ch)) == NULL) {
    set_fighting(GRAPPLED(ch), ch);
  }
  GRAPTYPE(GRAPPLED(ch)) = -1;
  GRAPPLING(GRAPPLED(ch)) = NULL;
  GRAPPLED(ch) = NULL;
  GRAPTYPE(ch) = -1;
 } else {
  act("@c$N@W struggles to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);
  act("@WYou struggle to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_VICT);
  act("@c$N@W struggles to break loose of your hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_CHAR);
  if (rand_number(1, 3) == 3) {
   int64_t dmg = GET_MAX_HIT(ch) * 0.025;
   hurt(0, 0, ch, GRAPPLED(ch), NULL, dmg, 0);
   if (GET_POS(GRAPPLED(ch)) == POS_SLEEPING) {
    act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);
    act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_VICT);
    act("@c$N@W manages to break loose of your hold!@n", TRUE, GRAPPLED(ch), 0, ch, TO_CHAR);
    GRAPTYPE(GRAPPLED(ch)) = -1;
    GRAPPLING(GRAPPLED(ch)) = NULL;
    GRAPPLED(ch) = NULL;
    GRAPTYPE(ch) = -1;
   }
  }
  WAIT_STATE(ch, PULSE_2SEC);
 }  
 }
}

ACMD(do_regenerate) {

 int64_t amt = 0;
 int skill = 0;
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 skill = init_skill(ch, SKILL_REGENERATE);

 if (skill < 1) {
  send_to_char(ch, "You are incapable of regenerating.\r\n");
  return;
 }
 if (GET_SUPPRESS(ch) > 0) {
  skill = GET_SUPPRESS(ch);
 }

 if (!*arg) {
  send_to_char(ch, "Regenerate how much PL?\r\nMax percent you can regen: %d\r\nSyntax: regenerate (1 - 100)\r\n", skill);
  return;
 }

 if (GET_HIT(ch) >= gear_pl(ch)) {
  send_to_char(ch, "You do not need to regenerate, you are at full health.\r\n");
  return;
 }

 if (GET_SUPPRESS(ch) > 0 && GET_HIT(ch) >= ((gear_pl(ch) / 100) * GET_SUPPRESS(ch))) {
  send_to_char(ch, "You do not need to regenerate, you are at full health.\r\n");
  return;
 }

 int num = atoi(arg);

 if (num <= 0) {
  send_to_char(ch, "What is the point of that?\r\nSyntax: regenerate (1 - 100)\r\n");
  return;
 }

 if (num > 100) {
  send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
  return;
 }

 if (num > skill) {
  send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
  return;
 }

 if (GET_SUPPRESS(ch) > 0 && num > GET_SUPPRESS(ch)) {
  send_to_char(ch, "You can't regenerate that much!\r\nMax you can regen: %d\r\n", skill);
  return;
 }

 amt = (gear_pl(ch) * 0.01) * num;
 if (amt > 1)
  amt /= 2;

 if (IS_BIO(ch)) {
  amt = amt * 0.9;
 }

 int64_t life = (GET_LIFEFORCE(ch) - amt * 0.8), energy = (GET_MANA(ch) - amt * 0.2);

 if ((life <= 0 || energy <= 0) && !IS_NPC(ch)) {
  send_to_char(ch, "Your life force or ki are too low to regenerate that much.\r\n");
  send_to_char(ch, "@YLF Needed@D: @C%s@w, @YKi Needed@D: @C%s@w.@n\r\n", add_commas(amt * 0.8), add_commas(amt * 0.2));
  return;
 } else if (IS_NPC(ch) && energy <= 0) {
  return;
 }

 GET_HIT(ch) += amt * 2;
 if (!IS_NPC(ch))
  GET_LIFEFORCE(ch) -= amt * 0.8;
 GET_MANA(ch) -= amt * 0.2;
 if (GET_HIT(ch) > gear_pl(ch)) {
  GET_HIT(ch) = gear_pl(ch);
 }
   reveal_hiding(ch, 0);
 if (GET_SUPPRESS(ch) > 0 && GET_HIT(ch) > ((GET_MAX_HIT(ch) / 100) * GET_SUPPRESS(ch))) {
  GET_HIT(ch) = ((GET_MAX_HIT(ch) / 100) * GET_SUPPRESS(ch));
  send_to_char(ch, "@mYou regenerate to your suppression limit.@n\r\n");
 }
 if (GET_HIT(ch) >= gear_pl(ch)) {
  act("You concentrate your ki and regenerate your body completely.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n concentrates and regenerates $s body completely.", TRUE, ch, 0, 0, TO_ROOM);
 }
 else if (amt < GET_MAX_HIT(ch) / 10) {
 act("You concentrate your ki and regenerate your body a little.", TRUE, ch, 0, 0, TO_CHAR);
 act("$n concentrates and regenerates $s body a little.", TRUE, ch, 0, 0, TO_ROOM);
 }
 else if (amt < GET_MAX_HIT(ch) / 5) {
 act("You concentrate your ki and regenerate your body some.", TRUE, ch, 0, 0, TO_CHAR);
 act("$n concentrates and regenerates $s body some.", TRUE, ch, 0, 0, TO_ROOM);
 }
 else if (amt < GET_MAX_HIT(ch) / 2) {
 act("You concentrate your ki and regenerate your body a great deal.", TRUE, ch, 0, 0, TO_CHAR);
 act("$n concentrates and regenerates $s body a great deal.", TRUE, ch, 0, 0, TO_ROOM);
 }
 else if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
 act("You concentrate your ki and regenerate you nearly completely.", TRUE, ch, 0, 0, TO_CHAR);
 act("$n concentrates and regenerates $s body nearly completely.", TRUE, ch, 0, 0, TO_ROOM);
 }
 improve_skill(ch, SKILL_REGENERATE, 0);
 if (AFF_FLAGGED(ch, AFF_BURNED)) {
  send_to_char(ch, "Your burns are healed now.\r\n");
  act("$n@w's burns are now healed.@n", TRUE, ch, 0, 0, TO_ROOM);
  null_affect(ch, AFF_BURNED);
 }

 if (!IS_NPC(ch)) {
  if (GET_LIMBCOND(ch, 1) <= 0) {
   act("You regrow your right arm!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regrows $s right arm!", TRUE, ch, 0, 0, TO_ROOM);
   GET_LIMBCOND(ch, 1) = 100;
  } else if (GET_LIMBCOND(ch, 1) >= 0 && GET_LIMBCOND(ch, 1) < 50) {
   act("Your broken right arm mends itself!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regenerates $s broken right arm!", TRUE, ch, 0, 0, TO_ROOM);
   GET_LIMBCOND(ch, 1) = 100;
  }
  if (GET_LIMBCOND(ch, 2) <= 0) {
   GET_LIMBCOND(ch, 2) = 100;
   act("You regrow your left arm!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regrows $s left arm!", TRUE, ch, 0, 0, TO_ROOM);
  } else if (GET_LIMBCOND(ch, 2) > 0 && GET_LIMBCOND(ch, 2) < 50) {
   act("Your broken left arm mends itself!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regenerates $s broken left arm!", TRUE, ch, 0, 0, TO_ROOM);
   GET_LIMBCOND(ch, 2) = 100;
  }
  if (GET_LIMBCOND(ch, 4) <= 0) {
   GET_LIMBCOND(ch, 4) = 100;
   act("You regrow your left leg!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regrows $s left leg!", TRUE, ch, 0, 0, TO_ROOM);
  } else if (GET_LIMBCOND(ch, 4) > 0 && GET_LIMBCOND(ch, 4) < 50) {
   act("Your broken left leg mends itself!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regenerates $s broken left leg!", TRUE, ch, 0, 0, TO_ROOM);
   GET_LIMBCOND(ch, 4) = 100;
  }
  if (GET_LIMBCOND(ch, 3) <= 0) {
   GET_LIMBCOND(ch, 3) = 100;
   act("You regrow your right leg!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regrows $s right leg!", TRUE, ch, 0, 0, TO_ROOM);
  }  else if (GET_LIMBCOND(ch, 3) > 0 && GET_LIMBCOND(ch, 3) < 50) {
   GET_LIMBCOND(ch, 3) = 100;
   act("Your broken right leg mends itself!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regenerates $s broken right leg!", TRUE, ch, 0, 0, TO_ROOM);
  }
  if (!PLR_FLAGGED(ch, PLR_TAIL) && IS_BIO(ch)) {
   SET_BIT_AR(PLR_FLAGS(ch), PLR_TAIL);
   act("You regrow your tail!", TRUE, ch, 0, 0, TO_CHAR);
   act("$n regrows $s tail!", TRUE, ch, 0, 0, TO_ROOM);
  }
  improve_skill(ch, SKILL_REGENERATE, 0);
 }
}

ACMD(do_focus) 
{
 struct char_data *vict = NULL;
 char arg[MAX_INPUT_LENGTH];
 char name[MAX_INPUT_LENGTH];

 *name = '\0';
 *arg = '\0';

 two_arguments(argument, arg, name);

 if (!*arg) {
  send_to_char(ch, "Yes but what do you want to focus?\r\n");
  return;
 }
 if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
 }
 if (AFF_FLAGGED(ch, AFF_CURSE)) {
  send_to_char(ch, "You are cursed and can't focus!\r\n");
  return;
 }
 else if (!(strcmp(arg, "tough"))) { 
  if (!know_skill(ch, SKILL_TSKIN)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_STONESKIN)) {
    send_to_char(ch, "You already have tough skin!\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to infuse into your skin.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_TSKIN) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your skin, but fail in making it tough!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s skin, but fails in making it tough!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    int duration = GET_INT(ch) / 20;
    assign_affect(ch, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your skin, making it tough!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s skin, making it tough!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
  } // End of no vict tough skin

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Focus your ki into who's skin?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_STONESKIN)) {
     send_to_char(ch, "They already have tough skin!\r\n");
     return;
    }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to infuse into their skin.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_TSKIN) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's skin, but fail in making it tough!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your skin, but fails in making it tough!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's skin, but fails in making it tough!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     int duration = roll_aff_duration(GET_INT(ch), 2);
     assign_affect(vict, AFF_STONESKIN, SKILL_TSKIN, duration, 0, 0, 0, 0, 0, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's skin, making it tough!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your skin, making it tough!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's skin, making it tough!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
   }
   
  } // End of victim of tough skin
 } // End of tough skin

 else if (!(strcmp(arg, "might"))) {
  if (!know_skill(ch, SKILL_MIGHT)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_MIGHT)) {
    send_to_char(ch, "You already have mighty muscles!\r\n");
    return;
   } else if (GET_BONUS(ch, BONUS_WIMP) > 0 && GET_STR(ch) + 10 > 25) {
    send_to_char(ch, "Your body is not able to withstand increasing its strength beyond 25.\r\n");
    return;
   } else if (GET_BONUS(ch, BONUS_FRAIL) > 0 && GET_STR(ch) + 2 > 25) {
    send_to_char(ch, "Your body is not able to withstand increasing its strength beyond 25.\r\n");
    return;
   } else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to infuse into your muscles.\r\n");
    return;
   } else if (GET_SKILL(ch, SKILL_MIGHT) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your muscles, but fail in making them mighty!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s muscles, but fails in making them mighty!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   } else {
    SET_BIT_AR(AFF_FLAGS(ch), AFF_MIGHT);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    int duration = roll_aff_duration(GET_INT(ch), 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(ch, AFF_MIGHT, SKILL_MIGHT, duration, 10, 2, 0, 0, 0, 0);
    reveal_hiding(ch, 0);
    act("You focus ki into your muscles, making them mighty!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s muscles, making them mighty!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
  } // End of no vict might

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Focus your ki into who's muscles?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_MIGHT)) {
     send_to_char(ch, "They already have mighty muscles!\r\n");
     return;
    } else if (GET_BONUS(vict, BONUS_WIMP) > 0 && GET_STR(vict) + 10 > 25) {
     send_to_char(ch, "Their body is not able to withstand increasing its strength beyond 25.\r\n");
     return;
    } else if (GET_BONUS(vict, BONUS_FRAIL) > 0 && GET_CON(vict) + 2 > 25) {
     send_to_char(ch, "Their body is not able to withstand increasing its constitution beyond 25.\r\n");
     return;
    } else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    } else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to infuse into their muscles.\r\n");
     return;
    } else if (GET_SKILL(ch, SKILL_MIGHT) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's muscles, but fail in making them mighty!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your muscles, but fails in making them mighty!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's muscles, but fails in making them mighty!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    } else {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     int duration = roll_aff_duration(GET_INT(ch), 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_MIGHT, SKILL_MIGHT, duration, 10, 2, 0, 0, 0, 0);
     reveal_hiding(ch, 0);
     act("You focus ki into $N's muscles, making them mighty!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your muscles, making them mighty!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's muscles, making them mighty!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
   }

  } // End of victim of might
 } // End of might

 else if (!(strcmp(arg, "wither"))) {
  if (!know_skill(ch, SKILL_WITHER)) {
   return;
  }
    if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
     send_to_char(ch, "Focus your ki into who's muscles?\r\n");
     return;
    }
    if (ch == vict) {
     send_to_char(ch, "You don't want to wither your own body!\r\n");
     return;
    }
    if (!can_kill(ch, vict, NULL, 2)) {
     return;
    }
    if (AFF_FLAGGED(vict, AFF_WITHER)) {
     send_to_char(ch, "They already have been withered!\r\n");
     return;
    } else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to wither them.\r\n");
     return;
    } else if (GET_SKILL(ch, SKILL_WITHER) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's body, but fail in withering it!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your body, but fails in withering it!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's body, but fails in withering it!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    } else {
     SET_BIT_AR(AFF_FLAGS(vict), AFF_WITHER);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     vict->real_abils.str -= 3;
     vict->real_abils.cha -= 3;
     save_char(vict);
     reveal_hiding(ch, 0);
     act("You focus ki into $N's body, and succeed in withering it!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your body, and succeeds in withering it!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's body, and succeeds in withering it!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
 } // End of wither

 else if (!(strcmp(arg, "enlighten"))) {
  if (!know_skill(ch, SKILL_ENLIGHTEN)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_ENLIGHTEN)) {
    send_to_char(ch, "You already have superior wisdom!\r\n");
    return;
   }
   else if (GET_BONUS(ch, BONUS_FOOLISH) > 0 && GET_WIS(ch) + 10 > 25) {
    send_to_char(ch, "You're not able to withstand increasing your wisdom beyond 25.\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to use this skill.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_ENLIGHTEN) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your mind, but fail in awakening it to cosmic wisdom!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s mind, but fails in awakening it to cosmic wisdom!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    int duration = roll_aff_duration(GET_INT(ch), 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(ch, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, 10, 0);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your mind, awakening it to cosmic wisdom!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s mind, awakening it to cosmic wisdom!", TRUE, ch, 0, 0, TO_ROOM);
    if (IS_JINTO(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 15 && rand_number(1, 4) >= 3) {
     int64_t gain = 0;
     GET_PRACTICES(ch, GET_CLASS(ch)) -= 15;
     if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 100) {
      gain = level_exp(ch, GET_LEVEL(ch) + 1) * 0.15;
      GET_EXP(ch) += gain;
      send_to_char(ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
     } else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 60) {
      gain = level_exp(ch, GET_LEVEL(ch) + 1) * 0.10;
      GET_EXP(ch) += gain;
      send_to_char(ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
     } else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 40) {
      gain = level_exp(ch, GET_LEVEL(ch) + 1) * 0.05;
      GET_EXP(ch) += gain;
      send_to_char(ch, "@GYou gain @g%s@G experience due to your excellence with this skill.@n\r\n", add_commas(gain));
     }
    }
    return;
   }
  } // End of no vict enlighten
  
    else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Focus your ki into who's mind?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_ENLIGHTEN)) {
     send_to_char(ch, "They already have superior wisdom!\r\n");
     return;
    }
    else if (GET_BONUS(vict, BONUS_FOOLISH) > 0 && GET_WIS(vict) + 10 > 25) {
     send_to_char(ch, "They're not able to withstand increasing their wisdom beyond 25.\r\n");
     return;
    }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to use this skill.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_ENLIGHTEN) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's mind, but fail in awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your mind, but fails in awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's mind, but fails in awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     int duration = roll_aff_duration(GET_INT(ch), 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_ENLIGHTEN, SKILL_ENLIGHTEN, duration, 0, 0, 0, 0, 10, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's mind, awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your mind, awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's mind, awakening it to cosmic wisdom!", TRUE, ch, 0, vict, TO_NOTVICT);
    if (IS_JINTO(ch) && level_exp(vict, GET_LEVEL(vict) + 1) - GET_EXP(vict) > 0 && GET_PRACTICES(ch, GET_CLASS(ch)) >= 15 && rand_number(1, 4) >= 3) {
     int64_t gain = 0;
     GET_PRACTICES(ch, GET_CLASS(ch)) -= 15;
     if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 100) {
      gain = level_exp(vict, GET_LEVEL(vict) + 1) * 0.15;
      GET_EXP(vict) += gain;
      send_to_char(vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
     } else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 60) {
      gain = level_exp(vict, GET_LEVEL(vict) + 1) * 0.10;
      GET_EXP(vict) += gain;
      send_to_char(vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
     } else if (GET_SKILL(ch, SKILL_ENLIGHTEN) >= 40) {
      gain = level_exp(vict, GET_LEVEL(vict) + 1) * 0.05;
      GET_EXP(vict) += gain;
      send_to_char(vict, "@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n\r\n", add_commas(gain));
     }
    }
     return;
    }
   }

  } // End of victim of enlighten
 } // End of enlighten

 else if (!(strcmp(arg, "genius"))) {
  if (!know_skill(ch, SKILL_GENIUS)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_GENIUS)) {
    send_to_char(ch, "You already have superior intelligence!\r\n");
    return;
   }
   else if (GET_BONUS(ch, BONUS_DULL) > 0 && GET_INT(ch) + 10 > 25) {
    send_to_char(ch, "You're not able to withstand increasing your intelligence beyond 25.\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to infuse into your mind.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_GENIUS) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your mind, but fail in making it work faster!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s muscles, but fails in making it work faster!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    int duration = roll_aff_duration(GET_INT(ch), 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(ch, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, 10, 0, 0, 0);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your mind, making it work faster!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s mind, making it work faster!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
  } // End of no vict genius

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Focus your ki into who's mind?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_GENIUS)) {
     send_to_char(ch, "They already have superior intelligence!\r\n");
     return;
    }
    else if (GET_BONUS(vict, BONUS_DULL) > 0 && GET_INT(vict) + 10 > 25) {
     send_to_char(ch, "They're not able to withstand increasing their intelligence beyond 25.\r\n");
     return;
    }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to infuse into their mind.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_GENIUS) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's mind, but fail in making it work faster!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your mind, but fails in making it work faster!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's mind, but fails in making it work faster!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     int duration = roll_aff_duration(GET_INT(ch), 2);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_GENIUS, SKILL_GENIUS, duration, 0, 0, 10, 0, 0, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's mind, making it work faster!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your mind, making it work faster!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's mind, making it work faster!", TRUE, ch, 0, vict, TO_NOTVICT);
     if ((vict->master == ch || ch->master == vict || ch->master == vict->master) && AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP)) {
      if (IS_KAI(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 && rand_number(1, 3) == 3) {
       GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.05;
      }
     }
     return;
    }
   }

  } // End of victim of genius
 } // End of genius

 else if (!(strcmp(arg, "flex"))) {
  if (!know_skill(ch, SKILL_FLEX)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_FLEX)) {
    send_to_char(ch, "You already have superior agility!\r\n");
    return;
   }
   else if (GET_BONUS(ch, BONUS_CLUMSY) > 0 && GET_DEX(ch) + 10 > 25) {
    send_to_char(ch, "You're not able to withstand increasing your agility beyond 25.\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to infuse into your limbs.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_FLEX) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki into your limbs, but fail in making them more flexible!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s muscles, but fails in making them more flexible!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    int duration = roll_aff_duration(GET_INT(ch), 2);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(ch, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, 10, 0, 0);
    reveal_hiding(ch, 0);
    act("You focus ki into your limbs, making them more flexible!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki into $s limbs, making them more flexible!", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
  } // End of no vict FLEX

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Focus your ki into who's limbs?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_FLEX)) {
     send_to_char(ch, "They already have superior agility!\r\n");
     return;
    }
   else if (GET_BONUS(vict, BONUS_CLUMSY) > 0 && GET_DEX(vict) + 3 > 25) {
    send_to_char(ch, "They're not able to withstand increasing their agility beyond 25.\r\n");
    return;
   }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to infuse into their limbs.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_FLEX) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's limbs, but fail in making them more flexible!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your limbs, but fails in making them more flexible!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's limbs, but fails in making them more flexible!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     int duration = roll_aff_duration(GET_INT(ch), 2);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_FLEX, SKILL_FLEX, duration, 0, 0, 0, 10, 0, 0);
     reveal_hiding(ch, 0);
     act("You focus ki into $N's limbs, making them more flexible!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your limbs, making them more flexible!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki into $N's limbs, making them more flexible!", TRUE, ch, 0, vict, TO_NOTVICT);
     if ((vict->master == ch || ch->master == vict || ch->master == vict->master) && AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP)) {
      if (IS_KAI(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 && rand_number(1, 3) == 3) {
       GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.05;
      }
     }
     return;
    }
   }

  } // End of victim of FLEX
 } // End of FLEX

 else if (!(strcmp(arg, "bless"))) {
  if (!know_skill(ch, SKILL_BLESS)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_BLESS)) {
    send_to_char(ch, "You already are blessed!\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to bless.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_BLESS) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki while chanting spiritual words. Your blessing does nothing though, you must have messed up!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki while chanting spiritual words. $n seems disappointed.", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    int duration = roll_aff_duration(GET_INT(ch), 3);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(ch, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    if (IS_KABITO(ch)) {
     GET_BLESSLVL(ch) = GET_SKILL(ch, SKILL_BLESS);
    } else {
     GET_BLESSLVL(ch) = 0;
    }
    act("You focus ki while chanting spiritual words. You feel your body recovering at above normal speed!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki while chanting spiritual words. $n smiles after finishing $s chant.", TRUE, ch, 0, 0, TO_ROOM);
    if (AFF_FLAGGED(ch, AFF_CURSE)) {
     send_to_char(ch, "Your cursing was nullified!\r\n");
     null_affect(ch, AFF_CURSE);
    }
    return;
   }
  } // End of no vict BLESS

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Bless who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_BLESS)) {
     send_to_char(ch, "They already have been blessed!\r\n");
     return;
    }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to bless.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_BLESS) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while chanting spiritual words. Your blessing fails!", TRUE, ch, 0, 0, TO_CHAR);
     act("$n focuses ki while chanting spiritual words. $n places a hand on your head, but nothing happens!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while chanting spiritual words. $n places a hand on $N's head, but nothing happens!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     int duration = roll_aff_duration(GET_INT(ch), 3);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_BLESS, SKILL_BLESS, duration, 0, 0, 0, 0, 0, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     if (IS_KAI(ch)) {
      GET_BLESSLVL(vict) = GET_SKILL(ch, SKILL_BLESS);
     } else {
      GET_BLESSLVL(vict) = 0;
     }
     act("You focus ki while chanting spiritual words. Blessing $N with faster regeneration!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki while chanting spiritual words. $n then places a hand on your head, blessing you!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while chanting spiritual words. $n then places a hand on $N's head, blessing them!", TRUE, ch, 0, vict, TO_NOTVICT);
     if ((vict->master == ch || ch->master == vict || ch->master == vict->master) && AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(vict, AFF_GROUP)) {
      if (IS_KAI(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 && rand_number(1, 3) == 3) {
       GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.05;
      }
     }
     if (AFF_FLAGGED(vict, AFF_CURSE)) {
      send_to_char(vict, "Your cursing was nullified!\r\n");
      null_affect(vict, AFF_CURSE);
     }
     return;
    }
   }

  } // End of victim of BLESS
 } // End of BLESS

 else if (!(strcmp(arg, "curse"))) {
  if (!know_skill(ch, SKILL_CURSE)) {
   return;
  }
  if (!*name) {
   if (AFF_FLAGGED(ch, AFF_CURSE)) {
    send_to_char(ch, "You already are cursed!\r\n");
    return;
   }
   else if (IS_DEMON(ch)) {
    send_to_char(ch, "You are immune to curses!\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to CURSE.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_CURSE) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki while chanting demonic words. Your cursing does nothing though, you must have messed up!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki while chanting demonic words. $n seems disappointed.", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    int duration = roll_aff_duration(GET_INT(ch), 3);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
    assign_affect(vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki while chanting demonic words. You feel your body recovering at below normal speed!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki while chanting demonic words. $n grins after finishing $s chant.", TRUE, ch, 0, 0, TO_ROOM);
    if (AFF_FLAGGED(ch, AFF_BLESS)) {
     send_to_char(ch, "Your blessing was nullified!\r\n");
     null_affect(ch, AFF_BLESS);
    }
    return;
   }
  } // End of no vict CURSE

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Curse who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 0)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (AFF_FLAGGED(vict, AFF_CURSE)) {
     send_to_char(ch, "They already have been cursed!\r\n");
     return;
    }
    else if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (IS_DEMON(vict)) {
     send_to_char(ch, "They are immune to curses!\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to CURSE.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_CURSE) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while chanting demonic words. Your cursing fails!", TRUE, ch, 0, 0, TO_CHAR);
     act("$n focuses ki while chanting demonic words. $n places a hand on your head, but nothing happens!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while chanting demonic words. $n places a hand on $N's head, but nothing happens!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     int duration = roll_aff_duration(GET_INT(ch), 3);;
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_CURSE, SKILL_CURSE, duration, 0, 0, 0, 0, 0, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while chanting demonic words. cursing $N with slower regeneration!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki while chanting demonic words. $n then places a hand on your head, cursing you!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while chanting demonic words. $n then places a hand on $N's head, cursing them!", TRUE, ch, 0, vict, TO_NOTVICT);
     if (AFF_FLAGGED(vict, AFF_BLESS)) {
      send_to_char(vict, "Your blessing was nullified!\r\n");
      null_affect(vict, AFF_BLESS);
     }
     return;
    }
   }

  } // End of victim of CURSE
 } // End of CURSE

 else if (!(strcmp(arg, "yoikominminken")) || !(strcmp(arg, "yoik"))) {
   if (!know_skill(ch, SKILL_YOIK)) {
    return;
   }
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Use Yoikominminken on who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 0)) {
    return;
   }
   else {
    if (AFF_FLAGGED(vict, AFF_SLEEP)) {
     send_to_char(ch, "They already have been put to sleep!\r\n");
     return;
    } else if (PLR_FLAGGED(vict, PLR_EYEC)) {
     send_to_char(ch, "Their eyes are closed!\r\n");
     return;
    } else if (AFF_FLAGGED(vict, AFF_BLIND)) {
     send_to_char(ch, "They appear to be blind!\r\n");
     return;
    } else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to use Yoikominminken.\r\n");
     return;
    } else if (GET_BONUS(vict, BONUS_INSOMNIAC)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while moving your hands in lulling patterns, but $N doesn't look the least bit sleepy!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki while moving $s hands in a lulling pattern, but you just don't feel tired.", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while moving $s hands in a lulling pattern, but $N doesn't look the least bit sleepy!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    } else if (GET_SKILL(ch, SKILL_YOIK) < axion_dice(0) || (GET_INT(ch) + rand_number(1, 3) < GET_INT(vict) + rand_number(1, 5))) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while moving your hands in lulling patterns, but fail to put $N to sleep!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki while moving $s hands in a lulling pattern, but you resist the technique!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while moving $s hands in a lulling pattern, but $N resists the technique!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    } else {
     int duration = rand_number(1, 2);
                                        /* Str , Con, Int, Agl, Wis, Spd */
     assign_affect(vict, AFF_SLEEP, SKILL_YOIK, duration, 0, 0, 0, 0, 0, 0);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki while moving your hands in lulling patterns, putting $N to sleep!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki while moving $s hands in a lulling pattern, before you realise it you are asleep!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki while moving $s hands in a lulling pattern, putting $N to sleep!", TRUE, ch, 0, vict, TO_NOTVICT);
     GET_POS(vict) = POS_SLEEPING;
     if (AFF_FLAGGED(vict, AFF_FLYING)) {
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_FLYING);
      GET_ALT(vict) = 0;
     }
     return;
    }
  } 
 } // End of Yoik

 else if (!(strcmp(arg, "vigor"))) {
  if (!know_skill(ch, SKILL_VIGOR)) {
   return;
  }
  if (!*name) {
   if (GET_MANA(ch) < GET_MAX_MANA(ch) / 10) {
    send_to_char(ch, "You do not have enough ki to use vigor.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_VIGOR) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
    reveal_hiding(ch, 0);
    act("You focus ki into your very cells, but fail at re-engerizing them!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki and glows green for a moment, $e then frowns.", TRUE, ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }
   else if (GET_MOVE(ch) >= GET_MAX_MOVE(ch)) {
    send_to_char(ch, "You already have full stamina.\r\n");
    return;
   }
   else {
    if (GET_BONUS(ch, BONUS_HEALER) > 0) {
     GET_MOVE(ch) += GET_MAX_MANA(ch) / 8;
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 8;
    } else {
     GET_MOVE(ch) += GET_MAX_MANA(ch) / 10;
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
    }
    if (GET_MOVE(ch) > GET_MAX_MOVE(ch)) {
     GET_MOVE(ch) = GET_MAX_MOVE(ch);
    }

    reveal_hiding(ch, 0);
    act("You focus ki into your very cells, and manage to re-energize them!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki and glows green for a moment, $e then smiles.", TRUE, ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }
  } // End of no vict VIGOR

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "VIGOR who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (IS_NPC(vict)) {
     send_to_char(ch, "Whatever would you waste your ki on them for?\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 10) {
     send_to_char(ch, "You do not have enough ki to use vigor.\r\n");
     return;
    }
   else if (GET_MOVE(vict) >= GET_MAX_MOVE(vict)) {
    send_to_char(ch, "They already have full stamina.\r\n");
    return;
   }
    else if (GET_SKILL(ch, SKILL_VIGOR) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
     reveal_hiding(ch, 0);
     act("You focus ki into $N's very cells, and fail at re-energizing them!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your very cells, but nothing happens!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and $N glows green for a moment, $N frowns.", TRUE, ch, 0, vict, TO_NOTVICT);
     WAIT_STATE(ch, PULSE_2SEC);
     return;
    }
    else {
     if (GET_BONUS(ch, BONUS_HEALER) > 0) {
      GET_MOVE(vict) += GET_MAX_MANA(vict) / 8;
      GET_MANA(ch) -= GET_MAX_MANA(ch) / 8;
     } else {
      GET_MOVE(vict) += GET_MAX_MANA(vict) / 10;
      GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
     }
     reveal_hiding(ch, 0);
     act("You focus ki into $N's very cells, and manage to re-energize them!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki into your very cells, and manages to re-energize them!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and $N glows green for a moment, $N smiles.", TRUE, ch, 0, vict, TO_NOTVICT);
     WAIT_STATE(ch, PULSE_2SEC);
     return;
    }
   }

  } // End of victim of VIGOR
 } // End of VIGOR

 else if (!(strcmp(arg, "cure"))) {
  if (!know_skill(ch, SKILL_CURE)) {
   return;
  }
  if (!*name) {
   if (!AFF_FLAGGED(ch, AFF_POISON)) {
    send_to_char(ch, "You are not poisoned!\r\n");
    return;
   }
   else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "You do not have enough ki to cure.\r\n");
    return;
   }
   else if (GET_SKILL(ch, SKILL_CURE) < axion_dice(0)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki and aim a pulsing light at your body. Nothing happens!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki and aims a pulsing light at $s body. Nothing seems to happen.", TRUE, ch, 0, 0, TO_ROOM);
    return;
   }
   else {
    affect_from_char(ch, SPELL_POISON);
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
    reveal_hiding(ch, 0);
    act("You focus ki and aim a pulsing light at your body. You feel the poison in your blood disappear!", TRUE, ch, 0, 0, TO_CHAR);
    act("$n focuses ki and aims a pulsing light at $s body. $n smiles.", TRUE, ch, 0, 0, TO_ROOM);
    null_affect(ch, AFF_POISON);
    return;
   }
  } // End of no vict cure

  else {
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "cure who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 2)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Use focus %s, not focus %s %s.\r\n", arg, arg, GET_NAME(vict));
     return;
    }
    if (!AFF_FLAGGED(vict, AFF_POISON)) {
     send_to_char(ch, "They are not poisoned!\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to cure.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_CURE) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki and aim a pulsing light at $N's body. Nothing happens.", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki and aims a pulsing light at your body. You are STILL poisoned!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and aims a pulsing light at $N's body. $N looks disappointed.", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     affect_from_char(vict, SPELL_POISON);
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki and aim a pulsing light at $N's body. $e is cured.", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki and aims a pulsing light at your body. You have been cured of your poison!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and aims a pulsing light at $N's body. $N smiles.", TRUE, ch, 0, vict, TO_NOTVICT);
     null_affect(vict, AFF_POISON);
     return;
    }
   }

  } // End of victim of cure
 } // End of cure

 else if (!(strcmp(arg, "poison"))) {

   if (!know_skill(ch, SKILL_POISON)) {
    return;
   }
   if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Poison who?\r\n");
    return;
   }
   if (!can_kill(ch, vict, NULL, 0)) {
    return;
   }
   else {
    if (ch == vict) {
     send_to_char(ch, "Why poison yourself?\r\n");
     return;
    }
    if (IS_NPC(vict)) {
     if (MOB_FLAGGED(vict, MOB_NOPOISON)) {
      send_to_char(ch, "You get the feeling that this being is immune to poison.\r\n");
      return;
     }
    }
    if (AFF_FLAGGED(vict, AFF_POISON)) {
     send_to_char(ch, "They already have been poisoned!\r\n");
     return;
    }
    else if (GET_MANA(ch) < GET_MAX_MANA(ch) / 20) {
     send_to_char(ch, "You do not have enough ki to poison.\r\n");
     return;
    }
    else if (GET_SKILL(ch, SKILL_POISON) < axion_dice(0)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki and fling poison at $N. You missed!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki and flings poison at you, but misses!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and flings poison at $N, but misses!", TRUE, ch, 0, vict, TO_NOTVICT);
     return;
    }
    else {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 20;
     reveal_hiding(ch, 0);
     act("You focus ki and fling poison at $N! The poison burns into $s skin!", TRUE, ch, 0, vict, TO_CHAR);
     act("$n focuses ki and flings poison at you! The poison burns into your skin!", TRUE, ch, 0, vict, TO_VICT);
     act("$n focuses ki and flings poison at $N! The poison burns into $s skin!", TRUE, ch, 0, vict, TO_NOTVICT);
     if (IS_NPC(vict)) {
      set_fighting(vict, ch);
     }
     if (IS_MUTANT(vict) && (GET_GENOME(vict, 0) == 7 || GET_GENOME(vict, 1) == 7)) {
      act("However $N seems unaffected by the poison.", TRUE, ch, 0, vict, TO_CHAR);
      act("Your natural immunity to poison prevents it from affecting you.", TRUE, ch, 0, vict, TO_VICT);
      act("However $N seems unaffected by the poison.", TRUE, ch, 0, vict, TO_NOTVICT);
     } else {
      vict->poisonby = ch;
      if (GET_CHARGE(ch) > 0) {
       send_to_char(ch, "You lose your concentration and release your charged ki!\r\n");
       do_charge(ch, "release", 0, 0);
      }
      int duration = GET_INT(ch) / 20;
      assign_affect(vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
     }
     return;
    }
   }
 } // End of POISON

 else {
  send_to_char(ch, "What do you want to focus?\r\n");
  return;
 }
}

ACMD(do_kaioken)
{
 char arg[MAX_INPUT_LENGTH];
 int roll = axion_dice(0), x = 0, pass = FALSE;
 int64_t boost = 0;
 one_argument(argument, arg);

 if (!check_skill(ch, SKILL_KAIOKEN)) {
  return;
 }
  if (GET_ALIGNMENT(ch) <= -50) {
  send_to_char(ch, "Your heart is too corrupt to use that technique!\r\n");
  return;
  } 
 if (!IS_NPC(ch)) {
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }
 }

 if (!*arg) {
  send_to_char(ch, "What level of kaioken do you want to try and achieve?\r\n"
                   "Syntax: kaioken 1-20\r\n");
  return;
 }

 if (GET_KAIOKEN(ch) > 0) {
  send_to_char(ch, "You drop out of kaioken.\r\n");
  act("$n@w drops out of kaioken.@n", TRUE, ch, 0, 0, TO_ROOM);
  if (GET_HIT(ch) - (gear_pl(ch) / 10) * GET_KAIOKEN(ch) > 0) {
   GET_HIT(ch) -= (gear_pl(ch) / 10) * GET_KAIOKEN(ch);
  }
  else {
   GET_HIT(ch) = 1;
  }
  GET_KAIOKEN(ch) = 0;
  return;
 }

 x = atoi(arg);

 if (x <= 0 || x > 20) {
  send_to_char(ch, "That level of kaioken dosn't exist...\r\n"
                   "Syntax: kaioken 1-20\r\n");
  return;
 }

 if (!IS_NPC(ch)) {
  if ((IS_TRANSFORMED(ch) || (IS_HOSHIJIN(ch) && GET_PHASE(ch) > 0)) && x > 5) {
   send_to_char(ch, "You can not manage a kaioken level higher than 5 when transformed.\r\n");
   return;
  }
 }

 if (GET_MANA(ch) < ((GET_MAX_MANA(ch) / 50) * x)) {
  send_to_char(ch, "You do not have enough ki to focus into your body for that level.\r\n");
  return;
 }

 int xnum = (x * 5) + 1;
 roll = rand_number(1, xnum);
 reveal_hiding(ch, 0);

 if (init_skill(ch, SKILL_KAIOKEN) < roll) {
  send_to_char(ch, "You try to focus your ki into your body but mess up somehow.\r\n");
  act("$n tries to use kaioken but messes up somehow.", TRUE, ch, 0, 0, TO_ROOM);
  GET_MANA(ch) -= (GET_MAX_MANA(ch) / 50) * x;
  improve_skill(ch, SKILL_KAIOKEN, 1);
  WAIT_STATE(ch, PULSE_1SEC);
  return;
 }

 switch (x) {
  case 1:
  case 2:
    pass = TRUE;
   break;
  case 3:
   if (GET_MAX_HIT(ch) < 5000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 4:
   if (GET_MAX_HIT(ch) < 10000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 5:
   if (GET_MAX_HIT(ch) < 15000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 6:
   if (GET_MAX_HIT(ch) < 25000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 7:
   if (GET_MAX_HIT(ch) < 35000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 8:
   if (GET_MAX_HIT(ch) < 50000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 9:
   if (GET_MAX_HIT(ch) < 75000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 10:
   if (GET_MAX_HIT(ch) < 100000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 11:
   if (GET_MAX_HIT(ch) < 150000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 12:
   if (GET_MAX_HIT(ch) < 200000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 13:
   if (GET_MAX_HIT(ch) < 250000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 14:
   if (GET_MAX_HIT(ch) < 300000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 15:
   if (GET_MAX_HIT(ch) < 400000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 16:
   if (GET_MAX_HIT(ch) < 500000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 17:
   if (GET_MAX_HIT(ch) < 600000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 18:
   if (GET_MAX_HIT(ch) < 700000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 19:
   if (GET_MAX_HIT(ch) < 800000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  case 20:
   if (GET_MAX_HIT(ch) < 1000000) {
    act("@rA blazing red aura bursts up around your body, flashing intensely before your body gives out and you release the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@rA blazing red aura bursts up around @R$n's @rbody, flashing intensely before $s body gives out and $e releases the kaioken because of the pressure!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else {
    pass = TRUE;
   }
   break;
  default:
   pass = TRUE;
  break;
 }

 if (pass == FALSE) {
  improve_skill(ch, SKILL_KAIOKEN, 1);
  GET_MANA(ch) -= ((GET_MAX_MANA(ch) / 50) * x);
  return;
 }

 boost = (gear_pl(ch) / 10) * x;
 if (GET_HIT(ch) > gear_pl(ch)) {
  GET_HIT(ch) = gear_pl(ch);
 }
 GET_HIT(ch) += boost;
 GET_MANA(ch) -= ((GET_MAX_MANA(ch) / 50) * x);

 GET_KAIOKEN(ch) = x;
 send_to_char(ch, "@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", GET_KAIOKEN(ch));
 act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", TRUE, ch, 0, 0, TO_ROOM);
 improve_skill(ch, SKILL_KAIOKEN, 1);
 REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_POWERUP);

 WAIT_STATE(ch, PULSE_1SEC);
}

ACMD(do_plant)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[100], obj_name[100];
  int roll = 0, detect = 0, fail = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Plant what on who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL) && GET_ADMLEVEL(ch) == ADMLVL_NONE) {
    send_to_char(ch, "That isn't such a good idea...\r\n");
    return;
  }

  roll = roll_skill(ch, SKILL_SLEIGHT_OF_HAND) + rand_number(1, 3);
  fail = rand_number(1, 105);

  if (HAS_FEAT(ch, FEAT_DEFT_HANDS))
    roll += 2;

  if (GET_POS(vict) < POS_SLEEPING)
    detect = 0;
  else
    detect = (roll_skill(vict, SKILL_SPOT) + rand_number(1, 3));

  /* NO NO With Imp's and Shopkeepers, and if player planting is not allowed */
  if ((ADM_FLAGGED(vict, ADM_NOSTEAL) || GET_MOB_SPEC(vict) == shop_keeper) && GET_ADMLEVEL(ch) < 5)
    roll = -10;         /* Failure */


  if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying))) {
   send_to_char(ch, "You don't have that to plant on them.\r\n"); 
   return;
  }
  if (roll <= detect && roll <= fail) {
   reveal_hiding(ch, 0);
   act("@C$n@w tries to plant $p@w on you!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@C$n@w tries to plant $p@w on @c$N@w!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", TRUE, ch, obj, vict, TO_CHAR);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  } else if (roll <= fail) {
   act("@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", TRUE, ch, obj, vict, TO_CHAR);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  } else if (GET_OBJ_WEIGHT(obj) + gear_weight(vict) > CAN_CARRY_W(vict)) {
   reveal_hiding(ch, 0);
   act("@C$n@w tries to plant $p@w on you!@n", TRUE, ch, obj, vict, TO_VICT);
   act("@C$n@w tries to plant $p@w on @c$N@w!@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n", TRUE, ch, obj, vict, TO_CHAR);
   WAIT_STATE(ch, PULSE_2SEC);
  } else if (roll <= detect) {
   act("@cYou feel like the weight of your inventory has changed.@n", TRUE, ch, obj, vict, TO_VICT);
   act("@c$N@w looks around after feeling $S pockets.@n", TRUE, ch, obj, vict, TO_NOTVICT);
   act("@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", TRUE, ch, obj, vict, TO_CHAR);
   obj_from_char(obj);
   obj_to_char(obj, vict);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  } else {
   act("@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", TRUE, ch, obj, vict, TO_CHAR);
   obj_from_char(obj);
   obj_to_char(obj, vict);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }
}

ACMD(do_forgery) {

  struct obj_data *obj2, *obj3 = NULL;
  struct obj_data *obj, *obj4 = NULL, *next_obj;
  int found = FALSE;
  char arg[MAX_INPUT_LENGTH];
 
  one_argument(argument, arg);

  if (IS_NPC(ch)) {
   return;
  }

  if (!know_skill(ch, SKILL_FORGERY)) {
   return;
  }

  if (!*arg) {
   send_to_char(ch, "Okay, make a forgery of what?\r\n");
   return;
  }

  if (!(obj2 = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
   send_to_char(ch, "You want to make a fake copy of what?\r\n");
   return;
  }

  for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
   if (found == FALSE && GET_OBJ_VNUM(obj) == 19 && (!OBJ_FLAGGED(obj, ITEM_BROKEN) && !OBJ_FLAGGED(obj, ITEM_FORGED))) {
    found = TRUE;
    obj4 = obj;
   }
  }

  if (found == FALSE || obj4 == NULL) {
   send_to_char(ch, "You need a forgery kit.\r\n");
   return;
  }

  if (GET_OBJ_VNUM(obj2) == 19) {
   send_to_char(ch, "You can't duplicate a forgery kit.\r\n");
   return;
  }

  if (OBJ_FLAGGED(obj2, ITEM_FORGED)) {
   send_to_char(ch, "%s is forgery, there is no reason to make a fake of a fake!\r\n", obj2->short_description);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }

  if (OBJ_FLAGGED(obj2, ITEM_BROKEN)) {
   send_to_char(ch, "%s is broken, there is no reason to make a fake of this mess!\r\n", obj2->short_description);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }  

  if (GET_OBJ_VNUM(obj2) >= 60000 || GET_OBJ_VNUM(obj2) == 0) {
   send_to_char(ch, "You can not make a forgery of that! It's far too squishy....");
   return;
  }
  if (GET_OBJ_VNUM(obj2) >= 18800 && GET_OBJ_VNUM(obj2) <= 18999) {
   send_to_char(ch, "You can not make a forgery of that!\r\n");
   return;
  }
  if (GET_OBJ_VNUM(obj2) >= 19080 && GET_OBJ_VNUM(obj2) <= 19199) {
   send_to_char(ch, "You can not make a forgery of that!\r\n");
   return;
  }
  if (GET_OBJ_VNUM(obj2) >= 4 && GET_OBJ_VNUM(obj2) <= 6) {
	send_to_char(ch, "You can not make a forgery of that!\r\n");
	return;
  }

  if (OBJ_FLAGGED(obj2, ITEM_PROTECTED)) {
   send_to_char(ch, "You don't know where to begin with this work of ART.\r\n");
   return;
  }

  reveal_hiding(ch, 0);
  act("@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", TRUE, ch, obj2, 0, TO_ROOM);
  improve_skill(ch, SKILL_FORGERY, 1);
  if (GET_SKILL(ch, SKILL_FORGERY) < axion_dice(0)) {
   if (rand_number(1, 10) >= 9) { /* Uh oh */
    send_to_char(ch, "In the middle of creating a forgery of %s you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.\r\n", obj2->short_description);
    extract_obj(obj4);
    extract_obj(obj2);
    return;
   }
   send_to_char(ch, "You start to make a forgery of %s but screw up and waste your forgery kit..\r\n", obj2->short_description);
   act("@c$n@w tried to duplicate $p but screws up somehow.@n", TRUE, ch, obj2, 0, TO_ROOM);
   obj_from_char(obj4);
   extract_obj(obj4);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }

  int loadn = GET_OBJ_VNUM(obj2);

  obj3 = read_object(loadn, VIRTUAL);
  add_unique_id(obj3);
  obj_to_char(obj3, ch);

   /* Set Object Variables */
    SET_BIT_AR(GET_OBJ_EXTRA(obj3), ITEM_FORGED);
    GET_OBJ_WEIGHT(obj3) = rand_number(GET_OBJ_WEIGHT(obj3) / 2, GET_OBJ_WEIGHT(obj3));

  obj_from_char(obj4);
  extract_obj(obj4);
  send_to_char(ch, "You make an excellent forgery of %s@n!\r\n", obj2->short_description);
  act("@c$n@w makes a perfect forgery of $p.@n", TRUE, ch, obj2, 0, TO_ROOM);
  WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_appraise) {
  int i, found;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  one_argument(argument, arg);

  if (IS_NPC(ch)) {
   return;
  }

  if (!know_skill(ch, SKILL_APPRAISE)) {
   return;
  }

  if (!*arg) {
   send_to_char(ch, "Okay, appraise what?\r\n");
   return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
   send_to_char(ch, "You want to appraise what?\r\n");
   return;
  }

  reveal_hiding(ch, 0);
  act("@c$n@w looks at $p, turning it over in $s hands.@n", TRUE, ch, obj, 0, TO_ROOM);
  improve_skill(ch, SKILL_APPRAISE, 1);
  if (GET_SKILL(ch, SKILL_APPRAISE) < axion_dice(-10)) {
   send_to_char(ch, "You fail to perceive the worth of %s..\r\n", obj->short_description);
   act("@c$n@w looks stumped about $p.@n", TRUE, ch, obj, 0, TO_ROOM);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }

  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
   send_to_char(ch, "%s is broken!\r\n", obj->short_description);
   act("@c$n@w looks at $p and frowns.@n", TRUE, ch, obj, 0, TO_ROOM);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }

  if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
   send_to_char(ch, "%s is fake and worthless!\r\n", obj->short_description);
   act("@c$n@w looks at $p with an angry face.@n", TRUE, ch, obj, 0, TO_ROOM);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
  }
  found = FALSE;
  int displevel = GET_OBJ_LEVEL(obj);

   if (GET_OBJ_TYPE(obj) == ITEM_WEAPON && OBJ_FLAGGED(obj, ITEM_CUSTOM))
    displevel = 20;

  send_to_char(ch, "%s is worth: %s\r\nMin Lvl: %d\r\n", obj->short_description, add_commas(GET_OBJ_COST(obj)), displevel);
  if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
     if (OBJ_FLAGGED(obj, ITEM_WEAPLVL1)) {
      send_to_char(ch, "Weapon Level: 1\nDamage Bonus: 5%s\r\n", "%");
     }
     else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL2)) {
      send_to_char(ch, "Weapon Level: 2\nDamage Bonus: 10%s\r\n", "%");
     }
     else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL3)) {
      send_to_char(ch, "Weapon Level: 3\nDamage Bonus: 20%s\r\n", "%");
     }
     else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL4)) {
      send_to_char(ch, "Weapon Level: 4\nDamage Bonus: 30%s\r\n", "%");
     }
     else if (OBJ_FLAGGED(obj, ITEM_WEAPLVL5)) {
      send_to_char(ch, "Weapon Level: 5\nDamage Bonus: 50%s\r\n", "%");
     }
  }
  send_to_char(ch, "Size: %s\r\n", size_names[GET_OBJ_SIZE(obj)]);
  if (OBJ_FLAGGED(obj, ITEM_SLOT1) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
   send_to_char(ch, "Token Slots  : @m0/1@n\n");
  } else if (OBJ_FLAGGED(obj, ITEM_SLOT1) && OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
   send_to_char(ch, "Token Slots  : @m1/1@n\n");
  } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
   send_to_char(ch, "Token Slots  : @m0/2@n\n");
  } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && OBJ_FLAGGED(obj, ITEM_SLOT_ONE) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
   send_to_char(ch, "Token Slots  : @m1/2@n\n");
  } else if (OBJ_FLAGGED(obj, ITEM_SLOT2) && !OBJ_FLAGGED(obj, ITEM_SLOTS_FILLED)) {
   send_to_char(ch, "Token Slots  : @m2/2@n\n");
  }
  send_to_char(ch, "Bonuses:");
  act("@c$n@w looks at $p and nods, a satisfied look on $s face.@n", TRUE, ch, obj, 0, TO_ROOM);
  int percent = FALSE;
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].modifier) {
      if (obj->affected[i].location == APPLY_REGEN || obj->affected[i].location == APPLY_TRAIN || obj->affected[i].location == APPLY_LIFEMAX) {
       percent = TRUE;
      }
      sprinttype(obj->affected[i].location, apply_types, buf, sizeof(buf));
      send_to_char(ch, "%s %+d%s to %s", found++ ? "," : "", obj->affected[i].modifier, percent == TRUE ? "%" : "", buf);
      percent = FALSE;
      switch (obj->affected[i].location) {
      case APPLY_FEAT:
        send_to_char(ch, " (%s)", feat_list[obj->affected[i].specific].name);
        break;
      case APPLY_SKILL:
        send_to_char(ch, " (%s)", spell_info[obj->affected[i].specific].name);
        break;
      }
    }
 }
 if (!found)
    send_to_char(ch, " None");
 char buf2[MAX_STRING_LENGTH];
 sprintbitarray(GET_OBJ_PERM(obj), affected_bits, AF_ARRAY_MAX, buf2);
 send_to_char(ch, "\nSpecial: %s\r\n", buf2); 

 WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_disguise) {
 int skill = 0, roll = 0;

 if (IS_NPC(ch)) {
  send_to_char(ch, "You forgot your disguise off in mobland.\r\n");
  return;
 }
 
 if (PLR_FLAGGED(ch, PLR_DISGUISED)) {
  send_to_char(ch, "You stop disguising yourself.\r\n");
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_DISGUISED);
  act("@C$n @wpulls off $s disguise and reveals $mself!", TRUE, ch, 0, 0, TO_ROOM);
  return;
 }

 if (!know_skill(ch, SKILL_DISGUISE)) {
  return;
 }

 if (!GET_EQ(ch, WEAR_HEAD)) {
  send_to_char(ch, "You can't disguise your identity without anything on your head.\r\n");
  return;
 }

 if (GET_MOVE(ch) < GET_MOVE(ch) / 50) {
  send_to_char(ch, "You are too tired to try that right now.\r\n");
  return;
 }
 
 skill = GET_SKILL(ch, SKILL_DISGUISE);
 roll = axion_dice(-10);
 
 if (skill > roll) {
  send_to_char(ch, "You managed to disguise yourself with some skilled manipulation of your headwear.\r\n");
  act("@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_DISGUISED);
  return;
 }
 else {
  send_to_char(ch, "You finish attempting to disguise yourself, but realize you failed and need to try again.\r\n");
  act("@C$n @wattempts and fails to disguise $mself properly and must try again.", TRUE, ch, 0, 0, TO_ROOM);
  if (GET_MOVE(ch) >= GET_MOVE(ch) / 50) {
   GET_MOVE(ch) -= GET_MOVE(ch) / 50;
  }
  else {
   GET_MOVE(ch) = 0;
  }
  return;
 }
}

ACMD(do_eavesdrop) {
  int dir;
  char buf[100];
  one_argument(argument, buf);

  if (GET_EAVESDROP(ch) > 0) {
    send_to_char(ch, "You stop eavesdropping.\r\n");
    GET_EAVESDROP(ch) = real_room(0);
    GET_EAVESDIR(ch) = -1;
    return;
  }

  if (!*buf) {
    send_to_char(ch, "In which direction would you like to eavesdrop?\r\n");
    return;
  }
  if ((dir = search_block(buf, dirs, FALSE)) < 0) {
    send_to_char(ch, "Which directions is that?\r\n");
    return;
  }
  if (!know_skill(ch, SKILL_EAVESDROP)) {
    return;
  }
  if (EXIT(ch, dir)) {
    if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(ch, buf);
    } else {
      GET_EAVESDROP(ch) = GET_ROOM_VNUM(EXIT(ch, dir)->to_room);
      GET_EAVESDIR(ch) = dir;
      send_to_char(ch, "Okay.\r\n");
    }
  } else
    send_to_char(ch, "There is not a room there...\r\n");
}

ACMD(do_zanzoken)
{

 int prob = 0, perc = 0;
 int64_t cost = 0;
 

 if (!know_skill(ch, SKILL_ZANZOKEN) && !IS_NPC(ch)) {
  return;
 }

 if (AFF_FLAGGED(ch, AFF_ZANZOKEN)) {
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
  send_to_char(ch, "You release the ki you had prepared for a zanzoken.\r\n");
  return;
 }

 if (GRAPPLING(ch) || GRAPPLED(ch)) {
  send_to_char(ch, "You are busy in a grapple!\r\n");
  return;
 }

 if (!IS_NPC(ch)) {
  prob = GET_SKILL(ch, SKILL_ZANZOKEN);
 } else {
  prob = rand_number(80, 90);
 }
 perc = axion_dice(0);
 cost = GET_MAX_MANA(ch) / 50;

 if (prob > 75) {
  cost *= 2;
 }  else if (prob > 50) {
  cost *= 4;
 } else if (prob >= 25) {
  cost *= 8;
 } else if (prob < 25) {
  cost *= 10;
 }

 if (GET_MANA(ch) < cost) {
  send_to_char(ch, "You do not have enough ki.\r\n");
  return;
 }

 if (prob < perc) {
  send_to_char(ch, "You focus your ki in preparation of a zanzoken but mess up and waste your ki!\r\n");
  improve_skill(ch, SKILL_ZANZOKEN, 2);
  GET_MANA(ch) -= cost;
  WAIT_STATE(ch, PULSE_2SEC);
  return;
 }
 
 act("@wYou focus your ki, preparing to move at super speeds if necessary.@n", TRUE, ch, 0, 0, TO_CHAR);
 GET_MANA(ch) -= cost;
 SET_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
 improve_skill(ch, SKILL_ZANZOKEN, 2);
 WAIT_STATE(ch, PULSE_2SEC);
}

ACMD(do_block)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (IS_NPC(ch))
   return;

  if (!*arg) {
   if (!BLOCKS(ch)) {
    send_to_char(ch, "You want to block who?\r\n");
    return;
   }
   if (BLOCKS(ch)) {
    act("@wYou stop blocking @c$N@w.@n", TRUE, ch, 0, BLOCKS(ch), TO_CHAR);
    act("@C$n@w stops blocking you.@n", TRUE, ch, 0, BLOCKS(ch), TO_VICT);
    act("@C$n@w stops blocking @c$N@w.@n", TRUE, ch, 0, BLOCKS(ch), TO_NOTVICT);
    vict = BLOCKS(ch);
    BLOCKED(vict) = NULL;
    BLOCKS(ch) = NULL;
    return;
   }
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "You do not see the target here.\r\n");
   return;
  }

  if (BLOCKS(ch) == vict) {
   send_to_char(ch, "They are already blocked by you!\r\n");
   return;
  }

  if (ch == vict) {
   send_to_char(ch, "You can't block yourself, are you mental?\r\n");
   return;
  }

  if (BLOCKED(vict)) {
   send_to_char(ch, "They are already blocked by someone else!\r\n");
   return;
  }

  if (BLOCKS(ch)) {
    act("@wYou stop blocking @c$N@w.@n", TRUE, ch, 0, BLOCKS(ch), TO_CHAR);
    act("@C$n@w stops blocking you.@n", TRUE, ch, 0, BLOCKS(ch), TO_VICT);
    act("@C$n@w stops blocking @c$N@w.@n", TRUE, ch, 0, BLOCKS(ch), TO_NOTVICT);
    struct char_data *oldv = BLOCKS(ch); 
    BLOCKED(oldv) = NULL;
    BLOCKS(ch) = vict;
    BLOCKED(vict) = ch;
    reveal_hiding(ch, 0);
    act("@wYou start blocking @c$N's@w escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_CHAR);
    act("@C$n@w starts blocking your escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_VICT);
    act("@C$n@w starts blocking @c$N's@w escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_NOTVICT);
   return;
  }
  else {
    BLOCKS(ch) = vict;
    BLOCKED(vict) = ch;
    reveal_hiding(ch, 0);
    act("@wYou start blocking @c$N's@w escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_CHAR);
    act("@C$n@w starts blocking your escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_VICT);
    act("@C$n@w starts blocking @c$N's@w escape.@n", TRUE, ch, 0, BLOCKS(ch), TO_NOTVICT);
   return;
  }

}

ACMD(do_eyec)
{

 if (IS_NPC(ch))
  return;

 if (PLR_FLAGGED(ch, PLR_EYEC)) {
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_EYEC);  
  act("@wYou open your eyes.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@w opens $s eyes.@n", TRUE, ch, 0, 0, TO_ROOM);
 }

 else if (!PLR_FLAGGED(ch, PLR_EYEC)) {
  SET_BIT_AR(PLR_FLAGS(ch), PLR_EYEC);
  act("@wYou close your eyes.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@w closes $s eyes.@n", TRUE, ch, 0, 0, TO_ROOM);
 }

 WAIT_STATE(ch, PULSE_1SEC);
}

ACMD(do_solar)
{
 struct char_data *vict = NULL, *next_v = NULL;

 int prob = 0, perc = 0, cost = 0, bonus = 0;

 if (!know_skill(ch, SKILL_SOLARF)) {
  return;
 }

  if (!limb_ok(ch, 0)) {
   return;
  }

 prob = GET_SKILL(ch, SKILL_SOLARF);
 perc = rand_number(0, 101);
 
 if (prob >= 75) {
  cost = GET_MAX_MANA(ch) / 50;
 }
 else if (prob >= 50) {
  cost = GET_MAX_MANA(ch) / 25;
 }
 else if (prob >= 25) {
  cost = GET_MAX_MANA(ch) / 20;
 }
 else if (prob < 25) {
  cost = GET_MAX_MANA(ch) / 15;
 }

 if (GET_MANA(ch) < cost) {
  send_to_char(ch, "You do not have enough ki.\r\n");
  return;
 }

 bonus = GET_INT(ch) / 3;
 prob += bonus;

 if (prob < perc) {
  act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' but nothing happens!@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' but nothing happens!@n", TRUE, ch, 0, 0, TO_ROOM);
  GET_MANA(ch) -= cost;   
  WAIT_STATE(ch, PULSE_3SEC);
  improve_skill(ch, SKILL_SOLARF, 0);
  return;
 }
 
 act("@WYou raise both your hands to either side of your face, while closing your eyes, and shout '@YSolar Flare@W' as a blinding light fills the area!@n", TRUE, ch, 0, 0, TO_CHAR);
 act("@C$n@W raises both $s hands to either side of $s face, while closing $s eyes, and shouts '@YSolar Flare@W' as a blinding light fills the area!@n", TRUE, ch, 0, 0, TO_ROOM);

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
   next_v = vict->next_in_room;
   
   if (vict == ch)
    continue;
   else if (PLR_FLAGGED(vict, PLR_EYEC))
    continue;
   else if (AFF_FLAGGED(vict, AFF_BLIND))
    continue;
   else if (GET_POS(vict) == POS_SLEEPING)
    continue;
   else {
    int duration = 1;
    assign_affect(vict, AFF_BLIND, SKILL_SOLARF, duration, 0, 0, 0, 0, 0, 0);
    act("@W$N@W is @YBLINDED@W!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@RYou are @YBLINDED@R!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@W$N@W is @YBLINDED@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   }
  }
  improve_skill(ch, SKILL_SOLARF, 0);
  GET_MANA(ch) -= cost;   
  WAIT_STATE(ch, PULSE_3SEC);
}

ACMD(do_heal)
{
  int64_t cost = 0, prob = 0, perc = 0, heal = 0, bonus = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!check_skill(ch, SKILL_HEAL)) {
   return;
  }

  if (!limb_ok(ch, 0)) {
   return;
  }

  if (!*arg) {
   send_to_char(ch, "You want to heal WHO?\r\n");
   return;
  }
  
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "You do not see the target here.\r\n");
   return;
  }

  prob = init_skill(ch, SKILL_HEAL);
  perc = rand_number(0, 110);

  if (prob >= 100) {
   cost = GET_MAX_MANA(ch) / 20;
   heal = GET_MAX_HIT(vict) / 5;
  }
  else if (prob >= 90) {
   cost = GET_MAX_MANA(ch) / 16;
   heal = GET_MAX_HIT(vict) / 10;
  }
  else if (prob >= 75) {
   cost = GET_MAX_MANA(ch) / 14;
   heal = GET_MAX_HIT(vict) / 12;
  }
  else if (prob >= 50) {
   cost = GET_MAX_MANA(ch) / 12;
   heal = GET_MAX_HIT(vict) / 15;
  }
  else if (prob >= 25) {
   cost = GET_MAX_MANA(ch) / 10;
   heal = GET_MAX_HIT(vict) / 20;
  }
  else if (prob < 25) {
   cost = GET_MAX_MANA(ch) / 6;
   heal = GET_MAX_HIT(vict) / 20;
  }

  if (GET_BONUS(ch, BONUS_HEALER) > 0) {
   heal += heal * .1;
  }

  if (heal < gear_pl(vict)) {
   heal += (heal / 100) * (GET_WIS(ch) / 4);
  }

  if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You do not have enough ki.\r\n");
   return;
  }

  if (GET_HIT(vict) >= gear_pl(vict)) {
   if (vict != ch) {
    send_to_char(ch, "They are already at full health.\r\n");
   } else {
    send_to_char(ch, "You are already at full health.\r\n");
   }
   return;   
  }

  if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) >= ((gear_pl(vict) / 100) * GET_SUPPRESS(vict))) {
   send_to_char(ch, "They are already at full health.\r\n");
   return;
  }

  bonus = (GET_INT(ch) / 2) + (GET_WIS(ch) / 3);
  prob += bonus;

  if (prob < perc) {
   if (vict != ch) {
    act("@WYou place your hands near @c$N@W, but fail to concentrate enough to heal them!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W places $s hands near you, but nothing happens!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n@W places $s hands near @c$N@W, but nothing happens.", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_MANA(ch) -= cost;
    improve_skill(ch, SKILL_HEAL, 0);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }
   if (vict == ch) {
    act("@WYou place your hands on your body, but fail to concentrate to heal yourself!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n@W places $s hands on $s body, but nothing happens.", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_MANA(ch) -= cost;
    improve_skill(ch, SKILL_HEAL, 0);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }
  }

  if (vict != ch) {
   if (GET_BONUS(ch, BONUS_HEALER) > 0) {
    heal += heal * .25;
   }
   act("@WYou place your hands near @c$N@W and an orange glow surrounds $M!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W places $s hands near you and an orange glow surrounds you!@n", TRUE, ch, 0, vict, TO_VICT);
   act("@C$n@W places $s hands near @c$N@W and an orange glow surrounds $M.", TRUE, ch, 0, vict, TO_NOTVICT);
   GET_MANA(ch) -= cost;
   if (GET_HIT(vict) + heal <= gear_pl(vict)) {
    GET_HIT(vict) += heal;
   }
   else {
    GET_HIT(vict) = gear_pl(vict);
   }
   if (IS_NAIL(ch)) {
    if (GET_SKILL(ch, SKILL_HEAL) >= 100) {
     GET_MOVE(ch) += heal * 0.4;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    } else if (GET_SKILL(ch, SKILL_HEAL) >= 60) {
     GET_MOVE(ch) += heal * 0.2;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    } else if (GET_SKILL(ch, SKILL_HEAL) >= 40) {
     GET_MOVE(ch) += heal * 0.1;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    }
   }

   if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) > (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict))) {
    GET_HIT(vict) = (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict));
    send_to_char(vict, "@mYou are healed to your suppression limit.@n\r\n");
   }
   null_affect(ch, AFF_POISON);
   null_affect(ch, AFF_BLIND);
    if (AFF_FLAGGED(vict, AFF_BURNED)) {
        send_to_char(vict, "Your burns are healed now.\r\n");
        act("$n@w's burns are now healed.@n", TRUE, vict, 0, 0, TO_ROOM);
        REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_BURNED);
    }
    if (AFF_FLAGGED(vict, AFF_HYDROZAP)) {
        send_to_char(vict, "You no longer feel a great thirst.\r\n");
        act("$n@w no longer looks as if they could drink an ocean.@n", TRUE, vict, 0, 0, TO_ROOM);
        REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_HYDROZAP);
    }
   GET_LIMBCOND(vict, 1) = 100;
   GET_LIMBCOND(vict, 2) = 100;
   GET_LIMBCOND(vict, 3) = 100;
   GET_LIMBCOND(vict, 4) = 100;
   if (GET_LIFEFORCE(vict) <= GET_LIFEMAX(vict) * 0.5 && !IS_ANDROID(vict)) {
    GET_LIFEFORCE(vict) += GET_LIFEMAX(ch) * 0.35;
    if (GET_LIFEFORCE(vict) > GET_LIFEMAX(ch)) {
     GET_LIFEFORCE(vict) = GET_LIFEMAX(ch);
    }
    send_to_char(vict, "You feel that your lifeforce has recovered some!\r\n");
   }
   improve_skill(ch, SKILL_HEAL, 0);
   if (vict->master == ch || ch->master == vict || ch->master == vict->master) {
    if (IS_NAIL(ch) && IS_NAMEK(ch) && level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 && GET_HIT(vict) <= gear_pl(vict) * 0.85 && rand_number(1, 3) == 3) {
     GET_EXP(ch) += level_exp(ch, GET_LEVEL(ch) + 1) * 0.005;
    }
   }

   WAIT_STATE(ch, PULSE_2SEC);
  }

  if (vict == ch) {
   if (GET_BONUS(ch, BONUS_HEALER) > 0) {
    heal += heal * .25;
   }
   act("@WYou place your hands on your body and an orange glow surrounds you!@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W places $s hands on $s body and an orange glow surrounds $m.", TRUE, ch, 0, vict, TO_NOTVICT);
   GET_MANA(ch) -= cost;
   if (GET_HIT(vict) + heal <= gear_pl(vict)) {
    GET_HIT(vict) += heal;
   }
   else {
    GET_HIT(vict) = gear_pl(vict);
   }
   if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) > (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict))) {
    GET_HIT(vict) = (((gear_pl(vict)) / 100) * GET_SUPPRESS(vict));
    send_to_char(vict, "@mYou are healed to your suppression limit.@n\r\n");
   }
   if (IS_NAIL(ch)) {
    if (GET_SKILL(ch, SKILL_HEAL) >= 100) {
     GET_MOVE(ch) += heal * 0.4;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    } else if (GET_SKILL(ch, SKILL_HEAL) >= 60) {
     GET_MOVE(ch) += heal * 0.2;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    } else if (GET_SKILL(ch, SKILL_HEAL) >= 40) {
     GET_MOVE(ch) += heal * 0.1;
     send_to_char(vict, "@GYou feel some of your stamina return as well!@n\r\n");
    }
   }
   REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_BLIND);
   GET_LIMBCOND(vict, 1) = 100;
   GET_LIMBCOND(vict, 2) = 100;
   GET_LIMBCOND(vict, 3) = 100;
   GET_LIMBCOND(vict, 4) = 100;
   if (!PLR_FLAGGED(vict, PLR_TAIL) && (IS_BIO(vict) || IS_ICER(vict))) {
    SET_BIT_AR(PLR_FLAGS(vict), PLR_TAIL);
   }
   if (!PLR_FLAGGED(vict, PLR_STAIL) && (IS_SAIYAN(vict) || IS_HALFBREED(vict))) {
    SET_BIT_AR(PLR_FLAGS(vict), PLR_STAIL);
   }
   improve_skill(ch, SKILL_HEAL, 0);
   WAIT_STATE(ch, PULSE_2SEC);
  }
    
  return;
}

ACMD(do_barrier)
{
  int prob = 0, perc = 0, size = 0;
  char arg[MAX_INPUT_LENGTH];  

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_BARRIER) && !GET_SKILL(ch, SKILL_AQUA_BARRIER)) {
   return;
  }

  if (!*arg) {
   send_to_char(ch, "[Syntax] barrier < 1-75 | release >\r\n");
   return;
  }

  if (AFF_FLAGGED(ch, AFF_SANCTUARY) && !strcasecmp("release", arg)) {
   act("@BYou dispel your barrier, releasing its energy.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@B$n@B dispels $s barrier, releasing its energy.@n", TRUE, ch, 0, 0, TO_ROOM);
   GET_BARRIER(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SANCTUARY);
   return;
  } else if (!strcasecmp("release", arg)) {
   send_to_char(ch, "You don't have a barrier.\r\n");
   return;
  }


  if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
   send_to_char(ch, "You already have a barrier, try releasing it.\r\n");
   return;
  }

  if (GET_COOLDOWN(ch) > 0) {
   send_to_char(ch, "You must wait a short period before concentrating again.\r\n");
   return;
  }

  size = atoi(arg);
  int64_t cost = 0;
  prob = 0;
  if (GET_SKILL(ch, SKILL_BARRIER)) {
   prob = init_skill(ch, SKILL_BARRIER);
  } else {
   prob = GET_SKILL(ch, SKILL_AQUA_BARRIER);
  }
  perc = axion_dice(0);

  cost = (GET_MAX_MANA(ch) * 0.01) * (size * 0.5);

  if (size > prob) {
   send_to_char(ch, "You can not create a barrier that is stronger than your skill in barrier.\r\n");
   return;
  }
  else if (size < 1) {
   send_to_char(ch, "You have to put at least some ki into the barrier!\r\n");
   return;
  }
  else if (size > 75) {
   send_to_char(ch, "You can't control a barrier with more than 75 percent!\r\n");
   return;
  }
  else if (GET_CHARGE(ch) < cost) {
   send_to_char(ch, "You do not have enough ki charged up!\r\n");
   return;
  }
  else if (prob < perc) {
   act("@BYou shout as you form a barrier of ki around your body, but you imbalance it and it explodes outward!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@B$n@B shouts as $e forms a barrier of ki around $s body, but it becomes imbalanced and explodes outward!@n", TRUE, ch, 0, 0, TO_ROOM);
   GET_CHARGE(ch) -= cost;
   if (GET_SKILL(ch, SKILL_BARRIER)) {
    improve_skill(ch, SKILL_BARRIER, 2);
   } else {
    improve_skill(ch, SKILL_AQUA_BARRIER, 2);
   }
   GET_COOLDOWN(ch) = 30;
   return;
  }
  else {
   if (GET_SKILL(ch, SKILL_BARRIER)) {
    act("@BYou shout as you form a barrier of ki around your body!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@B$n@B shouts as $e forms a barrier of ki around $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
   } else {
    act("@BYou shout as you form a barrier of ki and raging waters around your body!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@B$n@B shouts as $e forms a barrier of ki and raging waters around $s body!@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   GET_BARRIER(ch) = (GET_MAX_MANA(ch) / 100) * size;
   GET_CHARGE(ch) -= cost;
   if (GET_SKILL(ch, SKILL_BARRIER)) {
    improve_skill(ch, SKILL_BARRIER, 2);
   } else {
    improve_skill(ch, SKILL_AQUA_BARRIER, 2);
   }
   SET_BIT_AR(AFF_FLAGS(ch), AFF_SANCTUARY);
   GET_COOLDOWN(ch) = 20;
   return;
  }
}

ACMD(do_instant)
{

  int skill = 0, perc = 0, skill_num = 0, location = 0;
  int64_t cost = 0;
  struct char_data *tar = NULL;

  char arg[MAX_INPUT_LENGTH] = "";

  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
   if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
    ARENA_IDNUM(ch) = -1;
    send_to_char(ch, "You stop watching the arena action.\r\n");
   }
  }
  if (!know_skill(ch, SKILL_INSTANTT)) {
    return;
  } else if (!GET_SKILL(ch, SKILL_SENSE) && !PLR_FLAGGED(ch, PLR_SENSEM)) {
    send_to_char(ch, "You can't sense them to go to there!\r\n");
    return;
  } else if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
  } else if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  } else if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
   send_to_char(ch, "@rYou are in a pocket dimension!@n\r\n");
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_HELL)) {
   send_to_char(ch, "You can not leave where you are at!\r\n");
   return;
  } else if (!*arg) {
   send_to_char(ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
   send_to_char(ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
   return;
  }

  if (GET_SKILL(ch, SKILL_INSTANTT) > 75) {
   cost = GET_MAX_MANA(ch) / 40;
  } else if (GET_SKILL(ch, SKILL_INSTANTT) > 50) {
   cost = GET_MAX_MANA(ch) / 20;
  } else if (GET_SKILL(ch, SKILL_INSTANTT) > 25) {
   cost = GET_MAX_MANA(ch) / 15;
  } else if (GET_SKILL(ch, SKILL_INSTANTT) < 25) {
   cost = GET_MAX_MANA(ch) / 10;
  }

  if (GET_MANA(ch) - cost < 0) {
   send_to_char(ch, "You do not have enough ki to instantaneously move.\r\n");
   return;
  }

  perc = axion_dice(0);
  skill = GET_SKILL(ch, SKILL_INSTANTT);
  skill_num = SKILL_INSTANTT;

  if (!strcasecmp(arg, "planet-earth")) {
   location = 300;
  } else if (!strcasecmp(arg, "planet-namek")) {
   location = 10222;
  } else if (!strcasecmp(arg, "planet-frigid")) {
   location = 4017;
  } else if (!strcasecmp(arg, "planet-vegeta")) {
   location = 2200;
  } else if (!strcasecmp(arg, "planet-konack")) {
   location = 8006;
  } else if (!strcasecmp(arg, "planet-aether")) {
   location = 12024;
  } else if (!(tar = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
   send_to_char(ch, "@RThat target was not found.@n\r\n");
   send_to_char(ch, "Who or where do you want to instant transmission to? [target | planet-(planet name)]\r\n");
   send_to_char(ch, "Example: instant goku\nExample 2: instant planet-earth\r\n");
   return;
  }

  if (skill < perc || (FIGHTING(ch) && rand_number(1, 2) <= 1)) {
   if (tar != NULL) {
    if (tar != ch) {
     send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
     GET_MANA(ch) -= cost;
     improve_skill(ch, skill_num, 2);
     WAIT_STATE(ch, PULSE_2SEC);
     return;
    } else {
     send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
     return;
    }
   } else {
    send_to_char(ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
    GET_MANA(ch) -= cost;
    improve_skill(ch, skill_num, 2);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }
  }

 reveal_hiding(ch, 0);
 WAIT_STATE(ch, PULSE_2SEC);
 if (tar != NULL) {
  if (tar == ch) {
   send_to_char(ch, "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n");
   return;
  } else if (GRAPPLING(ch) && GRAPPLING(ch) == tar) {
   send_to_char(ch, "You are already in the same room with them and are grappling with them!\r\n");
   return;
  } else if (!read_sense_memory(ch, tar)) {
   send_to_char(ch, "You've never sensed them up close so you do not have a good bearing on their ki signal.\r\n");
   return;
  } else if (GET_ADMLEVEL(tar) > 0 && GET_ADMLEVEL(ch) < 1) {
   send_to_char(ch, "That immortal prevents you from reaching them.\r\n");
   return;
  } else if (IS_ANDROID(tar) || GET_HIT(tar) < (GET_HIT(ch) * 0.001) + 1) {
   send_to_char(ch, "You can't sense them well enough.\r\n");
   return;
  } else if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL) && ROOM_FLAGGED(IN_ROOM(tar), ROOM_AL)) {
   send_to_char(ch, "They are dead and can't be reached.\r\n");
   return;
  } else if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL) && ROOM_FLAGGED(IN_ROOM(tar), ROOM_RHELL)) {
   send_to_char(ch, "They are dead and can't be reached.\r\n");
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(tar), ROOM_NOINSTANT)) {
   send_to_char(ch, "You can not go there as it is a protected area!\r\n");
   return;
  }

  GET_MANA(ch) -= cost;
  act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move through the molecules of the universe faster than the speed of light. You stop as you arrive at $N@w!@n", TRUE, ch, 0, tar, TO_CHAR);
  act("@w$n@w appears in an instant out of nowhere right next to you!@n", TRUE, ch, 0, tar, TO_VICT);
  act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", TRUE, ch, 0, tar, TO_NOTVICT);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANSMISSION);
  handle_teleport(ch, tar, 0);
  improve_skill(ch, skill_num, 2);
 } else {
  GET_MANA(ch) -= cost;
  act("@wPlacing two fingers on your forehead you close your eyes and concentrate. Accelerating to such a speed that you move faster than light and arrive almost instantly at your destination. Having located the planet by its collective population's ki.@n", TRUE, ch, 0, 0, TO_CHAR);
  act("@w$n@w places two fingers on $s forehead and disappears in an instant!@n", TRUE, ch, 0, 0, TO_NOTVICT);
  handle_teleport(ch, NULL, location);
  improve_skill(ch, skill_num, 2);
 }

}

void load_shadow_dragons()
{
  struct char_data *mob = NULL;
  mob_rnum r_num;

  if (SHADOW_DRAGON1 > 0) {
   r_num = real_mobile(SHADOW_DRAGON1_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON1));
   mob = NULL;
  }

  if (SHADOW_DRAGON2 > 0) {
   r_num = real_mobile(SHADOW_DRAGON2_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON2));
   mob = NULL;
  }

  if (SHADOW_DRAGON3 > 0) {
   r_num = real_mobile(SHADOW_DRAGON3_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON3));
   mob = NULL;
  }

  if (SHADOW_DRAGON4 > 0) {
   r_num = real_mobile(SHADOW_DRAGON4_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON4));
   mob = NULL;
  }

  if (SHADOW_DRAGON5 > 0) {
   r_num = real_mobile(SHADOW_DRAGON5_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON5));
   mob = NULL;
  }

  if (SHADOW_DRAGON6 > 0) {
   r_num = real_mobile(SHADOW_DRAGON6_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON6));
   mob = NULL;
  }

  if (SHADOW_DRAGON7 > 0) {
   r_num = real_mobile(SHADOW_DRAGON7_VNUM);
   mob = read_mobile(r_num, REAL);
   char_to_room(mob, real_room(SHADOW_DRAGON7));
  }

 save_mud_time(&time_info);
}

void wishSYS(void)
{
  if (SHENRON == TRUE) {
   if (SELFISHMETER < 10) {
    switch (DRAGONC) {
     case 300:
      send_to_room(real_room(DRAGONR), "@WThe dragon balls on the ground begin to glow yellow in slow pulses.@n\r\n");
      send_to_planet(0, ROOM_EARTH, "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
      DRAGONC -= 1;
      break;
     case 295:
      send_to_room(real_room(DRAGONR), "@WSuddenly lightning shoots into the sky, twisting about as a roar can be heard for miles!@n\r\n");
      send_to_planet(0, ROOM_EARTH, "@DThe sky flashes with lightning.@n\r\n");
      DRAGONC -= 1;
      break;
     case 290:
      send_to_room(real_room(DRAGONR), "@WThe lightning takes shape and slowly the Eternal Dragon, Shenron, can be made out from the glow!@n\r\n");
      char_from_room(EDRAGON);
      char_to_room(EDRAGON, real_room(DRAGONR));
      DRAGONC -= 1;
      break;
     case 285:
      send_to_planet(0, ROOM_EARTH, "@DThe lightning stops suddenly, but the sky remains mostly dark.@n\r\n");
      DRAGONC -= 1;
      break;
     case 280:
      send_to_room(real_room(DRAGONR), "@WThe glow around Shenron becomes subdued as the Eternal Dragon coils so that his head is looking down on the dragon balls!@n\r\n");
      DRAGONC -= 1;
      break;
     case 275:
      send_to_room(real_room(DRAGONR), "@wShenron says, '@CWho summoned me? I will grant you any two wishes that are within my power.@w'@n\r\n");
      DRAGONC -= 1;
      break;
     case 180:
      send_to_room(real_room(DRAGONR), "@wShenron says, '@CMake your wish already, you only have 3 minutes remaining.@w'@n\r\n");
      DRAGONC -= 1;
      break;
     case 120:
      send_to_room(real_room(DRAGONR), "@wShenron says, '@CMake your wish. I am losing patience, you only have 2 minutes left.@w'@n\r\n");
      DRAGONC -= 1;
      break;
     case 60:
      send_to_room(real_room(DRAGONR), "@wShenron says, '@CMake your wish now! You only have 1 minute left.@w'@n\r\n");
      DRAGONC -= 1;
      break;
     case 0:
      send_to_room(real_room(DRAGONR), "Shenron growls and disappears with a blinding flash that is absorbed into the dragon balls. The glowing dragon balls then float high into the sky, splitting into several directions and streaking across the sky!@n\r\n");
      send_to_planet(0, ROOM_EARTH, "@DThe sky grows brighter again as the clouds disappear magicly.@n\r\n");
      extract_char(EDRAGON);
      SHENRON = FALSE;
      DRAGONC -= 1;
      save_mud_time(&time_info);
     break;
     default:
     DRAGONC -= 1;
     break;
    }
    if (WISH[0] == 1 && WISH[1] == 1) {
     DRAGONC = 0;
     WISH[0] = 0;
     WISH[1] = 0;
    }
   } else {
    send_to_room(real_room(DRAGONR), "@RThe dragon balls suddenly begin to crack and darkness begins to pour out through the cracks! Shenron begins to turn pitch black slowly as the darkness escapes. Suddenly Shenron explodes out into the distance in seven parts. Each part taking a dragon ball with it!@n\r\n");
    int num = rand_number(200, 20000), done = FALSE, place = 1;
     DRAGONC = 0;
     WISH[0] = 0;
     WISH[1] = 0;
    while (done == FALSE) {
     switch (place) {
      case 1:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) || ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON1 = num;
         place = 2;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 2:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) ||  ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON2 = num;
         place = 3;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 3:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) || ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON3 = num;
         place = 4;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 4:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) || ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON4 = num;
         place = 5;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 5:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) ||  ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON5 = num;
         place = 6;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 6:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) || ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON6 = num;
         place = 7;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
      case 7:
       if (real_room(num) != NOWHERE) {
        if (ROOM_FLAGGED(real_room(num), ROOM_EARTH) || ROOM_FLAGGED(real_room(num), ROOM_VEGETA) || ROOM_FLAGGED(real_room(num), ROOM_FRIGID) || ROOM_FLAGGED(real_room(num), ROOM_AETHER) || ROOM_FLAGGED(real_room(num), ROOM_NAMEK) || ROOM_FLAGGED(real_room(num), ROOM_KONACK) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT) || ROOM_FLAGGED(real_room(num), ROOM_YARDRAT)) {
         SHADOW_DRAGON7 = num;
         done = TRUE;
         num = rand_number(200, 20000);
        } else {
         num = rand_number(200, 20000);
        }
       } else {
        num = rand_number(200, 20000);
       }
      break;
     } /* End switch */
     save_mud_time(&time_info);
    } /* End while */
     struct char_data *mob = NULL;
     mob_rnum r_num;

     r_num = real_mobile(SHADOW_DRAGON1_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON1));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON2_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON2));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON3_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON3));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON4_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON4));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON5_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON5));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON6_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON6));
     mob = NULL;

     r_num = real_mobile(SHADOW_DRAGON7_VNUM);
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, real_room(SHADOW_DRAGON7));
     mob = NULL;
 
     extract_char(EDRAGON);
     SHENRON = FALSE;
     DRAGONC = 0;
   } /* End else */
  }
}

ACMD(do_summon)
{

   int summoned = FALSE, count = 0;
   int dball[7] = {20, 21, 22, 23, 24, 25, 26};
   int dball2[7] = {20, 21, 22, 23, 24, 25, 26};
   struct obj_data *obj, *next_obj;
   struct char_data *mob=NULL;
   mob_rnum r_num;


    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH)) {
     send_to_char(ch, "@wYou can not summon Shenron when you are not on earth.@n\r\n");
     return;
    } // are they on earth?

    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOINSTANT) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
     send_to_char(ch, "You can not summon shenron in this protected area!\r\n");
     return;
    }
    if (SECT(IN_ROOM(ch)) == SECT_INSIDE) {
     send_to_char(ch, "Go outside to summon Shenron!\r\n");
     return;
    }

    for (obj = ch->carrying; obj; obj = next_obj) {
     next_obj = obj->next_content;
     if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
      continue;
     }
     if (GET_OBJ_VNUM(obj) == dball[0]) {
      dball[0] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[1]) {
      dball[1] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[2]) {
      dball[2] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[3]) {
      dball[3] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[4]) {
      dball[4] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[5]) {
      dball[5] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[6]) {
      dball[6] = -1;
      count++;
      continue;
     }
     else {
      continue;
     } // end if
    } // end repeat for

    if (count == 7) {
     summoned = TRUE;
    }

    if (summoned == TRUE) {
     reveal_hiding(ch, 0);
     act("@WYou place the dragon balls on the ground and with both hands outstretched towards them you say '@CArise Eternal Dragon Shenron!@W'@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@W$n places the dragon balls on the ground and with both hands outstretched towards them $e says '@CArise Eternal Dragon Shenron!@W'@n", TRUE, ch, 0, 0, TO_ROOM);
     SHENRON = TRUE;
     DRAGONC = 300;
     DRAGONR = GET_ROOM_VNUM(IN_ROOM(ch));
     if (real_room(DRAGONR) == NOWHERE) {
      DRAGONR = GET_ROOM_VNUM(IN_ROOM(ch));
     }
     send_to_imm("Shenron summoned to room: %d\r\n", DRAGONR);

     if ((DRAGONZ = real_zone_by_thing(DRAGONR)) != NOWHERE) {
      DRAGONZ = real_zone_by_thing(DRAGONR);
     }

     for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;

      if (GET_OBJ_VNUM(obj) == dball2[0]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[0] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[1]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[1] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[2]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[2] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[3]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[3] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[4]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[4] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[5]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[5] = -1;
       continue;
      }
      else if (GET_OBJ_VNUM(obj) == dball2[6]) {
       obj_from_char(obj);
       extract_obj(obj);
       dball2[6] = -1;
       continue;
      }
      else {
       continue;
      }
     }
     if ((r_num = real_mobile(21)) == NOBODY) {
      send_to_imm("Shenron doesn't exist!");
      return;
     }
     mob = read_mobile(r_num, REAL);
     char_to_room(mob, 0);
     EDRAGON = mob;
     return;
    }
    else {
     send_to_char(ch, "@wYou do not have all the dragon balls and can not summon the dragon!@n\r\n");
     return;
    }
}

/* This handles transforming Current and Max PL/KI/ST them */
static void handle_transform(struct char_data *ch, int64_t add, double mult, double drain)
{

	if (!ch)
		return;
	if (IS_NPC(ch))
		return;
	else {
		long double cur, max, dapercent = GET_LIFEPERC(ch);
		/* Handle Pl */
		cur = (long double)(GET_HIT(ch));
		max = (long double)(GET_MAX_HIT(ch));
		GET_MAX_HIT(ch) = (GET_BASE_PL(ch) + add) * mult;
		if (android_can(ch) == 1)
			GET_MAX_HIT(ch) += 50000000;
		else if (android_can(ch) == 2)
			GET_MAX_HIT(ch) += 20000000;

		if ((GET_HIT(ch) + (add * (cur / max))) * mult <= gear_pl(ch)) {
			GET_HIT(ch) = (GET_HIT(ch) + (add * (cur / max))) * mult;
		}
		else if ((GET_HIT(ch) + (add * (cur / max))) * mult > gear_pl(ch)) {
			GET_HIT(ch) = gear_pl(ch);
		}

		/* Handle Ki */
		cur = (long double)(GET_MANA(ch));
		max = (long double)(GET_MAX_MANA(ch));
		GET_MAX_MANA(ch) = (GET_BASE_KI(ch) + add) * mult;
		if (android_can(ch) == 1)
			GET_MAX_MANA(ch) += 50000000;
		else if (android_can(ch) == 2)
			GET_MAX_MANA(ch) += 20000000;

		if ((GET_MANA(ch) + (add * (cur / max))) * mult <= GET_MAX_MANA(ch)) {
			GET_MANA(ch) = (GET_MANA(ch) + (add * (cur / max))) * mult;
		}
		else if ((GET_MANA(ch) + (add * (cur / max))) * mult > GET_MAX_MANA(ch)) {
			GET_MANA(ch) = GET_MAX_MANA(ch);
		}

		/* Handle St */
		GET_MOVE(ch) -= GET_MOVE(ch) * drain;
		cur = (long double)(GET_MOVE(ch));
		max = (long double)(GET_MAX_MOVE(ch));
		GET_MAX_MOVE(ch) = (GET_BASE_ST(ch) + add) * mult;
		if (android_can(ch) == 1)
			GET_MAX_MOVE(ch) += 50000000;
		else if (android_can(ch) == 2)
			GET_MAX_MOVE(ch) += 20000000;

		if ((GET_MOVE(ch) + (add * (cur / max))) * mult <= GET_MAX_MOVE(ch)) {
			GET_MOVE(ch) = (GET_MOVE(ch) + (add * (cur / max))) * mult;
		}
		else if ((GET_MOVE(ch) + (add * (cur / max))) * mult > GET_MAX_MOVE(ch)) {
			GET_MOVE(ch) = GET_MAX_MOVE(ch);
		}

		if (!IS_ANDROID(ch)) {
			/* Handle Lifeforce */
			/*R: Tried changing initial GET_LIFEFORCE to GET_LIFEMAX
			I: You are setting lifemax, which is set dynamically by what KI and Stamina's maxes are.
			This is updated all the time, so it isn't going to do anything for more than a fraction
			of a second. You need to adjust GET_LIFEFORCE to be the same percentage of the new max as
			it was the old. To do this you find out the old */
			/* Example: percent = ((long double)(GET_LIFEMAX(ch)) / 100) * (long double)(GET_LIFEFORCE(ch))


			This will give you the percent (in decimal form) of lifemax. Do this prior to the transformation
			change at the start of handle_transformation. Then here at the end you do:
			GET_LIFEFORCE(ch) = GET_LIFEMAX(ch) * percent;
			The percent will remain the same, but based off the new detransformed lifeforce max. Make sure the
			variable, percent, hasn't already been used. Declare it or something else for your code.*/

			GET_LIFEFORCE(ch) = GET_LIFEMAX(ch) * dapercent;
			if (GET_LIFEFORCE(ch) > GET_LIFEMAX(ch)) {
				GET_LIFEFORCE(ch) = GET_LIFEMAX(ch);
			}
		}


	}
}

/* This handles reverting Current and Max PL/KI/ST */
static void handle_revert(struct char_data *ch, uint64_t add, double mult)
{
	if (!ch)
		return;
	if (IS_NPC(ch))
		return;
	else {
		long double convert, dapercent = GET_LIFEPERC(ch);
		convert = ((long double)(GET_HIT(ch)) / (long double)(GET_MAX_HIT(ch)));
		GET_HIT(ch) = (GET_HIT(ch) - ((add * mult) * (convert))) / mult;
		convert = ((long double)(GET_MANA(ch)) / (long double)(GET_MAX_MANA(ch)));
		GET_MANA(ch) = (GET_MANA(ch) - ((add * mult) * (convert))) / mult;
		convert = ((long double)(GET_MOVE(ch)) / (long double)(GET_MAX_MOVE(ch)));
		GET_MOVE(ch) = (GET_MOVE(ch) - ((add * mult) * (convert))) / mult;
		/*R: Trying to add lifeforce revert */
		GET_LIFEFORCE(ch) = GET_LIFEMAX(ch) * dapercent;
		if (GET_LIFEFORCE(ch) > GET_LIFEMAX(ch)) {
			GET_LIFEFORCE(ch) = GET_LIFEMAX(ch);
		}

		if (GET_MOVE(ch) < 1) {
			GET_MOVE(ch) = 1;
		} if (GET_MANA(ch) < 1) {
			GET_MANA(ch) = 1;
		} if (GET_HIT(ch) < 1) {
			GET_HIT(ch) = 1;
		} if (GET_LIFEFORCE(ch) < 1) {
			GET_LIFEFORCE(ch) = 1;
		}
		if (GET_HIT(ch) > gear_pl(ch)) {
			GET_HIT(ch) = gear_pl(ch);
		}

		GET_MAX_HIT(ch) = GET_BASE_PL(ch);
		GET_MAX_MANA(ch) = GET_BASE_KI(ch);
		GET_MAX_MOVE(ch) = GET_BASE_ST(ch);
	}
}

ACMD(do_transform)
{

	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char buf3[MAX_INPUT_LENGTH];

	if (IS_NPC(ch)) {
		return;
	}
	two_arguments(argument, arg, arg2);

	if (PLR_FLAGGED(ch, PLR_OOZARU)) {
		send_to_char(ch, "You are the great Oozaru right now and can't transform!\r\n");
		return;
	}
	if (GET_KAIOKEN(ch) > 0) {
		send_to_char(ch, "You are in kaioken right now and can't transform!\r\n");
		return;
	}
	if (GET_SUPPRESS(ch) > 0) {
		send_to_char(ch, "You are suppressing right now and can't transform!\r\n");
		return;
	}
	if (GET_CLONES(ch) > 0) {
		send_to_char(ch, "You can't concentrate on transforming while your body is split into multiple forms!\r\n");
		return;
	}
	reveal_hiding(ch, 0);
	/*R: Hidden transformation stuff following this*/
	if (GET_BASE_PL(ch) < 50000) {
		send_to_char(ch, "@RYou are too weak to comprehend transforming!@n\r\n");
		return;
	}

	if (!*arg) {
		if (IS_HUMAN(ch)) {
			send_to_char(ch, "              @YSuper @CHuman@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YSuper @CHuman @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YSuper @CHuman @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YSuper @CHuman @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@YSuper @CHuman @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_SAIYAN(ch) && !PLR_FLAGGED(ch, PLR_LSSJ)) {
			send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@YSuper @CSaiyan @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_SAIYAN(ch) && PLR_FLAGGED(ch, PLR_LSSJ)) {
			send_to_char(ch, "                @YSuper @CSaiyan@n\r\n");
			send_to_char(ch, "@b-------------------------------------------------@n\r\n");
			send_to_char(ch, "@YSuper @CSaiyan @WFirst   @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YLegendary @CSuper Saiyan @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@b-------------------------------------------------@n\r\n");
		}
		else if (IS_HALFBREED(ch)) {
			send_to_char(ch, "              @YSuper @CSaiyan@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YSuper @CSaiyan @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YSuper @CSaiyan @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YSuper @CSaiyan @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_NAMEK(ch)) {
			send_to_char(ch, "              @YSuper @CNamek@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YSuper @CNamek @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YSuper @CNamek @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YSuper @CNamek @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@YSuper @CNamek @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_ICER(ch)) {
			send_to_char(ch, "              @YTransform@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YTransform @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YTransform @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YTransform @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@YTransform @WFourth @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_MUTANT(ch)) {
			send_to_char(ch, "              @YMutate@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YMutate @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YMutate @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YMutate @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_KONATSU(ch)) {
			send_to_char(ch, "              @YShadow@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YShadow @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YShadow @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YShadow @WThird  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_ANDROID(ch)) {
			send_to_char(ch, "              @YUpgrade@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@Y1.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@Y2.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@Y3.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@Y4.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@Y5.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 5) * 0.75) ? add_commas(trans_req(ch, 5)) : "??????????");
			send_to_char(ch, "@Y6.0 @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 6) * 0.75) ? add_commas(trans_req(ch, 6)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_BIO(ch)) {
			send_to_char(ch, "              @YPerfection@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YMature        @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YSemi-Perfect  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YPerfect       @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@YSuper Perfect @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 4) * 0.75) ? add_commas(trans_req(ch, 4)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_TRUFFLE(ch)) {
			send_to_char(ch, "              @YAscend@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YAscend @WFirst  @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YAscend @WSecond @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YAscend @WThird @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_MAJIN(ch)) {
			send_to_char(ch, "              @YMorph@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YMorph @WAffinity @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YMorph @WSuper    @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YMorph @WTrue     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else if (IS_KAI(ch)) {
			send_to_char(ch, "              @YMystic@n\r\n");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
			send_to_char(ch, "@YMystic @WFirst     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 1) * 0.75) ? add_commas(trans_req(ch, 1)) : "??????????");
			send_to_char(ch, "@YMystic @WSecond    @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 2) * 0.75) ? add_commas(trans_req(ch, 2)) : "??????????");
			send_to_char(ch, "@YMystic @WThird     @R-@G %s BPL Req\r\n", GET_BASE_PL(ch) >= (trans_req(ch, 3) * 0.75) ? add_commas(trans_req(ch, 3)) : "??????????");
			send_to_char(ch, "@b------------------------------------------------@n\r\n");
		}
		else {
			send_to_char(ch, "You do not have a transformation.\r\n");
			return;
		}
		if (trans_req(ch, 1) > 0) {
			if (GET_TRANSCLASS(ch) == 1) {
				send_to_char(ch, "\r\n@RYou have @rterrible@R transformation BPL Requirements.@n\r\n");
			}
			else if (GET_TRANSCLASS(ch) == 2) {
				send_to_char(ch, "\r\n@CYou have @caverage@C transformation BPL Requirements.@n\r\n");
			}
			else if (GET_TRANSCLASS(ch) == 3) {
				send_to_char(ch, "\r\n@GYou have @gGREAT@G transformation BPL Requirements.@n\r\n");
			}
		}
		return;
	}/* End of No Argument */

	 /* ----------------------------------Transformation Section--------------------------------------*/
	 /* Rillao: transloc, add new transes here */
	else if (!strcasecmp("first", arg)) {
		if (IS_HUMAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou spread your feet out and crouch slightly as a bright white aura bursts around your body. Torrents of white and blue energy burn upwards around your body while your muscles grow and become more defined at the same time. In a sudden rush of power you achieve @CSuper @cHuman @GFirst@W sending surrounding debris high into the sky!", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W crouches slightly while spreading $s feet as a bright white aura bursts up around $s body. Torrents of white and blue energy burn upwards around $s body while $s muscles grow and become more defined at the same time. In a sudden rush of power debris is sent flying high into the air with $m achieving @CSuper @cHuman @GFirst@W!", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				int add = 1000000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Human First Trans */


		if (IS_TRUFFLE(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WYour mind accelerates working through the mysteries of the universe while at the same time your body begins to change! Innate nano-technology within your body begins to activate, forming flexible metal plating across parts of your skin!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W begins to write complicated calculations in the air as though $e were possessed while at the same time $s body begins to change! Innate nano-technology within $s body begins to activate, forming flexible metal plating across parts of $s skin!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 1300000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of TRUFFLE First Trans */

		if (IS_KAI(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WThoughts begin to flow through your mind of events throughout your life. The progression leads up to more recent events and finally to this very moment. All of it's significance overwhelms you momentarily and your motivation and drive increase. As your attention is drawn back to your surroundings, you feel as though your thinking, senses, and reflexes have sharpened dramatically.  At the core of your being, a greater depth of power can be felt.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@W$n@W's face tenses, it becoming clear momentarily that they are deep in thought. After a brief lapse in focus, their attention seems to return to their surroundings. Though it's not apparent why they were so distracted, something definitely seems different about $m.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 1100000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of KAI First Trans */

		if (IS_KONATSU(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WA dark shadowy aura with flecks of white energy begins to burn around your body! Strength and agility can be felt rising up within as your form becomes blurred and ethereal looking. You smile as you realize your @GFirst @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA dark shadowy aura with flecks of white energy begins to burn around @C$n@W's body! $s form becomes blurred and ethereal-looking as $s muscles become strong and lithe. $e smiles as $e achieves $s @GFirst @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 1000000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Konatsu First Trans */

		if (IS_ICER(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou yell with pain as your body begins to grow and power surges within! Your legs expand outward to triple their previous length. Soon after your arms, chest, and head follow. Your horns grow longer and curve upwards while lastly your tail expands. You are left confidently standing, having completed your @GFirst @cTransformation@W.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W yells with pain as $s body begins to grow and power surges outward! $s legs expand outward to triple their previous length. Soon after $s arms, chest, and head follow. $s horns grow longer and curve upwards while lastly $s tail expands. $e is left confidently standing, having completed $s @GFirst @cTransformation@W.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 400000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of ICER First Trans */

		if (IS_MUTANT(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WYour flesh grows tougher as power surges up from within. Your fingernails grow longer, sharper, and more claw-like. Lastly your muscles double in size as you achieve your @GFirst @mMutation@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W flesh grows tougher as power surges up around $m. $s fingernails grow longer, sharper, and more claw-like. Lastly $s muscles double in size as $e achieves $s @GFirst @mMutation@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 100000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MUTANT First Trans */

		if (IS_HALFBREED(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				if (!PLR_FLAGGED(ch, PLR_LSSJ) && !PLR_FLAGGED(ch, PLR_FPSSJ) && rand_number(1, 500) >= 491 && GET_MAX_HIT(ch) > 19200000) {
					send_to_char(ch, "You have mastered the super saiyan first transformation and have achieved Full Power Super Saiyan! You will now no longer use stamina while in this form.\r\n");
					SET_BIT_AR(PLR_FLAGS(ch), PLR_FPSSJ);
				}
				int zone = 0;
				act("@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 900000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of HALFBREED First Trans */

		if (IS_NAMEK(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou crouch down and clench your fists as your muscles begin to bulge! Sweat pours down your body as the ground beneath your feet cracks and warps under the pressure of your rising ki! With a sudden burst that sends debris flying you realize a new plateau in your power, having achieved @CSuper @gNamek @GFirst@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wcrouches down and clenches $s fists as $s muscles begin to bulge! Sweat pours down $s body as the ground beneath $s feet cracks and warps under the pressure of  $s rising ki! With a sudden burst that sends debris flying $e seems to realize a new plateau in $s power, having achieved @CSuper @gNamek @GFirst@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 200000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of NAMEK First Trans */

		if (IS_SAIYAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				if (!PLR_FLAGGED(ch, PLR_LSSJ) && !PLR_FLAGGED(ch, PLR_FPSSJ) && rand_number(1, 500) >= 491 && GET_MAX_HIT(ch) > 19200000) {
					send_to_char(ch, "You have mastered the super saiyan first transformation and have achieved Full Power Super Saiyan! You will now no longer use stamina while in this form.\r\n");
					SET_BIT_AR(PLR_FLAGS(ch), PLR_FPSSJ);
				}
				int zone = 0;
				act("@WSomething inside your mind snaps as your rage spills over! Lightning begins to strike the ground all around you as you feel torrents of power rushing through every fiber of your being. Your hair suddenly turns golden as your eyes change to the color of emeralds. In a final rush of power a golden aura rushes up around your body! You have become a @CSuper @YSaiyan@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W screams in rage as lightning begins to crash all around! $s hair turns golden and $s eyes change to an emerald color as a bright golden aura bursts up around $s body! As $s energy stabilizes $e wears a fierce look upon $s face, having transformed into a @CSuper @YSaiyan@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 800000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .1);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Saiyan First Trans */

		return;
	}/* End of First Trans */

	else if (!strcasecmp("second", arg)) {
		if (IS_HUMAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WSuddenly a bright white aura bursts into existance around your body, you feel the intensity of your hidden potential boil until it can't be contained any longer! Waves of ki shoot out from your aura streaking outwards in many directions. A roar that shakes everything in the surrounding area sounds right as your energy reaches its potential and you achieve @CSuper @cHuman @GSecond@W!", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W is suddenly covered with a bright white aura as $e grits $s teeth, apparently struggling with the power boiling to the surface! Waves of ki shoot out from $s aura, streaking in several directions as a mighty roar shakes everything in the surrounding area. As $s aura calms $e smiles, having achieved @CSuper @cHuman @GSecond@W!", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}


				int add = 12000000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Human Second Trans */

		if (IS_KAI(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou feel a sudden rush of emotion, escalating almost to a loss of control as your thoughts race. Your heart begins to beat fast as memories mix with the raw emotion. A faint blue glow begins to surround you. As your emotions level off, you feel a deeper understanding of the universe as you know it. You visibly calm back down to an almost steely eyed resolve as you assess your surroundings. The blue aura wicks around you for a few moments and then dissipates. Thought it's full impact is not yet clear to you, you are left feeling as though both your power and inner strength have turned into nearly bottomless wells.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@W$n@W's appears to be hit by some sudden pangs of agony, their face contorted in pain.  After a moment a faint blue aura appears around them, glowing brighter as time passes. You can feel something in the pit of your stomach, letting you know that something very significant is changing around you. Before long $n@W's aura fades, leaving a very determined looking person in your presence.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1100000, 3);
				}


				int add = 115000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of KAI Second Trans */

		if (IS_TRUFFLE(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WComplete understanding of every physical thing floods your mind as the nano-technology within you continues to change your body! Your eyes change; becoming glassy, hard, and glowing. Your muscles merge with a nano-fiber strengthening them at the molecular level! Finally your very bones become plated in nano-metals that have yet to be invented naturally!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@.s nano-technology continues to change $s body! $s eyes change; becoming glassy, hard, and glowing. $s muscles merge with a nano-fiber strengthening them at the molecular level! Finally $s very bones become plated in nano-metals that have yet to be invented naturally!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1300000, 3);
				}


				int add = 80000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of TRUFFLE Second Trans */

		if (IS_KONATSU(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WThe shadowy aura surrounding your body burns larger than ever as dark bolts of purple electricity crackles across your skin. Your eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into your aura in an instant as you achieve your @GSecond @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WThe shadowy aura surrounding @C$n@W's body burns larger than ever as dark bolts of purple electricity crackles across $s skin. $s eyes begin to glow white as shockwaves of power explode outward! All the shadows in the immediate area are absorbed into $s aura in an instant as $e achieves $s @GSecond @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}


				int add = 56000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Konatsu Second Trans */

		if (IS_MUTANT(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WSpikes grow out from your elbows as your power begins to climb to new heights. The muscles along your forearms grow to double their former size as the spikes growing from your elbows flatten and sharpen into blades. You have achieved your @GSecond @mMutation@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WSpikes grow out from @C$n@W's elbows as $s power begins to climb to new heights. The muscles along $s forearms grow to double their former size as the spikes growing from $s elbows flatten and sharpen into blades. $e has achieved your @GSecond @mMutation@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 100000, 2);
				}


				int add = 8500000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MUTANT Second Trans */

		if (IS_ICER(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou yell as pain envelopes your body and a dark aura bursts up! The back of your head stretches out growing multiple spikes along its edges as it grows. In addition to this your shoulders stretch out forming pointed ridges. You cackle as lastly your nose disappears and your face becomes more lizard like. Energy swirls around your body as you realize your @GSecond @cTransformation@W.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W yells as pain envelopes $s body and a dark aura bursts up around $m! The back of $s head stretches out growing multiple spikes along its edges as it grows. In addition to this $s shoulders stretch out forming pointed ridges. $e cackles as lastly $s nose disappears and $s face becomes more lizard like. Energy swirls around $s body as $e realizes $s @GSecond @cTransformation@W.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 400000, 2);
				}


				int add = 7000000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of ICER Second Trans */

		if (IS_HALFBREED(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 900000, 2);
				}


				int add = 16500000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of HALFBREED Second Trans */

		if (IS_NAMEK(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou gasp in shock as a power within your body that you had not been aware of begins to surge to the surface! Your muscles grow larger as energy crackles between your antennae intensely! A shockwave of energy explodes outward as you achieve a new plateau in power, @CSuper @gNamek @GSecond@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wgasps in shock as a power within $s body begins to surge out! $s muscles grow larger as energy crackles between $s antennae intensely! A shockwave of energy explodes outward as $e achieves a new plateau in power, @CSuper @gNamek @GSecond@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 200000, 2);
				}


				int add = 4000000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of NAMEK Second Trans */

		if (IS_SAIYAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.2) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				if (!PLR_FLAGGED(ch, PLR_LSSJ)) {
					act("@WBlinding rage burns through your mind as a sudden eruption of energy surges forth! A golden aura bursts up around your body, glowing as bright as the sun. Rushing winds rocket out from your body in every direction as bolts of electricity begin to crackle in your aura. As your aura dims you are left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n", TRUE, ch, 0, 0, TO_CHAR);
					act("@C$n@W stands up straight with $s head back as $e releases an ear piercing scream! A blindingly bright golden aura bursts up around $s body, glowing as bright as the sun. As rushing winds begin to rocket out from $m in every direction, bolts of electricity flash and crackle in $s aura. As $s aura begins to dim $e is left standing confidently, having achieved @CSuper @YSaiyan @GSecond@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				}
				else {
					act("@WYou roar and then stand at your full height. You flex every muscle in your body as you feel your strength grow! Your eyes begin to glow @wwhite@W with energy, your hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around your body! You release your @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n", TRUE, ch, 0, 0, TO_CHAR);
					act("@C$n @Wroars and then stands at $s full height. Then $s muscles start to buldge and grow as $e flexes them! Suddenly $s eyes begin to glow @wwhite@W with energy, $s hair turns @Ygold@W, and at the same time a @wbright @Yg@yo@Yl@yd@Ye@yn@W aura flashes up around $s body! @C$n@W releases $s @YL@ye@Dg@We@wn@Yd@ya@Dr@Yy@W power upon the universe!@n", TRUE, ch, 0, 0, TO_ROOM);
				}
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				int add = 0;
				double mult = 0;

				if (!PLR_FLAGGED(ch, PLR_LSSJ)) {
					add = 20000000;
					mult = 3;
				}
				else {
					add = 185000000;
					mult = 6;
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 800000, 2);
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Saiyan Second Trans */


		return;
	}/* End of Second Trans */

	else if (!strcasecmp("third", arg)) {
		if (IS_HUMAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou clench both of your fists as the bright white aura around your body is absorbed back into your flesh. As it is absorbed, your muscles triple in size and electricity crackles across your flesh. You grin as you feel the power of @CSuper @cHuman @GThird@W!", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W clenches both of $s fists as the bright white aura around $s body is absorbed back into $s flesh. As it is absorbed, $s muscles triple in size and bright electricity crackles across $s flesh. $e smiles as $e achieves the power of @CSuper @cHuman @GThird@W!", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 12000000, 3);
				}


				int add = 50000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Human Third Trans */

		if (IS_TRUFFLE(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou have reached the final stage of enlightenment and the nano-technology thriving inside you begin to initiate the changes! Your neural pathways become refined, your reflexes honed, your auditory and ocular senses sharpening far beyond normal levels! Your gravitational awareness improves, increasing sensitivity and accuracy in your equilibrum!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n begins to mumble quietly, slowly at first and gradually picking up speed. A glint is seen from $s eyes and $s arms reach outwards briefly as $e appears to catch his balance. $s arms drop back to $s sides as balance is regained, a vicious smile on $s face.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 80000000, 4);
				}


				int add = 300000000;
				double mult = 5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of TRUFFLE Third Trans */

		if (IS_KAI(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WYour minds' eye becomes overwhelmed by secrets unimaginable. The threads of the very universe become visible in your heightened state of awareness. Reaching out, a single thread vibrates, producing a @Rred @Wcolor -- yours. Your fingertips brush against it and your senses become clouded by a vast expanse of white color and noise. As your vision and hearing return, you understand the threads tying every living being together. Your awareness has expanded beyond comprehension!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W's eyes grow wide, mouth agape. $s body begins to shiver uncontrollably! $s arms reaches out cautiously before falling back down to $s side. $s face relaxes visibly, features returning to a normal state. $s irises remain larger than before, a slight smile softening $s gaze.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1100000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 115000000, 4);
				}


				int add = 270000000;
				double mult = 5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of KAI Third Trans */

		if (IS_MUTANT(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WA dark cyan aura bursts up around your body as the ground begins to crack beneath you! You scream out in pain as your power begins to explode! Two large spikes grow out from your shoulder blades as you reach your @GThird @mMutation!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA dark cyan aura bursts up around @C$n@W's body as the ground begins to crack beneath $m and $e screams out in pain as $s power begins to explode! Two large spikes grow out from $s shoulder blades as $e reaches $s @GThird @mMutation!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 100000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 8500000, 3);
				}


				int add = 80000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MUTANT Third Trans */

		if (IS_KONATSU(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WThe shadowy aura around you explodes outward as your power begins to rise!  You're overcome with a sudden realization, that the shadows are an extension of yourself, that light isn't needed for your shadows to bloom.  With this newfound wisdom comes ability and power!  The color in your aura drains as the shadows slide inward and cling to your body like a second, solid black skin!  Shockwaves roll off of you in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath you, slowly spreading the shadows to the whole area, projecting onto any surface nearby!  Purple and black electricity crackle in your solid white aura, and you grin as you realize your @GThird @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WThe shadowy aura around $n explodes outward as $s power begins to rise!  Realization dawns on $s face, followed shortly by confidence! The color in $s aura drains as the shadows slide inward to cling to $s body like a second, solid black skin! Shockwaves roll off of $n in quick succession, pelting the surrounding area harshly!  Accompanying the waves, a pool of darkness blossoms underneath them, slowly spreading the shadows to the whole area, projecting onto any surface nearby! Purple and black electricity crackle in $s solid white aura, and he grins as $e realizes $s @GThird @DShadow @BForm@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 56000000, 4);
				}


				int add = 290000000;
				double mult = 5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Konatsu Third Trans */

		if (IS_ICER(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WA blinding light surrounds your body while your rising power begins to rip up the ground beneath you! Your skin and torso shell begin to crack as your new body struggles to free its self. Huge chunks of debris lift free of the ground as your power begins to rise to unbelievable heights. Suddenly your old skin and torso shell burst off from your body, leaving a sleek form glowing where they had been. Everything comes crashing down as your power evens out, leaving you with your @GThird @cTransformation @Wcompleted!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA blinding light surrounds @C$n@W's body while $s rising power begins to rip up the ground beneath $m! $s skin and torso shell begin to crack as $s new body struggles to free its self. Huge chunks of debris lift free of the ground as $s power begins to rise to unbelievable heights. Suddenly $s old skin and torso shell burst off from $s body, leaving a sleek form glowing where they had been. Everything comes crashing down as @C$n@W's power evens out, leaving $m with $s @GThird @cTransformation @Wcompleted!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 400000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 7000000, 3);
				}


				int add = 45000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of ICER Third Trans */

		if (IS_HALFBREED(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 900000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 16500000, 4);
				}


				int add = 240000000;
				double mult = 5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of HALFBREED Third Trans */

		if (IS_NAMEK(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WA fierce clear aura bursts up around your body as you struggle to control a growing power within! Energy leaks off of your aura at an astounding rate filling the air around you with small orbs of ki. As your power begins to level off the ambient ki hovering around you is absorbed inward in a sudden shock that leaves your skin glowing! You have achieved a rare power, @CSuper @gNamek @GThird@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA fierce clear aura bursts up around @C$n@W's body as $e struggles to control $s own power! Energy leaks off of $s aura at an astounding rate filling the air around $m with small orbs of ki. As $s power begins to level off the ambient ki hovering around $m is absorbed inward in a sudden shock that leaves $s skin glowing! $e has achieved a rare power, @CSuper @gNamek @GThird@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 200000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 4000000, 3);
				}


				int add = 65000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of NAMEK Third Trans */

		if (IS_SAIYAN(ch) && !PLR_FLAGGED(ch, PLR_LSSJ)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.1) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WElectricity begins to crackle around your body as your aura grows explosively! You yell as your powerlevel begins to skyrocket while your hair grows to multiple times the length it was previously. Your muscles become incredibly dense instead of growing in size, preserving your speed. Finally your irises appear just as your transformation becomes complete, having achieved @CSuper @YSaiyan @GThird@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WElectricity begins to crackle around @C$n@W, as $s aura grows explosively! $e yells as the energy around $m skyrockets and $s hair grows to multiple times its previous length. $e smiles as $s irises appear and $s muscles tighten up. $s transformation complete, $e now stands confidently, having achieved @CSuper @YSaiyan @GThird@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				int add = 80000000;
				double mult = 4;

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 800000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 20000000, 3);
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Saiyan Third Trans */

		return;
	}/* End of Third Trans */

	else if (!strcasecmp("fourth", arg)) {
		if (IS_HUMAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				int zone = 0;
				act("@WYou grit your teeth and clench your fists as a sudden surge of power begins to tear through your body! Your muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over you, flashes of multicolored light flaring up in rising stars around your new form, a corona of glory! You feel your ultimate potential realized as you ascend to @CSuper @cHuman @GFourth@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W grits $s teeth and clenches $s fists as a sudden surge of power begins to tear through $s body! $n@W's muscles lose volume and gain mass, condensing into sleek hyper efficiency as a spectacular shimmering white aura flows over $m, flashes of multicolored light flare up in rising stars around $s new form, a corona of glory! $n@W smiles as his ultimate potential is realized as $e ascends to @CSuper @cHuman @GFourth@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 12000000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 50000000, 4);
				}


				int add = 270000000;
				double mult = 4.5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Human Fourth Trans */

		if (IS_ICER(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				int zone = 0;
				act("@WA feeling of complete power courses through your viens as your body begins to change radically! You triple in height while a hard shell forms over your entire torso. Hard bones grow out from your head forming four ridges that jut outward. A hard covering grows up over your mouth and nose completing the transformation! A dark crimson aura flames around your body as you realize your @GFourth @cTransformation@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W's body begins to change radically! $e triples in height while a hard shell forms over $s entire torso. Hard bones grow out from $s head forming four ridges that jut outward. A hard covering grows up over $s mouth and nose completing the transformation! A dark crimson aura flames around @C$n@W's body as $e realizes $s @GFourth @cTransformation@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 400000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 7000000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 45000000, 4);
				}


				int add = 200000000;
				double mult = 5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of ICER Fourth Trans */

		if (IS_NAMEK(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				int zone = 0;
				act("@WAn inner calm fills your mind as your power surges higher than ever before. Complete clarity puts everything once questioned into perspective. While this inner calm is filling your mind, an outer storm of energy erupts around your body! The storm of energy boils and crackles while growing larger. You have achieved @CSuper @gNamek @GFourth@W, a mystery of the ages.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W smiles calmly as a look of complete understand fills $s eyes. While $e remains perfectly calm and detached a massivly powerful storm of energy erupts from his body. This storm of energy shimmers with the colors of the rainbow and boils and crackles with awesome power! $s smile disappears as he realizes a mysterious power of the ages, @CSuper @gNamek @GFourth@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 200000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 4000000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 65000000, 4);
				}


				int add = 230000000;
				double mult = 4.5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of NAMEK Fourth Trans */

		if (IS_SAIYAN(ch) && !PLR_FLAGGED(ch, PLR_LSSJ)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that form! Try 'revert'.\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				int zone = 0;
				act("@WHaving absorbed enough blutz waves, your body begins to transform! Red fur grows over certain parts of your skin as your hair grows longer and unkempt. A red outline forms around your eyes while the irises of those very same eyes change to an amber color. Energy crackles about your body violently as you achieve the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WHaving absorbed enough blutz waves, @C$n@W's body begins to transform! Red fur grows over certain parts of $s skin as $s hair grows longer and unkempt. A red outline forms around $s eyes while the irises of those very same eyes change to an amber color. Energy crackles about $s body violently as $e achieves the peak of saiyan perfection, @CSuper @YSaiyan @GFourth@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 800000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 20000000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 80000000, 4);
				}

				int add = 182000000;
				double mult = 5.5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, .2);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of Saiyan Fourth Trans */
		return;
	}/* End of Fourth Trans */
	else if (!strcasecmp("mature", arg)) {
		if (IS_BIO(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (GET_ABSORBS(ch) > 2) {
				send_to_char(ch, "You need to absorb something to transform!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@gYou bend over as @rpain@g wracks your body! Your limbs begin to grow out, becoming more defined and muscular. As your limbs finish growing outward you feel a painful sensation coming from your back as a long tail with a spike grows out of your back! As the pain subsides you stand up straight and a current of power shatters part of the ground beneath you. You have @rmatured@g beyond your @Gl@ga@Dr@gv@Ga@ge stage!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@W$n @gbends over as a @rpainful@g look covers $s face! $s limbs begin to grow out, becoming more defined and muscular. As $s limbs finish growing outward $e screams as a long tail with a spike grows rips out of $s back! As $e calms $e stands up straight and a current of power shatters part of the ground beneath $m. $e has @rmatured@g beyond $s @Gl@ga@Dr@gv@Ga@ge stage!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);


				int add = 1000000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				return;
			}
		} /* End of Bio First Trans */
		return;
	}/* End of Bio First Trans */

	else if (!strcasecmp("semi-perfect", arg) || !strcasecmp("Semi-Perfect", arg)) {
		if (IS_BIO(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_ABSORBS(ch) > 1) {
				send_to_char(ch, "You need to absorb something to transform!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WYour exoskeleton begins to glow spectacularly while the shape of your body begins to change. Your tail shrinks slightly. Your hands, feet, and facial features become more refined. While your body colors change slightly. The crests on your head change, standing up straighter on either side of your head as well. As you finish transforming a wave of power floods your being. You have achieved your @gSemi@D-@GPerfect @BForm@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W's exoskeleton begins to glow spectacularly while the shape of $s body begins to change. $s tail shrinks slightly. $s hands, feet, and facial features become more refined. While $s body colors change slightly. The crests on $s head change, standing up straighter on either side of $s head as well. As $e finishes transforming a wave of power rushes out from $m. $e has achieved $s @gSemi@D-@GPerfect @BForm@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}


				int add = 8000000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of BIO Second Trans */
	} /* End of Bio Second Trans */

	else if (!strcasecmp("perfect", arg)) {
		if (IS_BIO(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_ABSORBS(ch) > 0) {
				send_to_char(ch, "You need to absorb something to transform!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_MOVE(ch) <= (GET_MAX_MOVE(ch) * 0.2)) {
				send_to_char(ch, "You do not have enough stamina!");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WYour whole body is engulfed in blinding light as your exoskeleton begins to change shape! Your hands, feet, and facial features become more refined and humanoid. While your colors change, becoming more subdued and neutral. A bright golden aura bursts up around your body as you achieve your @GPerfect @BForm@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n@W whole body is engulfed in blinding light as $s exoskeleton begins to change shape! $s hands, feet, and facial features become more refined and humanoid. While $s colors change, becoming more subdued and neutral. A bright golden aura bursts up around $s body as $e achieves $s @GPerfect @BForm@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 8000000, 3);
				}


				int add = 70000000;
				double mult = 3.5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of BIO Third Trans */
	} /* End of Bio Third Trans */

	else if ((!strcasecmp("super", arg) || !strcasecmp("super perfect", arg)) && IS_BIO(ch)) {
		if (IS_BIO(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				int zone = 0;
				act("@WA rush of power explodes from your perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout your aura intensely while your muscles grow slightly larger but incredibly dense. You smile as you realize that you have taken your perfect form beyond imagination. You are now @CSuper @GPerfect@W!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA rush of power explodes from @C$n@W's perfect body, crushing nearby debris and sending dust billowing in all directions. Electricity crackles throughout $s aura intensely while $s muscles grow slightly larger but incredibly dense. $e smiles as $e has taken $s perfect form beyond imagination. $e is now @CSuper @GPerfect@W!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1000000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 8000000, 3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 70000000, 3.5);
				}


				int add = 400000000;
				double mult = 4;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of BIO Fourth Trans */
	} /* End of Bio Fourth Trans */

	else if (!strcasecmp("affinity", arg)) {
		if (IS_MAJIN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				int zone = 0;
				act("@WA dark pink aura bursts up around your body as images of good and evil fill your mind! You feel the power within your body growing intensely, reflecting your personal alignment as your body changes!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WA dark pink aura bursts up around @C$n@W's body as images of good and evil fill $s mind! $e feels the power within $s body growing intensely, reflecting $s personal alignment as $s body changes!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}


				int add = 1250000;
				double mult = 2;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MAJIN First Trans */

	} /* Majin First trans block */
	else if (!strcasecmp("super", arg)) {
		if (IS_MAJIN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			if (GET_ABSORBS(ch) > 0 && GET_LEVEL(ch) < 50) {
				send_to_char(ch, "You need to ingest someone before you can use that form.\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				int zone = 0;
				act("@WAn intense pink aura surrounds your body as it begins to change, taking on the characteristics of those you have ingested! Explosions of pink energy burst into existence all around you as your power soars to sights unseen!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WAn intense pink aura surrounds @C$n@W's body as it begins to change, taking on the characteristics of those $e has ingested! Explosions of pink energy burst into existence all around $m as $s power soars to sights unseen!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1250000, 2);
				}


				int add = 15000000;
				double mult = 3;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MAJIN Second Trans */

	} /* Majin Second trans block */

	else if (!strcasecmp("true", arg)) {

		if (IS_MAJIN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that form!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that form!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that transformation!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				int zone = 0;
				act("@WRipples of intense pink energy rush upwards around your body as it begins to morph into its truest form! The ground beneath your feet forms into a crater from the very pressure of your rising ki! Earthquakes shudder throughout the area as your finish morphing!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@WRipples of intense pink energy rush upwards around @C$n@W's body as it begins to morph into its truest form! The ground beneath $s feet forms into a crater from the very pressure of $s rising ki! Earthquakes shudder throughout the area as $e finishes morphing!@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((zone = real_zone_by_thing(IN_ROOM(ch))) != NOWHERE) {
					send_to_zone("An explosion of power ripples through the surrounding area!\r\n", zone);
				}

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 1250000, 2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					/* handle revert, Ch, add, mult */
					handle_revert(ch, 15000000, 3);
				}


				int add = 340000000;
				double mult = 4.5;

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				send_to_sense(0, "You sense a nearby power grow unbelievably!", ch);
				sprintf(buf3, "@D[@GBlip@D]@r Transformed Powerlevel@D: [@Y%s@D]", add_commas(GET_HIT(ch)));
				send_to_scouter(buf3, ch, 1, 0);
				return;
			}
		} /* End of MAJIN Third Trans */

	} /* Majin Second trans block */

	else if (!strcasecmp("1.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS2) || PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that upgrade!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS1)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 1)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 1)) {
				if (GET_TRANSCOST(ch, 1) == FALSE) {
					if (GET_PRACTICES(ch, GET_CLASS(ch)) < 50) {
						send_to_char(ch, "You need 50 practice points in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_PRACTICES(ch, GET_CLASS(ch)) -= 50;
						GET_TRANSCOST(ch, 1) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				int add = 5000000u;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 7500000u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				return;
			}
		} /* End of ANDROID First Trans */

	}
	else if (!strcasecmp("2.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3) || PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that upgrade!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 2)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 2)) {
				if (GET_TRANSCOST(ch, 2) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 2) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				}

				int add = 20000000u;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 30000000u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				return;
			}
		} /* End of ANDROID Second Trans */

	}
	else if (!strcasecmp("3.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already beyond that upgrade!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (!PLR_FLAGGED(ch, PLR_TRANS2)) {
				send_to_char(ch, "You havn't upgraded to 2.0 yet!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 3)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 3)) {
				if (GET_TRANSCOST(ch, 3) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 3) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				}

				int add = 125000000u;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 187500000u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				return;
			}
		} /* End of ANDROID Third Trans */

	}
	else if (!strcasecmp("4.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS5) || PLR_FLAGGED(ch, PLR_TRANS6)) {
				send_to_char(ch, "You are already beyond that upgrade!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (!PLR_FLAGGED(ch, PLR_TRANS3)) {
				send_to_char(ch, "You havn't upgraded to 3.0 yet!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 4)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 4)) {
				if (GET_TRANSCOST(ch, 4) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 4) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				}

				uint64_t add = 1000000000u;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 1500000000u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				return;
			}
		} /* End of ANDROID Fourth Trans */

	}
	else if (!strcasecmp("5.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS6)) {
				send_to_char(ch, "You are already beyond that upgrade!\r\n");
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS5)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (!PLR_FLAGGED(ch, PLR_TRANS4)) {
				send_to_char(ch, "You havn't upgraded to 4.0 yet!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 5)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 5)) {
				if (GET_TRANSCOST(ch, 5) == FALSE) {
					if (GET_RP(ch) < 1) {
						send_to_char(ch, "You need 1 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 1;
						GET_TRANSCOST(ch, 5) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS4)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				}

				uint64_t add = 25000000000u;
				//unsigned long long int add = 2500000000000;
				//add *= 100;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 3750000000u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS5);
				return;
			}
		} /* End of ANDROID Fifth Trans */

	}
	else if (!strcasecmp("6.0", arg)) {
		if (IS_ANDROID(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS6)) {
				send_to_char(ch, "You are already in that upgrade!\r\n");
				return;
			}
			if (!PLR_FLAGGED(ch, PLR_TRANS5)) {
				send_to_char(ch, "You havn't upgraded to 5.0 yet!\r\n");
				return;
			}
			if (GET_BASE_PL(ch) < trans_req(ch, 6)) {
				send_to_char(ch, "You are not strong enough to handle that upgrade!\r\n");
				return;
			}
			else if (GET_BASE_PL(ch) >= trans_req(ch, 6)) {
				if (GET_TRANSCOST(ch, 6) == FALSE) {
					if (GET_RP(ch) < 2) {
						send_to_char(ch, "You need 2 RPP in order to obtain a transformation for the first time.\r\n");
						return;
					}
					else {
						GET_RP(ch) -= 2;
						GET_TRANSCOST(ch, 6) = TRUE;
					}
				}
				act("@WYou stop for a moment as the nano-machines within your body reprogram and restructure you. You are now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@C$n @Wstops for a moment as the nano-machines within $s body reprogram and restructure $m. $e is now more powerful and efficient!@n", TRUE, ch, 0, 0, TO_ROOM);

				if (PLR_FLAGGED(ch, PLR_TRANS1)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS2)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS3)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS4)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				}
				if (PLR_FLAGGED(ch, PLR_TRANS5)) {
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS5);
				}

				uint64_t add = 1000000000u;
				add += 1000000000u;
				add += 1000000000u;
				add += 1000000000u;
				add += 1000000000u;
				double mult = 1;

				if (PLR_FLAGGED(ch, PLR_SENSEM)) {
					add += 750000000u;
					add *= 100u;
				}

				/* handle_transform: ch, add, mult, drain*/
				handle_transform(ch, add, mult, 0.0);

				SET_BIT_AR(PLR_FLAGS(ch), PLR_TRANS6);
				return;
			}
		} /* End of ANDROID Sixth Trans */
	}

	/* ----------------------------------Revert Section--------------------------------------*/
	/* Rillao: transloc, add new transes here */
	else if (!strcasecmp("revert", arg)) {
		if (IS_HUMAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {

				act("@wYou revert from @CSuper @cHuman @GFourth@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cHuman @GFourth@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 270000000, 4.5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CSuper @cHuman @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cHuman @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 50000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CSuper @cHuman @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cHuman @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 12000000, 3);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CSuper @cHuman @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cHuman @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 1000000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}
		else if (IS_SAIYAN(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {

				act("@wYou revert from @CSuper @cSaiyan @GFourth@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GFourth@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 182000000, 5.5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CSuper @cSaiyan @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 80000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				if (!PLR_FLAGGED(ch, PLR_LSSJ)) {
					act("@wYou revert from @CSuper @cSaiyan @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
					act("@w$n@w reverts from @CSuper @cSaiyan @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
					if ((GET_CHARGE(ch) > 0)) {
						do_charge(ch, "release", 0, 0);
					}
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

					/* handle revert, Ch, add, mult */
					handle_revert(ch, 20000000, 3);
				}
				else {
					act("@wYou revert from your @YLegendary @CSuper Saiyan@w form.@n", TRUE, ch, 0, 0, TO_CHAR);
					act("@w$n@w reverts from $s @YLegendary @CSuper Saiyan@w form@w.@n", TRUE, ch, 0, 0, TO_ROOM);
					if ((GET_CHARGE(ch) > 0)) {
						do_charge(ch, "release", 0, 0);
					}
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
					REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

					/* handle revert, Ch, add, mult */
					handle_revert(ch, 185000000, 6);
				}

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CSuper @cSaiyan @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 800000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}
		else if (IS_HALFBREED(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CSuper @cSaiyan @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 240000000, 5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CSuper @cSaiyan @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 16500000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CSuper @cSaiyan @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cSaiyan @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 900000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else if (IS_NAMEK(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {

				act("@wYou revert from @CSuper @cNamek @GFourth@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cNamek @GFourth@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 230000000, 4.5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CSuper @cNamek @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cNamek @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 65000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CSuper @cNamek @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cNamek @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 4000000, 3);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CSuper @cNamek @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CSuper @cNamek @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 200000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else if (IS_ICER(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS4)) {

				act("@wYou revert from @CTransform @GFourth@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CTransform @GFourth@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS4);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 200000000, 5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CTransform @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CTransform @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 45000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CTransform @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CTransform @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 7000000, 3);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CTransform @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CTransform @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 400000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else if (IS_MUTANT(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CMutate @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMutate @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 80000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CMutate @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMutate @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 8500000, 3);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CMutate @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMutate @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 100000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else if (IS_KONATSU(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CShadow @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CShadow @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 290000000, 5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CShadow @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CShadow @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 56000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CShadow @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CShadow @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 1000000, 2);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else if (IS_KAI(ch)) {
			if (PLR_FLAGGED(ch, PLR_TRANS3)) {

				act("@wYou revert from @CMystic @GThird@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMystic @GThird@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS3);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 270000000, 5);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			if (PLR_FLAGGED(ch, PLR_TRANS2)) {

				act("@wYou revert from @CMystic @GSecond@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMystic @GSecond@w.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS2);
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 115000000, 4);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else if (PLR_FLAGGED(ch, PLR_TRANS1)) {

				act("@wYou revert from @CMystic @GFirst@w.@n", TRUE, ch, 0, 0, TO_CHAR);
				act("@w$n@w reverts from @CMystic @GFirst.@n", TRUE, ch, 0, 0, TO_ROOM);
				if ((GET_CHARGE(ch) > 0)) {
					do_charge(ch, "release", 0, 0);
				}
				REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_TRANS1);

				/* handle revert, Ch, add, mult */
				handle_revert(ch, 1100000, 3);

				if (*arg2) {
					do_transform(ch, arg2, 0, 0);
				}
				return;
			}
			else {
				send_to_char(ch, "You do not need to revert from any form!\r\n");
				return;
			}
		}

		else {
			send_to_char(ch, "You do not need to revert from any form!\r\n");
		}
		return;
	}/* End revert segment */
	else if (IS_KANASSAN(ch) || IS_DEMON(ch)) {
		send_to_char(ch, "You do not have a transformation.\r\n");
		return;
	}
	else {
		send_to_char(ch, "What form?\r\n");
		return;
	} /* Fail message */
}/*End of Function */

ACMD(do_situp)
{

 int cost = 1, bonus = 0;

 if (IS_NPC(ch)) {
   send_to_char(ch, "You are a mob fool!\r\n");
   return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HELL)) {
    send_to_char(ch, "The fire makes it too hot!\r\n");
    return;
  }
 if (DRAGGING(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }
 if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

 if ((ROOM_GRAVITY(IN_ROOM(ch)) != 10 && IS_BARDOCK(ch)) || !IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * ((ROOM_GRAVITY(IN_ROOM(ch)) * 2) + GET_LEVEL(ch));
 } else if (IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * GET_LEVEL(ch);
 }

 if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
  cost += 1000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
  cost += 2000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
  cost += 7500000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
  cost += 15000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
  cost += 25000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
  cost += 50000000;
 }

 if (cost == 1 || cost == 0) {
  cost = 25;
 }
 if (GET_BONUS(ch, BONUS_HARDWORKER) > 0) {
  cost -= cost * .25;
 } else if (GET_BONUS(ch, BONUS_SLACKER) > 0) {
  cost += cost * .25;
 }

  if (GET_RELAXCOUNT(ch) >= 464) {
   cost *= 50;
  } else if (GET_RELAXCOUNT(ch) >= 232) {
   cost *= 15;
  } else if (GET_RELAXCOUNT(ch) >= 116) {
   cost *= 4;
  }

 if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch)) {
   send_to_char(ch, "You will gain nothing from exercising!\r\n");
   return;
 }

  if (!limb_ok(ch, 1)) {
   return;
  }

 if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You are too tired!\r\n");
   return;
  }
 if (PLR_FLAGGED(ch, PLR_SPAR)) {
   send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
   return;
 }
 if (FIGHTING(ch)) {
   send_to_char(ch, "You are fighting you moron!\r\n");
   return;
 }
 if (AFF_FLAGGED(ch, AFF_FLYING)) {
   send_to_char(ch, "You can't do situps in midair!\r\n");
   return;
 }
 else {
   if (ROOM_GRAVITY(IN_ROOM(ch)) == 0) {
    bonus = 1;
    act("@gYou do a situp.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10) {
    bonus = rand_number(3, 7);
    act("@gYou do a situp, feeling the strain of gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 20) {
    bonus = rand_number(8, 14);
    act("@gYou do a situp, and feel gravity's pull.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 30) {
    bonus = rand_number(14, 20);
    act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 40) {
    bonus = rand_number(20, 35);
    act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 50) {
    bonus = rand_number(40, 60);
    act("@gYou do a situp, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 100) {
    bonus = rand_number(180, 250);
    act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 200) {
    bonus = rand_number(400, 600);
    act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
    bonus = rand_number(800, 1200);
    act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
    bonus = rand_number(2000, 3000);
    act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
    bonus = rand_number(4000, 6000);
    act("@gYou do a situp, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
    bonus = rand_number(9000, 10000);
    act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
    bonus = rand_number(15000, 20000);
    act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
    bonus = rand_number(25000, 30000);
    act("@gYou do a situp, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a situp, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   if (cost >= GET_MAX_MOVE(ch) / 2) {
    send_to_char(ch, "This gravity is a great challenge for you!\r\n");
    bonus *= 10;
   }
   else if (cost >= GET_MAX_MOVE(ch) / 4) {
    send_to_char(ch, "This gravity is an awesome challenge for you!\r\n");
    bonus *= 8;
   }
   else if (cost >= GET_MAX_MOVE(ch) / 10) {
    send_to_char(ch, "This gravity is a good challenge for you!\r\n");
    bonus *= 6;
   }
   else if (cost < GET_MAX_MOVE(ch) / 1000) {
    send_to_char(ch, "This gravity is so easy to you, you could do it in your sleep...\r\n");
    bonus /= 8;
   }
   else if (cost < GET_MAX_MOVE(ch) / 100) {
    send_to_char(ch, "This gravity is the opposite of a challenge for you...\r\n");
    bonus /= 5;
   }
   else if (cost < GET_MAX_MOVE(ch) / 50) {
    send_to_char(ch, "This gravity is definitely not a challenge for you...\r\n");
    bonus /= 4;
   }
   else if (cost < GET_MAX_MOVE(ch) / 30) {
    send_to_char(ch, "This gravity is barely a challenge for you...\r\n");
    bonus /= 3;
   }
   else if (cost < GET_MAX_MOVE(ch) / 20) {
    send_to_char(ch, "This gravity is hardly a challenge for you...\r\n");
    bonus /= 2;
   }
   else {
    send_to_char(ch, "This gravity is just perfect for you...\r\n");
    bonus *= 4;
   }
   if (!soft_cap(ch, 2)) {
    bonus = 0;
   }
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
    bonus *= 10;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus *= 10;
    } else {
     bonus *= 5;
    }
   } else if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
    send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
    bonus *= 20;
   }
   if (bonus <= 0 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 1;
   }
   if (bonus <= 0 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 15;
   }
   if (bonus <= 0 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus = 12;
    } else {
     bonus = 6;
    }
   }
   /* Rillao: transloc, add new transes here */
    bonus += GET_LEVEL(ch) / 20;
    if (IS_NAMEK(ch)) {
    bonus -= bonus / 4;
    }
    if (GET_BONUS(ch, BONUS_HARDWORKER)) {
     bonus += bonus * 0.5;
    }
    if (GET_BONUS(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    send_to_char(ch, "You feel slightly more vigorous @D[@G+%s@D]@n.\r\n", add_commas(bonus));
     if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
      GET_MAX_MOVE(ch) += bonus * 3;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
      GET_MAX_MOVE(ch) += bonus * 4;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
      GET_MAX_MOVE(ch) += bonus * 5;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 1) {
      GET_MAX_MOVE(ch) += bonus * 2;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 2) {
      GET_MAX_MOVE(ch) += bonus * 3;
     }
     else {
      GET_MAX_MOVE(ch) += bonus;
     }
     GET_BASE_ST(ch) += bonus;
   if (ROOM_GRAVITY(IN_ROOM(ch)) <= 50) {
   WAIT_STATE(ch, PULSE_2SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 200) {
   WAIT_STATE(ch, PULSE_3SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 500) {
   WAIT_STATE(ch, PULSE_4SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 1000) {
   WAIT_STATE(ch, PULSE_5SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 5000) {
   WAIT_STATE(ch, PULSE_6SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 10000) {
   WAIT_STATE(ch, PULSE_7SEC);
   }
   GET_MOVE(ch) -= cost;
   if (GET_MOVE(ch) < 0) {
    GET_MOVE(ch) = 0;
   }
 }
}

ACMD(do_meditate)
{
  
  int64_t bonus = 0, cost = 1, weight = 0;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);


  if (IS_NPC(ch)) {
   send_to_char(ch, "You are a mob fool!\r\n");
   return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HELL)) {
    send_to_char(ch, "The fire makes it too hot!\r\n");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }

 if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch)) {
   send_to_char(ch, "You will gain nothing from exercising!\r\n");
   return;
 }

 if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

 if (DRAGGING(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }

 if (PLR_FLAGGED(ch, PLR_SPAR)) {
   send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
   return;
 }

 if (FIGHTING(ch)) {
   send_to_char(ch, "You are fighting you moron!\r\n");
   return;
 }

 if (GET_POS(ch) != POS_SITTING) {
   send_to_char(ch, "You need to be sitting to meditate.\r\n");
   return;
  }


 if (!*arg) {
   send_to_char(ch, "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break\r\n");
   return;
 }

 if (!strcasecmp(arg, "expand")) {
  int cost = 3500;
  if (IS_SAIYAN(ch)) {
   cost = 7000;
  }
  if (GET_PRACTICES(ch, GET_CLASS(ch)) < cost) {
   send_to_char(ch, "You do not have enough practice sessions to expand your mind and ability to remember skills.\r\n");
   send_to_char(ch, "%s needed.\r\n", add_commas(cost));
  } else if (GET_SLOTS(ch) >= 60 && GET_BONUS(ch, BONUS_GMEMORY) == 0) {
   send_to_char(ch, "You can not have any more slots through this process.\r\n");
  } else if (GET_SLOTS(ch) >= 65 && GET_BONUS(ch, BONUS_GMEMORY) == 1) {
   send_to_char(ch, "You can not have any more slots through this process.\r\n");
  } else {
   send_to_char(ch, "During your meditation you manage to expand your mind and get the feeling you could learn some new skills.\r\n");
   GET_SLOTS(ch) += 1;
   GET_PRACTICES(ch, GET_CLASS(ch)) -= cost;
   return;
  }
  return;
 } else if (!strcasecmp(arg, "break")) {
  if (MINDLINK(ch) == NULL) {
   send_to_char(ch, "You are not mind linked with anyone.\r\n");
   return;
  } else if (LINKER(ch) == 1) {
   send_to_char(ch, "This is not how you break YOUR mind link.\r\n");
   return;
  } else if (GET_MANA(ch) < GET_MAX_MANA(MINDLINK(ch)) * 0.05) {
   send_to_char(ch, "You do not have enough ki to manage a break.\r\n");
   return;
  } else if (GET_INT(ch) + rand_number(-5, 10) >= GET_INT(MINDLINK(ch)) + (GET_SKILL(MINDLINK(ch), SKILL_TELEPATHY) * 0.1)) {
   act("@rYou manage to break the mind link between you and @R$N@r!@n", FALSE, ch, 0, MINDLINK(ch), TO_CHAR);
   act("$n closes their eyes for a few seconds.", FALSE, ch, 0, MINDLINK(ch), TO_ROOM);
   send_to_char(MINDLINK(ch), "@rYour mind linked target manages to push you out!@n\r\n");
   if (GET_INT(MINDLINK(ch)) < axion_dice(-10) && !AFF_FLAGGED(MINDLINK(ch), AFF_SHOCKED)) {
    send_to_char(MINDLINK(ch), "Your mind is shocked by the flood of mental energy that pushed it out!@n\r\n");
    REMOVE_BIT_AR(AFF_FLAGS(MINDLINK(ch)), AFF_SHOCKED);
   }
 
   LINKER(MINDLINK(ch)) = 0;
   MINDLINK(MINDLINK(ch)) = NULL;
   MINDLINK(ch) = NULL;
   return;
  } else {
   act("@rYou struggle to free your mind of @R$N's@r link, but fail!@n", FALSE, ch, 0, MINDLINK(ch), TO_CHAR);
   act("$n closes their eyes for a few seconds, and appears to struggle quite a bit.", FALSE, ch, 0, MINDLINK(ch), TO_ROOM);
   send_to_char(MINDLINK(ch), "@rYour mind linked target struggles to free their mind, but fails!@n\r\n");
   GET_MANA(ch) -= GET_MAX_MANA(MINDLINK(ch)) * 0.05;
   return;
  }
 } else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
   send_to_char(ch, "Syntax: meditate (object)\nSyntax: meditate expand\nSyntax: meditate break\r\n");
   return;
 }

 if (GET_OBJ_VNUM(obj) == 79) {
  send_to_char(ch, "It's frozen to the surface.\r\n");
  return;
 }

 weight = GET_OBJ_WEIGHT(obj);

 if (SITTING(obj)) {
  weight += GET_WEIGHT(SITTING(obj));
 }

 if ((ROOM_GRAVITY(IN_ROOM(ch)) != 10 && IS_BARDOCK(ch)) || !IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * ((ROOM_GRAVITY(IN_ROOM(ch)) * 2) + GET_LEVEL(ch));
 } else if (IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * GET_LEVEL(ch);
 }
 cost += weight * ((ROOM_GRAVITY(IN_ROOM(ch)) + 1) / 5);

 if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
  cost += 1000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
  cost += 2000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
  cost += 7500000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
  cost += 15000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
  cost += 25000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
  cost += 50000000;
 }

 if (cost < weight) {
  cost = weight;
 }

 if (cost == 1 || cost == 0) {
  cost = 25;
 }
 if (GET_BONUS(ch, BONUS_HARDWORKER) > 0) {
  cost -= cost * .25;
 } else if (GET_BONUS(ch, BONUS_SLACKER) > 0) {
  cost += cost * .25;
 }

  if (GET_RELAXCOUNT(ch) >= 464) {
   cost *= 50;
  } else if (GET_RELAXCOUNT(ch) >= 232) {
   cost *= 15;
  } else if (GET_RELAXCOUNT(ch) >= 116) {
   cost *= 4;
  }

 if (GET_MANA(ch) < cost) {
   send_to_char(ch, "You don't have enough ki!\r\n");
   return;
  }

 else {
 /* Meditate Message */
 act("@cYou close your eyes and concentrate, lifting $p@c with your ki.@n", TRUE, ch, obj, 0, TO_CHAR);
 act("@c$n closes $s eyes and lifts $p@c with $s ki.@n", TRUE, ch, obj, 0, TO_ROOM);

 if (ROOM_GRAVITY(IN_ROOM(ch)) == 0) {
    bonus = 1;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10) {
    bonus = rand_number(2, 4);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 20) {
    bonus = rand_number(5, 10);
 } 
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 30) {
    bonus = rand_number(10, 15);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 40) {
    bonus = rand_number(15, 20);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 50) {
    bonus = rand_number(40, 60);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 100) {
    bonus = rand_number(180, 250);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 200) {
    bonus = rand_number(400, 600);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
    bonus = rand_number(800, 1200);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
    bonus = rand_number(2000, 3000);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
    bonus = rand_number(4000, 6000);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
    bonus = rand_number(9000, 10000);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
    bonus = rand_number(15000, 20000);
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
    bonus = rand_number(25000, 30000);
 }

 bonus += ((weight + 1) / 500) + 1;

   if (cost >= GET_MAX_MANA(ch) / 2) {
    send_to_char(ch, "The object weight and gravity are a great challenge for you!\r\n");
    bonus *= 10;
   }
   else if (cost >= GET_MAX_MANA(ch) / 4) {
    send_to_char(ch, "The object weight and gravity are an awesome challenge for you!\r\n");
    bonus *= 8;
   }
   else if (cost >= GET_MAX_MANA(ch) / 10) {
    send_to_char(ch, "The object weight and gravity are a good challenge for you!\r\n");
    bonus *= 6;
   }
   else if (cost < GET_MAX_MANA(ch) / 1000) {
    send_to_char(ch, "The object weight and gravity are so easy to you, you could do it in your sleep....\r\n");
    bonus /= 8;
   }
   else if (cost < GET_MAX_MANA(ch) / 100) {
    send_to_char(ch, "The object weight and gravity are the opposite of a challenge for you....\r\n");
    bonus /= 5;
   }
   else if (cost < GET_MAX_MANA(ch) / 50) {
    send_to_char(ch, "The object weight and gravity are definitely not a challenge for you....\r\n");
    bonus /= 4;
   }
   else if (cost < GET_MAX_MANA(ch) / 30) {
    send_to_char(ch, "The object weight and gravity are barely a challenge for you....\r\n");
    bonus /= 3;
   }
   else if (cost < GET_MAX_MANA(ch) / 20) {
    send_to_char(ch, "The object weight and gravity are hardly a challenge for you....\r\n");
    bonus /= 2;
   }
   else {
    send_to_char(ch, "This gravity is just perfect for you....\r\n");
    bonus *= 4;
   }
    if (!soft_cap(ch, 1)) {
     bonus = 0;
    }
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
    bonus *= 10;
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus *= 10;
    } else {
     bonus *= 5;
    }
   } else if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
    send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
    bonus *= 20;
   }
   if (bonus <= 0 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 1;
   }
   if (bonus <= 0 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 15;
   }
   if (bonus <= 1 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus = 12;
    } else {
     bonus = 6;
    }
   }
   if (bonus != 1 && IS_DEMON(ch) && rand_number(1, 100) >= 80) {
    send_to_char(ch, "Your spirit magnifies the strength of your body! @D[@G+%s@D]@n\r\n", add_commas(bonus / 2));
    GET_MAX_HIT(ch) += bonus / 2;
    GET_BASE_PL(ch) += bonus / 2;
    }

    bonus += GET_LEVEL(ch) / 20;
    if (IS_NAMEK(ch)) {
     bonus += bonus / 2;
    }
    if (GET_BONUS(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    /* Rillao: transloc, add new transes here */
    send_to_char(ch, "You feel your spirit grow stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus));
     if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
      GET_MAX_MANA(ch) += bonus * 3;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
      GET_MAX_MANA(ch) += bonus * 4;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
      GET_MAX_MANA(ch) += bonus * 5;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 1) {
      GET_MAX_MANA(ch) += bonus * 2;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 2) {
      GET_MAX_MANA(ch) += bonus * 3;
     }
     else {
     GET_MAX_MANA(ch) += bonus;
     }
    GET_BASE_KI(ch) += bonus;
   if (ROOM_GRAVITY(IN_ROOM(ch)) <= 50) {
   WAIT_STATE(ch, PULSE_2SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 200) {
   WAIT_STATE(ch, PULSE_3SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 500) {
   WAIT_STATE(ch, PULSE_4SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 1000) {
   WAIT_STATE(ch, PULSE_5SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 5000) {
   WAIT_STATE(ch, PULSE_6SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 10000) {
   WAIT_STATE(ch, PULSE_7SEC);
   }

   GET_MANA(ch) -= cost;
   if (GET_MANA(ch) < 0) {
    GET_MANA(ch) = 0;
   }
  }
}

ACMD(do_pushup)
{

 int cost = 1, bonus = 0;

 if (IS_NPC(ch)) {
   send_to_char(ch, "You are a mob fool!\r\n");
   return;
  }
 
  if (PLR_FLAGGED(ch, PLR_FISHING)) {
    send_to_char(ch, "Stop fishing first.\r\n");
    return;
  }
 if (DRAGGING(ch)) {
    send_to_char(ch, "You are dragging someone!\r\n");
    return;
  }
 if (CARRYING(ch)) {
    send_to_char(ch, "You are carrying someone!\r\n");
    return;
  }

 if ((ROOM_GRAVITY(IN_ROOM(ch)) != 10 && IS_BARDOCK(ch)) || !IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * ((ROOM_GRAVITY(IN_ROOM(ch)) * 2) + GET_LEVEL(ch));
 } else if (IS_BARDOCK(ch)) {
  cost += ROOM_GRAVITY(IN_ROOM(ch)) * GET_LEVEL(ch);
 }

 if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
  cost += 1000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
  cost += 2000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
  cost += 7500000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
  cost += 15000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
  cost += 25000000;
 }
 else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
  cost += 50000000;
 }

 if (cost == 1 || cost == 0) {
  cost = 25;
 }

 if (GET_BONUS(ch, BONUS_HARDWORKER) > 0) {
  cost -= cost * .25;
 } else if (GET_BONUS(ch, BONUS_SLACKER) > 0) {
  cost += cost * .25;
 }

  if (GET_RELAXCOUNT(ch) >= 464) {
   cost *= 50;
  } else if (GET_RELAXCOUNT(ch) >= 232) {
   cost *= 15;
  } else if (GET_RELAXCOUNT(ch) >= 116) {
   cost *= 4;
  }

 if (IS_ANDROID(ch) || IS_BIO(ch) || IS_MAJIN(ch) || IS_ARLIAN(ch)) {
   send_to_char(ch, "You will gain nothing from exercising!\r\n");
   return;
 }

  if (!limb_ok(ch, 0)) {
   return;
  }

 if (GET_MOVE(ch) < cost) {
   send_to_char(ch, "You are too tired!\r\n");
   return;
  }
 if (PLR_FLAGGED(ch, PLR_SPAR)) {
   send_to_char(ch, "You shouldn't be sparring if you want to work out, it could be dangerous.\r\n");
   return;
 }
 if (FIGHTING(ch)) {
   send_to_char(ch, "You are fighting you moron!\r\n");
   return;
 }
 if (AFF_FLAGGED(ch, AFF_FLYING)) {
   send_to_char(ch, "You can't do pushups in midair!\r\n");
   return;
 }

 else {
   if (ROOM_GRAVITY(IN_ROOM(ch)) == 0) {
    bonus = 1;
    act("@gYou do a pushup.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10) {
    bonus = rand_number(3, 7);
    act("@gYou do a pushup, feeling the strain of gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 20) {
    bonus = rand_number(8, 14);
    act("@gYou do a pushup, and feel gravity's pull.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 30) {
    bonus = rand_number(14, 20);
    act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 40) {
    bonus = rand_number(20, 35);
    act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 50) {
    bonus = rand_number(40, 60);
    act("@gYou do a pushup, and feel the burn.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 100) {
    bonus = rand_number(180, 250);
    act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 200) {
    bonus = rand_number(400, 600);
    act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 300) {
    bonus = rand_number(800, 1200);
    act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 400) {
    bonus = rand_number(2000, 3000);
    act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 500) {
    bonus = rand_number(4000, 6000);
    act("@gYou do a pushup, and really strain against the gravity.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000) {
    bonus = rand_number(9000, 10000);
    act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000) {
    bonus = rand_number(15000, 20000);
    act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000) {
    bonus = rand_number(25000, 30000);
    act("@gYou do a pushup, and it was a really hard one to finish.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@g$n does a pushup, while sweating profusely.@n", TRUE, ch, 0, 0, TO_ROOM);
   }
   if (cost >= GET_MAX_HIT(ch) / 2) {
    send_to_char(ch, "This gravity is a great challenge for you!\r\n");
    bonus *= 10;
   }
   else if (cost >= GET_MAX_HIT(ch) / 4) {
    send_to_char(ch, "This gravity is an awesome challenge for you!\r\n");
    bonus *= 8;
   }
   else if (cost >= GET_MAX_HIT(ch) / 10) {
    send_to_char(ch, "This gravity is a good challenge for you!\r\n");
    bonus *= 6;
   }
   else if (cost < GET_MAX_HIT(ch) / 1000) {
    send_to_char(ch, "This gravity is so easy to you, you could do it in your sleep....\r\n");
    bonus /= 8;
   }
   else if (cost < GET_MAX_HIT(ch) / 100) {
    send_to_char(ch, "This gravity is the opposite of a challenge for you....\r\n");
    bonus /= 5;
   }
   else if (cost < GET_MAX_HIT(ch) / 50) {
    send_to_char(ch, "This gravity is definitely not a challenge for you....\r\n");
    bonus /= 4;
   }
   else if (cost < GET_MAX_HIT(ch) / 30) {
    send_to_char(ch, "This gravity is barely a challenge for you....\r\n");
    bonus /= 3;
   }
   else if (cost < GET_MAX_HIT(ch) / 20) {
    send_to_char(ch, "This gravity is hardly a challenge for you....\r\n");
    bonus /= 2;
   }
   else {
    send_to_char(ch, "This gravity is just perfect for you....\r\n");
    bonus *= 4;
   }
   if (!soft_cap(ch, 0)) {
    bonus = 0;
   }
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    send_to_char(ch, "@rThis place feels like it operates on a different time frame, it feels great...@n\r\n");
    bonus *= 10; 
   } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus *= 10;
    } else {
     bonus *= 5;
    }
   } else if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
    send_to_char(ch, "@rThis place feels like... Magic.@n\r\n");
    bonus *= 20;
   }
   if (bonus <= 0 && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 1;
   }
   if (bonus <= 0 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
    bonus = 15;
   }
   if (bonus <= 1 && ROOM_FLAGGED(IN_ROOM(ch), ROOM_WORKOUT)) {
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19100 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19199) {
     bonus = 12;
    } else {
     bonus = 6;
    }
   }
    bonus += GET_LEVEL(ch) / 20;
    if (IS_NAMEK(ch)) {
     bonus -= bonus / 4;
    }
    if (GET_BONUS(ch, BONUS_HARDWORKER)) {
     bonus += bonus * 0.5;
    }
    if (GET_BONUS(ch, BONUS_LONER)) {
      bonus += bonus * 0.1;
    }
    /* Rillao: transloc, add new transes here */
    send_to_char(ch, "You feel slightly stronger @D[@G+%s@D]@n.\r\n", add_commas(bonus));
     if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS1)) {
      GET_MAX_HIT(ch) += bonus * 3;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS2)) {
      GET_MAX_HIT(ch) += bonus * 4;
     }
     else if (IS_TRUFFLE(ch) && PLR_FLAGGED(ch, PLR_TRANS3)) {
      GET_MAX_HIT(ch) += bonus * 5;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 1) {
      GET_MAX_HIT(ch) += bonus * 2;
     }
     else if (IS_HOSHIJIN(ch) && GET_PHASE(ch) == 2) {
      GET_MAX_HIT(ch) += bonus * 3;
     }
     else {
      if (IS_HUMAN(ch)) {
       bonus = bonus * 0.8;
      }
      GET_MAX_HIT(ch) += bonus;
     }
     GET_BASE_PL(ch) += bonus;
   if (ROOM_GRAVITY(IN_ROOM(ch)) <= 50) {
   WAIT_STATE(ch, PULSE_2SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 200) {
   WAIT_STATE(ch, PULSE_3SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 500) {
   WAIT_STATE(ch, PULSE_4SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 1000) {
   WAIT_STATE(ch, PULSE_5SEC);  
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 5000) {
   WAIT_STATE(ch, PULSE_6SEC);
   }
   else if (ROOM_GRAVITY(IN_ROOM(ch)) <= 10000) {
   WAIT_STATE(ch, PULSE_7SEC);
   }
   GET_MOVE(ch) -= cost;
   if (GET_MOVE(ch) < 0) {
    GET_MOVE(ch) = 0;
   }
 }
}

ACMD(do_spar)
{
 if (IS_NPC(ch)) {
  return;
 }
 else {
  if (PLR_FLAGGED(ch, PLR_SPAR)) {
   act("@wYou cease your sparring stance.@n", FALSE, ch, 0, 0, TO_CHAR);
   act("@C$n@w ceases $s sparring stance.@n", FALSE, ch, 0, 0, TO_ROOM);
  }
  if (!PLR_FLAGGED(ch, PLR_SPAR)) {
   act("@wYou move into your sparring stance.@n", FALSE, ch, 0, 0, TO_CHAR);
   act("@C$n@w moves into $s sparring stance.@n", FALSE, ch, 0, 0, TO_ROOM);
  }
  TOGGLE_BIT_AR(PLR_FLAGS(ch), PLR_SPAR);
 }
}

static void check_eq(struct char_data *ch)
{
  struct obj_data *obj;
  int i;
  for (i = 0; i < NUM_WEARS; i++) {
   if (GET_EQ(ch, i)) {
      obj = GET_EQ(ch, i);
    if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
     act("@W$p@W falls apart and you remove it.@n", FALSE, ch, obj, 0, TO_CHAR);
     act("@W$p@W falls apart and @C$n@W remove it.@n", FALSE, ch, obj, 0, TO_ROOM);
     perform_remove(ch, i);
     return;
    }
    if (obj == GET_EQ(ch, WEAR_WIELD1) && GET_LIMBCOND(ch, 1) <= 0) {
     act("@WWithout your right arm you let go of @c$p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
     act("@C$n@W lets go of @c$p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
     perform_remove(ch, i);
     return;
    }
    if (obj == GET_EQ(ch, WEAR_WIELD2) && GET_LIMBCOND(ch, 2) <= 0) {
     act("@WWithout your left arm you let go of @c$p@W!@n", FALSE, ch, obj, 0, TO_CHAR);
     act("@C$n@W lets go of @c$p@W!@n", FALSE, ch, obj, 0, TO_ROOM);
     perform_remove(ch, i);
     return;
    }
   }
  }
}

/* This handles many player specific routines. It may be a bit too bloated though. */
void base_update(void)
{
	struct descriptor_data *d;
	int cash = FALSE, inc = 0;
	int countch = FALSE, pcoun = 0;

	if (INTERESTTIME != 0 && INTERESTTIME <= time(0) && time(0) != 0) {
		INTERESTTIME = time(0) + 86400;
		LASTINTEREST = time(0);
		save_mud_time(&time_info);
		cash = TRUE;
		countch = TRUE;
	}

	if (TOPCOUNTDOWN > 0) {
		TOPCOUNTDOWN -= 4;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (!IS_PLAYING(d))
			continue;
		if (IS_NPC(d->character)) {
			if (ABSORBING(d->character) && IN_ROOM(d->character) != IN_ROOM(ABSORBING(d->character))) {
				send_to_char(d->character, "You stop absorbing %s!\r\n", GET_NAME(ABSORBING(d->character)));
				ABSORBBY(ABSORBING(d->character)) = NULL;
				ABSORBING(d->character) = NULL;
			}
			if (IS_ANDROID(d->character) && ABSORBING(d->character) && rand_number(1, 10) >= 7) {
				int64_t drain1 = GET_MAX_MANA(d->character) * 0.01, drain2 = GET_MAX_MOVE(d->character) * 0.01;
				struct char_data *drained = ABSORBING(d->character);
				if (GET_MOVE(drained) - drain2 < 0) {
					drain2 = GET_MOVE(drained);
				}
				if (GET_MANA(drained) - drain1 < 0) {
					drain1 = GET_MANA(drained);
				}
				GET_MOVE(d->character) += drain2;
				GET_MANA(d->character) += drain1;
				GET_HIT(d->character) += drain1 * 0.5;
				if (GET_MANA(d->character) >= GET_MAX_MANA(d->character)) {
					GET_MANA(d->character) = GET_MAX_MANA(d->character);
				}
				if (GET_MOVE(d->character) >= GET_MAX_MOVE(d->character)) {
					GET_MOVE(d->character) = GET_MAX_MOVE(d->character);
				}
				if (GET_HIT(d->character) >= GET_MAX_HIT(d->character)) {
					GET_HIT(d->character) = GET_MAX_HIT(d->character);
				}
				if (GET_MANA(d->character) == GET_MAX_MANA(d->character) && GET_MOVE(d->character) == GET_MAX_MOVE(d->character)) {
					do_absorb(d->character, NULL, 0, 0);
				}
			}
			continue;
		}
		if (countch == TRUE) {
			pcoun += 1;
		}
		if (!IS_NPC(d->character) && rand_number(1, 15) >= 14) {
			ash_burn(d->character);
		}
		if (AFF_FLAGGED(d->character, AFF_CURSE) && GET_LIFEFORCE(d->character) > GET_LIFEMAX(d->character) * 0.4) {
			GET_LIFEFORCE(d->character) -= GET_LIFEMAX(d->character) * 0.01;
			demon_refill_lf(d->character, GET_LIFEMAX(d->character) * 0.01);
			if (GET_LIFEFORCE(d->character) < GET_LIFEMAX(d->character) * 0.4) {
				GET_LIFEFORCE(d->character) = GET_LIFEMAX(d->character) * 0.4;
			}
		}
		if (GET_BACKSTAB_COOL(d->character) > 0) {
			GET_BACKSTAB_COOL(d->character) -= 1;
		}
		if (PLR_FLAGGED(d->character, PLR_GOOP) && d->character->gooptime == 60) {
			if (IS_BIO(d->character)) {
				act("@GConciousness slowly returns to you. You realize quickly that some of your cells have survived. You take control of your regenerative processes and focus on growing a new body!@n", TRUE, d->character, 0, 0, TO_CHAR);
			}
			else {
				act("@MSlowly you regain conciousness. The various split off chunks of your body begin to likewise stir.@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@MYou think you notice the chunks of @m$n@M's moving slightly.@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			d->character->gooptime -= 1;
		}
		else if (PLR_FLAGGED(d->character, PLR_GOOP) && d->character->gooptime == 30) {
			if (IS_BIO(d->character)) {
				act("@GFrom the collection of cells growing a crude form of your body starts to take shape!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@GYou start to notice a large mass of pulsing flesh growing before you!@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			else {
				act("@MYou will the various chunks of your body to return and slowly more and more of them begin to fly into you. Your body begins to grow larger and larger as this process unfolds!@n ", TRUE, d->character, 0, 0, TO_CHAR);
				act("@MThe various chunks of @m$n@M's body start to fly into the largest chunk! As the chunks collide they begin to form a larger and still growing blob of goo!@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			d->character->gooptime -= 1;
		}
		else if (PLR_FLAGGED(d->character, PLR_GOOP) && d->character->gooptime == 15) {
			if (IS_BIO(d->character)) {
				act("@GYour body has almost reached its previous form! Only a little more regenerating is needed!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@GThe lump of flesh has now grown to the size where the likeness of @g$n@G can be seen of it! It appears that $e is regenerating $s body from what was only a few cells!@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			else {
				act("@MYour body has reached half its previous size as your limbs ooze slowly out into their proper shape!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@m$n@M's body has regenerated to half its previous size! Slowly $s limbs ooze out into their proper shape! It won't be long now till $e has fully regenerated!@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			d->character->gooptime -= 1;
		}
		else if (PLR_FLAGGED(d->character, PLR_GOOP) && d->character->gooptime == 0)
		{
			if (IS_BIO(d->character))
			{
				GET_HIT(d->character) = gear_pl(d->character);
				act("@GYour body has fully regenerated! You flex your arms and legs outward with a rush of renewed strength!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@g$n@G's body has fully regenerated! Suddenly $e flexes $s arms and legs and a rush of power erupts from off of $s body!@n", TRUE, d->character, 0, 0, TO_ROOM);
			}
			//Zenkai Boost
			else if (IS_SAIYAN(d->character))
			{

				int zenkaiPL, zenkaiKi, zenkaiSt;
				zenkaiPL = GET_BASE_PL(d->character) * 1.03;
				zenkaiKi = GET_BASE_KI(d->character) * 1.015;
				zenkaiSt = GET_BASE_ST(d->character) * 1.015;

				GET_HIT(d->character) = gear_pl(d->character) * .5;
				GET_MANA(d->character) = GET_MAX_MANA(d->character) *.2;
				GET_MOVE(d->character) = GET_MAX_MOVE(d->character) *.2;
				if (!IN_ARENA(d->character))
				{
					GET_BASE_PL(d->character) = zenkaiPL;
					GET_BASE_KI(d->character) = zenkaiKi;
					GET_BASE_ST(d->character) = zenkaiSt;
					GET_MAX_HIT(d->character) = zenkaiPL;
					GET_MAX_MANA(d->character) = zenkaiKi;
					GET_MAX_MOVE(d->character) = zenkaiSt;
					send_to_char(d->character, "@D[@YZ@ye@wn@Wk@Ya@yi @YB@yo@wo@Ws@Yt@D] @WYou feel much stronger!\r\n");
					send_to_char(d->character, "@D[@RPL@Y:@n+%s@D] @D[@CKI@Y:@n+%s@D] @D[@GSTA@Y:@n+%s@D]@n\r\n", add_commas(zenkaiPL), add_commas(zenkaiKi), add_commas(zenkaiSt));
				}
				act("@RYou collapse to the ground, body pushed beyond the typical limits of exhaustion. The passage of time distorts and an indescribable amount of time passes as raw emotions pass through your very being. Your eyes open and focus with a newfound clarity as your unadulterated emotions and feelings revive you for a second wind!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@r$n@R collapses to the ground, seemingly dead. After a brief moment, their eyes flash open with a determined look on their face!", TRUE, d->character, 0, 0, TO_ROOM);
			}
			else {
				GET_HIT(d->character) = gear_pl(d->character);
				act("@MYour body has fully regenerated! You scream out in triumph and a short gust of steam erupts from your pores!@n", TRUE, d->character, 0, 0, TO_CHAR);
				act("@m$n@M's body has fully regenerated! Suddenly $e screams out in gleeful triumph and short gust of steam erupts from $s skin pores!", TRUE, d->character, 0, 0, TO_ROOM);
			}
			REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_GOOP);
		}
		else {
			d->character->gooptime -= 1;
		}
		if (GET_COOLDOWN(d->character) > 0) {
			GET_COOLDOWN(d->character) -= 2;
			if (GET_COOLDOWN(d->character) <= 0) {
				GET_COOLDOWN(d->character) = 0;
				send_to_char(d->character, "You can concentrate again.\r\n");
			}
		}
		/* Andros Start */
		if (GET_SDCOOLDOWN(d->character) > 0) {
			GET_SDCOOLDOWN(d->character) -= 10;
			if (GET_SDCOOLDOWN(d->character) <= 0) {
				GET_SDCOOLDOWN(d->character) = 0;
				send_to_char(d->character, "Your body has recovered from your last selfdestruct.\r\n");
			}
		} /* Andros End */
		if (CARRYING(d->character)) {
			if (IN_ROOM(CARRYING(d->character)) != IN_ROOM(d->character)) {
				carry_drop(d->character, 3);
			}
		}
		if (GET_DEFENDER(d->character)) {
			if (IN_ROOM(d->character) != IN_ROOM(GET_DEFENDER(d->character))) {
				GET_DEFENDING(GET_DEFENDER(d->character)) = NULL;
				GET_DEFENDER(d->character) = NULL;
			}
		}
		if (GET_DEFENDING(d->character)) {
			if (IN_ROOM(d->character) != IN_ROOM(GET_DEFENDING(d->character))) {
				GET_DEFENDER(GET_DEFENDING(d->character)) = NULL;
				GET_DEFENDING(d->character) = NULL;
			}
		}
		if (PLR_FLAGGED(d->character, PLR_TRANSMISSION)) {
			REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_TRANSMISSION);
		}
		if (!FIGHTING(d->character) && AFF_FLAGGED(d->character, AFF_POSITION)) {
			REMOVE_BIT_AR(AFF_FLAGS(d->character), AFF_POSITION);
		}
		if (SITS(d->character)) {
			if (IN_ROOM(d->character) != IN_ROOM(SITS(d->character))) {
				struct obj_data *chair = SITS(d->character);
				SITTING(chair) = NULL;
				SITS(d->character) = NULL;
			}
		}
		if (GET_PING(d->character) >= 1) {
			GET_PING(d->character) -= 1;
			if (PLR_FLAGGED(d->character, PLR_PILOTING) && GET_PING(d->character) == 0) {
				send_to_char(d->character, "Your radar is ready to calculate the direction of another destination.\r\n");
			}
		}
		if (GET_ADMLEVEL(d->character) < 1 && TOPCOUNTDOWN <= 0 && GET_LEVEL(d->character) > 0) {
			topWrite(d->character);
		}
		if (PLR_FLAGGED(d->character, PLR_SELFD) && !PLR_FLAGGED(d->character, PLR_SELFD2)) {
			if (rand_number(4, 100) < GET_SKILL(d->character, SKILL_SELFD)) {
				send_to_char(d->character, "You feel you are ready to self destruct!\r\n");
				SET_BIT_AR(PLR_FLAGS(d->character), PLR_SELFD2);
			}
		}
		if (!FIGHTING(d->character) && COMBO(d->character) > -1) {
			COMBO(d->character) = -1;
			COMBHITS(d->character) = 0;
		}
		if (MOON_UP && (IS_SAIYAN(d->character) || IS_HALFBREED(d->character)) && !PLR_FLAGGED(d->character, PLR_OOZARU)) {
			oozaru_add(d->character);
		}
		if (cash == TRUE && GET_BANK_GOLD(d->character) > 0) {
			inc = (GET_BANK_GOLD(d->character) / 50) * 2;
			GET_LINTEREST(d->character) = LASTINTEREST;
			if (inc >= 25000) {
				inc = 25000;
			}
			GET_BANK_GOLD(d->character) += inc;
			send_to_char(d->character, "@cBank Interest@D: @Y%s@n\r\n", add_commas(inc));
		}
		if (!IS_NPC(d->character)) {
			check_eq(d->character);
		}
		if (!IS_NPC(d->character) && ROOM_EFFECT(IN_ROOM(d->character)) >= 1 && rand_number(1, 100) >= 96) {
			if (ROOM_EFFECT(IN_ROOM(d->character)) <= 4) {
				switch (rand_number(1, 4)) {
				case 1:
					act("@RLava spews up violently from the cracks in the ground!@n", FALSE, d->character, 0, 0, TO_ROOM);
					act("@RLava spews up violently from the cracks in the ground!@n", FALSE, d->character, 0, 0, TO_CHAR);
					break;
				case 2:
					act("@RThe lava bubbles and gives off tremendous heat!@n", FALSE, d->character, 0, 0, TO_ROOM);
					act("@RThe lava bubbles and gives off tremendous heat!@n", FALSE, d->character, 0, 0, TO_CHAR);
					break;
				case 3:
					act("@RNoxious fumes rise from the bubbling lava!@n", FALSE, d->character, 0, 0, TO_ROOM);
					act("@RNoxious fumes rise from the bubbling lava!@n", FALSE, d->character, 0, 0, TO_CHAR);
					break;
				case 4:
					act("@RSome of the lava cools as it spreads further from the source!@n", FALSE, d->character, 0, 0, TO_ROOM);
					act("@RSome of the lava cools as it spreads further from the source!@n", FALSE, d->character, 0, 0, TO_CHAR);
					break;
				}
				ROOM_EFFECT(IN_ROOM(d->character)) += 1;
			}
			else if (ROOM_EFFECT(IN_ROOM(d->character)) == 5) {
				act("@RLava covers the entire area now!@n", FALSE, d->character, 0, 0, TO_ROOM);
				act("@RLava covers the entire area now!@n", FALSE, d->character, 0, 0, TO_CHAR);
				ROOM_EFFECT(IN_ROOM(d->character)) += 1;
			}
		}
		if (ABSORBING(d->character) && IN_ROOM(d->character) != IN_ROOM(ABSORBING(d->character))) {
			send_to_char(d->character, "You stop absorbing %s!\r\n", GET_NAME(ABSORBING(d->character)));
			ABSORBBY(ABSORBING(d->character)) = NULL;
			ABSORBING(d->character) = NULL;
		}
		if (IS_ANDROID(d->character) && ABSORBING(d->character)) {
			if (GET_MOVE(ABSORBING(d->character)) < (GET_MAX_MOVE(d->character) / 15) && GET_MANA(ABSORBING(d->character)) < (GET_MAX_MANA(d->character) / 15)) {
				act("@WYou stop absorbing stamina and ki from @c$N as they don't have enough for you to take@W!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_CHAR);
				act("@C$n@W stops absorbing stamina and ki from you!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_VICT);
				act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_NOTVICT);
				if (!FIGHTING(d->character) || FIGHTING(d->character) != ABSORBING(d->character)) {
					set_fighting(d->character, ABSORBBY(ABSORBING(d->character)));
				}
				if (!FIGHTING(ABSORBBY(ABSORBING(d->character))) || FIGHTING(ABSORBBY(ABSORBING(d->character))) != d->character) {
					set_fighting(ABSORBBY(ABSORBING(d->character)), d->character);
				}
				ABSORBBY(ABSORBING(d->character)) = NULL;
				ABSORBING(d->character) = NULL;
			}
		}
		if (IS_ANDROID(d->character) && ABSORBING(d->character) && rand_number(1, 9) >= 6) {
			if (GET_MOVE(ABSORBING(d->character)) > (GET_MAX_MOVE(d->character) / 15) || GET_MANA(ABSORBING(d->character)) > (GET_MAX_MANA(d->character) / 15)) {
				GET_MOVE(d->character) += GET_MAX_MOVE(d->character) * 0.08;
				GET_MANA(d->character) += GET_MAX_MANA(d->character) * 0.08;
				GET_MOVE(ABSORBING(d->character)) -= GET_MAX_MOVE(d->character) / 20;
				GET_MANA(ABSORBING(d->character)) -= GET_MAX_MANA(d->character) / 20;
				act("@WYou absorb stamina and ki from @c$N@W!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_CHAR);
				act("@C$n@W absorbs stamina and ki from you!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_VICT);
				send_to_char(ABSORBING(d->character), "@wTry 'escape'!@n\r\n");
				act("@C$n@W absorbs stamina and ki from @c$N@w!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_NOTVICT);
				if (GET_HIT(d->character) < gear_pl(d->character)) {
					GET_HIT(d->character) += GET_MAX_MANA(d->character) * 0.04;
					if (GET_HIT(d->character) > gear_pl(d->character)) {
						GET_HIT(d->character) = gear_pl(d->character);
					}
					send_to_char(d->character, "@CYou convert a portion of the absorbed energy into refilling your powerlevel.@n\r\n");
				}
				if (GET_MOVE(ABSORBING(d->character)) <= 0) {
					GET_MOVE(ABSORBING(d->character)) = 1;
				}
				if (GET_MANA(ABSORBING(d->character)) <= 0) {
					GET_MANA(ABSORBING(d->character)) = 1;
				}
				if (GET_MOVE(d->character) > GET_MAX_MOVE(d->character) && GET_MANA(d->character) < GET_MAX_MANA(d->character)) {
					GET_MOVE(d->character) = GET_MAX_MOVE(d->character);
				}
				else if (GET_MOVE(d->character) < GET_MAX_MOVE(d->character) && GET_MANA(d->character) > GET_MAX_MANA(d->character)) {
					GET_MANA(d->character) = GET_MAX_MANA(d->character);
				}
				else if (GET_MOVE(d->character) >= GET_MAX_MOVE(d->character) && GET_MANA(d->character) >= GET_MAX_MANA(d->character)) {
					GET_MANA(d->character) = GET_MAX_MANA(d->character);
					GET_MOVE(d->character) = GET_MAX_MOVE(d->character);
					act("@WYou stop absorbing stamina and ki from @c$N as you are full@W!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_CHAR);
					act("@C$n@W stops absorbing stamina and ki from you!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_VICT);
					act("@C$n@W stops absorbing stamina and ki from @c$N@w!@n", TRUE, d->character, 0, ABSORBING(d->character), TO_NOTVICT);
					if (!FIGHTING(d->character) || FIGHTING(d->character) != ABSORBING(d->character)) {
						set_fighting(d->character, ABSORBBY(ABSORBING(d->character)));
					}
					if (!FIGHTING(ABSORBBY(ABSORBING(d->character))) || FIGHTING(ABSORBBY(ABSORBING(d->character))) != d->character) {
						set_fighting(ABSORBBY(ABSORBING(d->character)), d->character);
					}
					ABSORBBY(ABSORBING(d->character)) = NULL;
					ABSORBING(d->character) = NULL;
				}
				int sum = 1;
				int mum = 1;
				int ium = 1;
				if (!soft_cap(d->character, 0)) {
					sum = 0;
				}
				if (!soft_cap(d->character, 2)) {
					mum = 0;
				}
				if (!soft_cap(d->character, 1)) {
					ium = 0;
				}

				if (sum == 1) {
					if (rand_number(1, 8) >= 6) {
						int gain = rand_number(GET_LEVEL(d->character) / 2, GET_LEVEL(d->character) * 3) + (GET_LEVEL(d->character) * 18);
						if (GET_LEVEL(d->character) > 30) {
							gain += rand_number(GET_LEVEL(d->character) * 2, GET_LEVEL(d->character) * 4) + (GET_LEVEL(d->character) * 50);
						}
						if (GET_LEVEL(d->character) > 60) {
							gain *= 2;
						}
						if (GET_LEVEL(d->character) > 80) {
							gain *= 3;
						}
						if (GET_LEVEL(d->character) > 90) {
							gain *= 4;
						}
						send_to_char(d->character, "@gYou gain +@G%d@g permanent powerlevel!@n\r\n", gain);
						if (group_bonus(d->character, 2) == 7) {
							if (PLR_FLAGGED(d->character->master, PLR_SENSEM)) {
								int gbonus = gain * 0.15;
								gain += gbonus;
								send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
							}
						}
						GET_MAX_HIT(d->character) += gain;
						GET_BASE_PL(d->character) += gain;
					}
				}
				if (mum == 1) {
					if (rand_number(1, 8) >= 6) {
						int gain = rand_number(GET_LEVEL(d->character) / 2, GET_LEVEL(d->character) * 3) + (GET_LEVEL(d->character) * 18);
						if (GET_LEVEL(d->character) > 30) {
							gain += rand_number(GET_LEVEL(d->character) * 2, GET_LEVEL(d->character) * 4) + (GET_LEVEL(d->character) * 50);
						}
						if (GET_LEVEL(d->character) > 60) {
							gain *= 2;
						}
						if (GET_LEVEL(d->character) > 80) {
							gain *= 3;
						}
						if (GET_LEVEL(d->character) > 90) {
							gain *= 4;
						}
						send_to_char(d->character, "@gYou gain +@G%d@g permanent stamina!@n\r\n", gain);
						if (group_bonus(d->character, 2) == 7) {
							if (PLR_FLAGGED(d->character->master, PLR_SENSEM)) {
								int gbonus = gain * 0.15;
								gain += gbonus;
								send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
							}
						}
						GET_MAX_MOVE(d->character) += gain;
						GET_BASE_ST(d->character) += gain;
					}
				}
				if (ium == 1) {
					if (rand_number(1, 8) >= 6) {
						int gain = rand_number(GET_LEVEL(d->character) / 2, GET_LEVEL(d->character) * 3) + (GET_LEVEL(d->character) * 18);
						if (GET_LEVEL(d->character) > 30) {
							gain += rand_number(GET_LEVEL(d->character) * 2, GET_LEVEL(d->character) * 4) + (GET_LEVEL(d->character) * 50);
						}
						if (GET_LEVEL(d->character) > 60) {
							gain *= 2;
						}
						if (GET_LEVEL(d->character) > 80) {
							gain *= 3;
						}
						if (GET_LEVEL(d->character) > 90) {
							gain *= 4;
						}
						send_to_char(d->character, "@gYou gain +@G%d@g permanent ki!@n\r\n", gain);
						if (group_bonus(d->character, 2) == 7) {
							if (PLR_FLAGGED(d->character->master, PLR_SENSEM)) {
								int gbonus = gain * 0.15;
								gain += gbonus;
								send_to_char(d->character, "The leader of your group conveys an extra bonus! @D[@G+%s@D]@n \r\n", add_commas(gbonus));
							}
						}
						GET_MAX_MANA(d->character) += gain;
						GET_BASE_KI(d->character) += gain;
					}
				}
				if (sum == 0) {
					if (rand_number(1, 8) >= 6) {
						int gain = 1;
						send_to_char(d->character, "@gYou gain +@G%d@g permanent powerlevel. You may need to level.@n\r\n", gain);
						GET_MAX_HIT(d->character) += gain;
						GET_BASE_PL(d->character) += gain;
					}
				}
				if (mum == 0) {
					if (rand_number(1, 8) >= 6) {
						int gain = 1;
						send_to_char(d->character, "@gYou gain +@G%d@g permanent stamina. You may need to level.@n\r\n", gain);
						GET_MAX_MOVE(d->character) += gain;
						GET_BASE_ST(d->character) += gain;
					}
				}
				if (ium == 0) {
					if (rand_number(1, 8) >= 6) {
						int gain = 1;
						send_to_char(d->character, "@gYou gain +@G%d@g permanent ki. You may need to level.@n\r\n", gain);
						GET_MAX_MANA(d->character) += gain;
						GET_BASE_KI(d->character) += gain;
					}
				}
			}
		}
		if (BLOCKS(d->character)) {
			struct char_data *vict = BLOCKS(d->character);
			if (IN_ROOM(vict) != IN_ROOM(d->character)) {
				BLOCKED(vict) = NULL;
				BLOCKS(d->character) = NULL;
			}
		}
		if (GET_OVERFLOW(d->character) == TRUE) {
			mudlog(NRM, ADMLVL_GOD, TRUE, "OVERFLOW: %s has caused an overflow, check for illegal activity.", GET_NAME(d->character));
			GET_OVERFLOW(d->character) = FALSE;
		}
		if (GET_SPAM(d->character) > 0) {
			GET_SPAM(d->character) = 0;
		}
		else
			continue;
	}

	if (countch == TRUE) {
		PCOUNT = pcoun;
		PCOUNTDAY = time(0);
	}

	if (TOPCOUNTDOWN <= 0) {
		TOPCOUNTDOWN = 60;
	}
}

static int has_scanner(struct char_data *ch)
{
	struct obj_data *obj, *next_obj;
	int success = 0;



	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj && GET_OBJ_VNUM(obj) == 13600) {
			success = 1;
		}
	}

	return (success);
}

ACMD(do_snet)
{
 int channel = 0, global = FALSE, call = -1, reached = FALSE;
 struct descriptor_data *i;
 char voice[150], arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
 char hist[MAX_INPUT_LENGTH];

 half_chop(argument, arg, arg2);
 
 struct obj_data *obj = NULL;
 struct obj_data *obj2 = NULL;

 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HBTC)) {
   send_to_char(ch, "This is a different dimension!\r\n");
   return;
 }
 if (IN_ARENA(ch)) {
  send_to_char(ch, "Lol, no.\r\n");
  return;
 }
 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PAST)) {
   send_to_char(ch, "This is the past, you can't talk on scouter net!\r\n");
   return;
 }
 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HELL)) {
   send_to_char(ch, "The fire eats your transmission!\r\n");
   return;
 }
 if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
  send_to_char(ch, "Your signal will not be able to escape the walls of the pocket dimension.\r\n");
  return;
 }

 if (!IS_NPC(ch)) {
  if (GET_EQ(ch, WEAR_EYE)) {
   obj = GET_EQ(ch, WEAR_EYE);
  } else {
   send_to_char(ch, "You do not have a scouter on.\r\n");
   return;
  }
 }

 if (!*arg) {
  send_to_char(ch, "[Syntax] snet < [1-999] | check | #(scouter number) | * message | message>\r\n");
  return;
 }

 if (strstr(arg, "#")) {
  search_replace(arg, "#", "");
  call = atoi(arg);
  if (call <= -1) {
   send_to_char(ch, "Call what personal scouter number?\r\n");
   return;
  }
 }

 if (!strcasecmp(arg, "check")) {
  send_to_char(ch, "Your personal scouter number is: %d\r\n", GET_ID(ch));
  return;
 }

 if (call <= -1) {
  channel = atoi(arg);
 }
 
 if (channel > 0) {
   SFREQ(obj) = channel;
   if (channel > 999) {
     SFREQ(obj) = 999;
   }
   act("@wYou push some buttons on $p@w and change its channel.", TRUE, ch, obj, 0, TO_CHAR);
   act("@C$n@w pushes some buttons on $p@w and changes its channel.", TRUE, ch, obj, 0, TO_ROOM);
   return;
 }
 else {
  if (GET_BONUS(ch, BONUS_MUTE) > 0) {
   send_to_char(ch, "You are unable to speak though.\r\n");
   return;
  }
  if (SFREQ(obj) == 0) {
   SFREQ(obj) = 1;
  }
  if (!strcasecmp(arg, "*") && call <= -1) {
   global = TRUE;
  }
  if (GET_VOICE(ch) != NULL) {
   sprintf(voice, "%s", GET_VOICE(ch));
  }
  if (GET_VOICE(ch) == NULL) {
   sprintf(voice, "A generic voice");
  }
  for (i = descriptor_list; i; i = i->next) {
   if (STATE(i) != CON_PLAYING) {
    continue;
   }
   if (i->character == ch) {
    continue;
   }
   if (IN_ROOM(i->character) == IN_ROOM(ch)) {
    continue;
   }
   if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_HBTC)) {
    continue;
   }
   if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_PAST)) {
    continue;
   }
   if ((ROOM_FLAGGED(IN_ROOM(i->character), ROOM_RHELL) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL)) || (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_AL) && !ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL))) {
    continue;
   }
   if ((!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_RHELL) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL)) || (!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_AL) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL))) {
    continue;
   }
   if (GET_POS(i->character) == POS_SLEEPING) {
    continue;
   }
   if (GET_ROOM_VNUM(IN_ROOM(i->character)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(i->character)) <= 19899) {
    continue;
   }
   if (GET_EQ(i->character, WEAR_EYE)) {
    obj2 = GET_EQ(i->character, WEAR_EYE);
    if (SFREQ(obj2) == 0) {
     SFREQ(obj2) = 1;
    }
    if (global == FALSE && call <= -1 && SFREQ(obj2) == SFREQ(obj) && GET_ADMLEVEL(i->character) < 1) {
      send_to_char(i->character, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
      *hist = '\0';
      sprintf(hist, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
      add_history(i->character, hist, HIST_SNET);
      if (has_scanner(i->character)) {
       char *blah = sense_location(ch);
       send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
       free(blah);
      }
      continue;
    } /* It is the right freq */
    else if (global == TRUE && call <= -1 && GET_ADMLEVEL(i->character) < 1) {
      send_to_char(i->character, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg2));
      *hist = '\0';
      sprintf(hist, "@C%s is heard @W(@c%s@W), @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", SFREQ(obj), CAP(arg2));
      add_history(i->character, hist, HIST_SNET);
      if (has_scanner(i->character)) {
       char *blah = sense_location(ch);
       send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
       free(blah);
      }
      continue;
    }
    else if (call > -1 && GET_ID(i->character) == call) {
     send_to_char(i->character, "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", GET_ID(ch), !*arg2 ? "" : CAP(arg2));
      *hist = '\0';
      sprintf(hist, "@C%s is heard @W(@c%s@W), @D[@R#@W%d @Ycalling YOU@D] @G%s@n\r\n", voice, readIntro(i->character, ch) == 1 ? get_i_name(i->character, ch) : "Unknown", GET_ID(ch), !*arg2 ? "" : CAP(arg2));
      add_history(i->character, hist, HIST_SNET);
      if (has_scanner(i->character)) {
       char *blah = sense_location(ch);
       send_to_char(i->character, "@WScanner@D: @Y%s@n\r\n", blah);
       free(blah);
      }
     reached = TRUE;
    }
   } /* They have a scouter */
   if (GET_ADMLEVEL(i->character) > 0 && call <= -1) {
     send_to_char(i->character, "@C%s (%s) is heard, @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", voice, GET_NAME(ch), SFREQ(obj), CAP(arg), !*arg2 ? "" : arg2);
     continue;
   } else if (GET_ADMLEVEL(i->character) > 0) {
     send_to_char(i->character, "@C%s (%s) is heard, @D[@WCall to @R#@Y%d@D] @G%s@n\r\n", voice, GET_NAME(ch), call, !*arg2 ? "" : CAP(arg2));
     continue;
   }
  } /* End switch */
 if (call <= -1) {
  if (global == FALSE) {
    reveal_hiding(ch, 3);
    send_to_char(ch, "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", SFREQ(obj), arg, !*arg2 ? "" : arg2);
      *hist = '\0';
      sprintf(hist, "@CYou @D[@WSNET FREQ@D: @Y%d@D] @G%s %s@n\r\n", SFREQ(obj), arg, !*arg2 ? "" : arg2);
      add_history(ch, hist, HIST_SNET);
    char over[MAX_STRING_LENGTH];
    sprintf(over, "@C$n@W says into $s scouter, '@G@G%s %s@W'@n\r\n", CAP(arg), !*arg2 ? "" : arg2);
    act(over, TRUE, ch, 0, 0, TO_ROOM);
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL)) {
    send_to_char(ch, "@mThe transmission only reaches those who are in the afterlife.@n\r\n");
   }
  }
  if (global == TRUE) {
   reveal_hiding(ch, 3);
   send_to_char(ch, "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", SFREQ(obj), !*arg2 ? "" : CAP(arg2));
      *hist = '\0';
      sprintf(hist, "@CYou @D[@WSNET FREQ@D: @Y%d @mBroadcast@D] @G%s@n\r\n", SFREQ(obj), !*arg2 ? "" : CAP(arg2));
      add_history(ch, hist, HIST_SNET);
    char over[MAX_STRING_LENGTH];
    sprintf(over, "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
    act(over, TRUE, ch, 0, 0, TO_ROOM);
   if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_RHELL) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_AL)) {
    send_to_char(ch, "@mThe transmission only reaches those who are in the afterlife.@n\r\n");
   }
   }
  } else {
    reveal_hiding(ch, 3);
    send_to_char(ch, "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
      *hist = '\0';
      sprintf(hist, "@CYou call @D[@R#@W%d@D] @G%s@n\r\n", call, !*arg2 ? "" : CAP(arg2));
      add_history(ch, hist, HIST_SNET);
    char over[MAX_STRING_LENGTH];
    sprintf(over, "@C$n@W says into $s scouter, '@G@G%s@W'@n\r\n", !*arg2 ? "" : CAP(arg2));
    act(over, TRUE, ch, 0, 0, TO_ROOM);
   if (reached == FALSE) {
    send_to_char(ch, "@mThe transmission didn't reach them.@n\r\n");
   }
  }
 } /* end if statement */
} /*end snet command */

ACMD(do_scouter)
{
    struct char_data *vict = NULL;
    struct descriptor_data *i;
    int count = 0;
    char arg[MAX_INPUT_LENGTH];

     one_argument(argument, arg);

    if (!HAS_ARMS(ch)) {
     send_to_char(ch, "You have no available arms!\r\n");
     return;
    }

    struct obj_data *obj = GET_EQ(ch, WEAR_EYE);
    if (!obj) {
     send_to_char(ch, "You do not even have a scouter!");
     obj = NULL;
     return;
    }
    else {
     if (!*arg) {
      send_to_char(ch, "[Syntax] scouter < target | scan>\r\n");
      return;
     }
     reveal_hiding(ch, 3);
     if (!strcasecmp("scan", arg)) {
        for (i = descriptor_list; i; i = i->next) {
         if (STATE(i) != CON_PLAYING) {
         continue;
         }
         else if (i->character == ch) {
           continue;
         }
         else if (IS_ANDROID(i->character)) {
           continue;
         }
         else if (planet_check(ch, i->character)) {
          int dir = find_first_step(IN_ROOM(ch), IN_ROOM(i->character));
          int same = FALSE;
          char pathway[MAX_STRING_LENGTH];
         
          if (IN_ZONE(ch) == IN_ZONE(i->character))
           same = TRUE;

          switch (dir) {
           case BFS_ERROR:
            sprintf(pathway, "@rERROR");
            break;
           case BFS_ALREADY_THERE:
            sprintf(pathway, "@RHERE");
            break;
           case BFS_NO_PATH:
            send_to_char(ch, "@MUNKNOWN");
            break;
           default:
            send_to_char(ch, "@G%s\r\n", dirs[dir]);
            break;
          }

          char *blah = sense_location(i->character);
          if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(i->character) >= 150000) {
           send_to_char(ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == TRUE ? pathway : blah);
          }
          else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(i->character) >= 5000000) {
           send_to_char(ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == TRUE ? pathway : blah);
          }
          else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(i->character) >= 15000000) {
           send_to_char(ch, "@D<@GPowerlevel Detected@D:@w ?????????@D> @w---> @C%s@n\r\n", same == TRUE ? pathway : blah);
          }
          else {
           send_to_char(ch, "@D<@GPowerlevel Detected@D: [@Y%s@D]@w ---> @C%s@n\r\n", add_commas(GET_HIT(i->character)), same == TRUE ? pathway : blah);
          }
          ++count;
          free(blah);
         }
       }
       if (count == 0) {
        send_to_char(ch, "You didn't detect anyone of notice.\r\n");
        return;
       }
       else if (count >= 1) {
        send_to_char(ch, "%d powerlevels detected.\r\n", count);
        return;
       }
     }
     if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "They don't seem to be here.\r\n");
      return;
     }
     if (IS_ANDROID(vict)) {
       act("$n points $s scouter at you.", FALSE, ch, 0, vict, TO_VICT);
       act("$n points $s scouter at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
	   send_to_char(ch, "@D,==================================|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D|@1@RReading target...                 @n@D|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D:                 @RERROR@n@D|@n\r\n");
       send_to_char(ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D:                 @RERROR@n@D|@n\r\n");
	   send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D:                 @RERROR@n@D|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D:                 @RERROR@n@D|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D`==================================|@n\r\n");
      return;
     }
     else {
      if (OBJ_FLAGGED(obj, ITEM_BSCOUTER) && GET_HIT(vict) >= 150000) {
       act("$n points $s scouter at you.", FALSE, ch, 0, vict, TO_VICT);
       act("$n points $s scouter at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
       perform_remove(ch, WEAR_EYE);
       send_to_char(ch, "Your scouter overloads and explodes!\r\n");
       act("$n's scouter explodes!", FALSE, ch, 0, 0, TO_ROOM);
       extract_obj(obj);
       save_char(ch);
       return;
      }
      else if (OBJ_FLAGGED(obj, ITEM_MSCOUTER) && GET_HIT(vict) >= 5000000) {
       act("$n points $s scouter at you.", FALSE, ch, 0, vict, TO_VICT);
       act("$n points $s scouter at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
       perform_remove(ch, WEAR_EYE);
       send_to_char(ch, "Your scouter overloads and explodes!\r\n");
       act("$n's scouter explodes!", FALSE, ch, 0, 0, TO_ROOM);
       extract_obj(obj);
       save_char(ch);
       return;
      }
      else if (OBJ_FLAGGED(obj, ITEM_ASCOUTER) && GET_HIT(vict) >= 15000000) {
       act("$n points $s scouter at you.", FALSE, ch, 0, vict, TO_VICT);
       act("$n points $s scouter at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
       perform_remove(ch, WEAR_EYE);
       send_to_char(ch, "Your scouter overloads and explodes!\r\n");
       act("$n's scouter explodes!", FALSE, ch, 0, 0, TO_ROOM);
       extract_obj(obj);
       save_char(ch);
       return;
      }
      else {
       long double percent = 0.0, cur = 0.0, max = 0.0;
       int64_t stam = GET_MOVE(vict), mstam = GET_MAX_MOVE(vict);
       if (stam <= 0)
        stam = 1;
       if (mstam <= 0)
        mstam = 1;

       cur = (long double)(stam);
       max = (long double)(mstam);

       percent = (cur / max) * 100;
       act("$n points $s scouter at you.", FALSE, ch, 0, vict, TO_VICT);
       act("$n points $s scouter at $N.", FALSE, ch, 0, vict, TO_NOTVICT);
       send_to_char(ch, "@D,==================================|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D|@1@RReading target...                 @n@D|@n\r\n");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D|@1@RP@r@1o@Rw@r@1e@1@Rr L@r@1e@Rv@r@1e@1@Rl@1@D: @Y%21s@n@D|@n\r\n", add_commas(GET_HIT(vict)));
       if (!IS_NPC(vict)) {
        send_to_char(ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(GET_CHARGE(vict)));
       } else if (IS_NPC(vict)) {
        send_to_char(ch, "@D|@1@CC@c@1ha@1@Cr@c@1ge@1@Cd Ki @1@D: @Y%21s@n@D|@n\r\n", add_commas(vict->mobcharge * rand_number(GET_LEVEL(ch) * 50, GET_LEVEL(ch) * 200)));
       }
       if (percent < 10)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Exhausted");
       else if (percent < 25)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Extremely Tired");
       else if (percent < 50)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Very Tired");
       else if (percent < 75)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Tired");
       else if (percent < 90)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Winded");
       else if (percent < 100)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Untired");
       else if (percent >= 100)
        send_to_char(ch, "@D|@1@YS@y@1ta@1@Ym@y@1in@1@Ya    @1@D: @Y%21s@n@D|@n\r\n", "Energetic");
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       int check = FALSE;
       send_to_char(ch, "@D|@1@GE@g@1x@Gt@g@1r@Ga I@g@1nf@Go @D: ");
       if (AFF_FLAGGED(vict, AFF_ZANZOKEN)) {
         send_to_char(ch, "@Y%21s@n@D|@n\n", "Zanzoken Prepared");
         check = TRUE;
       } if (AFF_FLAGGED(vict, AFF_HASS)) {
         send_to_char(ch, "%s@Y%21s@n@D|@n\n", check == TRUE ? "@D|@1             " : "", "Accelerated Arms");
         check = TRUE;
       } if (AFF_FLAGGED(vict, AFF_HEALGLOW)) {
         send_to_char(ch, "%s@Y%21s@n@D|@n\n", check == TRUE ? "@D|@1             " : "", "Healing Glow Prepared");
         check = TRUE;
       } if (AFF_FLAGGED(vict, AFF_POISON)) {
         send_to_char(ch, "%s@Y%21s@n@D|@n\n", check == TRUE ? "@D|@1             " : "", "Poisoned");
         check = TRUE;
       } if (PLR_FLAGGED(vict, PLR_SELFD)) {
         send_to_char(ch, "%s@Y%21s@n@D|@n\n", check == TRUE ? "@D|@1             " : "", "Explosive Energy");
         check = TRUE;
       } if (check == FALSE) {
         send_to_char(ch, "%s@Y%21s@n@D|@n\n", check == TRUE ? "@D|@1             " : "", "None Detected.");
       }
       send_to_char(ch, "@D|@1                                  @n@D|@n\r\n");
       send_to_char(ch, "@D`==================================|@n\r\n");
      }
     }
    }
}

int dball_count(struct char_data *ch) {

   int dball[7] = {20, 21, 22, 23, 24, 25, 26};
   int count = 0;
   struct obj_data *obj = NULL, *next_obj = NULL;

    for (obj = ch->carrying; obj; obj = next_obj) {
     next_obj = obj->next_content;
     if (GET_OBJ_VNUM(obj) == dball[0]) {
      dball[0] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[1]) {
      dball[1] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[2]) {
      dball[2] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[3]) {
      dball[3] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[4]) {
      dball[4] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[5]) {
      dball[5] = -1;
      count++;
      continue;
     }
     else if (GET_OBJ_VNUM(obj) == dball[6]) {
      dball[6] = -1;
      count++;
      continue;
     }
     else {
      continue;
     } // end if
    } // end repeat for

  if (count >= 1) {
   return 1;
  }
  else {
   return 0;
  }
}

ACMD(do_quit)
{
    if (IS_NPC(ch) || !ch->desc)
     return;


    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PAST)) {
      send_to_char(ch, "This is the past, you can't quit here!\r\n");
      return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 2002 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 2011) {
      send_to_char(ch, "You can't quit in the arena!\r\n");
      return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 101 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 139) {
      send_to_char(ch, "You can't quit in the mud school!\r\n");
      return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) >= 19800 && GET_ROOM_VNUM(IN_ROOM(ch)) <= 19899) {
      send_to_char(ch, "You can't quit in a pocket dimension!\r\n");
      return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2069) {
      send_to_char(ch, "You can't quit here!\r\n");
      return;
    }
    if (MINDLINK(ch) && LINKER(ch) == 0) {
     send_to_char(ch, "@RYou feel like the mind that is linked with yours is preventing you from quiting!@n\r\n");
     if (IN_ROOM(MINDLINK(ch)) != NOWHERE) {
      look_at_room(IN_ROOM(MINDLINK(ch)), ch, 0);
      send_to_char(ch, "You get an impression of where this interference is originating from.\r\n");
     }
     return;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2070) {
      send_to_char(ch, "You can't quit here!\r\n");
      return;
    }
    if (dball_count(ch)) {
     send_to_char(ch, "You can not quit while you have dragon balls! Place them somewhere first.");
     return;
    }

  if (subcmd != SCMD_QUIT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));
    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");
    if (ch->followers || ch->master)
     die_follower(ch);
    if (ch == ch_selling)
     stop_auction(AUC_QUIT_CANCEL, NULL);

    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    /* If someone is quitting in their house, let them load back here. */
    if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_PAST) && (GET_ROOM_VNUM(IN_ROOM(ch)) < 19800 || GET_ROOM_VNUM(IN_ROOM(ch)) > 19899)) {
     if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
     }
    }
    if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PAST)) {
     if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
       GET_LOADROOM(ch) = GET_ROOM_VNUM(real_room(1561));
     }
    }

    Crash_rentsave(ch, 0);
    cp(ch);

    extract_char(ch);		/* Char is saved before extracting. */
  }
  /* Remove any snoopers */
  if (ch->desc->snoop_by) {
    write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (CONFIG_AUTO_SAVE && GET_ADMLEVEL(ch) < 1) {
      send_to_char(ch, "Saving.\r\n");
      write_aliases(ch);
      save_char(ch);
      Crash_crashsave(ch);
      if (GET_ROOM_VNUM(IN_ROOM(ch)) < 19800 || GET_ROOM_VNUM(IN_ROOM(ch)) > 19899) {
       if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
        GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
 
       }
      }
      return;
    }
    send_to_char(ch, "Saving.\r\n");
  }

  write_aliases(ch);
  if (GET_ROOM_VNUM(IN_ROOM(ch)) < 19800 || GET_ROOM_VNUM(IN_ROOM(ch)) > 19899) {
   if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
   }
  }
  save_char(ch);
  Crash_crashsave(ch);
  cp(ch);

}

/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

ACMD(do_steal)
{

 if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 <= GET_SLOTS(ch)) {
  send_to_char(ch, "You learn the very veeeery basics of theft. Which is don't get caught.\r\n");
  SET_SKILL(ch, SKILL_SLEIGHT_OF_HAND, 1);
 } else if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 > GET_SLOTS(ch)) {
  send_to_char(ch, "You can't learn any more skills and thus can not steal right now!\r\n");
  return;
 }
 

 struct char_data *vict;
 struct obj_data *obj;
 char arg[500], arg2[500];
 int gold = 0, prob = GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND), perc = 0, eq_pos;

 two_arguments(argument, arg, arg2);

 if (!*arg) {
  send_to_char(ch, "An important basic of theft is actually having a victim!\r\n");
  return;
 }

 if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
  send_to_char(ch, "Steal what from who?\r\n");
  return;
 } else if (vict == ch) {
  send_to_char(ch, "Come on now, that's rather stupid!\r\n");
  return;
 } else if (!can_kill(ch, vict, NULL, 0)) {
  return;
 } else if (GET_LEVEL(ch) <= 8) {
  send_to_char(ch, "You are trapped inside the newbie shield until level 9 and can't piss off those bigger and better than you. Awww...\r\n");
  return;
 } else if (MOB_FLAGGED(vict, MOB_NOKILL) && GET_ADMLEVEL(ch) == ADMLVL_NONE) {
  send_to_char(ch, "That isn't such a good idea...\r\n");
  return;
 }

 if (GET_MOVE(ch) < (GET_MAX_MOVE(ch) / 40) + IS_CARRYING_W(ch)) {
  send_to_char(ch, "You do not have enough stamina.\r\n");
  return;
 }

 if (!IS_NPC(vict) && GET_SKILL(vict, SKILL_SPOT)) {
  perc = GET_SKILL(vict, SKILL_SPOT);
  perc += GET_INT(vict);
 } else {
  perc = rand_number(GET_INT(vict), GET_INT(vict) + 10);
  if (IS_NPC(vict))
   perc += GET_LEVEL(vict) * 0.25;
 }

 if (GET_POS(vict) == POS_SITTING)
  perc -= 5;
 if (GET_POS(vict) == POS_RESTING)
  perc -= 10;
 if (GET_POS(vict) <= POS_SLEEPING)
  perc -= 25; /* High chance to succeed. */

 prob += GET_DEX(ch);

 perc += rand_number(-5, 5); /* Random factor */
 prob += rand_number(-5, 5); /* Random factor */

 if (axion_dice(0) > 100 && GET_POS(vict) != POS_SLEEPING) { /* Unlucky! */
  reveal_hiding(ch, 0);
  act("@r$N@R just happens to glance in your direction! What terrible luck!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@RYou just happen to glance behind you and spot @r$n@R trying to STEAL from you!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@r$N@R just happens to glance in @r$n's@R direction and catches $m trying to STEAL!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  prob = -1000;
 }

 if (prob + 20 < perc && GET_POS(vict) != POS_SLEEPING) { /* Critical failure! */
  reveal_hiding(ch, 0);
  act("@rYou are caught trying to stick your hand in @R$N's@r possessions!@n", TRUE, ch, 0, vict, TO_CHAR);
  act("@rYou catch @R$n@r trying to rummage through your possessions!@n", TRUE, ch, 0, vict, TO_VICT);
  act("@R$n@R is caught by @R$N@r as $e sticks $s hand in @R$N's@r possessions!@n", TRUE, ch, 0, vict, TO_NOTVICT);
  WAIT_STATE(ch, PULSE_3SEC);
  if (IS_NPC(vict)) {
   set_fighting(vict, ch);
  }
  improve_skill(vict, SKILL_SPOT, 1);
 } else { /* Ok it wasn't a critical failure... */
  if (!strcasecmp(arg, "zenni")) { /* MOOLA! */
   if (prob > perc) {
    if (GET_GOLD(vict) > 0) {
     if (GET_GOLD(vict) > 100) {
      gold = (GET_GOLD(vict) / 100) * rand_number(1, 10);
     } else {
      gold = GET_GOLD(vict);
     }
     if (gold + GET_GOLD(ch) > GOLD_CARRY(ch)) {
      send_to_char(ch, "You can't hold that much more zenni on your person!\r\n");
      return;
     }
     GET_GOLD(vict) -= gold;
     GET_GOLD(ch) += gold;
     if (!IS_NPC(vict)) {
      SET_BIT_AR(PLR_FLAGS(vict), PLR_STOLEN);
      mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "THEFT: %s has stolen %s zenni@n from %s", GET_NAME(ch), add_commas(gold), GET_NAME(vict));
     }
     if (gold > 1)
      send_to_char(ch, "Bingo!  You got %d zenni.\r\n", gold);
     else
      send_to_char(ch, "You manage to swipe a solitary zenni.\r\n");
     if (axion_dice(0) > prob) {
       send_to_char(ch, "You think that your movements might have been a bit obvious.\r\n");
       reveal_hiding(ch, 0);
       act("@R$n@r just stole zenni from @R$N@r!@n", TRUE, ch, 0, vict, TO_ROOM);
       send_to_char(vict, "You feel like something may be missing...\r\n");
       if (IS_NPC(vict) && rand_number(1, 3) == 3) {
        set_fighting(vict, ch);
       }
       improve_skill(vict, SKILL_SPOT, 1);
     }
     improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);
     return;
    } else {
     send_to_char(ch, "It appears like they are broke...\r\n");
     return;
    }
   } else { /* Failure! */
    reveal_hiding(ch, 0);
    act("@rYou are caught trying to steal zenni from @R$N@r!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@rYou catch @R$n's@r hand trying to snatch your zenni!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@R$N@r catches @R$n's@r hand trying to snatch $S zenni!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    WAIT_STATE(ch, PULSE_3SEC);
    if (IS_NPC(vict)) {
     set_fighting(vict, ch);
    }
    improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
    improve_skill(vict, SKILL_SPOT, 1);
    return;
   }
  } else { /* It's an object... */
   if (!(obj = get_obj_in_list_vis(ch, arg, NULL, vict->carrying))) {
      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
        if (GET_EQ(vict, eq_pos) && (isname(arg, GET_EQ(vict, eq_pos)->name)) && CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
          obj = GET_EQ(vict, eq_pos);
          break;
        }
     if (!obj) {
      act("$E isn't wearing that item.", FALSE, ch, 0, vict, TO_CHAR);
      return;
     } else if (GET_POS(vict) > POS_SLEEPING) {
      send_to_char(ch, "Steal worn equipment from them while they are awake? That's a stupid idea...\r\n");
      return;
     } else if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj) ) {
      send_to_char(ch, "Impossible!\r\n");
      return;
     } else if (GET_OBJ_VNUM(obj) >= 20000) {
      send_to_char(ch, "You can't steal that!\r\n");
      return;
     } else if (GET_OBJ_VNUM(obj) >= 18800 && GET_OBJ_VNUM(obj) <= 18999) {
      send_to_char(ch, "You can't steal that!\r\n");
      return;
     } else if (GET_OBJ_VNUM(obj) >= 19100 && GET_OBJ_VNUM(obj) <= 19199) {
      send_to_char(ch, "You can't steal that!\r\n");
      return;
	 } else if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
      send_to_char(ch, "No stealing keys!\r\n");
      return;
	  } else if (OBJ_FLAGGED(obj, ITEM_NOSTEAL)) {
	   send_to_char(ch, "You can't steal that!\r\n");
	   return;
     } else if (GET_OBJ_WEIGHT(obj) + gear_weight(ch) > CAN_CARRY_W(ch)) {
      send_to_char(ch, "You can't carry that much weight.\r\n");
      return;
     } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
      send_to_char(ch, "You don't have the room for it right now!\r\n");
      return;
     } else if (prob > perc) { /* Right off their back :) */
      act("You unequip $p and steal it.", FALSE, ch, obj, vict, TO_CHAR);
      if (axion_dice(0) > prob) {
       send_to_char(ch, "You think that your movements might have been a bit obvious.\r\n");
       reveal_hiding(ch, 0);
       act("@R$n@r just stole $p@r from @R$N@r!@n", TRUE, ch, obj, vict, TO_ROOM);
       send_to_char(vict, "You feel your body being disturbed.\r\n");
       improve_skill(vict, SKILL_SPOT, 1);
      }
      obj_to_char(unequip_char(vict, eq_pos), ch);
      improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);      
      return;
     } else { /* Failure! */
      reveal_hiding(ch, 0);
      GET_POS(vict) = POS_SITTING;
      act("@rYou are caught trying to steal $p@r from @R$N@r!@n", TRUE, ch, obj, vict, TO_CHAR);
      act("@rYou feel your body being shifted while you sleep and wake up to find @R$n@r trying to steal $p@r from you!@n", TRUE, ch, obj, vict, TO_VICT);
      act("@R$N@r catches @R$n's@r trying to $p@r from $M during $S sleep!@n", TRUE, ch, obj, vict, TO_NOTVICT);
      WAIT_STATE(ch, PULSE_3SEC);
      if (IS_NPC(vict)) {
       GET_POS(vict) = POS_STANDING;
       set_fighting(vict, ch);
      }
      improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
      improve_skill(vict, SKILL_SPOT, 1);
      return;
     }
   } else {/* It's in their inventory */
     if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj) ) {
      send_to_char(ch, "Impossible!\r\n");
      return;
     } else if (GET_OBJ_VNUM(obj) >= 20000) {
      send_to_char(ch, "You can't steal that!\r\n");
      return;
     } else if (OBJ_FLAGGED(obj, ITEM_NOSTEAL)) {
	   send_to_char(ch, "You can't steal that!\r\n");
	   return;
	  } else if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
      send_to_char(ch, "No stealing keys!\r\n");
      return;
     } else if (GET_OBJ_WEIGHT(obj) + gear_weight(ch) > CAN_CARRY_W(ch)) {
      send_to_char(ch, "You can't carry that much weight.\r\n");
      return;
     } else if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
      send_to_char(ch, "You don't have the room for it right now!\r\n");
      return;
     } else if (prob > perc) { /* Right out of their pockets */
      act("You steal $p from $N.", FALSE, ch, obj, vict, TO_CHAR);
      obj_from_char(obj);
      obj_to_char(obj, ch);
      if (!IS_NPC(vict)) {
       SET_BIT_AR(PLR_FLAGS(vict), PLR_STOLEN);
       mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "THEFT: %s has stolen %s@n from %s", GET_NAME(ch), obj->short_description, GET_NAME(vict));
      }
      if (axion_dice(0) > prob) {
       reveal_hiding(ch, 0);
       send_to_char(ch, "You think that your movements might have been a bit obvious.\r\n");
       act("@R$n@r just stole $p@r from @R$N@r!@n", TRUE, ch, obj, vict, TO_ROOM);
       send_to_char(vict, "You feel like something may be missing...\r\n");
       if (IS_NPC(vict) && rand_number(1, 3) == 3) {
        set_fighting(vict, ch);
       }
       improve_skill(vict, SKILL_SPOT, 1);
      }
      improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 1);      
      return;
     } else { /* Failure! */
      reveal_hiding(ch, 0);
      act("@rYou are caught trying to steal $p@r from @R$N@r!@n", TRUE, ch, obj, vict, TO_CHAR);
      act("@rYou catch @R$n@r trying to steal $p@r from you!@n", TRUE, ch, obj, vict, TO_VICT);
      act("@R$N@r catches @R$n's@r trying to $p@r!@n", TRUE, ch, obj, vict, TO_NOTVICT);
      WAIT_STATE(ch, PULSE_3SEC);
      if (IS_NPC(vict)) {
       GET_POS(vict) = POS_STANDING;
       set_fighting(vict, ch);
      }
      improve_skill(ch, SKILL_SLEIGHT_OF_HAND, 2);
      improve_skill(vict, SKILL_SPOT, 1);
      return;
     }
   }
  } /* End of an object */
 }
}

ACMD(do_practice) {
  char arg[200];

  /* if (IS_NPC(ch))
    return; */

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills with your trainer.\r\n");
  else
    send_to_char(ch, "Use the skills command unless you are at your trainer.\r\n");
    /*list_skills(ch);*/
}

ACMD(do_skills)
{
    char arg[1000];
    one_argument(argument, arg);
    if (IS_NPC(ch)) {
     return;
    }
    list_skills(ch, arg);
}

ACMD(do_visible)
{
  int appeared = 0;

  if (GET_ADMLEVEL(ch)) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    appeared = 1;
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }

  if (AFF_FLAGGED(ch, AFF_ETHEREAL) && affectedv_by_spell(ch, ART_EMPTY_BODY)) {
    affectv_from_char(ch, ART_EMPTY_BODY);
    if (AFF_FLAGGED(ch, AFF_ETHEREAL)) {
      send_to_char(ch, "Returning to the material plane will not be so easy.\r\n");
    } else {
      send_to_char(ch, "You return to the material plane.\r\n");
      if (!appeared)
        act("$n flashes into existence.", FALSE, ch, 0, 0, TO_ROOM);
    }
    appeared = 1;
  }

  if (!appeared)
    send_to_char(ch, "You are already visible.\r\n");
}

ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
  }
}

static int perform_group(struct char_data *ch, struct char_data *vict, int highlvl, int lowlvl, int64_t highpl, int64_t lowpl)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  if (GET_BONUS(vict, BONUS_LONER) > 0) {
   act("$n is the loner type and refuses to be in your group.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (GET_LEVEL(vict) + 12 < highlvl) {
   act("$n isn't experienced enough to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (GET_LEVEL(vict) > lowlvl + 12) {
   act("$n is too experienced to be in your group with its current members.", TRUE, vict, 0, ch, TO_VICT);
   return (0);
  }

  if (highlvl >= 100) {
   if (gear_pl(vict) > highpl * 1.5) {
    act("$n is too powerful right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }

   if (gear_pl(vict) < lowpl * 0.5) {
    act("$n is too weak right now to be in a level 100 group with you.", TRUE, vict, 0, ch, TO_VICT);
    return (0);
   }
  }

  SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}

static void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      send_to_char(ch, "@D----------------@n\r\n");
      if (GET_HIT(k) > GET_MAX_HIT(k) / 10) {
      snprintf(buf, sizeof(buf), "@gL@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
	      add_commas(GET_HIT(k)), add_commas(GET_MANA(k)), add_commas(GET_MOVE(k)), GET_LEVEL(k),
              CLASS_ABBR(k), RACE_ABBR(k));
      }
      if (GET_HIT(k) <= (GET_MAX_HIT(k) - gear_weight(k)) / 10) {
      snprintf(buf, sizeof(buf), "@gL@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]@n",
              add_commas(GET_HIT(k)), add_commas(GET_MANA(k)), add_commas(GET_MOVE(k)), GET_LEVEL(k),
              CLASS_ABBR(k), RACE_ABBR(k));
      }
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;
      send_to_char(ch, "@D----------------@n\r\n");
      if (GET_HIT(f->follower) > (GET_MAX_HIT(f->follower) - gear_weight(f->follower)) / 10) {
      snprintf(buf, sizeof(buf), "@gF@D: @w$N @W- @D[@RPL@Y: @c%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
              add_commas(GET_HIT(f->follower)), add_commas(GET_MANA(f->follower)), add_commas(GET_MOVE(f->follower)),
	      GET_LEVEL(f->follower), CLASS_ABBR(f->follower), RACE_ABBR(f->follower));
      }
      if (GET_HIT(f->follower) <= (GET_MAX_HIT(f->follower) - gear_weight(f->follower)) / 10) {
      snprintf(buf, sizeof(buf), "@gF@D: @w$N @W- @D[@RPL@Y: @r%s @CKi@Y: @c%s @GST@Y: @c%s@D] [@w%2d %s %s@D]",
              add_commas(GET_HIT(f->follower)), add_commas(GET_MANA(f->follower)), add_commas(GET_MOVE(f->follower)),
              GET_LEVEL(f->follower), CLASS_ABBR(f->follower), RACE_ABBR(f->follower));
      }
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
      send_to_char(ch, "@D----------------@n\r\n");
  }
}

ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found, highlvl = 0, lowlvl = 0;
  int64_t highpl = 0, lowpl = 0;

  one_argument(argument, buf);

  if (GET_BONUS(ch, BONUS_LONER) > 0) {
   send_to_char(ch, "You can not group as you prefer to be alone.\r\n");
   return;
  }


  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You cannot enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  highlvl = GET_LEVEL(ch);
  lowlvl = GET_LEVEL(ch);
  highpl = gear_pl(ch);
  lowpl = gear_pl(ch);

  for (found = 0, f = ch->followers; f; f = f->next) {
   if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
    if (GET_LEVEL(f->follower) > highlvl) {
     highlvl = GET_LEVEL(f->follower);
    }
    if (GET_LEVEL(f->follower) < lowlvl) {
     lowlvl = GET_LEVEL(f->follower);
    }
   }
  }

  int foundwas = 0;  

  if (!strcasecmp(buf, "all")) {
    perform_group(ch, ch, GET_LEVEL(ch), GET_LEVEL(ch), highpl, lowpl);
    for (found = 0, f = ch->followers; f; f = f->next) {
      foundwas = found;
      found += perform_group(ch, f->follower, highlvl, lowlvl, highpl, lowpl);
      if (found > foundwas) {
        if (GET_LEVEL(f->follower) > highlvl)
         highlvl = GET_LEVEL(f->follower);
        else if (GET_LEVEL(f->follower) < lowlvl)
         lowlvl = GET_LEVEL(f->follower);
      }
    }
     if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP)) {
     if (!AFF_FLAGGED(ch, AFF_GROUP)) {
      send_to_char(ch, "You form a group, with you as leader.\r\n");
      SET_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
     }
      perform_group(ch, vict, highlvl, lowlvl, highpl, lowpl);
    }
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}

ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
        GET_GROUPKILLS(f->follower) = 0;
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    GET_GROUPKILLS(ch) = 0;
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_GROUP);
  GET_GROUPKILLS(tch) = 0;

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}

ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %"I64T"/%"I64T"H, %"I64T"/%"I64T"M, %"I64T"/%"I64T"V\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, TRUE, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, TRUE, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    GET_GOLD(ch) -= amount;
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) && f->follower != ch &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num > 0 && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    GET_GOLD(ch) += share;

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d zenni; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d zenni %s not splitable, so %s keeps the money.\r\n", rest, (rest == 1) ? "was" : "were", GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	GET_GOLD(f->follower) += share;
	send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d zenni among %d members -- %d zenni each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d zenni %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
  } else {
    send_to_char(ch, "How much zenni do you wish to split with your group?\r\n");
    return;
  }
}

ACMD(do_use)
{
  char buf[100], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item = NULL;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  if (!mag_item) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
        send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
        return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but in the
       *  function which handles 'quaff', 'recite', and 'use'.
       */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only swallow beans.\r\n");
      return;
    }
    if (IS_ANDROID(ch)) {
      send_to_char(ch, "You can't swallow beans, you are an android.\r\n");
      return;
    }
    if (OBJ_FLAGGED(mag_item, ITEM_FORGED)) {
     send_to_char(ch, "You can't swallow that, it is fake!\r\n");
     return;
    }
    if (OBJ_FLAGGED(mag_item, ITEM_BROKEN)) {
     send_to_char(ch, "You can't swallow that, it is broken!\r\n");
     return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if (IS_ANDROID(ch)) {
      send_to_char(ch, "You are not biological enough to use these, Tincan.\r\n");
      return;
    } else {
     switch (GET_OBJ_VNUM(mag_item))
     {
      case 381:
       if (GET_MOVE(ch) >= GET_MAX_MOVE(ch)) {
        send_to_char(ch, "Your stamina is full.\r\n");
        return;
       }
       act("@WYou place the $p@W against your chest and feel a rush of stamina as it automatically administers the dose.@n", TRUE, ch, mag_item, 0, TO_CHAR);
       act("@C$n@W places an $p@W against $s chest and a loud click is heard.@n", TRUE, ch, mag_item, 0, TO_ROOM);
       if (GET_SKILL(ch, SKILL_FIRST_AID) > 0) {
        send_to_char(ch, "@CYour skill in First Aid has helped increase the use of the injector. You gain more stamina as a result.@n\r\n");
        GET_MOVE(ch) += GET_MAX_MOVE(ch) * 0.25;
        if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
         GET_MOVE(ch) = GET_MAX_MOVE(ch);
       } else {
        GET_MOVE(ch) += GET_MAX_MOVE(ch) * 0.1;
        if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
         GET_MOVE(ch) = GET_MAX_MOVE(ch);
       }
       extract_obj(mag_item);
       return;
      case 382:
       if (AFF_FLAGGED(ch, AFF_BURNED)) {
        act("@WYou gently apply the salve to your burns.@n", TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W gently applies a burn salve to $s burns.@n", TRUE, ch, mag_item, 0, TO_ROOM);
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_BURNED);
        extract_obj(mag_item);
       } else {
        send_to_char(ch, "You are not burned.\r\n");
       }
       return;
      case 383:
       if (AFF_FLAGGED(ch, AFF_POISON)) {
        act("@WYou place the $p@W against your neck and feel a rush of relief as the antitoxiin enters your bloodstream.@n", TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W places an $p@W against $s neck and a loud click is heard.@n", TRUE, ch, mag_item, 0, TO_ROOM);
        null_affect(ch, AFF_POISON);
        extract_obj(mag_item);
       } else {
        send_to_char(ch, "You are not poisoned.\r\n");
       }
       return;
      case 385:
        act("@WYou drink the contents of the vial before disposing of it.@n", TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W dinks a $p and then disposes of it.@n", TRUE, ch, mag_item, 0, TO_ROOM);
       if (AFF_FLAGGED(ch, AFF_BLIND)) {
        act("@WYour eyesight has returned!@n", TRUE, ch, mag_item, 0, TO_CHAR);
        act("@C$n@W eyesight seems to have returned.@n", TRUE, ch, mag_item, 0, TO_ROOM);
        null_affect(ch, AFF_BLIND);
       }
        int refreshed = FALSE;
        if (GET_HIT(ch) <= gear_pl(ch) * 0.99) {
         GET_HIT(ch) += large_rand(gear_pl(ch) * 0.08, gear_pl(ch) * 0.16);
         if (GET_HIT(ch) > gear_pl(ch))
          GET_HIT(ch) = gear_pl(ch);
         refreshed = TRUE;
        } else if (GET_MANA(ch) <= gear_pl(ch) * 0.99) {
         GET_MANA(ch) += large_rand(GET_MAX_MANA(ch) * 0.08, GET_MAX_MANA(ch) * 0.16);
         if (GET_MANA(ch) > GET_MAX_MANA(ch))
          GET_MANA(ch) = GET_MAX_MANA(ch);
         refreshed = TRUE;
        } else if (GET_MOVE(ch) <= GET_MAX_MOVE(ch) * 0.99) {
         GET_MOVE(ch) += large_rand(GET_MAX_MOVE(ch) * 0.08, GET_MAX_MOVE(ch) * 0.16);
         if (GET_MOVE(ch) > GET_MAX_MOVE(ch))
          GET_MOVE(ch) = GET_MAX_MOVE(ch);
         refreshed = TRUE;
        }
        if (refreshed == TRUE) {
         send_to_char(ch, "@CYou feel refreshed!\r\n");
        }
        extract_obj(mag_item);
       return;
      default:
       send_to_char(ch, "That is not something you can apparently use.\r\n");
       return;
     }
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_value)
{
  char arg[MAX_INPUT_LENGTH];
  int value_lev;

  one_argument(argument, arg);

  if (!*arg) {
    switch (subcmd) {
    case SCMD_WIMPY:
        if (GET_WIMP_LEV(ch)) {
          send_to_char(ch, "Your current wimp level is %d powerlevel.\r\n", GET_WIMP_LEV(ch));
          return;
        } else {
          send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
          return;
        }
      break;
    }
  }

  if (isdigit(*arg)) {
    switch (subcmd) {
    case SCMD_WIMPY:
      /* 'wimp_level' is a player_special. -gg 2/25/98 */
      if (IS_NPC(ch))
        return;
      if ((value_lev = atoi(arg)) != 0) {
        if (value_lev < 0)
	  send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (value_lev > GET_MAX_HIT(ch))
	  send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
        else if (value_lev > (GET_MAX_HIT(ch) * 0.5))
	  send_to_char(ch, "You can't set your wimp level above half your powerlevel.\r\n");
        else {
	  send_to_char(ch, "Okay, you'll wimp out if you drop below %d powerlevel.\r\n", value_lev);
	  GET_WIMP_LEV(ch) = value_lev;
        }
      } else {
        send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
        GET_WIMP_LEV(ch) = 0;
      }
      break;
    default:
      log("Unknown subcmd to do_value %d called by %s", subcmd, GET_NAME(ch));
      break;
    }
  } else
    send_to_char(ch, "Specify a value.  (0 to disable)\r\n");
}

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { P | K | T | S | F | H | G | L | C | M | all/on | none/off }\r\n");
    return;
  }

  if (!strcasecmp(argument, "on") || !strcasecmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_FURY);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISTIME);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISGOLD);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPRAC);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISHUTH);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPERC);
  } else if (!strcasecmp(argument, "off") || !strcasecmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FURY);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISTIME);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISGOLD);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPRAC);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISHUTH);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPERC);
  } else {
    /*REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);*/

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'p':
	TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 's':
	TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      case 'k':
	TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
	break;
      case 't':
	TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPTNL);
	break;
      case 'h':
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISTIME);
        break;
      case 'g':
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISGOLD);
        break;
      case 'l':
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPRAC);
        break;
      case 'c':
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPERC);
        break;
      case 'm':
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISHUTH);
        break;
      case 'f':
        if (!IS_HALFBREED(ch)) {
         send_to_char(ch, "Only halfbreeds use fury.\r\n");
        }
        TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_FURY);
        break;
      default:
	send_to_char(ch, "Usage: prompt { P | K | T | S | F | H | G | L | all/on | none/off }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }
  send_to_imm("[A new %s has been filed by: %s]\r\n", CMD_NAME, GET_NAME(ch));

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
     *  This is from do_gen_write() and indicates that it cannot call the
     *  stat() system call on the file required.  The error string at the
     *  end of the line should explain what the problem is.
     */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    /*  SYSERR_DESC:
     *  This is from do_gen_write(), and will be output if the file in
     *  question cannot be opened for appending to.  The error string
     *  at the end of the line should explain what the problem is.
     */

    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  fprintf(fl, "@D[@WUser: @c%-10s@D] [@WChar: @C%-10s@D] [@WRoom: @G%-4d@D] [@WDate: @Y%6.6s@D]@b \n-----------@w\n%s\n", GET_USER(ch) ? GET_USER(ch) : "ERR", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), (tmp + 4), argument);
  fprintf(fl, "@D-------------------------------@n\n");
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}

#define TOG_OFF 0
#define TOG_ON  1

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear newbie.\r\n",
    "You are now deaf to newbie.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear ooc.\r\n",
    "You are now deaf to ooc.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
     "You will now Auto-Assist.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autosac disabled.\r\n",
    "Autosac enabled.\r\n"},
    {"You will no longer attempt to be sneaky.\r\n",
    "You will try to move as silently as you can.\r\n"},
    {"You will no longer attempt to stay hidden.\r\n",
    "You will try to stay hidden.\r\n"},
    {"You will no longer automatically memorize spells in your list.\r\n",
    "You will automatically memorize spells in your list.\r\n"},
    {"Viewing newest board messages at top of list.\r\n",
      "Viewing newest board messages at bottom of list.\r\n"},
    {"Compression will be used if your client supports it.\r\n",
     "Compression will not be used even if your client supports it.\r\n"},
    {"",
     ""},
    {"You are no longer hidden from view on the who list and public channels.\r\n",
      "You are now hidden from view on the who list and public channels.\r\n"},
    {"You will now be told that you have mail on prompt.\r\n",
     "You will no longer be told that you have mail on prompt.\r\n"},
    {"You will no longer receive automatic hints.\r\n",
     "You will now receive automatic hints.\r\n"},
    {"Screen Reader Friendly Mode Deactivated.\r\n",
     "Screen Reader Friendly Mode Activated..\r\n"},
    {"You will now see equipment when looking at someone.\r\n",
     "You will no longer see equipment when looking at someone.\r\n"},
    {"You will now listen to the music channel.\r\n",
     "You will no longer listen to the music channel.\r\n"},
    {"You will now parry attacks.\r\n",
     "You will no longer parry attacks.\r\n"},
    {"You will no longer keep cybernetic limbs with death.\r\n",
     "You will now keep cybernetic limbs with death.\r\n"},
    {"You will no longer worry about acquiring steaks from animals.\r\n",
     "You will now acquire steaks from animal if you can.\r\n"},
    {"You will now accept things being given to you.\r\n",
     "You will no longer accept things being given to you.\r\n"},
    {"You will no longer instruct those you spar with.\r\n",
     "You will now instruct those you spar with.\r\n"},
    {"You will no longer view group health.\r\n",
     "You will now view group health.\r\n"},
    {"You will no longer view item health.\r\n",
     "You will now view item health.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
    result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) {
      send_to_char(ch, "Immortals only, sorry.\r\n");  	
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK))
      mudlog(CMP, GET_LEVEL(ch), TRUE, 
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC: 
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC); 
    break; 
  case SCMD_SNEAK:
    result = AFF_TOG_CHK(ch, AFF_SNEAK);
    break;
  case SCMD_HIDE:
    if ((GET_CHARGE(ch) > 0 && GET_PREFERENCE(ch) != PREFERENCE_KI) || (GET_CHARGE(ch) > GET_MAX_MANA(ch) * 0.1 && GET_PREFERENCE(ch) == PREFERENCE_KI) || PLR_FLAGGED(ch, PLR_POWERUP) || AFF_FLAGGED(ch, AFF_FLYING)) {
     send_to_char(ch, "You stand out too much to hide right now!\r\n");
     return;
    } else if (PLR_FLAGGED(ch, PLR_HEALT)) {
      send_to_char(ch, "You are inside a healing tank!\r\n");
      return;
	}
    if (!GET_SKILL(ch, SKILL_HIDE) && slot_count(ch) + 1 <= GET_SLOTS(ch)) {
     send_to_char(ch, "@GYou learn the very minimal basics to hiding.@n\r\n");
     SET_SKILL(ch, SKILL_HIDE, rand_number(1, 5));
    } else if (!GET_SKILL(ch, SKILL_HIDE) && slot_count(ch) + 1 > GET_SLOTS(ch)) {
     send_to_char(ch, "@RYou need more skill slots in order to learn this skill.@n\r\n");
     return;
    }
    result = AFF_TOG_CHK(ch, AFF_HIDE);
    break;
  case SCMD_AUTOMEM: 
    result = PRF_TOG_CHK(ch, PRF_AUTOMEM); 
    break; 
  case SCMD_VIEWORDER:
    result = PRF_TOG_CHK(ch, PRF_VIEWORDER);
    break;
  case SCMD_TEST:
    if (GET_ADMLEVEL(ch) >= 1) {
     TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_TEST);
     send_to_char(ch, "Okay. Testing is now: %s\r\n", PRF_FLAGGED(ch, PRF_TEST) ? "On" : "Off");
     if (PRF_FLAGGED(ch, PRF_TEST)) {
      send_to_char(ch, "Make sure to remove nohassle as well.\r\n");
     }
     return;
    }
    else {
     send_to_char(ch, "You are not an immortal.\r\n");
     return;
    }
    break;
  case SCMD_NOCOMPRESS:
    if (CONFIG_ENABLE_COMPRESSION) {
      result = PRF_TOG_CHK(ch, PRF_NOCOMPRESS);
      break;
    } else {
      send_to_char(ch, "Sorry, compression is globally disabled.\r\n");
    }
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_WHOHIDE:
    result = PRF_TOG_CHK(ch, PRF_HIDE);
    break;
  case SCMD_NMWARN:
    result = PRF_TOG_CHK(ch, PRF_NMWARN);
    break;
  case SCMD_HINTS:
    result = PRF_TOG_CHK(ch, PRF_HINTS);
    break;
  case SCMD_NODEC:
    result = PRF_TOG_CHK(ch, PRF_NODEC);
    break;
  case SCMD_NOEQSEE:
    result = PRF_TOG_CHK(ch, PRF_NOEQSEE);
    break;
  case SCMD_NOMUSIC:
    result = PRF_TOG_CHK(ch, PRF_NOMUSIC);
    break;
  case SCMD_NOPARRY:
    result = PRF_TOG_CHK(ch, PRF_NOPARRY);
    break;
  case SCMD_LKEEP:
    result = PRF_TOG_CHK(ch, PRF_LKEEP);
    break;
  case SCMD_CARVE:
    result = PRF_TOG_CHK(ch, PRF_CARVE);
    break;
  case SCMD_NOGIVE:
    result = PRF_TOG_CHK(ch, PRF_NOGIVE);
    break;
  case SCMD_INSTRUCT:
    result = PRF_TOG_CHK(ch, PRF_INSTRUCT);
    break;
  case SCMD_GHEALTH:
    result = PRF_TOG_CHK(ch, PRF_GHEALTH);
    break;
  case SCMD_IHEALTH:
    result = PRF_TOG_CHK(ch, PRF_IHEALTH);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    /*  SYSERR_DESC:
     *  This is the same as the unhandled case in do_gen_ps(), but in the
     *  function which handles 'compact', 'brief', and so forth.
     */
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
  num_lines = 0,
  req_lines = 0,
  i,
  j;
  int l;
  char field[100], value[100], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
 } fields[] = {
     { "none",           6,    "Does Nothing" },
     { "bug",            ADMLVL_IMMORT,    "../lib/misc/bugs"},
     { "typo",           ADMLVL_IMMORT,   "../lib/misc/typos"},
     { "report",         ADMLVL_IMMORT,    "../lib/misc/ideas"},
     { "xnames",         4,     "../lib/misc/xnames"},
     { "levels",         4,    "../log/levels" },
     { "rip",            4,    "../log/rip" },
     { "players",        4,    "../log/newplayers" },
     { "rentgone",       4,    "../log/rentgone" },
     { "errors",         4,    "../log/errors" },
     { "godcmds",        4,    "../log/godcmds" },
     { "syslog",         ADMLVL_IMMORT,    "../syslog" },
     { "crash",          ADMLVL_IMMORT,    "../syslog.CRASH" },
     { "immlog",         ADMLVL_IMMORT,    "../lib/misc/request" },
     { "customs",        ADMLVL_IMMORT,    "../lib/misc/customs" },
     { "todo",           5,    "../todo" },
     { "\n", 0, "\n" }
};

   skip_spaces(&argument);

   if (!*argument) {
     strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
     for (j = 0, i = 1; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         sprintf(buf+strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     send_to_char(ch, buf);
     return;
   }

   two_arguments(argument, field, value);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
     if (!strncmp(field, fields[l].cmd, strlen(field)))
     break;

   if(*(fields[l].cmd) == '\n') {
     send_to_char(ch, "That is not a valid option!\r\n");
     return;
   }

   if (GET_ADMLEVEL(ch) < fields[l].level) {
     send_to_char(ch, "You are not godly enough to view that file!\r\n");
     return;
   }

   if (!strcasecmp(field, "request")) {
    GET_BOARD(ch, 2) = time(0);
   }

   if(!*value)
     req_lines = 15; /* default is the last 15 lines */
   else
     req_lines = atoi(value);
   
   if (!(req_file=fopen(fields[l].file,"r"))) {
     mudlog(BRF, ADMLVL_IMPL, TRUE,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }

   get_line(req_file,line);
   while (!feof(req_file)) {
     num_lines++;
     get_line(req_file,line);
   }
   rewind(req_file);

   req_lines = MIN(MIN(req_lines, num_lines),5000);
   
   buf[0] = '\0';

   get_line(req_file,line);
   while (!feof(req_file)) {
     cur_line++;
     if(cur_line > (num_lines - req_lines)) 
       sprintf(buf+strlen(buf),"%s\r\n", line);
	   
     get_line(req_file,line);
   }
   fclose(req_file);

   page_string(ch->desc, buf, 1);

}

ACMD(do_compare)
{
  char arg1[100];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1, *obj2;
  struct char_data *tchar;
  int value1 = 0, value2 = 0, o1, o2;
  char *msg = NULL;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Compare what to what?\n\r");
    return;
  }

  o1 = generic_find(arg1, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  o2 = generic_find(arg2, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj2);

  if (!o1 || !o2) {
    send_to_char(ch, "You do not have that item.\r\n");
    return;
  }
  if ( obj1 == obj2 ) {
    msg = "You compare $p to itself.  It looks about the same.";
  } else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2)) {
    msg = "You can't compare $p and $P.";
  } else {
    switch ( GET_OBJ_TYPE(obj1) ) {
      default:
      msg = "You can't compare $p and $P.";
      break;
      case ITEM_ARMOR:
      value1 = GET_OBJ_VAL(obj1, VAL_ARMOR_APPLYAC);
      value2 = GET_OBJ_VAL(obj2, VAL_ARMOR_APPLYAC);
      break;
      case ITEM_WEAPON:
      value1 = (1 + GET_OBJ_VAL(obj1, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj1, VAL_WEAPON_DAMDICE);
      value2 = (1 + GET_OBJ_VAL(obj2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj2, VAL_WEAPON_DAMDICE);
      break;
    }
  }

  if ( msg == NULL ) {
    if ( value1 == value2 )
      msg = "$p and $P look about the same.";
    else if ( value1  > value2 )
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act( msg, FALSE, ch, obj1, obj2, TO_CHAR );
  return;
}

ACMD(do_break)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int cmbrk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you break SOMETHING.\r\n");
    return;
  }

  if (!(cmbrk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to break!\r\n");
    return;
  }


  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, "Seems like it's already broken!\r\n");
    return;
  }

  /* Ok, break it! */
  send_to_char(ch, "You ruin %s.\r\n", obj->short_description);
  act("$n ruins $p.", FALSE, ch, obj, 0, TO_ROOM);
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 0;
  TOGGLE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);

  return;
}

ACMD(do_fix)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *obj4 = NULL, *rep, *next_obj;
  struct char_data *dummy = NULL;
  int cmbrk, found = FALSE, self = FALSE, custom = FALSE;

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_REPAIR)) {
   return;
  }

  if (!*arg) {
    send_to_char(ch, "Usually you fix SOMETHING.\r\n");
    return;
  }

  if (!strcasecmp("self", arg)) {
   if (!IS_ANDROID(ch)) {
    send_to_char(ch, "Only androids can fix their bodies with repair kits.\r\n");
    return;
   } else {
    self = TRUE;
   }
  }

  if (self == FALSE) {
   if (!(cmbrk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP|FIND_OBJ_ROOM, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to fix!\r\n");
    return;
   }

   if ((cmbrk) && GET_OBJ_VAL(obj, VAL_ALL_HEALTH) >= 100) {
    send_to_char(ch, "But it isn't even damaged!\r\n");
    return;
   }

   if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
    send_to_char(ch, "That is fake, why bother fixing it?\r\n");
    return;
   }

   switch (GET_OBJ_VAL(obj, VAL_ALL_MATERIAL)) {
    case MATERIAL_ORGANIC:
    case MATERIAL_FOOD:
    case MATERIAL_PAPER:
    case MATERIAL_LIQUID:
     send_to_char(ch, "You can't repair that.\r\n");
     return;
     break;
   }

   if (GET_OBJ_VNUM(obj) == 20099 || GET_OBJ_VNUM(obj) == 20098) {
    custom = TRUE;
   }
  }


  for (rep = ch->carrying; rep; rep = next_obj) {
       next_obj = rep->next_content;
   if (custom == FALSE) {
    if (found == FALSE && GET_OBJ_VNUM(rep) == 48 && (!OBJ_FLAGGED(rep, ITEM_BROKEN) && !OBJ_FLAGGED(rep, ITEM_FORGED))) {
     found = TRUE;
     obj4 = rep;
    }
   } else {
    if (found == FALSE && GET_OBJ_VNUM(rep) == 13593 && (!OBJ_FLAGGED(rep, ITEM_BROKEN) && !OBJ_FLAGGED(rep, ITEM_FORGED))) {
     found = TRUE;
     obj4 = rep;
    }
   }
  }

  if (found == FALSE && custom == FALSE) {
   send_to_char(ch, "You do not even have a repair kit.\r\n");
   return;
  } else if (found == FALSE && custom == TRUE) {
   send_to_char(ch, "You do not even have a Nano-tech Repair Orb.\r\n");
   return;
  }

  if (self == FALSE) {
   if (GET_SKILL(ch, SKILL_REPAIR) < axion_dice(0)) {
    act("You try to repair $p but screw up..", TRUE, ch, obj, 0, TO_CHAR);
    act("$n tries to repair $p but screws up..", TRUE, ch, obj, 0, TO_ROOM);
    extract_obj(obj4);
    improve_skill(ch, SKILL_REPAIR, 1);
    WAIT_STATE(ch, PULSE_2SEC);
    return;
   }

  
   if (GET_OBJ_VAL(obj, VAL_ALL_HEALTH) + GET_SKILL(ch, SKILL_REPAIR) < 100) {
    send_to_char(ch, "You repair %s a bit.\r\n", obj->short_description);
    act("$n repairs $p a bit.", FALSE, ch, obj, 0, TO_ROOM);
    GET_OBJ_VAL(obj, VAL_ALL_HEALTH) += GET_SKILL(ch, SKILL_REPAIR);
    REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
   }
   else {
    send_to_char(ch, "You repair %s completely.\r\n", obj->short_description);
    act("$n repairs $p completely.", FALSE, ch, obj, 0, TO_ROOM);
    GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = 100;
    REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);
   }
   if (obj->carried_by == NULL && !PLR_FLAGGED(ch, PLR_REPLEARN) && (level_exp(ch, GET_LEVEL(ch) + 1) - GET_EXP(ch) > 0 || GET_LEVEL(ch) >= 100)) {
    int64_t gain = (level_exp(ch, GET_LEVEL(ch) + 1) * 0.0003) * GET_SKILL(ch, SKILL_REPAIR);
    send_to_char(ch, "@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+%s@D]@n\r\n", add_commas(gain));
    SET_BIT_AR(PLR_FLAGS(ch), PLR_REPLEARN);
    gain_exp(ch, gain);
   } else if (rand_number(2, 12) >= 10 && PLR_FLAGGED(ch, PLR_REPLEARN)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_REPLEARN);
    send_to_char(ch, "@mYou think you might be on to something...@n\r\n");
   }
   improve_skill(ch, SKILL_REPAIR, 1);
   extract_obj(obj4);
   WAIT_STATE(ch, PULSE_2SEC);
   return;
 } else { /* For androids repairing themselves */

   if (GET_HIT(ch) >= gear_pl(ch)) {
    send_to_char(ch, "Your body is already in peak condition.\r\n");
    return; 
   } else if (GET_SKILL(ch, SKILL_REPAIR) < axion_dice(0)) {
    act("You try to repair your body but screw up..", TRUE, ch, 0, 0, TO_CHAR);
    act("$n tries to repair $s body but screws up..", TRUE, ch, 0, 0, TO_ROOM);
    extract_obj(obj4);
    improve_skill(ch, SKILL_REPAIR, 1);
    WAIT_STATE(ch, PULSE_5SEC);
    return;
   } else {
    act("You use the repair kit to fix part of your body...", TRUE, ch, 0, 0, TO_CHAR);
    act("$n works on their body with a repair kit.", TRUE, ch, 0, 0, TO_ROOM);
    int64_t mult = GET_SKILL(ch, SKILL_REPAIR);
    int64_t add = ((gear_pl(ch) * 0.005) + 10) * mult;
    GET_HIT(ch) += add;
    extract_obj(obj4);
    if (GET_HIT(ch) > gear_pl(ch)) {
     GET_HIT(ch) = gear_pl(ch);
     send_to_char(ch, "Your body has been totally repaired.\r\n");
     WAIT_STATE(ch, PULSE_5SEC);
    } else {
     send_to_char(ch, "Your body still needs some work done to it.\r\n");
     WAIT_STATE(ch, PULSE_5SEC);
    }
   }
 }
}

/* new spell memorization code */
/* remove a spell from a character's innate linked list */
void innate_remove(struct char_data * ch, struct innate_node * inn)
{
  struct innate_node *temp;

  if (ch->innate == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(inn, ch->innate, next, temp);
  free(inn);
}

void innate_add(struct char_data * ch, int innate, int timer)
{
  struct innate_node * inn;

  CREATE(inn, struct innate_node, 1);
  inn->timer = timer;
  inn->spellnum = innate;
  inn->next = ch->innate;
  ch->innate = inn;
}

/* Returns true if the spell/skillnum is innate to the character
   (as opposed to just practiced).
 */
int is_innate(struct char_data *ch, int spellnum) 
{
  switch(spellnum) {
  case SPELL_FAERIE_FIRE:
    if(GET_RACE(ch) == RACE_ANIMAL)
      return TRUE;
    break;
  }
  return FALSE;
}

/* returns FALSE if the spell is found in the innate linked list. 
  This just means the ability is ready IF they can even get the
  ability. (see is_innate). 
*/
int is_innate_ready(struct char_data *ch, int spellnum)
{
  struct innate_node *inn, *next_inn;

  for(inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if(inn->spellnum == spellnum)
      return FALSE;
  }
  return TRUE;
}

/* Adds a node to the linked list with a timer which indicates
   that the innate skill/spell is NOT ready to be used.

   is_innate_ready(...) should be called first to determine if
   there is already a timer set for the spellnum.
*/
void add_innate_timer(struct char_data *ch, int spellnum) 
{
  int timer = 6; /* number of ticks */

  switch(spellnum) {
    case SPELL_FAERIE_FIRE:
      timer = 6;   
      break;
    case ABIL_LAY_HANDS:
      timer = 12;
      break;
  }
  if(is_innate_ready(ch, spellnum)) {
    innate_add(ch, spellnum, timer);
  } else {
    send_to_char(ch, "BUG!\r\n");
  }
}


/* called when a player enters the game to set permanent affects.
   Usually these will stay set, but certain circumstances will
   cause them to wear off (ie. removing eq with a perm affect).
*/
void add_innate_affects(struct char_data *ch) 
{
  switch(GET_RACE(ch)) {
  case RACE_DEMON:
  case RACE_ICER:
  case RACE_ANDROID:
  case RACE_BIO:
    affect_modify(ch, APPLY_NONE, 0, 0, AFF_INFRAVISION, TRUE);
    break;
  }
  affect_total(ch);
}

/* Called to update the innate timers */
void update_innate(struct char_data *ch)
{
  struct innate_node *inn, *next_inn;

  for (inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if (inn->timer > 0) { 
      inn->timer--;
    } else {
      switch(inn->spellnum) {
      case ABIL_LAY_HANDS:
        send_to_char(ch, "Your special healing abilities have returned.\r\n");
        break;
      default:
        send_to_char(ch, "You are now able to use your innate %s again.\r\n", spell_info[inn->spellnum].name);
        break;
      }  
      innate_remove(ch, inn);
    }
  } 
}

static int spell_in_book(struct obj_data *obj, int spellnum)
{
  int i;
  bool found = FALSE;

  if (!obj->sbinfo)
    return FALSE;

  for (i=0; i < SPELLBOOK_SIZE; i++)
    if (obj->sbinfo[i].spellname == spellnum) {
      found = TRUE;
      break;
    }

  if (found)
    return 1;

  return 0;
}

static int spell_in_scroll(struct obj_data *obj, int spellnum)
{
  if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) == spellnum)
    return TRUE;

  return FALSE;
}

static int spell_in_domain(struct char_data *ch, int spellnum)
{
  if (spell_info[spellnum].domain == DOMAIN_UNDEFINED) {
    return FALSE;
  }

  return TRUE;
}


const room_vnum freeres[NUM_ALIGNS] = {
/* LAWFUL_GOOD */	1000,
/* NEUTRAL_GOOD */	1000,
/* CHAOTIC_GOOD */	1000,
/* LAWFUL_NEUTRAL */	1000,
/* NEUTRAL_NEUTRAL */	1000,
/* CHAOTIC_NEUTRAL */	1000,
/* LAWFUL_EVIL */	1000,
/* NEUTRAL_EVIL */	1000,
/* CHAOTIC_EVIL */	1000
};


ACMD(do_resurrect)
{
  room_rnum rm;
  struct affected_type *af, *next_af;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!AFF_FLAGGED(ch, AFF_SPIRIT)) {
    send_to_char(ch, "But you're not even dead!\r\n");
    return;
  }

  send_to_char(ch, "You take an experience penalty and pray for charity resurrection.\r\n");
  gain_exp(ch, -(level_exp(ch, GET_LEVEL(ch)) - level_exp(ch, GET_LEVEL(ch) - 1)));

  for (af = ch->affected; af; af = next_af) {
    next_af = af->next;
    if (af->location == APPLY_NONE && af->type == -1 &&
        (af->bitvector == AFF_SPIRIT || af->bitvector == AFF_ETHEREAL))
      affect_remove(ch, af);
  }

  if (GET_HIT(ch) < 1)
    GET_HIT(ch) = 1;

  if ((rm = real_room(freeres[ALIGN_TYPE(ch)])) == NOWHERE)
    rm = real_room(CONFIG_MORTAL_START);

  if (rm != NOWHERE) {
    char_from_room(ch);
    char_to_room(ch, rm);
    look_at_room(IN_ROOM(ch), ch, 0);
  }

  act("$n's body forms in a pool of @Bblue light@n.", TRUE, ch, 0, 0, TO_ROOM);
}

static void show_clan_info(struct char_data *ch) {

  send_to_char(ch,   "@c----------------------------------------\r\n"
                     "@c|@WProvided by@D: @YAlister of Aeonian Dreams@c|\r\n"
                     "@c|@YWith many changes made by Iovan.      @c|\r\n"
                     "@c----------------------------------------@w\r\n"
                     "  Commands are:\r\n"
                     "@c--@YClan Members Only@c-----@w\r\n"
                     "  clan members\r\n"
                     "  clan bank\r\n"
                     "  clan deposit\r\n"
                     "  clan leave    <clan>\r\n"
                     "@c--@YClan Mod/Highrank@c-----@w\r\n"
                     "  clan decline  <person> <clan>\r\n"
                     "  clan enroll   <person> <clan>\r\n"
                     "@c--@YClan Moderators Only@c--@w\r\n"
                     "  clan withdraw\r\n"
                     "  clan infow\r\n"
                     "  clan setjoin  <free | restricted> <clan>\r\n"
                     "  clan setleave <free | restricted> <clan>\r\n"
                     "  clan expel    <person> <clan>\r\n"
                     "  clan highrank <new highrank title>\r\n"
                     "  clan midrank  <new midrank title>\r\n"
                     "  clan rank     <person> < 0 / 1 / or 2>\r\n"
                     "  clan makemod  <person> <clan>\r\n"
                     "@c--@YAnyone@c----------------@w\r\n"
                     "  clan list\r\n"
		     "  clan info     <clan>\r\n"
		     "  clan apply    <clan>\r\n"
		     "  clan join     <clan>\r\n");
  if(GET_ADMLEVEL(ch) >= ADMLVL_IMPL)
    send_to_char(ch, "@c--@YImmort@c----------------@w\r\n"
                     "  clan create   <clan>\r\n"
		     "  clan destroy  <clan>\r\n"
		     "  clan reload   <clan>\r\n"
                     "  clan bset     <clan>\n");

}

ACMD(do_clan) {

  char arg1[100];
  char arg2[MAX_INPUT_LENGTH];

  if(ch == NULL || IS_NPC(ch))
    return;

  half_chop(argument, arg1, arg2);

  if(!*arg1) {
    show_clan_info(ch);
    return;
  }


  // immortal-only commands
  if(GET_ADMLEVEL(ch) >= ADMLVL_IMPL) {

    if(!(strcmp(arg1, "create"))) {
      if(!*arg2)
	show_clan_info(ch);
      else if(isClan(arg2))
	send_to_char(ch, "There is already a clan with the name, %s.\r\n", arg2);
      else {
	send_to_char(ch, "You create a clan with the name, %s.\r\n", arg2);
	clanCreate(arg2);
        mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has created a clan named %s.", GET_NAME(ch), arg2);
      }
      return;
    }
    else if(!(strcmp(arg1, "destroy"))) {
      if(!*arg2)
	show_clan_info(ch);
      else if(!isClan(arg2))
	send_to_char(ch, "No clan with the name %s exists.\r\n", arg2);
      else {
	send_to_char(ch, "You destroy %s.\r\n", arg2);
	clanDestroy(arg2);
      }
      return;
    }
    else if(!(strcmp(arg1, "bset"))) {
      if(!*arg2)
        show_clan_info(ch);
      else if(!isClan(arg2))
        send_to_char(ch, "No clan with the name %s exists.\r\n", arg2);
      else if(GET_ADMLEVEL(ch) < 5)
        show_clan_info(ch);
      else {
        clanBSET(arg2, ch);
      }
      return;
    }
    else if(!(strcmp(arg1, "reload"))) {
      if(!*arg2)
	show_clan_info(ch);
      else if(!isClan(arg2))
	send_to_char(ch, "No clan with the name %s exists.\r\n", arg2);
      else if(clanReload(arg2))
	send_to_char(ch, "Data for %s has been reloaded.\r\n", arg2);
      else
	send_to_char(ch, "Failed to reload the data for %s.\r\n", arg2);
      return;
    }
  }

  // commands available to everybody
  if(!(strcmp(arg1, "apply"))) {
    if(!*arg2)
      show_clan_info(ch);
    else if(!isClan(arg2))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg2);
    else if(GET_CLAN(ch) != NULL && clanIsMember(GET_CLAN(ch), ch))
      send_to_char(ch, "You are already a member of %s.\r\n", GET_CLAN(ch));
    else if(clanOpenJoin(arg2)) {
      send_to_char(ch, "You can just join %s, it is open.\r\n", arg2);
      return;
    }
    else {
      if (GET_CLAN(ch) != NULL && checkCLAN(ch) == TRUE) {
       checkAPP(ch);
       send_to_char(ch, "You stop applying to %s\r\n", GET_CLAN(ch));
       clanDecline(GET_CLAN(ch), ch);
       if (GET_CLAN(ch)) {
        free(GET_CLAN(ch));
       }
       GET_CLAN(ch) = strdup("None.");
      }
      send_to_char(ch, "You apply to become a member of %s.\r\n", arg2);
      clanApply(arg2, ch);
      return;
    }
   return;
  }
  if(!(strcmp(arg1, "join"))) {
    if(!*arg2)
      show_clan_info(ch);
    else if(!isClan(arg2))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg2);
    else if(clanIsMember(arg2, ch))
      send_to_char(ch, "You are already a member of %s.\r\n", arg2);
    else if(GET_CLAN(ch) != NULL && checkCLAN(ch) == TRUE && strstr(GET_CLAN(ch), "Applying") == FALSE)
      send_to_char(ch, "You are already a member of %s, you need to leave it first.\r\n", GET_CLAN(ch));
    else if(clanOpenJoin(arg2)) {
      if (GET_CLAN(ch) != NULL && checkCLAN(ch) == TRUE) {
       checkAPP(ch);
       send_to_char(ch, "You stop applying to %s\r\n", GET_CLAN(ch));
       clanDecline(GET_CLAN(ch), ch);
       if (GET_CLAN(ch)) {
        free(GET_CLAN(ch));
       }
       GET_CLAN(ch) = strdup("None.");
      }
      send_to_char(ch, "You are now a member of %s.\r\n", arg2);
      clanInduct(arg2, ch);
      return;
    }
    else {
      send_to_char(ch, "%s isn't open, you must apply instead.\r\n", arg2);
      return;
    }
   return;
  }
  else if(!(strcmp(arg1, "leave"))) {
    if(!*arg2)
      show_clan_info(ch);
    else if(!isClan(arg2))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg2);
    else if(!clanIsMember(arg2, ch))
      send_to_char(ch, "You aren't even a member of %s.\r\n", arg2);
    else if(!(clanOpenLeave(arg2) || clanIsModerator(arg2, ch)))
      send_to_char(ch, "You must be expelled from %s in order to leave it.\r\n", arg2);
    else {
      send_to_char(ch, "You are no longer a member of %s.\r\n", arg2);
      clanExpel(arg2, ch);
      return;
    }
   return;
  }
  else if(!(strcmp(arg1, "infow"))) {
   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
      if(clanIsModerator(GET_CLAN(ch), ch) == FALSE)
       send_to_char(ch, "You must be a moderator to edit the clan's information.\r\n");
      else {
        clanINFOW(GET_CLAN(ch), ch);
        REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);
      }
     return;
   }
  }
  else if (!(strcmp(arg1, "deposit"))) {
   long bank = 0;

   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
    if(!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
    }
    else if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_CBANK) && clanBANY(GET_CLAN(ch), ch) == FALSE) {
     send_to_char(ch, "You are not in your clan bank and your clan doesn't have bank anywhere.\r\n");
     return;
    }
    else if (!*arg2) {
     send_to_char(ch, "How much do you want to deposit?\r\n");
     return;
    }
    else if (atoi(arg2) <= 0) {
     send_to_char(ch, "It needs to be a value higher than 0...\r\n");
     return;
    }
    else if (GET_GOLD(ch) < atoi(arg2)) {
     send_to_char(ch, "You do not have that much to deposit!\r\n");
     return;
    }
    else {
     bank = atoi(arg2);
     GET_GOLD(ch) -= bank;
     clanBANKADD(GET_CLAN(ch), ch, bank);
     send_to_char(ch, "You have deposited %s into the clan bank.\r\n", add_commas(bank));
    }
   }
   return;
  }
  else if(!(strcmp(arg1, "highrank"))) {

   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
    if(!clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not leading a clan.\r\n");
     return;
    }
    else if (!*arg2) {
     send_to_char(ch, "What name are you going to make the rank?\r\n");
     return;
    }
    else if (strlen(arg2) > 20) {
     send_to_char(ch, "The name length can't be longer than 20 characters.\r\n");
     return;
    }
    else if (strstr(arg2, "@")) {
     send_to_char(ch, "No colorcode allowed in the ranks.\r\n");
     return;
    }
    else {
     clanHIGHRANK(GET_CLAN(ch), ch, arg2);
     send_to_char(ch, "High rank set.\r\n");
    }
    return;
   }
  }
  else if(!(strcmp(arg1, "midrank"))) {

   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
    if(!clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not leading a clan.\r\n");
     return;
    }
    else if (!*arg2) {
     send_to_char(ch, "What name are you going to make the rank?\r\n");
     return;
    }
    else if (strlen(arg2) > 20) {
     send_to_char(ch, "The name length can't be longer than 20 characters.\r\n");
     return;
    }
    else if (strstr(arg2, "@")) {
     send_to_char(ch, "No colorcode allowed in the ranks.\r\n");
     return;
    }
    else {
     clanMIDRANK(GET_CLAN(ch), ch, arg2);
     send_to_char(ch, "Mid rank set.\r\n");
    }
    return;
   }
  }
  else if(!(strcmp(arg1, "rank"))) {
   struct char_data *vict = NULL;
    char arg3[100];
    char name[100];
    half_chop(arg2, name, arg3);

   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
    if(!clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not leading a clan.\r\n");
     return;
    }
    else if (!*arg2) {
     send_to_char(ch, "Who's rank do you want to change?\r\n");
     return;
    }
    else if(!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
     send_to_char(ch, "That person is no where to be found in the entire universe.\r\n");
     return;
    }
    else if (GET_CLAN(vict) == NULL || !(strcmp(GET_CLAN(vict), "None."))) {
     send_to_char(ch, "That person is not even in a clan, let alone your's.\r\n");
     return;
    }
    else if (!clanIsMember(GET_CLAN(ch), vict)) {
     send_to_char(ch, "You can only rank those in your clan and only if below leader.\r\n");
     return;
    }
    else if (clanIsModerator(GET_CLAN(ch), vict)) {
     send_to_char(ch, "You can't rank a fellow leader, you require imm assistance.\r\n");
     return;
    }
    else if (!*arg3) {
     send_to_char(ch, "What rank do you want to set them to?\r\n[ 0 = Member, 1 = Midrank, 2 = Highrank]\r\n");
     return;
    }    
    int num = atoi(arg3);
    if (num < 0 || num > 2) {
     send_to_char(ch, "It must be above zero and lower than three...\r\n");
     return;
    }
    else if (GET_CRANK(vict) == num) {
     send_to_char(ch, "They are already that rank!\r\n");
     return;
    }
    else if (GET_CRANK(vict) > num) {
     clanRANK(GET_CLAN(ch), ch, vict, num);
     switch (num) {
      case 0:
       send_to_char(ch, "You demote %s.\r\n", GET_NAME(vict));
       send_to_char(vict, "%s has demoted your clan rank to member!\r\n", GET_NAME(ch));
       break;
      case 1:
       send_to_char(ch, "You demote %s.\r\n", GET_NAME(vict));
       send_to_char(vict, "%s has demoted your clan rank to midrank!\r\n", GET_NAME(ch));
       break;
     }
     return;
    }
    else if (GET_CRANK(vict) < num) {
     clanRANK(GET_CLAN(ch), ch, vict, num);
     switch (num) {
      case 1:
       send_to_char(ch, "You promote %s.\r\n", GET_NAME(vict));
       send_to_char(vict, "%s has promoted your clan rank to midrank!\r\n", GET_NAME(ch));
       break;
      case 2:
       send_to_char(ch, "You promote %s.\r\n", GET_NAME(vict));
       send_to_char(vict, "%s has promoted your clan rank to highrank!\r\n", GET_NAME(ch));
       break;
     }
     return;
    }
   }   
  }
  else if(!(strcmp(arg1, "withdraw"))) {
   long bank = 0;

   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }

   else {
    if(!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
    }
    else if (!*arg2) {
     send_to_char(ch, "How much do you want to withdraw?\r\n");
     return;
    }
    else if (!(clanIsModerator(GET_CLAN(ch), ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL)) {
      send_to_char(ch, "You do not have the power to withdraw from the clan bank.\r\n");
      return;
    }
    else if (atoi(arg2) <= 0) {
     send_to_char(ch, "It needs to be a value higher than 0...\r\n");
     return;
    }
    else if (GET_GOLD(ch) + atoi(arg2) > GOLD_CARRY(ch)) {
     send_to_char(ch, "You can not hold that much zenni!\r\n");
     return;
    }
    else {
     bank = atoi(arg2);
     if (clanBANKSUB(GET_CLAN(ch), ch, bank)) {
      send_to_char(ch, "You have withdrawn %s from the clan bank.\r\n", add_commas(bank));
      GET_GOLD(ch) += bank;
     }
     else {
      send_to_char(ch, "There isn't that much in the clan's bank!\r\n");
     }
    }
   }
   return;
  }
  else if(!(strcmp(arg1, "bank"))) {
   long bank = 0;
   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
   }
   else {
    if(!clanIsMember(GET_CLAN(ch), ch) && !clanIsModerator(GET_CLAN(ch), ch)) {
     send_to_char(ch, "You are not in a clan.\r\n");
     return;
    }
    bank = clanBANK(GET_CLAN(ch), ch);
    send_to_char(ch, "@W[ @C%-20s @W]@w has @D(@Y%s@D)@w zenni in its clan bank.\r\n", GET_CLAN(ch), add_commas(bank));
   }
   return;
  }
  else if(!(strcmp(arg1, "members"))) {
   if (GET_CLAN(ch) == NULL || !(strcmp(GET_CLAN(ch), "None."))) {
    send_to_char(ch, "You are not even in a clan.\r\n");
    return;
   } else {
    handle_clan_member_list(ch);
   }
  }
  else if(!(strcmp(arg1, "expel"))) {
    int is_file = FALSE, player_i = 0;
    struct char_data *vict;
    char arg3[100];
    char name[MAX_INPUT_LENGTH];
    char name1[100];
    half_chop(arg2, name1, arg3);

    if(!*arg3 || !*name1) {
      show_clan_info(ch);
    } else if(!isClan(arg3)) {
      send_to_char(ch, "%s is not a valid clan.\r\n", arg3);
    } else if(!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL)) {
      send_to_char(ch, "Only leaders can expel people from a clan.\r\n");
    } else if(clanOpenJoin(arg3)) {
      send_to_char(ch, "You can't kick someone out of an open-join clan.\r\n");
    } else if(!(vict = get_char_vis(ch, name1, NULL, FIND_CHAR_WORLD))) {
       CREATE(vict, struct char_data, 1);
       clear_char(vict);
       CREATE(vict->player_specials, struct player_special_data, 1);

       sprintf(name, "%s", rIntro(ch, name1));

       if ((player_i = load_char(name, vict)) > -1) {
        is_file = TRUE;
        if (!clanIsMember(arg3, vict)) {
         send_to_char(ch, "%s isn't even a member of %s.\r\n", GET_NAME(vict), arg3);
        } else if (clanIsModerator(arg3, vict) && GET_ADMLEVEL(ch) < ADMLVL_IMPL) {
         send_to_char(ch, "You do not have the power to kick a leader out of %s.\r\n", arg3);
        } else {
         send_to_char(ch, "You expel %s from %s.\r\n", GET_NAME(vict), arg3);
         clanExpel(arg3, vict);
        }
       } else if ((player_i = load_char(name1, vict)) > -1) {
        is_file = TRUE;
        if (!clanIsMember(arg3, vict)) {
         send_to_char(ch, "%s isn't even a member of %s.\r\n", GET_NAME(vict), arg3);
        } else if (clanIsModerator(arg3, vict) && GET_ADMLEVEL(ch) < ADMLVL_IMPL) {
         send_to_char(ch, "You do not have the power to kick a leader out of %s.\r\n", arg3);
        } else {
         send_to_char(ch, "You expel %s from %s.\r\n", GET_NAME(vict), arg3);
         clanExpel(arg3, vict);
        }
       } else {
          free_char(vict);
          send_to_char(ch, "%s does not seem to exist.\r\n", name1);
          return;
       }
       if (is_file == TRUE) {
        free_char(vict);
       }
       return;
    } else if(!clanIsMember(arg3, vict)) {
      send_to_char(ch, "%s isn't even a member of %s.\r\n", GET_NAME(vict), arg3);
    } else if(clanIsModerator(arg3, vict) && GET_ADMLEVEL(ch) < ADMLVL_IMPL) {
      send_to_char(ch, "You do not have the power to kick a leader out of %s.\r\n", arg3);
    } else {
      send_to_char(ch, "You expel %s from %s.\r\n", GET_NAME(vict), arg3);
      send_to_char(vict, "You have been expelled from %s.\r\n", arg3);
      clanExpel(arg3, vict);
      return;
    }
   return;
  }
  else if(!(strcmp(arg1, "decline"))) {
    struct char_data *vict;
    char arg3[100];
    char name[100];
    half_chop(arg2, name, arg3);

    if(!*arg3 || !*name)
      show_clan_info(ch);
    else if(!isClan(arg3))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg3);
    else if((clanIsModerator(arg3, ch) == FALSE && clanIsMember(arg3, ch) == FALSE && GET_ADMLEVEL(ch) < ADMLVL_IMPL) || (GET_CRANK(ch) < 2 && GET_ADMLEVEL(ch) < ADMLVL_IMPL))
      send_to_char(ch, "Only leaders or highrank can decline people from entering a clan.\r\n");
    else if(!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s is not around at the moment.\r\n", name);
    else if(!clanIsApplicant(arg3, vict))
      send_to_char(ch, "%s isn't applying to join %s.\r\n", GET_NAME(vict), arg3);
    else {
      send_to_char(ch, "You decline %s enterance to %s.\r\n", GET_NAME(vict), arg3);
      send_to_char(vict, "You have been declined enterance to %s.\r\n", arg3);
      clanDecline(arg3, vict);
    }
   return;
  }
  else if(!(strcmp(arg1, "enroll"))) {
    struct char_data *vict;
    char arg3[100];
    char name[100];
    half_chop(arg2, name, arg3);

    if(!*arg3 || !*name)
      show_clan_info(ch);
    else if(!isClan(arg3))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg3);
    else if (!clanIsMember(arg3, ch) && GET_ADMLEVEL(ch) < 1)
      send_to_char(ch, "You are not in that clan.\r\n");
    else if(!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL) && GET_CRANK(ch) < 2)
      send_to_char(ch, "Only leaders or captains can enroll people into their clan.\r\n");
    else if(!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s is not around at the moment.\r\n", name);
    else if(!clanIsApplicant(arg3, vict))
      send_to_char(ch, "%s isn't applying to join %s.\r\n", GET_NAME(vict), arg3);
    else if (GET_TRP(vict) < 5)
      send_to_char(ch, "%s needs to have at least earned 5 RPP at some point to join a clan.\r\n", GET_NAME(vict));
    else {
      send_to_char(ch, "You enroll %s into %s.\r\n", GET_NAME(vict), arg3);
      send_to_char(vict, "You have been enrolled into %s.\r\n", arg3);
      clanInduct(arg3, vict);
    }
   return;
  }
  else if(!(strcmp(arg1, "makemod"))) {
    struct char_data *vict;
    char arg3[100];
    char name[100];
    half_chop(arg2, name, arg3);

    if(!*arg3 || !*name)
      show_clan_info(ch);
    else if(!isClan(arg3))
      send_to_char(ch, "%s is not a valid clan.\r\n", arg3);
    else if(!(clanIsModerator(arg3, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
      send_to_char(ch, "Only leaders can make other people in a clan a leader.\r\n");
    else if(!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s is not around at the moment.\r\n", name);
    else {
      send_to_char(ch, "You make %s a leader of %s.\r\n", GET_NAME(vict), arg3);
      send_to_char(vict, "You have been made a leader of %s.\r\n", arg3);
      clanMakeModerator(arg3, vict);
    }
   return;
  }
  else if(!(strcmp(arg1, "setleave"))) {
    char name[100];
    char setting[100];
    half_chop(arg2, setting, name);

    if(!*name || !*setting)
      show_clan_info(ch);
    else if(!isClan(name))
      send_to_char(ch, "%s is not a valid clan.\r\n", name);
    else if(!(clanIsModerator(name, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
      send_to_char(ch, "Only leaders can change that.\r\n");
    else if(!strcmp(setting, "free")) {
      send_to_char(ch, "Members of %s are free to leave as they please.\r\n", name);
      clanSetOpenLeave(name, TRUE);
    }
    else if(!strcmp(setting, "restricted")) {
      send_to_char(ch, "Members of %s can no longer leave as they please.\r\n", name);
      clanSetOpenLeave(name, FALSE);
    }
    else
      send_to_char(ch, "Leave access may only be set to free or restricted.\r\n");
   return;
  }
  else if(!(strcmp(arg1, "setjoin"))) {
    char name[100];
    char setting[100];
    half_chop(arg2, setting, name);

    if(!*name || !*setting)
      show_clan_info(ch);
    else if(!isClan(name))
      send_to_char(ch, "%s is not a valid clan.\r\n", name);
    else if(!(clanIsModerator(name, ch) || GET_ADMLEVEL(ch) >= ADMLVL_IMPL))
      send_to_char(ch, "Only leaders can change that.\r\n");
    else if(!strcmp(setting, "free")) {
      send_to_char(ch, "People may now freely join %s.\r\n", name);
      clanSetOpenJoin(name, TRUE);
    }
    else if(!strcmp(setting, "restricted")) {
      send_to_char(ch, "People must be enrolled into %s to join.\r\n", name);
      clanSetOpenJoin(name, FALSE);
    }
    else
      send_to_char(ch, "Leave access my only be set to free or restricted.\r\n");
   return;
  }
  else if(!(strcmp(arg1, "list")))
    listClans(ch);
  else if(!(strcmp(arg1, "info"))) {
    if(!*arg2)
      show_clan_info(ch);
    else
      listClanInfo(arg2, ch);
  }
  else {
    show_clan_info(ch);
    send_to_char(ch, "These are viable options.\r\n");
  }
}

ACMD(do_pagelength)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You current page length is set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else if (is_number(arg)) {
    GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg), 5), 50);
    send_to_char(ch, "Okay, your page length is now set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else {
    send_to_char(ch, "Please specify a number of lines (5 - 50).\r\n");
  }
}

ACMD(do_aid)
{
  struct char_data *vict;
  struct obj_data *obj = NULL, *aid_obj = NULL, *aid_prod = NULL, *next_obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int dc = 0, found = FALSE, num = 47, num2 = 0, survival = 0;

  if (IS_NPC(ch))
    return;

  if (GET_SKILL(ch, SKILL_SURVIVAL)) {
   survival = 1;
  }

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(ch, "Syntax: aid heal (target)\r\n");
    send_to_char(ch, "        aid adrenex\r\n");
    send_to_char(ch, "        aid antitoxin\r\n");
    send_to_char(ch, "        aid salve\r\n");
    send_to_char(ch, "        aid formula-82\r\n");
    return;
  }

 if (!strcasecmp(arg, "adrenex")) {
  num = 380;
  num2 = 381;
 } else if (!strcasecmp(arg, "antitoxin")) {
  num = 380;
  num2 = 383;
 } else if (!strcasecmp(arg, "salve")) {
  num = 380;
  num2 = 382;
 } else if (!strcasecmp(arg, "formula-82")) {
  num = 380;
  num2 = 385;
 }

  for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
   if (found == FALSE && GET_OBJ_VNUM(obj) == num && (!OBJ_FLAGGED(obj, ITEM_BROKEN) && !OBJ_FLAGGED(obj, ITEM_FORGED))) {
    found = TRUE;
    aid_obj = obj;
   }
  }
 
  if (found == FALSE || aid_obj == NULL) {
    if (num == 47) {
     send_to_char(ch, "You need bandages to be able to use first aid.\r\n");
    } else {
     send_to_char(ch, "You need a TCX-Medical Equipment Construction Kit.\r\n");
    }
    return;
  }
  if (num == 47) {
   if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Apply first aid to who?\r\n");
    return;
   } else if (IS_NPC(vict)) {
    send_to_char(ch, "What ever for?\r\n");
    return;
   } else if (IS_ANDROID(vict)) {
    send_to_char(ch, "They are an android!\r\n");
    return;
   }

  if (!AFF_FLAGGED(vict, AFF_SPIRIT) && !PLR_FLAGGED(vict, PLR_BANDAGED)) {
    if (vict != ch) {
     send_to_char(ch, "You attempt to lend first aid to %s.\r\n", GET_NAME(vict));
    }
    act("$n attempts to bandage $N's wounds.", TRUE, ch, 0, vict, TO_ROOM);
    dc = axion_dice(0);
    if ((GET_SKILL(ch, SKILL_FIRST_AID) + 1) > dc) {
      send_to_char(ch, "You bandage %s's wounds.\r\n", GET_NAME(vict));
      int64_t roll = ((gear_pl(vict) / 100) * (GET_WIS(ch) / 4)) + gear_pl(vict) * 0.25;
      if (GET_BONUS(ch, BONUS_HEALER) > 0) {
       roll += roll * .1;
      }
      GET_HIT(vict) += roll;
      if (GET_HIT(vict) > gear_pl(vict)) {
       GET_HIT(vict) = gear_pl(vict);
      }
      if (GET_SUPPRESS(vict) > 0 && GET_HIT(vict) > ((GET_MAX_HIT(vict) / 100) * GET_SUPPRESS(vict))) {
       GET_HIT(vict) = ((GET_MAX_HIT(vict) / 100) * GET_SUPPRESS(vict));
       send_to_char(vict, "@mYou are healed to your suppression limit.@n\r\n");
      }
      send_to_char(vict, "Your wounds are bandaged by %s!\r\n", GET_NAME(ch));
      act("$n's wounds are stablized by $N!", TRUE, vict, 0, ch, TO_NOTVICT);
      SET_BIT_AR(PLR_FLAGS(vict), PLR_BANDAGED);
      extract_obj(aid_obj);
    } else {
      if (vict != ch) {
       send_to_char(ch, "You fail to bandage their wounds properly, wasting the set of bandages...\r\n");
       act("$N fails to bandage $n's wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_NOTVICT);
       act("$N fails to bandage your wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_CHAR);
      }
      else {
       act("$N fails to bandage $s wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_NOTVICT);
       act("You fail to bandage your wounds properly, wasting an entire set of bandages...", TRUE, vict, 0, ch, TO_VICT);
      }
      extract_obj(aid_obj);
    }
    improve_skill(ch, SKILL_FIRST_AID, 1);
  } else if (PLR_FLAGGED(vict, PLR_BANDAGED)) {
    send_to_char(ch, "They are already bandaged!\r\n");
  } else if (AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "The dead don't need first aid.\r\n");
  } else {
    send_to_char(ch, "They apparently do not need bandaging.\r\n");
  }
 } else if (num2 == 381) {
    if (GET_SKILL(ch, SKILL_FIRST_AID) < 65) {
     send_to_char(ch, "You need at least a skill level of 65 in First Aid.\r\n");
     return;
    } else {
     if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(15)) {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you begin to construct an Andrenex Adreneline Injector you screw up and end up breaking the water tight seal. The adreneline leaks out and is wasted.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(aid_obj);
     } else {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully construct an Adrenex Adreneline Injector@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Adrenex Adreneline Injector!@n", TRUE, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(num2, VIRTUAL);
      add_unique_id(aid_prod);
      obj_to_char(aid_prod, ch);
      extract_obj(aid_obj);      
      improve_skill(ch, SKILL_FIRST_AID, 1);
     }
    }  
 } else if (num2 == 382) {
    if (GET_SKILL(ch, SKILL_FIRST_AID) < 50) {
     send_to_char(ch, "You need at least a skill level of 50 in First Aid.\r\n");
     return;
    } else {
     if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(10)) {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you go to put the salve ingredients into the kit's salve compartment and set the temperature you accidentally set it too high. The salve is burned and ruined. Yes you managed to burn a burn salve.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(aid_obj);
     } else {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully boil a burn salve to perfection and it is automatically placed in a jar.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a jar of burn salve!@n", TRUE, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(num2, VIRTUAL);
      add_unique_id(aid_prod);
      obj_to_char(aid_prod, ch);
      extract_obj(aid_obj);      
      improve_skill(ch, SKILL_FIRST_AID, 1);
     }
    }
 } else if (num2 == 383) {
    if (GET_SKILL(ch, SKILL_FIRST_AID) < 40) {
     send_to_char(ch, "You need at least a skill level of 40 in First Aid.\r\n");
     return;
    } else {
     if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(5)) {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete the Antitoxin Injector you notice that you didn't seal the syringe properly and it all leaks out.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(aid_obj);
     } else {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble the Antitoxin Injector.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Antitoxin Injector!@n", TRUE, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(num2, VIRTUAL);
      add_unique_id(aid_prod);
      obj_to_char(aid_prod, ch);
      extract_obj(aid_obj);      
      improve_skill(ch, SKILL_FIRST_AID, 1);
     }
    }
 } else if (num2 == 385) {
    if (GET_SKILL(ch, SKILL_FIRST_AID) < 40) {
     send_to_char(ch, "You need at least a skill level of 40 in First Aid.\r\n");
     return;
    } else {
     if (GET_SKILL(ch, SKILL_FIRST_AID) < axion_dice(15)) {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. As you complete a vial of Formula 82 you notice that you read the mixture measurements wrong. You dispose of the vile vial immediately.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A frown forms on $s face as it appears that $e has failed.@n", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(aid_obj);
     } else {
      act("@WYou unlock and open the TCX-M.E.C.K. case. The case hisses as its lid opens. Your knowledge in basic medical devices and treatments helps you as you successfully assemble a vial of Formula 82.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@C$n@W holds a steel case up and opens it. The case hisses as its lid opens. @C$n@W wastes no time as $e reaches into the case and begins constructing something. A moment later $e holds up a completed Vial of Formula 82!@n", TRUE, ch, 0, 0, TO_ROOM);
      aid_prod = read_object(num2, VIRTUAL);
      add_unique_id(aid_prod);
      obj_to_char(aid_prod, ch);
      extract_obj(aid_obj);
      improve_skill(ch, SKILL_FIRST_AID, 1);
     }
    }
 }

 WAIT_STATE(ch, PULSE_3SEC);
}

ACMD(do_aura) {
   char arg[MAX_INPUT_LENGTH];
   
   one_argument(argument, arg);
   
 if (!*arg) {
   send_to_char(ch, "Syntax: aura light\r\n");
   return;
  } 
  
 if (GET_CHARGE(ch)) {
   send_to_char(ch, "You can't focus enough on this while charging.");
   return;
  }
  
 if (PLR_FLAGGED(ch, PLR_POWERUP)) {
   send_to_char(ch, "You are busy powering up!\r\n");
   return;
  }
   
 if (!strcasecmp(arg, "light")) {
  if (GET_SKILL(ch, SKILL_FOCUS) && GET_SKILL(ch, SKILL_CONCENTRATION) < 75) {
    send_to_char(ch, "You need at least a skill level of 75 in Focus and Concentration to use this.\r\n");
	return;
  } else {
     if (PLR_FLAGGED(ch, PLR_AURALIGHT)) {
	   send_to_char(ch, "Your aura fades as you stop shining light.\r\n");
	   act("$n's aura fades as they stop shining light on the area.", TRUE, ch, 0, 0, TO_ROOM);
	   REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_AURALIGHT);
	   world[IN_ROOM(ch)].light--;
        
     } else if (GET_MANA(ch) > GET_MAX_MANA(ch) * 0.12) {
	 if (IN_ROOM(ch) != NOWHERE) {
	  world[IN_ROOM(ch)].light++;
         }
          reveal_hiding(ch, 0);
	  GET_MANA(ch) -= GET_MAX_MANA(ch) * 0.12;
          send_to_char(ch, "A bright %s aura begins to burn around you as you provide light to the surrounding area!\r\n", aura_types[GET_AURA(ch)]);	
	  char bloom[MAX_INPUT_LENGTH];
          sprintf(bloom, "@wA %s aura flashes up brightly around $n@w as they provide light to the area.@n", aura_types[GET_AURA(ch)]);
          act(bloom, TRUE, ch, 0, 0, TO_ROOM);
	  SET_BIT_AR(PLR_FLAGS(ch), PLR_AURALIGHT);

     } else {
         send_to_char(ch, "You don't have enough KI to do that.\r\n");
	 return;
     }   
  }
 }
}
