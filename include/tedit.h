#ifndef __TEDIT_H__
#define __TEDIT_H__

#include "structs.h"

extern ACMD(do_tedit);
extern void tedit_string_cleanup(struct descriptor_data *d, int terminator);
extern void news_string_cleanup(struct descriptor_data *d, int terminator);

#endif