/************************************************************************
 * Generic OLC Library - Mobiles / genmob.c			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#include "genmob.h"
#include "utils.h"
#include "db.h"
#include "genolc.h"
#include "shop.h"
#include "genzon.h"
#include "htree.h"
#include "guild.h"
#include "dg_scripts.h"
#include "handler.h"
#include "dg_olc.h"

/* From db.c */
void init_mobile_skills(void);


int update_mobile_strings(struct char_data *t, struct char_data *f);
void check_mobile_strings(struct char_data *mob);
void check_mobile_string(mob_vnum i, char **string, const char *dscr);
int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd);
int copy_mobile_strings(struct char_data *t, struct char_data *f);

#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* local functions */
void extract_mobile_all(mob_vnum vnum);

int add_mobile(struct char_data *mob, mob_vnum vnum)
{
  int rnum, i, found = FALSE, shop, guild, cmd_no;
  zone_rnum zone;
  struct char_data *live_mob;

  if ((rnum = real_mobile(vnum)) != NOBODY) {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile(&mob_proto[rnum], mob);

    /* Now re-point all existing mobile strings to here. */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (rnum == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[rnum]);

    add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
    log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return rnum;
  }

  RECREATE(mob_proto, struct char_data, top_of_mobt + 2);
  RECREATE(mob_index, struct index_data, top_of_mobt + 2);
  top_of_mobt++;

  for (i = top_of_mobt; i > 0; i--) {
    if (vnum > mob_index[i - 1].vnum) {
      mob_proto[i] = *mob;
      mob_proto[i].nr = i;
      copy_mobile_strings(mob_proto + i, mob);
      mob_index[i].vnum = vnum;
      mob_index[i].number = 0;
      mob_index[i].func = 0;
      found = i;
      break;
    }
    mob_index[i] = mob_index[i - 1];
    mob_proto[i] = mob_proto[i - 1];
    mob_proto[i].nr++;
    htree_add(mob_htree, mob_index[i].vnum, i);
  }
  if (!found) {
    mob_proto[0] = *mob;
    mob_proto[0].nr = 0;
    copy_mobile_strings(&mob_proto[0], mob);
    mob_index[0].vnum = vnum;
    mob_index[0].number = 0;
    mob_index[0].func = 0;
    htree_add(mob_htree, mob_index[0].vnum, 0);
  }

  log("GenOLC: add_mobile: Added mobile %d at index #%d.", vnum, found);

#if CONFIG_GENOLC_MOBPROG
  GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
  GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
  while (OLC_MPROGL(d)) {
    GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
    OLC_MPROGL(d) = OLC_MPROGL(d)->next;
  }
#endif

  /* Update live mobile rnums. */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) += (GET_MOB_RNUM(live_mob) != NOTHING && GET_MOB_RNUM(live_mob) >= found);

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M')
	ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= found);

  /* Update shop keepers. */
  if (shop_index)
    for (shop = 0; shop <= top_shop; shop++)
      SHOP_KEEPER(shop) += (SHOP_KEEPER(shop) != NOTHING && SHOP_KEEPER(shop) >= found);

  /* Update guild masters */
  if (guild_index)
    for (guild = 0; guild <= top_guild; guild++)
      GM_TRAINER(guild) += (GM_TRAINER(guild) != NOTHING && GM_TRAINER(guild) >= found);

  add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
  return found;
}

int copy_mobile(struct char_data *to, struct char_data *from)
{
  free_mobile_strings(to);
  *to = *from;
  check_mobile_strings(from);
  copy_mobile_strings(to, from);
  return TRUE;
}

void extract_mobile_all(mob_vnum vnum)
{
  struct char_data *next, *ch;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;
    if (GET_MOB_VNUM(ch) == vnum)
      extract_char(ch);
  }
}

