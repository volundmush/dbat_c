//
// Created by basti on 11/12/2022.
//

#ifndef CIRCLE_ITEM_SEARCH_H
#define CIRCLE_ITEM_SEARCH_H

#include "structs.h"

extern bool item_is_usable(struct obj_data *obj);
extern struct obj_data *find_obj_in_list_lambda(struct obj_data *head, bool(*f)(struct obj_data *obj));
extern struct obj_data *find_obj_in_list_vnum(struct obj_data *head, obj_vnum vn);
extern struct obj_data *find_obj_in_list_vnum_good(struct obj_data *head, obj_vnum vn);
extern struct obj_data *find_obj_in_list_type(struct obj_data *head, int item_type);
extern struct obj_data *find_obj_in_list_flag(struct obj_data *head, bitvector_t flag);
extern struct obj_data *find_obj_in_list_name(struct obj_data *head, char *name);
extern struct obj_data *find_obj_in_list_id(struct obj_data *head, long item_id);
#endif //CIRCLE_ITEM_SEARCH_H
