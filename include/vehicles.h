//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_VEHICLES_H
#define CIRCLE_VEHICLES_H

#include "structs.h"


// functions
void drive_in_direction(struct char_data *ch, struct obj_data *vehicle, int dir);
struct obj_data *find_control(struct char_data *ch);
struct obj_data *find_vehicle_by_vnum(int vnum);
struct obj_data *find_hatch_by_vnum(int vnum);
struct obj_data *get_obj_in_list_type(int type, struct obj_data *list);


// commands
ACMD(do_warp);
ACMD(do_drive);
ACMD(do_ship_fire);


#endif //CIRCLE_VEHICLES_H
