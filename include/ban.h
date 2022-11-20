//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_BAN_H
#define CIRCLE_BAN_H

#include "structs.h"

extern struct ban_list_element *ban_list;
extern int num_invalid;

extern void load_banned(void);

extern bool isbanned(char *hostname);

extern void Free_Invalid_List(void);

extern bool Valid_Name(char *newname);

extern void Read_Invalid_List(void);

extern ACMD(do_ban);

extern ACMD(do_unban);

#endif //CIRCLE_BAN_H
