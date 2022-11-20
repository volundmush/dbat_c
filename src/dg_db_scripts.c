/* ************************************************************************
*  File: dg_db_scripts.c                         Part of Death's Gate MUD *
*                                                                         *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
************************************************************************ */

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "constants.h"

extern void half_chop(char *string, char *arg1, char *arg2);

extern bitvector_t asciiflag_conv(char *flag);

/* local functions */
void trig_data_init(struct trig_data *this_data);

void parse_trigger(FILE *trig_f, int nr) {
    int t[2], k, attach_type;
    char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
    struct cmdlist_element *cle;
    struct index_data *t_index;
    struct trig_data *trig;

    CREATE(trig, struct trig_data, 1);
    CREATE(t_index, struct index_data, 1);

    t_index->vnum = nr;
    t_index->number = 0;
    t_index->func = NULL;
    t_index->proto = trig;

    snprintf(errors, sizeof(errors), "trig vnum %d", nr);

    trig->nr = top_of_trigt;
    trig->name = fread_string(trig_f, errors);

    get_line(trig_f, line);
    k = sscanf(line, "%d %s %d", &attach_type, flags, t);
    trig->attach_type = (int8_t) attach_type;
    trig->trigger_type = (long) asciiflag_conv(flags);
    trig->narg = (k == 3) ? t[0] : 0;

    trig->arglist = fread_string(trig_f, errors);

    cmds = s = fread_string(trig_f, errors);

    CREATE(trig->cmdlist, struct cmdlist_element, 1);
    trig->cmdlist->cmd = strdup(strtok(s, "\n\r"));
    cle = trig->cmdlist;

    while ((s = strtok(NULL, "\n\r"))) {
        CREATE(cle->next, struct cmdlist_element, 1);
        cle = cle->next;
        cle->cmd = strdup(s);
    }

    free(cmds);

    trig_index[top_of_trigt++] = t_index;
}


/*
 * create a new trigger from a prototype.
 * nr is the real number of the trigger.
 */
struct trig_data *read_trigger(int nr) {
    struct index_data *t_index;
    struct trig_data *trig;

    if (nr >= top_of_trigt) return NULL;
    if ((t_index = trig_index[nr]) == NULL)
        return NULL;

    CREATE(trig, struct trig_data, 1);
    trig_data_copy(trig, t_index->proto);

    t_index->number++;

    return trig;
}


void trig_data_init(struct trig_data *this_data) {
    this_data->nr = NOTHING;
    this_data->data_type = 0;
    this_data->name = NULL;
    this_data->trigger_type = 0;
    this_data->cmdlist = NULL;
    this_data->curr_state = NULL;
    this_data->narg = 0;
    this_data->arglist = NULL;
    this_data->depth = 0;
    this_data->wait_event = NULL;
    this_data->purged = 0;
    this_data->var_list = NULL;

    this_data->next = NULL;
}


void trig_data_copy(struct trig_data *this_data, const struct trig_data *trg) {
    trig_data_init(this_data);

    this_data->nr = trg->nr;
    this_data->attach_type = trg->attach_type;
    this_data->data_type = trg->data_type;
    if (trg->name)
        this_data->name = strdup(trg->name);
    else {
        this_data->name = strdup("unnamed trigger");
        log("Trigger with no name! (%d)", trg->nr);
    }
    this_data->trigger_type = trg->trigger_type;
    this_data->cmdlist = trg->cmdlist;
    this_data->narg = trg->narg;
    if (trg->arglist) this_data->arglist = strdup(trg->arglist);
}

/* for mobs and rooms: */
void dg_read_trigger(FILE *fp, void *proto, int type) {
    char line[READ_SIZE];
    char junk[8];
    int vnum, rnum, count;
    struct char_data *mob;
    struct room_data *room;
    struct trig_proto_list *trg_proto, *new_trg;

    get_line(fp, line);
    count = sscanf(line, "%7s %d", junk, &vnum);

    if (count != 2) {
        mudlog(BRF, ADMLVL_BUILDER, 1,
               "SYSERR: Error assigning trigger! - Line was\n  %s", line);
        return;
    }

    rnum = real_trigger(vnum);
    if (rnum == NOTHING) {
        switch (type) {
            case MOB_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, 1,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (mob: %s - %d)",
                       vnum, GET_NAME((struct char_data *) proto), GET_MOB_VNUM((struct char_data *) proto));
                break;
            case WLD_TRIGGER:
                mudlog(BRF, ADMLVL_BUILDER, 1,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (room:%d)",
                       vnum, GET_ROOM_VNUM(((struct room_data *) proto)->number));
                break;
            default:
                mudlog(BRF, ADMLVL_BUILDER, 1,
                       "SYSERR: dg_read_trigger: Trigger vnum #%d asked for but non-existant! (?)", vnum);
                break;
        }
        return;
    }

    switch (type) {
        case MOB_TRIGGER:
            CREATE(new_trg, struct trig_proto_list, 1);
            new_trg->vnum = vnum;
            new_trg->next = NULL;

            mob = (struct char_data *) proto;
            trg_proto = mob->proto_script;
            if (!trg_proto) {
                mob->proto_script = trg_proto = new_trg;
            } else {
                while (trg_proto->next)
                    trg_proto = trg_proto->next;
                trg_proto->next = new_trg;
            }
            break;
        case WLD_TRIGGER:
            CREATE(new_trg, struct trig_proto_list, 1);
            new_trg->vnum = vnum;
            new_trg->next = NULL;
            room = (struct room_data *) proto;
            trg_proto = room->proto_script;
            if (!trg_proto) {
                room->proto_script = trg_proto = new_trg;
            } else {
                while (trg_proto->next)
                    trg_proto = trg_proto->next;
                trg_proto->next = new_trg;
            }

            if (rnum != NOTHING) {
                if (!(room->script))
                    CREATE(room->script, struct script_data, 1);
                add_trigger(SCRIPT(room), read_trigger(rnum), -1);
            } else {
                mudlog(BRF, ADMLVL_BUILDER, 1,
                       "SYSERR: non-existant trigger #%d assigned to room #%d",
                       vnum, room->number);
            }
            break;
        default:
            mudlog(BRF, ADMLVL_BUILDER, 1,
                   "SYSERR: Trigger vnum #%d assigned to non-mob/obj/room", vnum);
    }
}

