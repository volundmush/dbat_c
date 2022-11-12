//
// Created by basti on 10/21/2021.
//

#ifndef CIRCLE_MODIFY_H
#define CIRCLE_MODIFY_H

#include "structs.h"


// functions
extern void paginate_string(char *str, struct descriptor_data *d);
extern void smash_tilde(char *str);
extern void show_string(struct descriptor_data *d, char *input);

// commands
extern ACMD(do_skillset);

#endif //CIRCLE_MODIFY_H
