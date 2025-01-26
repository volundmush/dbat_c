#pragma once
#include "structs.h"

void assedit_setup(struct descriptor_data *d, int number);
void assedit_disp_menu(struct descriptor_data *d);
void assedit_delete(struct descriptor_data *d);
void assedit_edit_extract(struct descriptor_data *d);
void assedit_edit_inroom(struct descriptor_data *d);
void nodigit(struct descriptor_data *d);
void assedit_parse(struct descriptor_data *d, char *arg);

ACMD(do_assedit);

