#pragma once
#include "structs.h"

void hedit_parse(struct descriptor_data *, char *);
void hedit_setup_new(struct descriptor_data *);
void hedit_setup_existing(struct descriptor_data *, int);
void hedit_save_to_disk(struct descriptor_data *);
void hedit_save_internally(struct descriptor_data *);
void hedit_string_cleanup(struct descriptor_data *, int);

ACMD(do_oasis_hedit);
ACMD(do_helpcheck);
ACMD(do_hindex);