int delete_mobile(mob_rnum refpt)
{
  struct char_data *live_mob;
  int counter, cmd_no;
  mob_vnum vnum;
  zone_rnum zone;
  
#if CIRCLE_UNSIGNED_INDEX
  if (refpt == NOBODY || refpt > top_of_mobt) {
#else
  if (refpt < 0 || refpt > top_of_mobt) {
#endif
    log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
    return NOBODY;
  }

  vnum = mob_index[refpt].vnum;
  extract_mobile_all(vnum);

  for (counter = refpt; counter < top_of_mobt; counter++) {
    mob_index[counter] = mob_index[counter + 1];
    mob_proto[counter] = mob_proto[counter + 1];
    mob_proto[counter].nr = counter;
  }

  top_of_mobt--;
  RECREATE(mob_index, struct index_data, top_of_mobt + 1);
  RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

  /* Update live mobile rnums.  */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) -= (GET_MOB_RNUM(live_mob) >= refpt);

  /* Update zone table.  */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M' && ZCMD(zone, cmd_no).arg1 == refpt)
        /* Should probably try to find dependant commands as well? 
         * Should probably save the zone file too? */
        delete_zone_command(&zone_table[zone], cmd_no);

  /* Update shop keepers.  */
  if (shop_index)
    for (counter = 0; counter <= top_shop; counter++) {
      /* Find the shop for this keeper and reset it's keeper to
       * -1 to keep the shop so it could be assigned to someone else */
      if (SHOP_KEEPER(counter) == refpt) {
        SHOP_KEEPER(counter) = -1;
      }
    }

  /* Update guild masters */
  if (guild_index)
    for (counter = 0; counter <= top_guild; counter++) {
      /* Find the guild for this trainer and reset it's trainer to
       * -1 to keep the guild so it could be assigned to someone else */
      if (GM_TRAINER(counter) == refpt) {
        GM_TRAINER(counter) = -1;
      }
    }

  save_mobiles(real_zone_by_thing(vnum));

  return refpt;
}

int copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->name)
    t->name = strdup(f->name);
  if (f->title)
    t->title = strdup(f->title);
  if (f->short_descr)
    t->short_descr = strdup(f->short_descr);
  if (f->long_descr)
    t->long_descr = strdup(f->long_descr);
  if (f->description)
    t->description = strdup(f->description);
  return TRUE;
}

int update_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->name)
    t->name = f->name;
  if (f->title)
    t->title = f->title;
  if (f->short_descr)
    t->short_descr = f->short_descr;
  if (f->long_descr)
    t->long_descr = f->long_descr;
  if (f->description)
    t->description = f->description;
  return TRUE;
}

int free_mobile_strings(struct char_data *mob)
{
  if (mob->name)
    free(mob->name);
  if (mob->title)
    free(mob->title);
  if (mob->short_descr)
    free(mob->short_descr);
  if (mob->long_descr)
    free(mob->long_descr);
  if (mob->description)
    free(mob->description);
  return TRUE;
}


/* Free a mobile structure that has been edited. Take care of existing mobiles 
 * and their mob_proto!  */
int free_mobile(struct char_data *mob)
{
  mob_rnum i;

  if (mob == NULL)
    return FALSE;

  /* Non-prototyped mobile.  Also known as new mobiles.  */
  if ((i = GET_MOB_RNUM(mob)) == NOBODY) {
    free_mobile_strings(mob);
    /* free script proto list */
    free_proto_script(mob, MOB_TRIGGER);
   } else {	/* Prototyped mobile. */
    if (mob->name && mob->name != mob_proto[i].name)
      free(mob->name);
    if (mob->title && mob->title != mob_proto[i].title)
      free(mob->title);
    if (mob->short_descr && mob->short_descr != mob_proto[i].short_descr)
      free(mob->short_descr);
    if (mob->long_descr && mob->long_descr != mob_proto[i].long_descr)
      free(mob->long_descr);
    if (mob->description && mob->description != mob_proto[i].description)
      free(mob->description);
    /* free script proto list if it's not the prototype */
    if (mob->proto_script && mob->proto_script != mob_proto[i].proto_script)
      free_proto_script(mob, MOB_TRIGGER);
  }
  while (mob->affected)
    affect_remove(mob, mob->affected);

  /* free any assigned scripts */
  if (SCRIPT(mob))
    extract_script(mob, MOB_TRIGGER);

  free(mob);
  return TRUE;
}

