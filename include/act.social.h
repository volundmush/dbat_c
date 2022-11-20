//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_ACT_SOCIAL_H
#define CIRCLE_ACT_SOCIAL_H

#include "structs.h"

// functions
extern void boot_social_messages(void);

extern void free_social_messages(void);

extern void free_action(struct social_messg *mess);

extern void free_command_list(void);

extern char *fread_action(FILE *fl, int nr);

extern void create_command_list(void);

// commands
extern ACMD(do_action);

extern ACMD(do_insult);

extern ACMD(do_gmote);

#endif //CIRCLE_ACT_SOCIAL_H
