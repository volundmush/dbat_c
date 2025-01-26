/**************************************************************************
*  File: dg_variables.c                                                   *
*  Usage: contains the functions dealing with variable substitution.      *
*                                                                         *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00 $                                           *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "dg_event.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "spells.h"
#include "oasis.h"
#include "class.h"
#include "races.h"

/* Utility functions */

/*
 * Thanks to James Long for his assistance in plugging the memory leak
 * that used to be here.   -- Welcor
 */
/* adds a variable with given name and value to trigger */
void add_var(struct trig_var_data **var_list, char *name, const char *value, long id)
{
  struct trig_var_data *vd;

  if (strchr(name, '.')) {
    log("add_var() : Attempt to add illegal var: %s", name);
    return;
  }

  for (vd = *var_list; vd && strcasecmp(vd->name, name); vd = vd->next);

  if (vd && (!vd->context || vd->context==id)) {
    free(vd->value);
    CREATE(vd->value, char, strlen(value) + 1);
  }

  else {
    CREATE(vd, struct trig_var_data, 1);

    CREATE(vd->name, char, strlen(name) + 1);
    strcpy(vd->name, name);                            /* strcpy: ok*/

    CREATE(vd->value, char, strlen(value) + 1);

    vd->next = *var_list;
    vd->context = id;
    *var_list = vd;
  }

  strcpy(vd->value, value);                            /* strcpy: ok*/
}


/* perhaps not the best place for this, but I didn't want a new file */
char *skill_percent(struct char_data *ch, char *skill)
{
  static char retval[16];
  int skillnum;

  skillnum = find_skill_num(skill, SKTYPE_SKILL);
  if (skillnum<=0) return("unknown skill");

  snprintf(retval, sizeof(retval), "%d", GET_SKILL(ch, skillnum));
  return retval;
}

/*
   search through all the persons items, including containers
   and 0 if it doesnt exist, and greater then 0 if it does!
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000

   Now also searches by vnum -- Welcor
   Now returns the number of matching objects -- Welcor 02/04
*/

