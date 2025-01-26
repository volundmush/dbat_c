#pragma once
#include "sysdep.h"
void htree_add(IDXTYPE tree_type, IDXTYPE key, IDXTYPE value);
IDXTYPE htree_find(IDXTYPE tree_type, IDXTYPE key);
void htree_del(IDXTYPE tree_type, IDXTYPE key);
