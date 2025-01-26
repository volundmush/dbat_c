/************************************************************************
 * Generic OLC Library - General / genolc.c			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/
#define __GENOLC_C__
#include "genolc.h"
#include "gengld.h"
#include "cedit.h"
#include "genmob.h"
#include "genobj.h"
#include "genshp.h"
#include "genwld.h"
#include "modify.h"
#include "genzon.h"
#include "oasis.h"
#include "utils.h"
#include "comm.h"

/* List of zones to be saved.  */
struct save_list_data *save_list;

/* Structure defining all known save types.  */
struct {
  int save_type;
  int (*func)(IDXTYPE rnum);
  const char *message;
} save_types[] = {
  { SL_MOB, save_mobiles , "mobile" },
  { SL_OBJ, save_objects, "object" },
  { SL_SHP, save_shops, "shop" },
  { SL_WLD, save_rooms, "room" },
  { SL_ZON, save_zone, "zone" },
  { SL_CFG, save_config, "config" },
  { SL_GLD, save_guilds, "guild" },
  { SL_ACT, NULL, "social" },
  { SL_HLP, NULL, "help" },
  { -1, NULL, NULL },
};

int genolc_checkstring(struct descriptor_data *d, char *arg)
{
  smash_tilde(arg);
  return TRUE;
}

char *str_udup(const char *txt)
{
  return strdup((txt && *txt) ? txt : "undefined");
}

/* Original use: to be called at shutdown time.  */
int save_all(void)
{
  while (save_list) {
    if (save_list->type < 0 || save_list->type > SL_MAX) {
      switch (save_list->type) {
        case SL_ACT:
          log("Actions not saved - can not autosave. Use 'aedit save'.");
        save_list = save_list->next;	/* Fatal error, skip this one. */
          break;
        case SL_HLP:
          log("Help not saved - can not autosave. Use 'hedit save'.");
          save_list = save_list->next;    /* Fatal error, skip this one. */
          break;
        default:
      log("SYSERR: GenOLC: Invalid save type %d in save list.\n", save_list->type);
          break;
      }
    } else if ((*save_types[save_list->type].func) (real_zone(save_list->zone)) < 0)
      save_list = save_list->next;	/* Fatal error, skip this one. */
  }

  return TRUE;
}

/* NOTE: This changes the buffer passed in.  */
void strip_cr(char *buffer)
{
  int rpos, wpos;

  if (buffer == NULL)
    return;

  for (rpos = 0, wpos = 0; buffer[rpos]; rpos++) {
    buffer[wpos] = buffer[rpos];
    wpos += (buffer[rpos] != '\r');
  }
  buffer[wpos] = '\0';
}

void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from)
{
  struct extra_descr_data *wpos;

  CREATE(*to, struct extra_descr_data, 1);
  wpos = *to;

  for (; from; from = from->next, wpos = wpos->next) {
    wpos->keyword = str_udup(from->keyword);
    wpos->description = str_udup(from->description);
    if (from->next)
      CREATE(wpos->next, struct extra_descr_data, 1);
  }
}

void free_ex_descriptions(struct extra_descr_data *head)
{
  struct extra_descr_data *thised, *next_one;

  if (!head) {
    log("free_ex_descriptions: NULL pointer or NULL data.");
    return;
  }

  for (thised = head; thised; thised = next_one) {
    next_one = thised->next;
    if (thised->keyword)
      free(thised->keyword);
    if (thised->description)
      free(thised->description);
    free(thised);
  }
}

int remove_from_save_list(zone_vnum zone, int type)
{
  struct save_list_data *ritem, *temp;

  for (ritem = save_list; ritem; ritem = ritem->next)
    if (ritem->zone == zone && ritem->type == type)
      break;

  if (ritem == NULL) {
    log("SYSERR: remove_from_save_list: Saved item not found. (%d/%d)", zone, type);
    return FALSE;
  }
  REMOVE_FROM_LIST(ritem, save_list, next, temp);
  free(ritem);
  return TRUE;
}

int add_to_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  zone_rnum rznum;
  
  if (type == SL_CFG)
    return FALSE; 
    
  rznum = real_zone(zone);
  if (rznum == NOWHERE || rznum > top_of_zone_table) {
     if (zone != AEDIT_PERMISSION && zone != HEDIT_PERMISSION) {
      log("SYSERR: add_to_save_list: Invalid zone number passed. (%d => %d, 0-%d)", zone, rznum, top_of_zone_table);
      return FALSE;
    }
  }
  
  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return FALSE;
  
  CREATE(nitem, struct save_list_data, 1);
  nitem->zone = zone;
  nitem->type = type;
  nitem->next = save_list;
  save_list = nitem;
  return TRUE;
}

int in_save_list(zone_vnum zone, int type)
{
  struct save_list_data *nitem;
  
  for (nitem = save_list; nitem; nitem = nitem->next)
    if (nitem->zone == zone && nitem->type == type)
      return TRUE;
  
  return FALSE;
}

void free_save_list(void)
{
  struct save_list_data *sld, *next_sld;

  for (sld = save_list; sld; sld = next_sld) {
    next_sld = sld->next;
    free(sld);
  }
}

/* Used from do_show(), ideally.  */
ACMD(do_show_save_list)
{
  if (save_list == NULL)
    send_to_char(ch, "All world files are up to date.\r\n");
  else {
    struct save_list_data *item;

    send_to_char(ch, "The following files need saving:\r\n");
    for (item = save_list; item; item = item->next) {
      if (item->type != SL_CFG)
        send_to_char(ch, " - %s data for zone %d.\r\n", save_types[item->type].message, item->zone);
      else
        send_to_char(ch, " - Game configuration data.\r\n");
    }
  }
}

room_vnum genolc_zonep_bottom(struct zone_data *zone)
{
  return zone->bot;
}

zone_vnum genolc_zone_bottom(zone_rnum rznum)
{
  return zone_table[rznum].bot;
}

int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}

