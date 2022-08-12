//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_HEDIT_H
#define CIRCLE_HEDIT_H

#include "structs.h"

void hedit_parse(struct descriptor_data *, char *);
static void hedit_setup_new(struct descriptor_data *);
static void hedit_setup_existing(struct descriptor_data *, int);
static void hedit_save_to_disk(struct descriptor_data *);
static void hedit_save_internally(struct descriptor_data *);
void hedit_string_cleanup(struct descriptor_data *, int);

ACMD(do_oasis_hedit);
ACMD(do_helpcheck);
ACMD(do_hindex);

#endif //CIRCLE_HEDIT_H