int save_mobiles(zone_rnum zone_num)
{
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  int written;
  char mobfname[64], usedfname[64];

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table) {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  snprintf(mobfname, sizeof(mobfname), "%s%d.new", MOB_PREFIX, zone_table[zone_num].number);
  if ((mobfd = fopen(mobfname, "w")) == NULL) {
    mudlog(BRF, ADMLVL_GOD, TRUE, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return FALSE;
  }

  for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (write_mobile_record(i, &mob_proto[rmob], mobfd) < 0)
      log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  fputs("$\n", mobfd);
  written = ftell(mobfd);
  fclose(mobfd);
  snprintf(usedfname, sizeof(usedfname), "%s%d.mob", MOB_PREFIX, zone_table[zone_num].number);
  remove(usedfname);
  rename(mobfname, usedfname);
  
  if (in_save_list(zone_table[zone_num].number, SL_MOB)) {
    remove_from_save_list(zone_table[zone_num].number, SL_MOB);
    create_world_index(zone_table[zone_num].number, "mob");
    log("GenOLC: save_mobiles: Saving mobiles '%s'", usedfname);
  }
  return written;
}

#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  char wmmarg[MAX_STRING_LENGTH], wmmcom[MAX_STRING_LENGTH];
  MPROG_DATA *mob_prog;

  for (mob_prog = GET_MPROG(mob); mob_prog; mob_prog = mob_prog->next) {
    wmmarg[MAX_STRING_LENGTH - 1] = '\0';
    wmmcom[MAX_STRING_LENGTH - 1] = '\0';
    strip_cr(strncpy(wmmarg, mob_prog->arglist, MAX_STRING_LENGTH - 1));
    strip_cr(strncpy(wmmcom, mob_prog->comlist, MAX_STRING_LENGTH - 1));
    fprintf(fd,	"%s %s~\n"
		"%s%c\n",
	medit_get_mprog_type(mob_prog), wmmarg,
	wmmcom, STRING_TERMINATOR
    );
    if (mob_prog->next == NULL)
      fputs("|\n", fd);
  }
  return TRUE;
}
#endif

int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  struct affected_type *aff;
  int i;
  extern const int race_def_sizetable[NUM_RACES + 1];

  if (get_size(mob) != race_def_sizetable[GET_RACE(mob)])
    fprintf(fd, "Size: %d\n", get_size(mob));
  if (GET_ATTACK(mob) != 0)
    fprintf(fd, "BareHandAttack: %d\n", GET_ATTACK(mob));
  if (GET_STR(mob) != 0)
    fprintf(fd, "Str: %d\n", GET_STR(mob));
  if (GET_DEX(mob) != 0)
    fprintf(fd, "Dex: %d\n", GET_DEX(mob));
  if (GET_INT(mob) != 0)
    fprintf(fd, "Int: %d\n", GET_INT(mob));
  if (GET_WIS(mob) != 0)
    fprintf(fd, "Wis: %d\n", GET_WIS(mob));
  if (GET_CON(mob) != 0)
    fprintf(fd, "Con: %d\n", GET_CON(mob));
  if (GET_CHA(mob) != 0)
    fprintf(fd, "Cha: %d\n", GET_CHA(mob));
  if (mob_proto + real_mobile(mvnum) != mob) { /* Not saving a prototype */
    fprintf(fd, "Hit: %"I64T"\nMaxHit: %"I64T"\nMana: %"I64T"\nMaxMana: %"I64T"\nMoves: %"I64T"\nMaxMoves: %"I64T"\n",
            GET_HIT(mob), GET_MAX_HIT(mob), GET_MANA(mob), GET_MAX_MANA(mob),
            GET_MOVE(mob), GET_MAX_MOVE(mob));
    for (aff = mob->affected; aff; aff = aff->next)
      if (aff->type)
        fprintf(fd, "Affect: %d %d %d %d %d %d\n", aff->type, aff->duration,
                aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
    for (aff = mob->affectedv; aff; aff = aff->next)
      if (aff->type)
        fprintf(fd, "AffectV: %d %d %d %d %d %d\n", aff->type, aff->duration,
                aff->modifier, aff->location, (int)aff->bitvector, aff->specific);
  }
  for (i = 0; i <= NUM_FEATS_DEFINED; i++)
    if (HAS_FEAT(mob, i))
      fprintf(fd, "Feat: %d %d\n", i, HAS_FEAT(mob, i));
  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    if (GET_SKILL_BASE(mob, i))
      fprintf(fd, "Skill: %d %d\n", i, HAS_FEAT(mob, i));
  for (i = 0; i <= NUM_FEATS_DEFINED; i++)
    if (GET_SKILL_BONUS(mob, i))
      fprintf(fd, "SkillMod: %d %d\n", i, HAS_FEAT(mob, i));
  for (i = 0; i < NUM_CLASSES; i++) {
    if (GET_CLASS_NONEPIC(mob, i))
      fprintf(fd, "Class: %d %d\n", i, GET_CLASS_NONEPIC(mob, i));
    if (GET_CLASS_EPIC(mob, i))
      fprintf(fd, "EpicClass: %d %d\n", i, GET_CLASS_EPIC(mob, i));
  }
  fputs("E\n", fd);
  return TRUE;
}


