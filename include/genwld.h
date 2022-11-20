/************************************************************************
 * Generic OLC Library - Rooms / genwld.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#ifndef __GENWLD_H__
#define __GENWLD_H__

#include "structs.h"

extern room_rnum add_room(struct room_data *);

extern bool delete_room(room_rnum rnum);

extern bool save_rooms(zone_rnum zone_num);

extern bool copy_room(struct room_data *to, struct room_data *from);

extern room_rnum duplicate_room(room_vnum to, room_rnum from);

extern bool copy_room_strings(struct room_data *dest, struct room_data *source);

extern bool free_room_strings(struct room_data *);

#endif