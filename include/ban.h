//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_BAN_H
#define CIRCLE_BAN_H

#include "structs.h"

extern struct ban_list_element *ban_list;
extern int num_invalid;


void load_banned(void);
int isbanned(char *hostname);
void Free_Invalid_List(void);
int Valid_Name(char *newname);
void Read_Invalid_List(void);

ACMD(do_ban);
ACMD(do_unban);

#endif //CIRCLE_BAN_H