void dg_obj_trigger(char *line, struct obj_data *obj) {
    char junk[8];
    int vnum, rnum, count;
    struct trig_proto_list *trg_proto, *new_trg;

    count = sscanf(line, "%s %d", junk, &vnum);

    if (count != 2) {
        mudlog(BRF, ADMLVL_BUILDER, 1,
               "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  %s", line);
        return;
    }

    rnum = real_trigger(vnum);
    if (rnum == NOTHING) {
        mudlog(BRF, ADMLVL_BUILDER, 1,
               "SYSERR: Trigger vnum #%d asked for but non-existant! (Object: %s - %d)",
               vnum, obj->short_description, GET_OBJ_VNUM(obj));
        return;
    }

    CREATE(new_trg, struct trig_proto_list, 1);
    new_trg->vnum = vnum;
    new_trg->next = NULL;

    trg_proto = obj->proto_script;
    if (!trg_proto) {
        obj->proto_script = trg_proto = new_trg;
    } else {
        while (trg_proto->next) trg_proto = trg_proto->next;
        trg_proto->next = new_trg;
    }
}

void assign_triggers(void *i, int type) {
    struct char_data *mob = NULL;
    struct obj_data *obj = NULL;
    struct room_data *room = NULL;
    int rnum;
    struct trig_proto_list *trg_proto;

    switch (type) {
        case MOB_TRIGGER:
            mob = (struct char_data *) i;
            trg_proto = mob->proto_script;
            while (trg_proto) {
                rnum = real_trigger(trg_proto->vnum);
                if (rnum == NOTHING) {
                    mudlog(BRF, ADMLVL_BUILDER, 1,
                           "SYSERR: trigger #%d non-existant, for mob #%d",
                           trg_proto->vnum, mob_index[mob->nr].vnum);
                } else {
                    if (!SCRIPT(mob))
                        CREATE(SCRIPT(mob), struct script_data, 1);
                    add_trigger(SCRIPT(mob), read_trigger(rnum), -1);
                }
                trg_proto = trg_proto->next;
            }
            break;
        case OBJ_TRIGGER:
            obj = (struct obj_data *) i;
            trg_proto = obj->proto_script;
            while (trg_proto) {
                rnum = real_trigger(trg_proto->vnum);
                if (rnum == NOTHING) {
                    log("SYSERR: trigger #%d non-existant, for obj #%d",
                        trg_proto->vnum, obj_index[obj->item_number].vnum);
                } else {
                    if (!SCRIPT(obj))
                        CREATE(SCRIPT(obj), struct script_data, 1);
                    add_trigger(SCRIPT(obj), read_trigger(rnum), -1);
                }
                trg_proto = trg_proto->next;
            }
            break;
        case WLD_TRIGGER:
            room = (struct room_data *) i;
            trg_proto = room->proto_script;
            while (trg_proto) {
                rnum = real_trigger(trg_proto->vnum);
                if (rnum == NOTHING) {
                    mudlog(BRF, ADMLVL_BUILDER, 1,
                           "SYSERR: trigger #%d non-existant, for room #%d",
                           trg_proto->vnum, room->number);
                } else {
                    if (!SCRIPT(room))
                        CREATE(SCRIPT(room), struct script_data, 1);
                    add_trigger(SCRIPT(room), read_trigger(rnum), -1);
                }
                trg_proto = trg_proto->next;
            }
            break;
        default:
            mudlog(BRF, ADMLVL_BUILDER, 1,
                   "SYSERR: unknown type for assign_triggers()");
            break;
    }
}
