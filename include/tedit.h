#ifndef __TEDIT_H__
#define __TEDIT_H__

#include "structs.h"

ACMD(do_tedit);
void tedit_string_cleanup(struct descriptor_data *d, int terminator);
void news_string_cleanup(struct descriptor_data *d, int terminator);

#endif