int item_in_list(char *item, obj_data *list)
{
  obj_data *i;
  int count = 0;

  if (!item || !*item)
  	return 0;

  if (*item == UID_CHAR) {
    long id = atol(item + 1);

    for (i = list; i; i = i->next_content) {
      if (id == GET_ID(i))
        count++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  } else if (is_number(item) > -1) { /* check for vnum */
    obj_vnum ovnum = atoi(item);

    for (i = list; i; i = i->next_content) {
      if (GET_OBJ_VNUM(i) == ovnum)
        count++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  } else {
    for (i = list; i; i = i->next_content) {
      if (isname(item, i->name))
        count++;
      if (GET_OBJ_TYPE(i) == ITEM_CONTAINER)
        count += item_in_list(item, i->contains);
    }
  }
  return count;
}

/*
   BOOLEAN return, just check if a player or mob
   has an item of any sort, searched for by name
   or id.
   searching equipment as well as inventory,
   and containers.
   Jamie Nelson (mordecai@timespace.co.nz)
   MUD -- 4dimensions.org:6000
*/

int char_has_item(char *item, struct char_data *ch)
{

  /* If this works, no more searching needed */
  if (get_object_in_equip(ch, item) != NULL)
    return 1;

  if (item_in_list(item, ch->carrying) == 0)
    return 0;
  else
    return 1;
}

int text_processed(char *field, char *subfield, struct trig_var_data *vd,
                   char *str, size_t slen)
{
  char *p, *p2;
  char tmpvar[MAX_STRING_LENGTH];

  if (!strcasecmp(field, "strlen")) {                     /* strlen    */
    char limit[200];
    sprintf(limit, "%"SZT, strlen(vd->value));
    snprintf(str, slen, "%d", atoi(limit));
    return TRUE;
  } else if (!strcasecmp(field, "trim")) {                /* trim      */
    /* trim whitespace from ends */
    snprintf(tmpvar, sizeof(tmpvar)-1 , "%s", vd->value); /* -1 to use later*/
    p = tmpvar;
    p2 = tmpvar + strlen(tmpvar) - 1;
    while (*p && isspace(*p)) p++;
    while ((p<=p2) && isspace(*p2)) p2--;
    if (p>p2) { /* nothing left */
      *str = '\0';
      return TRUE;
    }
    *(++p2) = '\0';                                         /* +1 ok (see above) */
    snprintf(str, slen, "%s", p);
    return TRUE;
  } else if (!strcasecmp(field, "contains")) {            /* contains  */
    if (str_str(vd->value, subfield))
      strcpy(str, "1");
    else
      strcpy(str, "0");
    return TRUE;
  } else if (!strcasecmp(field, "car")) {                 /* car       */
    char *car = vd->value;
    while (*car && !isspace(*car))
      *str++ = *car++;
    *str = '\0';
    return TRUE;

  } else if (!strcasecmp(field, "cdr")) {                 /* cdr       */
    char *cdr = vd->value;
    while (*cdr && !isspace(*cdr)) cdr++; /* skip 1st field */
    while (*cdr && isspace(*cdr)) cdr++;  /* skip to next */

    snprintf(str, slen, "%s", cdr);
    return TRUE;
  } else if (!strcasecmp(field, "charat")) {              /* CharAt    */
    size_t len = strlen(vd->value), dgindex = atoi(subfield);
    if (dgindex > len || dgindex < 1)
      strcpy(str, "");
    else
    	snprintf(str, slen, "%c", vd->value[dgindex - 1]);
    return TRUE;
  } else if (!strcasecmp(field, "mudcommand")) {
    /* find the mud command returned from this text */
/* NOTE: you may need to replace "cmd_info" with "complete_cmd_info", */
/* depending on what patches you've got applied.                      */
    extern const struct command_info cmd_info[];
/* on older source bases:    extern struct command_info *cmd_info; */
    int length, cmd;
    for (length = strlen(vd->value), cmd = 0;
         *cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(cmd_info[cmd].command, vd->value, length))
        break;

    if (*cmd_info[cmd].command == '\n')
      *str = '\0';
    else
      snprintf(str, slen, "%s", cmd_info[cmd].command);
    return TRUE;
  }

  return FALSE;
}


/* sets str to be the value of var.field */
void find_replacement(void *go, struct script_data *sc, trig_data *trig, int type, char *var, char *field, char *subfield, char *str, size_t slen)
{
  struct trig_var_data *vd=NULL;
  char_data *ch, *c = NULL, *rndm;
  obj_data *obj, *o = NULL;
  struct room_data *room, *r = NULL;
  char *name;
  int num, count, i, j, doors;

  char *send_cmd[]       = {"msend ",       "osend ",       "wsend "      };
  char *echo_cmd[]       = {"mecho ",       "oecho ",       "wecho "      };
  char *echoaround_cmd[] = {"mechoaround ", "oechoaround ", "wechoaround "};
  char *door[]           = {"mdoor ",       "odoor ",       "wdoor "      };
  char *force[]          = {"mforce ",      "oforce ",      "wforce "     };
  char *load[]           = {"mload ",       "oload ",       "wload "      };
  char *purge[]          = {"mpurge ",      "opurge ",      "wpurge "     };
  char *teleport[]       = {"mteleport ",   "oteleport ",   "wteleport "  };
  /* the x kills a 'shadow' warning in gcc. */
  char *xdamage[]        = {"mdamage ",     "odamage ",     "wdamage "    };
  char *zoneecho[]       = {"mzoneecho ",   "ozoneecho ",   "wzoneecho "  };
  char *asound[]         = {"masound ",     "oasound ",     "wasound "    };
  char *at[]             = {"mat ",         "oat ",         "wat "        };
  /* there is no such thing as wtransform, thus the wecho below  */
  char *transform[]      = {"mtransform ",  "otransform ",  "wecho "      };
  char *recho[]          = {"mrecho ",      "orecho ",      "wrecho "     };

  *str = '\0';

  /* X.global() will have a NULL trig */
  if (trig)
    for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
      if (!strcasecmp(vd->name, var))
        break;

  /* some evil waitstates could crash the mud if sent here with sc==NULL*/
  if (!vd && sc)
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!strcasecmp(vd->name, var) &&
          (vd->context==0 || vd->context==sc->context))
        break;

  if (!*field) {
    if (vd)
      snprintf(str, slen, "%s", vd->value);
    else {
      if (!strcasecmp(var, "self")) {
        switch (type) {
        case MOB_TRIGGER:
          snprintf(str, slen, "%c%d", UID_CHAR, GET_ID((char_data *) go));
          break;
        case OBJ_TRIGGER:
          snprintf(str, slen, "%c%d", UID_CHAR, GET_ID((obj_data *) go));
          break;
        case WLD_TRIGGER:
          snprintf(str, slen, "%c%d", UID_CHAR, ((room_data *)go)->number + ROOM_ID_BASE);
          break;
        }
      }
      else if (!strcasecmp(var, "global")) {
        /* so "remote varname %global%" will work */
        snprintf(str, slen, "%d", ROOM_ID_BASE);
        return;
      }
      else if (!strcasecmp(var, "ctime"))
        snprintf(str, slen, "%ld", time(0));
      else if (!strcasecmp(var, "door"))
        snprintf(str, slen, "%s", door[type]);
      else if (!strcasecmp(var, "force"))
        snprintf(str, slen, "%s", force[type]);
      else if (!strcasecmp(var, "load"))
        snprintf(str, slen, "%s", load[type]);
      else if (!strcasecmp(var, "purge"))
        snprintf(str, slen, "%s", purge[type]);
      else if (!strcasecmp(var, "teleport"))
        snprintf(str, slen, "%s", teleport[type]);
      else if (!strcasecmp(var, "damage"))
        snprintf(str, slen, "%s", xdamage[type]);
      else if (!strcasecmp(var, "send"))
        snprintf(str, slen, "%s", send_cmd[type]);
      else if (!strcasecmp(var, "echo"))
        snprintf(str, slen, "%s", echo_cmd[type]);
      else if (!strcasecmp(var, "echoaround"))
        snprintf(str, slen, "%s", echoaround_cmd[type]);
      else if (!strcasecmp(var, "zoneecho"))
        snprintf(str, slen, "%s", zoneecho[type]);
      else if (!strcasecmp(var, "asound"))
        snprintf(str, slen, "%s", asound[type]);
      else if (!strcasecmp(var, "at"))
        snprintf(str, slen, "%s", at[type]);
      else if (!strcasecmp(var, "transform"))
        snprintf(str, slen, "%s", transform[type]);
      else if (!strcasecmp(var, "recho"))
        snprintf(str, slen, "%s", recho[type]);
      else
        *str = '\0';
    }

    return;
  }

  else {
    if (vd) {
      name = vd->value;

      switch (type) {
      case MOB_TRIGGER:
        ch = (char_data *) go;

        if ((o = get_object_in_equip(ch, name)));
        else if ((o = get_obj_in_list(name, ch->carrying)));
        else if (IN_ROOM(ch) != NOWHERE && (c = get_char_in_room(&world[IN_ROOM(ch)], name)));
        else if ((o = get_obj_in_list(name,world[IN_ROOM(ch)].contents)));
        else if ((c = get_char(name)));
        else if ((o = get_obj(name)));
        else if ((r = get_room(name))) {}

        break;
      case OBJ_TRIGGER:
        obj = (obj_data *) go;

        if ((c = get_char_by_obj(obj, name)));
        else if ((o = get_obj_by_obj(obj, name)));
        else if ((r = get_room(name))) {}

        break;
      case WLD_TRIGGER:
        room = (struct room_data *) go;

        if ((c = get_char_by_room(room, name)));
        else if ((o = get_obj_by_room(room, name)));
        else if ((r = get_room(name))) {}

        break;
      }
    }

    else {
      if (!strcasecmp(var, "self")) {
        switch (type) {
        case MOB_TRIGGER:
          c = (char_data *) go;
          r = NULL;
          o = NULL;  /* NULL assignments added to avoid self to always be    */
          break;     /* the room.  - Welcor        */
        case OBJ_TRIGGER:
          o = (obj_data *) go;
          c = NULL;
          r = NULL;
          break;
        case WLD_TRIGGER:
          r = (struct room_data *) go;
          c = NULL;
          o = NULL;
          break;
        }
      }

      else if (!strcasecmp(var, "global")) {
        struct script_data *thescript = SCRIPT(&world[0]);
        *str = '\0';
        if (!thescript) {
          script_log("Attempt to find global var. Apparently the void has no script.");
          return;
        }
        for (vd = thescript->global_vars; vd ; vd = vd->next)
          if (!strcasecmp(vd->name, field))
            break;

        if (vd)
          snprintf(str, slen, "%s", vd->value);

        return;
      }
      else if (!strcasecmp(var, "people")) {
        snprintf(str, slen, "%d",((num = atoi(field)) > 0) ? trgvar_in_room(num) : 0);
        return;
      }
      else if (!strcasecmp(var, "time")) {
        if (!strcasecmp(field, "hour"))
          snprintf(str, slen, "%d", time_info.hours);
        else if (!strcasecmp(field, "day"))
          snprintf(str, slen, "%d", time_info.day + 1);
        else if (!strcasecmp(field, "month"))
          snprintf(str, slen, "%d", time_info.month + 1);
        else if (!strcasecmp(field, "year"))
          snprintf(str, slen, "%d", time_info.year);
        else *str = '\0';
        return;
      }
/*

      %findobj.<room vnum X>(<object vnum/id/name>)%
        - count number of objects in room X with this name/id/vnum
      %findmob.<room vnum X>(<mob vnum Y>)%
        - count number of mobs in room X with vnum Y

for example you want to check how many PC's are in room with vnum 1204.
as PC's have the vnum -1...
you would type:
in any script:
%echo% players in room 1204: %findmob.1204(-1)%

Or say you had a bank, and you want a script to check the number of
bags
of gold (vnum: 1234)
in the vault (vnum: 453) now and then. you can just use
%findobj.453(1234)% and it will return the number of bags of gold.

**/

      /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
      else if (!strcasecmp(var, "findmob")) {
        if (!field || !*field || !subfield || !*subfield) {
          script_log("findmob.vnum(mvnum) - illegal syntax");
          strcpy(str, "0");
        } else {
          room_rnum rrnum = real_room(atoi(field));
          mob_vnum mvnum = atoi(subfield);

          if (rrnum == NOWHERE) {
            script_log("findmob.vnum(ovnum): No room with vnum %d", atoi(field));
            strcpy(str, "0");
          } else {
            for (i = 0, ch = world[rrnum].people; ch; ch = ch->next_in_room)
              if (GET_MOB_VNUM(ch) == mvnum)
                i++;

            snprintf(str, slen, "%d", i);
          }
        }
      }
      /* addition inspired by Jamie Nelson - mordecai@xtra.co.nz */
      else if (!strcasecmp(var, "findobj")) {
        if (!field || !*field || !subfield || !*subfield) {
          script_log("findobj.vnum(ovnum) - illegal syntax");
          strcpy(str, "0");
        } else {
          room_rnum rrnum = real_room(atoi(field));

          if (rrnum == NOWHERE) {
            script_log("findobj.vnum(ovnum): No room with vnum %d", atoi(field));
            strcpy(str, "0");
          } else {
            /* item_in_list looks within containers as well. */
            snprintf(str, slen, "%d", item_in_list(subfield, world[rrnum].contents));
          }
        }
      }
      else if (!strcasecmp(var, "random")) {
        if (!strcasecmp(field, "char")) {
          rndm = NULL;
          count = 0;

          if (type == MOB_TRIGGER) {
            ch = (char_data *) go;
            for (c = world[IN_ROOM(ch)].people; c; c = c->next_in_room)
              if ((c != ch) && valid_dg_target(c, DG_ALLOW_GODS) &&
                  CAN_SEE(ch, c)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          else if (type == OBJ_TRIGGER) {
            for (c = world[obj_room((obj_data *) go)].people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {
                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          else if (type == WLD_TRIGGER) {
            for (c = ((struct room_data *) go)->people; c;
                 c = c->next_in_room)
              if (valid_dg_target(c, DG_ALLOW_GODS)) {

                if (!rand_number(0, count))
                  rndm = c;
                count++;
              }
          }

          if (rndm)
            snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(rndm));
          else
            *str = '\0';
        }

        else if (!strcasecmp(field, "dir")) {
          room_rnum in_room = NOWHERE;

          switch (type) {
            case WLD_TRIGGER:
              in_room = real_room(((struct room_data *) go)->number);
              break;
            case OBJ_TRIGGER:
              in_room = obj_room((struct obj_data *) go);
              break;
            case MOB_TRIGGER:
              in_room = IN_ROOM((struct char_data *)go);
              break;
          }
          if (in_room == NOWHERE) {
            *str = '\0';
          } else {
            doors = 0;
            room = &world[in_room];
            for (i = 0; i < NUM_OF_DIRS ; i++)
              if (R_EXIT(room, i))
                doors++;

            if (!doors) {
              *str = '\0';
            } else {
              for ( ; ; ) {
                doors = rand_number(0, NUM_OF_DIRS-1);
                if (R_EXIT(room, doors))
                  break;
              }
              snprintf(str, slen, "%s", dirs[doors]);
            }
          }
        }
        else
          snprintf(str, slen, "%d", ((num = atoi(field)) > 0) ? rand_number(1, num) : 0);

        return;
      }
    }

    if (c) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      else if (!strcasecmp(field, "global")) { /* get global of something else */
        if (IS_NPC(c) && c->script) {
          find_replacement(go, c->script, NULL, MOB_TRIGGER,
            subfield, NULL, NULL, str, slen);
        }
      }
      /* set str to some 'non-text' first */
      *str = '\x1';

      switch (LOWER(*field)) {
        case 'a':
          if (!strcasecmp(field, "aaaaa")) {
               strcpy(str, "0");
          }
          else if (!strcasecmp(field, "affect")) {
             if (subfield && *subfield) {
               int affect = get_flag_by_name(affected_bits, subfield);
               if (affect != NOFLAG && AFF_FLAGGED(c, affect))
                 strcpy(str, "1");
               else
                 strcpy(str, "0");
             } else
               strcpy(str, "0");
          }
          else if (!strcasecmp(field, "alias"))
            snprintf(str, slen, "%s", GET_PC_NAME(c));

          else if (!strcasecmp(field, "align")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
             GET_ALIGNMENT(c) = MAX(-1000, MIN(addition, 1000));
            }
        	snprintf(str, slen, "%d", GET_ALIGNMENT(c));
          }
          break;
        case 'b':
          if (!strcasecmp(field, "bank")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_BANK_GOLD(c) += addition;
            }
            snprintf(str, slen, "%d", GET_GOLD(c));
          }
          break;
        case 'c':
          if (!strcasecmp(field, "canbeseen")) {
            if ((type == MOB_TRIGGER) && !CAN_SEE(((char_data *)go), c))
              strcpy(str, "0");
            else
              strcpy(str, "1");
          }
          else if (!strcasecmp(field, "carry")) {
            if (!IS_NPC(c) && CARRYING(c))
             strcpy(str, "1");
            else
             strcpy(str, "0");
          }
          else if (!strcasecmp(field, "clan")) {
            if (GET_CLAN(c) != NULL && strstr(GET_CLAN(c), subfield))
             strcpy(str, "1");
            else
             strcpy(str, "0");
          }
          else if (!strcasecmp(field, "class")) {
           if (!IS_NPC(c))
            snprintf(str, slen, "%s", pc_class_types[(int)GET_CLASS(c)]);
           else
            snprintf(str, slen, "blank");
          }

          else if (!strcasecmp(field, "con")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_CON(c) += addition;
              if (GET_CON(c) > max) GET_CON(c) = max;
              if (GET_CON(c) < 3) GET_CON(c) = 3;
            }
            snprintf(str, slen, "%d", GET_CON(c));
          }
          else if (!strcasecmp(field, "cha")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_CHA(c) += addition;
              if (GET_CHA(c) > max) GET_CHA(c) = max;
              if (GET_CHA(c) < 3) GET_CHA(c) = 3;
            }
            snprintf(str, slen, "%d", GET_CHA(c));
          }
          break;
        case 'd':
          if (!strcasecmp(field, "dead")) {
           if (AFF_FLAGGED(c, AFF_SPIRIT))
            strcpy(str, "1");
           else
            strcpy(str, "0");
          }
          else if (!strcasecmp(field, "death")) {
            snprintf(str, slen, "%ld", GET_DTIME(c));
          }
          else if (!strcasecmp(field, "dex")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_DEX(c) += addition;
              if (GET_DEX(c) > max) GET_DEX(c) = max;
              if (GET_DEX(c) < 3) GET_DEX(c) = 3;
            }
            snprintf(str, slen, "%d", GET_DEX(c));
          }
          else if (!strcasecmp(field, "drag")) {
            if (!IS_NPC(c) && DRAGGING(c))
             strcpy(str, "1");
            else
             strcpy(str, "0");
          }
          else if (!strcasecmp(field, "drunk")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
            GET_COND(c, DRUNK) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, DRUNK));
          }
          break;
        case 'e':
          if (!strcasecmp(field, "eq")) {
            int pos;
            if (!subfield || !*subfield)
              *str = '\0';
            else if (*subfield == '*') {
              for (i = 0, j = 0; i < NUM_WEARS; i++)
                if (GET_EQ(c, i)) {
                  j++;
                  break;
                }
              if (j > 0)
                strcpy(str,"1");
              else
                *str = '\0';
            } else if ((pos = find_eq_pos_script(subfield)) < 0 || !GET_EQ(c, pos))
              *str = '\0';
            else
              snprintf(str, slen, "%c%d",UID_CHAR, GET_ID(GET_EQ(c, pos)));
          }
          if (!strcasecmp(field, "exp")) {
            if (subfield && *subfield) {
              int64_t addition = MIN(atoi(subfield), 2100000000);

              gain_exp(c, addition);
            }
            snprintf(str, slen, "%"I64T"", GET_EXP(c));
          }
          break;
        case 'f':
          if (!strcasecmp(field, "fighting")) {
            if (FIGHTING(c))
              snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(FIGHTING(c)));
            else
              *str = '\0';
          }
          else if (!strcasecmp(field, "flying")) {
               if (AFF_FLAGGED(c, AFF_FLYING))
                 strcpy(str, "1");
               else
                 strcpy(str, "0");
          }
          else if (!strcasecmp(field, "follower")) {
            if (!c->followers || !c->followers->follower)
              *str = '\0';
            else
              snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(c->followers->follower));
          }
          break;
        case 'g':
          if (!strcasecmp(field, "gold")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_GOLD(c) += addition;
            }
            snprintf(str, slen, "%d", GET_GOLD(c));
          }
          break;
        case 'h':
          if (!strcasecmp(field, "has_item")) {
            if (!(subfield && *subfield))
              *str = '\0';
            else
              snprintf(str, slen, "%d", char_has_item(subfield, c));
          }
          else if (!strcasecmp(field, "hisher"))
            snprintf(str, slen, "%s", HSHR(c));

          else if (!strcasecmp(field, "heshe"))
            snprintf(str, slen, "%s", HSSH(c));

          else if (!strcasecmp(field, "himher"))
            snprintf(str, slen, "%s", HMHR(c));

          else if (!strcasecmp(field, "hitp")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_HIT(c) += addition;
              update_pos(c);
            }
            snprintf(str, slen, "%"I64T"", GET_HIT(c));
          }
          else if (!strcasecmp(field, "hunger")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_COND(c, HUNGER) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, HUNGER));
          }
          break;
        case 'i':
          if (!strcasecmp(field, "id"))
            snprintf(str, slen, "%d", GET_ID(c));

          /* new check for pc/npc status */
          else if (!strcasecmp(field, "is_pc")) {
            if (IS_NPC(c))
              strcpy(str, "0");
            else
              strcpy(str, "1");
          }

          else if (!strcasecmp(field, "inventory")) {
            if(subfield && *subfield) {
              for (obj = c->carrying;obj;obj=obj->next_content) {
                if(GET_OBJ_VNUM(obj)==atoi(subfield)) {
                  snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(obj)); /* arg given, found */
                  return;
                }
              }
              if (!obj)
                *str = '\0'; /* arg given, not found */
            } else { /* no arg given */
              if (c->carrying) {
                snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(c->carrying));
              } else {
                *str = '\0';
              }
            }
          }

          else if (!strcasecmp(field, "is_killer")) {
            if (subfield && *subfield) {
              if (!strcasecmp("on", subfield))
                SET_BIT_AR(PLR_FLAGS(c), PLR_KILLER);
              else if (!strcasecmp("off", subfield))
                REMOVE_BIT_AR(PLR_FLAGS(c), PLR_KILLER);
            }
            if (PLR_FLAGGED(c, PLR_KILLER))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          }

          else if (!strcasecmp(field, "is_thief")) {
            if (subfield && *subfield) {
              if (!strcasecmp("on", subfield))
                SET_BIT_AR(PLR_FLAGS(c), PLR_THIEF);
              else if (!strcasecmp("off", subfield))
                REMOVE_BIT_AR(PLR_FLAGS(c), PLR_THIEF);
            }
            if (PLR_FLAGGED(c, PLR_THIEF))
              strcpy(str, "1");
            else
              strcpy(str, "0");
          }

          else if (!strcasecmp(field, "int")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_INT(c) += addition;
              if (GET_INT(c) > max) GET_INT(c) = max;
              if (GET_INT(c) < 3) GET_INT(c) = 3;
            }
            snprintf(str, slen, "%d", GET_INT(c));
          }
          break;
        case 'l':
          if (!strcasecmp(field, "level"))
            snprintf(str, slen, "%d", GET_LEVEL(c));
          break;
        case 'm':
          if (!strcasecmp(field, "maxhitp")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_MAX_HIT(c) = MAX(GET_MAX_HIT(c) + addition, 1);
            }
            snprintf(str, slen, "%"I64T"", GET_MAX_HIT(c));
          }

          else if (!strcasecmp(field, "mana")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_MANA(c) += addition;
            }
            snprintf(str, slen, "%"I64T"", GET_MANA(c));
          }
          else if (!strcasecmp(field, "maxmana")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_MAX_MANA(c) = MAX(GET_MAX_MANA(c) + addition, 1);
            }
            snprintf(str, slen, "%"I64T"", GET_MAX_MANA(c));
          }

          else if (!strcasecmp(field, "move")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_MOVE(c) += addition;
            }
            snprintf(str, slen, "%"I64T"", GET_MOVE(c));
          }

          else if (!strcasecmp(field, "maxmove")) {
            if (subfield && *subfield) {
              int64_t addition = atol(subfield);
              GET_MAX_MOVE(c) = MAX(GET_MAX_MOVE(c) + addition, 1);
            }
            snprintf(str, slen, "%"I64T"", GET_MAX_MOVE(c));
          }

          else if (!strcasecmp(field, "master")) {
            if (!c->master)
              *str = '\0';
            else
              snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(c->master));
          }
          break;
        case 'n':
          if (!strcasecmp(field, "name")) {
             snprintf(str, slen, "%s", GET_NAME(c));
          }
          else if (!strcasecmp(field, "next_in_room")) {
            if (c->next_in_room)
              snprintf(str, slen,"%c%d",UID_CHAR, GET_ID(c->next_in_room));
            else
              *str = '\0';
          }
          break;
        case 'p':
          /* Thanks to Christian Ejlertsen for this idea
             And to Ken Ray for speeding the implementation up :)*/
          if (!strcasecmp(field, "pos")) {
            if (subfield && *subfield) {
              for (i = POS_SLEEPING; i <= POS_STANDING; i++) {
                /* allows : Sleeping, Resting, Sitting, Fighting, Standing */
                if (!strncasecmp(subfield, position_types[i], strlen(subfield))) {
                  GET_POS(c) = i;
                  break;
                }
              }
            }
            snprintf(str, slen, "%s", position_types[GET_POS(c)]);
          }
          else if (!strcasecmp(field, "prac")) {
            if (IS_NPC(c)) {
             if (IN_ROOM(c) != NOWHERE) {
              send_to_room(IN_ROOM(c), "Error!: Report this trigger error to the coding authorities!\r\n");
             }
            }
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_PRACTICES(c, GET_CLASS(c)) = MAX(0, GET_PRACTICES(c, GET_CLASS(c)) + addition);
            }
            snprintf(str, slen, "%d", GET_PRACTICES(c, GET_CLASS(c)));
          }
          else if (!strcasecmp(field, "plr")) {
             if (subfield && *subfield) { 
               int plr = get_flag_by_name(player_bits, subfield); 
               if (plr != NOFLAG && PLR_FLAGGED(c, plr)) 
                 strcpy(str, "1"); 
               else 
                 strcpy(str, "0"); 
             } else 
               strcpy(str, "0"); 
           }
          else if (!strcasecmp(field, "pref")) {
             if (subfield && *subfield) { 
               int pref = get_flag_by_name(preference_bits, subfield); 
               if (pref != NOFLAG && PRF_FLAGGED(c, pref)) 
                 strcpy(str, "1"); 
               else 
                 strcpy(str, "0"); 
             } else 
               strcpy(str, "0"); 
           }
          break;
        case 'r':
          if (!strcasecmp(field, "room")) {  /* in NOWHERE, return the void */
/* see note in dg_scripts.h */
#ifdef ACTOR_ROOM_IS_UID
            snprintf(str, slen, "%c%d",UID_CHAR,
               (IN_ROOM(c)!= NOWHERE) ? world[IN_ROOM(c)].number + ROOM_ID_BASE : ROOM_ID_BASE);
#else
            snprintf(str, slen, "%d", (IN_ROOM(c)!= NOWHERE) ? world[IN_ROOM(c)].number : 0);
#endif
          }
