/************************************************************************
 * Generic OLC Library - Zones / genzon.h			v1.0	*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#ifndef __GENZON_H__
#define __GENZON_H__

#include "structs.h"
#include "db.h"

extern zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
extern void remove_room_zone_commands(zone_rnum zone, room_rnum room_num);
extern int save_zone(zone_rnum zone_num);
extern int count_commands(struct reset_com *list);
extern void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
extern void remove_cmd_from_list(struct reset_com **list, int pos);
extern int new_command(struct zone_data *zone, int pos);
extern void delete_zone_command(struct zone_data *zone, int pos);
extern zone_rnum real_zone(zone_vnum vznum);
extern zone_rnum real_zone_by_thing(room_vnum vznum);

/* Make delete_zone() */

#endif