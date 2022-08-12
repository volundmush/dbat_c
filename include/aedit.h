//
// Created by basti on 10/22/2021.
//

#ifndef CIRCLE_AEDIT_H
#define CIRCLE_AEDIT_H

#include "structs.h"

void aedit_disp_menu(struct descriptor_data * d);
void aedit_parse(struct descriptor_data * d, char *arg);
void aedit_setup_new(struct descriptor_data *d);
void aedit_setup_existing(struct descriptor_data *d, int real_num);
void aedit_save_to_disk(struct descriptor_data *d);
void aedit_save_internally(struct descriptor_data *d);
int aedit_find_command(const char *txt);

// commands
ACMD(do_oasis_aedit);

#endif //CIRCLE_AEDIT_H
