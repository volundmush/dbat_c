#pragma once
#include "structs.h"

ACMD(do_tedit);

void tedit_string_cleanup(struct descriptor_data *d, int terminator);
void news_string_cleanup(struct descriptor_data *d, int terminator);