#ifdef GET_RACE
          else if (!strcasecmp(field, "race")) {
            if IS_NPC(c) {
              sprinttype(GET_RACE(c), race_names, str, slen);
             /* *str='\0';*/
            } else {
              sprinttype(GET_RACE(c), race_names, str, slen);
            }
          }
#endif
          else if (!strcasecmp(field, "rpp")) {
           if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_RP(c) += addition;
            }

            snprintf(str, slen, "%d", GET_RP(c));
          }

          break;
        case 's':
          if (!strcasecmp(field, "sex"))
            snprintf(str, slen, "%s", genders[(int)GET_SEX(c)]);

          else if (!strcasecmp(field, "str")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_STR(c) += addition;
              if (GET_STR(c) > max) GET_STR(c) = max;
              if (GET_STR(c) < 3) GET_STR(c) = 3;
            }
            snprintf(str, slen, "%d", GET_STR(c));
          }

          else if (!strcasecmp(field, "size")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = search_block(subfield, size_names, FALSE)) > -1) {
                (c)->size = ns;
              }
            }
          sprinttype(get_size(c), size_names, str, slen);
          }

          else if (!strcasecmp(field, "skill"))
            snprintf(str, slen, "%s", skill_percent(c, subfield));

          else if (!strcasecmp(field, "skillset")) {
            if (!IS_NPC(c) && subfield && *subfield) {
              char skillname[MAX_INPUT_LENGTH], *amount;
              amount = one_word(subfield, skillname);
              skip_spaces(&amount);
              if (amount && *amount && is_number(amount)) {
                int skillnum = find_skill_num(skillname, SKTYPE_SKILL);
                if (skillnum > 0) {
                  int new_value = MAX(0, MIN(100, atoi(amount)));
                  SET_SKILL(c, skillnum, new_value);
                }
              }
            }
            *str = '\0'; /* so the parser know we recognize 'skillset' as a field */
          }
          else if (!strcasecmp(field, "saving_fortitude")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE_MOD(c, SAVING_FORTITUDE) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE_MOD(c, SAVING_FORTITUDE));
          }
          else if (!strcasecmp(field, "saving_reflex")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE_MOD(c, SAVING_REFLEX) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE_MOD(c, SAVING_REFLEX));
          }
          else if (!strcasecmp(field, "saving_will")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_SAVE_MOD(c, SAVING_WILL) += addition;
            }
            snprintf(str, slen, "%d", GET_SAVE_MOD(c, SAVING_WILL));
          }

          break;
        case 't':
          if (!strcasecmp(field, "thirst")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_COND(c, THIRST) = MAX(-1, MIN(addition, 24));
            }
            snprintf(str, slen, "%d", GET_COND(c, THIRST));
          }
          else if (!strcasecmp(field, "tnl")) {
            snprintf(str, slen, "%d", level_exp(c, GET_LEVEL(c) + 1));
          }
         break;
        case 'v':
          if (!strcasecmp(field, "vnum")) {
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", IS_NPC(c) ? (int)(GET_MOB_VNUM(c) == atoi(subfield)) : -1 );
            } else {
              if (IS_NPC(c))
                snprintf(str, slen, "%d", GET_MOB_VNUM(c));
              else
              /*
               * for compatibility with unsigned indexes
               * - this is deprecated - use %actor.is_pc% to check
               * instead of %actor.vnum% == -1  --Welcor 09/03
               */
                strcpy(str, "-1");
            }
          }

          else if (!strcasecmp(field, "varexists")) {
            struct trig_var_data *remote_vd;
            strcpy(str, "0");
            if (SCRIPT(c)) {
              for (remote_vd = SCRIPT(c)->global_vars; remote_vd; remote_vd = remote_vd->next) {
                if (!strcasecmp(remote_vd->name, subfield)) break;
              }
              if (remote_vd) strcpy(str, "1");
            }
          }

          break;
        case 'w':
          if (!strcasecmp(field, "weight"))
            snprintf(str, slen, "%d", GET_WEIGHT(c));
          else if (!strcasecmp(field, "wis")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              int max = 100;
              GET_WIS(c) += addition;
              if (GET_WIS(c) > max) GET_WIS(c) = max;
              if (GET_WIS(c) < 3) GET_WIS(c) = 3;
            }
            snprintf(str, slen, "%d", GET_WIS(c));
          }
          break;
        case 'z':
          if (!strcasecmp(field, "zenni")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_GOLD(c) += addition;
            }
            snprintf(str, slen, "%d", GET_GOLD(c));
          }
        break;
      } /* switch *field */

      if (*str == '\x1') { /* no match found in switch */
        if (SCRIPT(c)) {
          for (vd = (SCRIPT(c))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d. unknown char field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), field);
        }
      }
    } /* if (c) ...*/

    else if (o) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      *str = '\x1';
      switch (LOWER(*field)) {
        case 'a':
          if (!strcasecmp(field, "affects")) {
            if (subfield && *subfield) {
              if (check_flags_by_name_ar(GET_OBJ_PERM(o), NUM_AFF_FLAGS, subfield, affected_bits) > 0)
                snprintf(str, slen, "1");
              else
                snprintf(str, slen, "0");
            } else
              snprintf(str, slen, "0");
          }
          break;
        case 'c':
          if (!strcasecmp(field, "cost")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_OBJ_COST(o) = MAX(0, addition + GET_OBJ_COST(o));
            }
            snprintf(str, slen, "%d", GET_OBJ_COST(o));
          }

          else if (!strcasecmp(field, "cost_per_day")) {
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_OBJ_RENT(o) = MAX(0, addition + GET_OBJ_RENT(o));
            }
            snprintf(str, slen, "%d", GET_OBJ_RENT(o));
          }

          else if (!strcasecmp(field, "carried_by")) {
            if (o->carried_by)
              snprintf(str, slen,"%c%d",UID_CHAR, GET_ID(o->carried_by));
            else
              *str = '\0';
          }

          else if (!strcasecmp(field, "contents")) {
            if (o->contains)
              snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(o->contains));
            else
              *str = '\0';
          }
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          else if (!strcasecmp(field, "count")) {
            if (GET_OBJ_TYPE(o) == ITEM_CONTAINER)
              snprintf(str, slen, "%d", item_in_list(subfield, o->contains));
            else
              strcpy(str, "0");
          }
          break;
        case 'e':
          if (!strcasecmp(field, "extra")) {
            if (subfield && *subfield) {
              if (check_flags_by_name_ar(GET_OBJ_EXTRA(o), NUM_ITEM_FLAGS, subfield, extra_bits) > 0)
                snprintf(str, slen, "1");
              else
                snprintf(str, slen, "0");
            } else 
              snprintf(str, slen, "0");
          } else {
            sprintbitarray(GET_OBJ_EXTRA(o), extra_bits, EF_ARRAY_MAX, str);
          }
          break;
        case 'h':
          /* thanks to Jamie Nelson (Mordecai of 4 Dimensions MUD) */
          if (!strcasecmp(field, "has_in")) {
            if (GET_OBJ_TYPE(o) == ITEM_CONTAINER)
              snprintf(str, slen, "%s", (item_in_list(subfield, o->contains) ? "1" : "0"));
            else
            	strcpy(str, "0");
          }
          if (!strcasecmp(field, "health")) {
           if (subfield && *subfield) {
              int addition = atoi(subfield);
              GET_OBJ_VAL(o, VAL_ALL_HEALTH) = MAX(1, addition + GET_OBJ_VAL(o, VAL_ALL_HEALTH));
              if (OBJ_FLAGGED(o, ITEM_BROKEN) && GET_OBJ_VAL(o, VAL_ALL_HEALTH) >= 100)
               REMOVE_BIT_AR(GET_OBJ_EXTRA(o), ITEM_BROKEN);
           }
             snprintf(str, slen, "%d", GET_OBJ_VAL(o, VAL_ALL_HEALTH));
          }
          break;
        case 'i':
          if (!strcasecmp(field, "id"))
            snprintf(str, slen, "%d", GET_ID(o));

          else if (!strcasecmp(field, "is_inroom")) {
            if (IN_ROOM(o) != NOWHERE)
              snprintf(str, slen,"%c%d",UID_CHAR, world[IN_ROOM(o)].number + ROOM_ID_BASE);
            else
              *str = '\0';
          }
          else if (!strcasecmp(field, "is_pc")) {
            strcpy(str, "-1");
          }
          else if (!strcasecmp(field, "itemflag")) {
             if (subfield && *subfield) {
               int item = get_flag_by_name(extra_bits, subfield);
               if (item != NOFLAG && OBJ_FLAGGED(o, item))
                 strcpy(str, "1");
               else
                 strcpy(str, "0");
             } else
               strcpy(str, "0");
           }
          break;
        case 'l':
          if (!strcasecmp(field, "level"))
            snprintf(str, slen, "%d", GET_OBJ_LEVEL(o));
          break;

        case 'n':
          if (!strcasecmp(field, "name")) {
           if (!subfield || !*subfield)
            snprintf(str, slen, "%s",  o->name);
           else {
            char blah[500];
            sprintf(blah, "%s %s", o->name, subfield);
            o->name = strdup(blah);
           }
          }
          else if (!strcasecmp(field, "next_in_list")) {
            if (o->next_content)
              snprintf(str, slen,"%c%d",UID_CHAR, GET_ID(o->next_content));
            else
              *str = '\0';
          }
          break;
        case 'r':
          if (!strcasecmp(field, "room")) {
            if (obj_room(o) != NOWHERE)
              snprintf(str, slen,"%c%d",UID_CHAR, world[obj_room(o)].number + ROOM_ID_BASE);
            else
              *str = '\0';
          }
          break;
        case 's':
          if (!strcasecmp(field, "shortdesc")) {
           if (!subfield || !*subfield)
            snprintf(str, slen, "%s",  o->short_description);
           else {
            char blah[500];
            sprintf(blah, "%s @wnicknamed @D(@C%s@D)@n", o->short_description, subfield);
            o->short_description = strdup(blah);
           }
          }
          else if (!strcasecmp(field, "setaffects")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = check_flags_by_name_ar(GET_OBJ_PERM(o), NUM_AFF_FLAGS, subfield, affected_bits)) > 0) {
                TOGGLE_BIT_AR(GET_OBJ_PERM(o), ns);
                snprintf(str, slen, "1");
              }
            }
          }
          else if (!strcasecmp(field, "setextra")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = check_flags_by_name_ar(GET_OBJ_EXTRA(o), NUM_ITEM_FLAGS, subfield, extra_bits)) > 0) {
                TOGGLE_BIT_AR(GET_OBJ_EXTRA(o), ns);
                snprintf(str, slen, "1");
              }
            }
          }

          else if (!strcasecmp(field, "size")) {
            if (subfield && *subfield) {
              int ns;
              if ((ns = search_block(subfield, size_names, FALSE)) > -1) {
                (o)->size = ns;
              }
            }
          sprinttype(GET_OBJ_SIZE(o), size_names, str, slen);
          }
          break;
        case 't':
          if (!strcasecmp(field, "type"))
            sprinttype(GET_OBJ_TYPE(o), item_types, str, slen);

          else if (!strcasecmp(field, "timer"))
            snprintf(str, slen, "%d", GET_OBJ_TIMER(o));
          break;
        case 'v':
          if (!strcasecmp(field, "vnum"))
            if (subfield && *subfield) {
              snprintf(str, slen, "%d", (int)(GET_OBJ_VNUM(o) == atoi(subfield)));
            } else {
              snprintf(str, slen, "%d", GET_OBJ_VNUM(o));
            }
          else if (!strcasecmp(field, "val0"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 0));

          else if (!strcasecmp(field, "val1"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 1));

          else if (!strcasecmp(field, "val2"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 2));

          else if (!strcasecmp(field, "val3"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 3));

          else if (!strcasecmp(field, "val4"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 4));

          else if (!strcasecmp(field, "val5"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 5));

          else if (!strcasecmp(field, "val6"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 6));

          else if (!strcasecmp(field, "val7"))
            snprintf(str, slen, "%d", GET_OBJ_VAL(o, 7));
          break;
        case 'w':
          if (!strcasecmp(field, "weight")){
            if (subfield && *subfield) {
              int addition = atoi(subfield);
              if (addition < 0 || addition > 0) {
               GET_OBJ_WEIGHT(o) = MAX(0, addition + GET_OBJ_WEIGHT(o));
              } else {
               GET_OBJ_WEIGHT(o) = 0;
              }
            }
            snprintf(str, slen, "%"I64T"", GET_OBJ_WEIGHT(o));
          }

          else if (!strcasecmp(field, "worn_by")) {
            if (o->worn_by)
              snprintf(str, slen,"%c%d",UID_CHAR, GET_ID(o->worn_by));
            else
              *str = '\0';
          }
          break;
      } /* switch *field */


      if (*str == '\x1') { /* no match in switch */
        if (SCRIPT(o)) { /* check for global var */
          for (vd = (SCRIPT(o))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            if (strcasecmp(GET_TRIG_NAME(trig), "Rename Object")) {
            script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                       GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
            }
          }
        } else {
          *str = '\0';
          if (strcasecmp(GET_TRIG_NAME(trig), "Rename Object")) {
          script_log("Trigger: %s, VNum %d, type: %d. unknown object field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        }
      }
    } /* if (o) ... */

    else if (r) {
      if (text_processed(field, subfield, vd, str, slen)) return;

      /* special handling of the void, as it stores all 'full global' variables */
      if (r->number == 0) {
        if (!SCRIPT(r)) {
          *str = '\0';
          script_log("Trigger: %s, Vnum %d, type %d. Trying to access Global var list of void. Apparently this has not been set up!",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type);
        } else {
          for (vd = (SCRIPT(r))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else
            *str = '\0';
        }
      }

      else if (!strcasecmp(field, "name"))
        snprintf(str, slen, "%s",  r->name);

      else if (!strcasecmp(field, "sector"))
        sprinttype(r->sector_type, sector_types, str, slen);
     
      else if (!strcasecmp(field, "gravity"))
        snprintf(str, slen,"%d",r->gravity);

      else if (!strcasecmp(field, "vnum")) {
        if (subfield && *subfield) {
          snprintf(str, slen, "%d", (int)(r->number == atoi(subfield)));
        } else {
          snprintf(str, slen,"%d",r->number);
        }
      } else if (!strcasecmp(field, "contents")) {
        if (subfield && *subfield) {
          for (obj = r->contents; obj; obj = obj->next_content) {
            if (GET_OBJ_VNUM(obj) == atoi(subfield)) {
              /* arg given, found */
              snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(obj));
              return;
            }
          }
          if (!obj)
            *str = '\0'; /* arg given, not found */
        } else { /* no arg given */
          if (r->contents) {
            snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(r->contents));
          } else {
            *str = '\0';
          }
        }
      }

      else if (!strcasecmp(field, "people")) {
        if (r->people)
          snprintf(str, slen, "%c%d", UID_CHAR, GET_ID(r->people));
        else
          *str = '\0';
      }
      else if (!strcasecmp(field, "id")) {
        room_rnum rnum = real_room(r->number);
        if (rnum != NOWHERE)
          snprintf(str, slen, "%d", world[rnum].number + ROOM_ID_BASE);
        else
          *str = '\0';
      }
      else if (!strcasecmp(field, "weather")) {
        const char *sky_look[] = {
          "sunny",
          "cloudy",
          "rainy",
          "lightning"
        };

        if (!IS_SET_AR(r->room_flags, ROOM_INDOORS))
          snprintf(str, slen, "%s", sky_look[weather_info.sky]);
        else
          *str = '\0';
      }
      else if (!strcasecmp(field, "fishing")) {
       room_rnum thisroom = real_room(r->number);
       if (ROOM_FLAGGED(thisroom, ROOM_FISHING))
         snprintf(str, slen, "1");
       else
         snprintf(str, slen, "0");  
      }
      else if (!strcasecmp(field, "zonenumber"))
        snprintf(str, slen, "%d",  zone_table[r->zone].number);
      else if (!strcasecmp(field, "zonename"))
        snprintf(str, slen, "%s",  zone_table[r->zone].name);
      else if (!strcasecmp(field, "roomflag")) {
        if (subfield && *subfield) {
          room_rnum thisroom = real_room(r->number);
          if (check_flags_by_name_ar(ROOM_FLAGS(thisroom), NUM_ROOM_FLAGS, subfield, room_bits) > 0)
            snprintf(str, slen, "1");
          else
            snprintf(str, slen, "0");
        } else
          snprintf(str, slen, "0");
      }
      else if (!strcasecmp(field, "north")) {
        if (R_EXIT(r, NORTH)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, NORTH)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, NORTH)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, NORTH)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, NORTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, NORTH)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, NORTH)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "east")) {
        if (R_EXIT(r, EAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, EAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, EAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, EAST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, EAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, EAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, EAST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "south")) {
        if (R_EXIT(r, SOUTH)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, SOUTH)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, SOUTH)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, SOUTH)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, SOUTH)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, SOUTH)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, SOUTH)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "west")) {
        if (R_EXIT(r, WEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, WEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, WEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, WEST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, WEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, WEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, WEST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "up")) {
        if (R_EXIT(r, UP)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, UP)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, UP)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, UP)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, UP)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, UP)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, UP)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "down")) {
        if (R_EXIT(r, DOWN)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, DOWN)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, DOWN)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, DOWN)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, DOWN)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, DOWN)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, DOWN)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "northwest")) {
        if (R_EXIT(r, NORTHWEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, NORTHWEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, NORTHWEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, NORTHWEST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, NORTHWEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, NORTHWEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, NORTHWEST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "northeast")) {
        if (R_EXIT(r, NORTHEAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, NORTHEAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, NORTHEAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, NORTHEAST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, NORTHEAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, NORTHEAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, NORTHEAST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "southwest")) {
        if (R_EXIT(r, SOUTHWEST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, SOUTHWEST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, SOUTHWEST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, SOUTHWEST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, SOUTHWEST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, SOUTHWEST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, SOUTHWEST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "southeast")) {
        if (R_EXIT(r, SOUTHEAST)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, SOUTHEAST)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, SOUTHEAST)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, SOUTHEAST)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, SOUTHEAST)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, SOUTHEAST)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, SOUTHEAST)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "inside")) {
        if (R_EXIT(r, INDIR)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, INDIR)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, INDIR)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, INDIR)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, INDIR)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, INDIR)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, INDIR)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else if (!strcasecmp(field, "outside")) {
        if (R_EXIT(r, OUTDIR)) {
          if (subfield && *subfield) {
            if (!strcasecmp(subfield, "vnum"))
              snprintf(str, slen, "%d", GET_ROOM_VNUM(R_EXIT(r, OUTDIR)->to_room));
            else if (!strcasecmp(subfield, "key"))
              snprintf(str, slen, "%d", R_EXIT(r, OUTDIR)->key);
            else if (!strcasecmp(subfield, "bits"))
              sprintbit(R_EXIT(r, OUTDIR)->exit_info ,exit_bits, str, slen);
            else if (!strcasecmp(subfield, "room")) {
              if (R_EXIT(r, OUTDIR)->to_room != NOWHERE)
                snprintf(str, slen, "%c%d", UID_CHAR, world[R_EXIT(r, OUTDIR)->to_room].number + ROOM_ID_BASE);
              else
                *str = '\0';
            }
          } else /* no subfield - default to bits */
            sprintbit(R_EXIT(r, OUTDIR)->exit_info ,exit_bits, str, slen);
        } else
          *str = '\0';
      }
      else {
        if (SCRIPT(r)) { /* check for global var */
          for (vd = (SCRIPT(r))->global_vars; vd; vd = vd->next)
            if (!strcasecmp(vd->name, field))
              break;
          if (vd)
            snprintf(str, slen, "%s", vd->value);
          else {
            *str = '\0';
            script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                         GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
          }
        } else {
          *str = '\0';
          script_log("Trigger: %s, VNum %d, type: %d. unknown room field: '%s'",
                     GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), type, field);
        }
      }
    } /* if (r).. */
  }
}

