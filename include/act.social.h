#pragma once
#include "structs.h"

// functions
void boot_social_messages(void);
void free_social_messages(void);
void free_action(struct social_messg *mess);
void free_command_list(void);
char *fread_action(FILE *fl, int nr);
void create_command_list(void);

// commands
ACMD(do_action);
ACMD(do_insult);
ACMD(do_gmote);

