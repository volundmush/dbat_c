#pragma once
/************************************************************************* *  File: reset.h                             Designed for CircleMUD 3.5  *
 *  Usage: implementation of pre-zone resets and post-zone resets         *
 *         functions called before and after a normal zone reset          *
 *                                                                        *
 *  All rights reserved.                                                  *
 *                                                                        *
 *  Copyright (C) 2007 Stefan Cole (a.k.a. Jamdog)                        *
 *  To see this in action, check out AderonMUD                            *
 ************************************************************************ */
#include "structs.h"

/* returned by pre-reset function */
#define PERFORM_NORMAL_RESET 0
#define BLOCK_NORMAL_RESET 1

/* Zone virtual numbers used by reset.c */
#define RESET_GAUNTLET 328

/* Miscellaneous defines */
#define NUM_GAUNTLET_ROOMS 20

struct gauntlet_mob {
    room_vnum vroom;
    mob_vnum vmob;
};

/* Reset functions */
bool pre_reset(zone_vnum znum);
void post_reset(zone_vnum znum);

/* Zone pre-reset functions */
bool prereset_gauntlet_zone(void);