/*
 * Now automatically checks if the variable has more then one field
 * in it. And if the field returns a name or a script UID or the like
 * it can recurse.
 * If you supply a value like, %actor.int.str% it wont blow up on you
 * either.
 * - Jamie Nelson 31st Oct 2003 01:03
 *
 * Now also lets subfields have variables parsed inside of them
 * so that:
 * %echo% %actor.gold(%actor.gold%)%
 * will double the actors gold every time its called.  etc...
 * - Jamie Nelson 31st Oct 2003 01:24
 */

/* substitutes any variables into line and returns it as buf */
void var_subst(void *go, struct script_data *sc, trig_data *trig,
               int type, char *line, char *buf)
{
  char tmp[MAX_INPUT_LENGTH], repl_str[MAX_INPUT_LENGTH];
  char *var = NULL, *field = NULL, *p = NULL;
  char tmp2[MAX_INPUT_LENGTH];
  char *subfield_p, subfield[MAX_INPUT_LENGTH];
  int left, len;
  int paren_count = 0;
  int dots = 0;

  /* skip out if no %'s */
  if (!strchr(line, '%')) {
    strcpy(buf, line);
    return;
  }
  /*lets just empty these to start with*/
  *repl_str = *tmp = *tmp2 = '\0';

  p = strcpy(tmp, line);
  subfield_p = subfield;

  left = MAX_INPUT_LENGTH - 1;

  while (*p && (left > 0)) {


    /* copy until we find the first % */
    while (*p && (*p != '%') && (left > 0)) {
      *(buf++) = *(p++);
      left--;
    }

    *buf = '\0';

    /* double % */
    if (*p && (*(++p) == '%') && (left > 0)) {
      *(buf++) = *(p++);
      *buf = '\0';
      left--;
      continue;
    }

    /* so it wasn't double %'s */
    else if (*p && (left > 0)) {

      /* search until end of var or beginning of field */
      for (var = p; *p && (*p != '%') && (*p != '.'); p++);

      field = p;
      if (*p == '.') {
        *(p++) = '\0';
        dots = 0;
        for (field = p; *p && ((*p != '%')||(paren_count > 0) || (dots)); p++) {
          if (dots > 0) {
            *subfield_p = '\0';
            find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));
            if (*repl_str) {
              snprintf(tmp2, sizeof(tmp2), "eval tmpvr %s", repl_str); //temp var
              process_eval(go, sc, trig, type, tmp2);
              strcpy(var, "tmpvr");
              field = p;
              dots = 0;
              continue;
            }
            dots = 0;
          } else if (*p=='(') {
            *p = '\0';
            paren_count++;
          } else if (*p==')') {
            *p = '\0';
            paren_count--;
          } else if (paren_count > 0) {
            *subfield_p++ = *p;
          } else if (*p=='.') {
            *p = '\0';
            dots++;
          }
        } /* for (field.. */
      } /* if *p == '.' */

      *(p++) = '\0';
      *subfield_p = '\0';

      if (*subfield) {
        var_subst(go, sc, trig, type, subfield, tmp2);
        strcpy(subfield, tmp2);
      }

      find_replacement(go, sc, trig, type, var, field, subfield, repl_str, sizeof(repl_str));

      strncat(buf, repl_str, left);
      len = strlen(repl_str);
      buf += len;
      left -= len;
    } /* else if *p .. */
  } /* while *p .. */
}