int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{

  char ldesc[MAX_STRING_LENGTH], ddesc[MAX_STRING_LENGTH];
  char fbuf1[MAX_STRING_LENGTH], fbuf2[MAX_STRING_LENGTH];
  char fbuf3[MAX_STRING_LENGTH], fbuf4[MAX_STRING_LENGTH];
  char abuf1[MAX_STRING_LENGTH], abuf2[MAX_STRING_LENGTH];
  char abuf3[MAX_STRING_LENGTH], abuf4[MAX_STRING_LENGTH];

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));

  fprintf(fd,	"#%d\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n",
	mvnum,
	GET_ALIAS(mob), STRING_TERMINATOR,
	GET_SDESC(mob), STRING_TERMINATOR,
	ldesc, STRING_TERMINATOR,
	ddesc, STRING_TERMINATOR
  );

  sprintascii(fbuf1, MOB_FLAGS(mob)[0]);
  sprintascii(fbuf2, MOB_FLAGS(mob)[1]);
  sprintascii(fbuf3, MOB_FLAGS(mob)[2]);
  sprintascii(fbuf4, MOB_FLAGS(mob)[3]);
  sprintascii(abuf1, AFF_FLAGS(mob)[0]);
  sprintascii(abuf2, AFF_FLAGS(mob)[1]);
  sprintascii(abuf3, AFF_FLAGS(mob)[2]);
  sprintascii(abuf4, AFF_FLAGS(mob)[3]);

  fprintf(fd, "%s %s %s %s %s %s %s %s %d E\n"
              "%d %d %d %"I64T"d%"I64T"+%"I64T" %dd%d+%d\n",
                fbuf1, fbuf2, fbuf3, fbuf4,
                abuf1, abuf2, abuf3, abuf4,
		GET_ALIGNMENT(mob),
		GET_HITDICE(mob), GET_FISHD(mob), 10 - (GET_ARMOR(mob) / 10),
                GET_HIT(mob), GET_MANA(mob), GET_MOVE(mob), GET_NDD(mob), GET_SDD(mob),
		GET_DAMAGE_MOD(mob)
  );
  fprintf(fd, 	"%d 0 %d %d\n"
		"%d %d %d\n",
		GET_GOLD(mob), GET_RACE(mob), GET_CLASS(mob),
		GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob)
  );

  if (write_mobile_espec(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  script_save_to_disk(fd, mob, MOB_TRIGGER);


#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  return TRUE;
}

void check_mobile_strings(struct char_data *mob)
{
  mob_vnum mvnum = mob_index[mob->nr].vnum;
  check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
  check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
  check_mobile_string(mvnum, &GET_ALIAS(mob), "alias list");
  check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
}

void check_mobile_string(mob_vnum i, char **string, const char *dscr)
{
  if (*string == NULL || **string == '\0') {
    char smbuf[128];
    sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, dscr);
    mudlog(BRF, ADMLVL_GOD, TRUE, smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.");
  }
}

