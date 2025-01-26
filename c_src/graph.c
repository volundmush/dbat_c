/* ************************************************************************
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "graph.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "maputils.h"
#include "vehicles.h"
#include "act.informative.h"

/* local functions */
int VALID_EDGE(room_rnum x, int y);
void bfs_enqueue(room_rnum room, int dir);
void bfs_dequeue(void);
void bfs_clear_queue(void);

struct bfs_queue_struct {
  room_rnum room;
  char dir;
  struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *bfs_queue_head = 0, *bfs_queue_tail = 0;

/* Utility macros */
#define MARK(room)	(SET_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))
#define TOROOM(x, y)	(world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y)	(EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))

int VALID_EDGE(room_rnum x, int y)
{
  if (world[x].dir_option[y] == NULL || TOROOM(x, y) == NOWHERE)
    return 0;
  if (CONFIG_TRACK_T_DOORS == FALSE && IS_CLOSED(x, y))
    return 0;
  if (ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK) || IS_MARKED(TOROOM(x, y)))
    return 0;

  return 1;
}

void bfs_enqueue(room_rnum room, int dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (bfs_queue_tail) {
    bfs_queue_tail->next = curr;
    bfs_queue_tail = curr;
  } else
    bfs_queue_head = bfs_queue_tail = curr;
}


void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = bfs_queue_head;

  if (!(bfs_queue_head = bfs_queue_head->next))
    bfs_queue_tail = 0;
  free(curr);
}


void bfs_clear_queue(void)
{
  while (bfs_queue_head)
    bfs_dequeue();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(room_rnum src, room_rnum target)
{
  int curr_dir;
  room_rnum curr_room;

  if (src == NOWHERE || target == NOWHERE || src > top_of_world || target > top_of_world) {
    log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src, target, __FILE__);
    return (BFS_ERROR);
  }
  if (src == target)
    return (BFS_ALREADY_THERE);

  /* clear marks first, some OLC systems will save the mark. */
  for (curr_room = 0; curr_room <= top_of_world; curr_room++) {
     UNMARK(curr_room);
  }
  MARK(src);
  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) {
    if (VALID_EDGE(src, curr_dir)) {
      MARK(TOROOM(src, curr_dir));
      bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
    }
  }
  /* now, do the classic BFS. */
  while (bfs_queue_head) {
    if (bfs_queue_head->room == target) {
      curr_dir = bfs_queue_head->dir;
      bfs_clear_queue();
      return (curr_dir);
    } else {
      for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++)
	if (VALID_EDGE(bfs_queue_head->room, curr_dir)) {
	  MARK(TOROOM(bfs_queue_head->room, curr_dir));
	  bfs_enqueue(TOROOM(bfs_queue_head->room, curr_dir), bfs_queue_head->dir);
	}
      bfs_dequeue();
    }
  }

  return (BFS_NO_PATH);
}

/********************************************************
* Functions and Commands which use the above functions. *
********************************************************/


ACMD(do_sradar)
{
  struct obj_data *vehicle = NULL, *controls = NULL;
  int dir = 0, noship = FALSE;
  char arg[MAX_INPUT_LENGTH];
  char planet[20];

  one_argument(argument, arg);

  if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) < 1) {
   send_to_char(ch, "You are not flying a ship, maybe you want detect?\r\n");
   return;
  }

  if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1) {
    send_to_char(ch,"@wYou have nothing to control here!\r\n");
    return;
  }

  if (!PLR_FLAGGED(ch, PLR_PILOTING) && GET_ADMLEVEL(ch) >= 1) {
   noship = TRUE;
  }
  else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0))) ) {
    send_to_char(ch, "@wYou can't find anything to pilot.\r\n");
    return;
  }

  if (noship == FALSE && SECT(IN_ROOM(vehicle)) != SECT_SPACE) {
    send_to_char(ch, "@wYour ship is not in space!\r\n");
    return;
  } if (noship == TRUE && SECT(IN_ROOM(ch)) != SECT_SPACE) {
    send_to_char(ch, "@wYou are not even in space!\r\n");
    return;
  } 

  if (!*arg) {
   if (GET_ADMLEVEL(ch) >= 1 && noship == TRUE) {
    printmap(ch->in_room, ch, 0, -1);
   } else {
    printmap(IN_ROOM(vehicle), ch, 0, GET_OBJ_VNUM(vehicle));
   }
  /*
   send_to_char(ch, "     @D----------------[@C Radar @D]-----------------@n\r\n");
   send_to_char(ch, "    @D|@GEE@D:@w Earth    @gNN@D:@w Namek   @YVV@D:@w Vegeta       @D|@n\r\n");
   send_to_char(ch, "    @D|@CFF@D:@w Frigid   @mKK@D:@w Konack  @BAA@D:@w Aether       @D|@n\r\n");
   send_to_char(ch, "    @D|@MYY@D:@w Yardrat  @CKK@D:@w Kanassa @mAA@D:@w Arlia        @D|@n\r\n");
   send_to_char(ch, "    @D|@WBB@D:@w Buoy     @m&&@D:@w Nebula  @yQQ@D:@w Asteroid     @D|@n\r\n");
   send_to_char(ch, "    @D|@b@1**@n@D:@w Wormhole @DSS@D:@w Station  @r#@D: @wUnknown Ship @D|@n\n");
   send_to_char(ch, "    @D|@6  @n@D:@w Star                                  @D|@n\n"); */
   return;
  }

  if (GET_PING(ch) > 0) {
    send_to_char(ch, "@wYou need to wait a few more seconds before pinging a destination again.\r\n");
    return;
  }

  if (noship == FALSE) {
  if (!strcasecmp(arg, "earth") || !strcasecmp(arg, "Earth")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(40979));
   sprintf(planet, "Earth");
  } else if (!strcasecmp(arg, "frigid") || !strcasecmp(arg, "Frigid")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(30889));
   sprintf(planet, "Frigid");
  } else if (!strcasecmp(arg, "konack") || !strcasecmp(arg, "Konack")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(27065));
   sprintf(planet, "Konack");
  } else if (!strcasecmp(arg, "vegeta") || !strcasecmp(arg, "Vegeta")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(32365));
   sprintf(planet, "Vegeta");
  } else if (!strcasecmp(arg, "aether") || !strcasecmp(arg, "Aether")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(41959));
   sprintf(planet, "Aether");
  } else if (!strcasecmp(arg, "namek") || !strcasecmp(arg, "Namek")) {
   dir = find_first_step(IN_ROOM(vehicle), real_room(42880));
   sprintf(planet, "Namek");
  } else if (!strcasecmp(arg, "buoy1") && GET_RADAR1(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy2") && GET_RADAR2(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy3") && GET_RADAR3(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy1") && GET_RADAR1(ch) > 0) {
   int rad = GET_RADAR1(ch);
   dir = find_first_step(IN_ROOM(vehicle), real_room(rad));
   sprintf(planet, "Buoy One");
  } else if (!strcasecmp(arg, "buoy2") && GET_RADAR2(ch) > 0) {
   int rad = GET_RADAR2(ch);
   dir = find_first_step(IN_ROOM(vehicle), real_room(rad));
   sprintf(planet, "Buoy Two");
  } else if (!strcasecmp(arg, "buoy3") && GET_RADAR3(ch) > 0) {
   int rad = GET_RADAR3(ch);
   dir = find_first_step(IN_ROOM(vehicle), real_room(rad));
   sprintf(planet, "Buoy Three");
  } else {
   send_to_char(ch, "@wThat is not an existing planet.@n\r\n");
   return;
  }
  }

  if (noship == TRUE) {
  if (!strcasecmp(arg, "earth") || !strcasecmp(arg, "Earth")) {
   dir = find_first_step(IN_ROOM(ch), real_room(40979));
   sprintf(planet, "Earth");
  } else if (!strcasecmp(arg, "frigid") || !strcasecmp(arg, "Frigid")) {
   dir = find_first_step(IN_ROOM(ch), real_room(30889));
   sprintf(planet, "Frigid");
  } else if (!strcasecmp(arg, "konack") || !strcasecmp(arg, "Konack")) {
   dir = find_first_step(IN_ROOM(ch), real_room(27065));
   sprintf(planet, "Konack");
  } else if (!strcasecmp(arg, "vegeta") || !strcasecmp(arg, "Vegeta")) {
   dir = find_first_step(IN_ROOM(ch), real_room(32365));
   sprintf(planet, "Vegeta");
  } else if (!strcasecmp(arg, "aether") || !strcasecmp(arg, "Aether")) {
   dir = find_first_step(IN_ROOM(ch), real_room(41959));
   sprintf(planet, "Aether");
  } else if (!strcasecmp(arg, "namek") || !strcasecmp(arg, "Namek")) {
   dir = find_first_step(IN_ROOM(ch), real_room(42880));
   sprintf(planet, "Namek");
  } else if (!strcasecmp(arg, "buoy1") && GET_RADAR1(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy2") && GET_RADAR2(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy3") && GET_RADAR3(ch) <= 0) {
    send_to_char(ch, "@wYou haven't launched that buoy.\r\n");
    return;
  } else if (!strcasecmp(arg, "buoy1") && GET_RADAR1(ch) > 0) {
   int rad = GET_RADAR1(ch);
   dir = find_first_step(IN_ROOM(ch), real_room(rad));
   sprintf(planet, "Buoy One");
  } else if (!strcasecmp(arg, "buoy2") && GET_RADAR2(ch) > 0) {
   int rad = GET_RADAR2(ch);
   dir = find_first_step(IN_ROOM(ch), real_room(rad));
   sprintf(planet, "Buoy Two");
  } else if (!strcasecmp(arg, "buoy3") && GET_RADAR3(ch) > 0) {
   int rad = GET_RADAR3(ch);
   dir = find_first_step(IN_ROOM(ch), real_room(rad));
   sprintf(planet, "Buoy Three");
  } else {
   send_to_char(ch, "@wThat is not an existing planet.@n\r\n");
   return;
  }
  }

  switch (dir) {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "@wThe radar shows that your are already there.@n\r\n");
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "@wYou should be in space to use the radar.@n\r\n");
    break;
   default:
    send_to_char(ch, "@wYour radar detects @C%s@w to the @G%s@n\r\n", planet, dirs[dir]);
    break;
   }
 GET_PING(ch) = 5;
 
}

ACMD(do_radar)
{
  int room = 0, dir, num = 0, found = FALSE, found2 = FALSE, fcount = 0 ;
  struct char_data *tch;
  struct obj_data *obj, *obj2, *next_obj;

  for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
       next_obj = obj2->next_content;
   if (GET_OBJ_VNUM(obj2) == 12 && (!OBJ_FLAGGED(obj2, ITEM_BROKEN)) && (!OBJ_FLAGGED(obj2, ITEM_FORGED))) {
    found2 = TRUE;
   }
  }
  if (found2 == FALSE) {
   send_to_char(ch, "You do not even have a dragon radar!\r\n");
   return;
  }

  if (IS_NPC(ch)) {
   send_to_char(ch, "You are a freaking mob!\r\n");
   return;
  }

 else {
    WAIT_STATE(ch, PULSE_2SEC);
 act("$n holds up a dragon radar and pushes its button.", FALSE, ch, 0, 0, TO_ROOM);
 while (num < 20000) {
 if (real_room(room) != NOWHERE) {
 for (obj = world[real_room(room)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
  if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
   continue;
  } else if (GET_OBJ_VNUM(obj) == 20 || GET_OBJ_VNUM(obj) == 21 || GET_OBJ_VNUM(obj) == 22 || GET_OBJ_VNUM(obj) == 23 || GET_OBJ_VNUM(obj) == 24 || GET_OBJ_VNUM(obj) == 25 || GET_OBJ_VNUM(obj) == 26) {
   dir = find_first_step(IN_ROOM(ch), IN_ROOM(obj));
   fcount += 1;
  switch (dir) {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball right here!\r\n", fcount);
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n", fcount);
    break;
   default:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball %s of here.\r\n", fcount, dirs[dir]);
    break;
   }
   found = TRUE;
  }
 }
 for (tch = world[real_room(room)].people; tch; tch = tch->next_in_room) {
  if (tch == ch) {
   continue;
  }
  for (obj = tch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
   if (OBJ_FLAGGED(obj, ITEM_FORGED)) {
    continue;
   } else if (GET_OBJ_VNUM(obj) == 20 || GET_OBJ_VNUM(obj) == 21 || GET_OBJ_VNUM(obj) == 22 || GET_OBJ_VNUM(obj) == 23 || GET_OBJ_VNUM(obj) == 24 || GET_OBJ_VNUM(obj) == 25 || GET_OBJ_VNUM(obj) == 26) {
      dir = find_first_step(IN_ROOM(ch), IN_ROOM(tch));
      fcount += 1;
  switch (dir) {
  case BFS_ERROR:
    send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
    break;
  case BFS_ALREADY_THERE:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball right here!\r\n", fcount);
    break;
  case BFS_NO_PATH:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a faint dragonball signal, but can not direct you further.\r\n", fcount);
    break;
   default:
    send_to_char(ch, "@D<@G%d@D>@w The radar detects a dragonball %s of here.\r\n", fcount, dirs[dir]);
    break;
   }
      found = TRUE;
   }
  }
  }
 }
  num += 1;
  room += 1;
 }
 if (found == FALSE) {
  send_to_char(ch, "The radar didn't detect any dragonballs on the planet.\r\n");
  return;
 }
 }
}

ACMD(do_track)
{
	char arg[MAX_INPUT_LENGTH];
	struct char_data *vict;
	struct descriptor_data *i;
	int count = 0, dir;

	/* The character must have the track skill. */
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SENSE)) {
		send_to_char(ch, "You have no idea how.\r\n");
		return;
	}
	if (GET_SUPPRESS(ch) <= 20 && GET_SUPPRESS(ch) > 0) {
		send_to_char(ch, "You are concentrating too hard on suppressing your powerlevel at this level of suppression.\r\n");
		return;
	}

	one_argument(argument, arg);
	if (!*arg && !FIGHTING(ch)) {
		send_to_char(ch, "Whom are you trying to sense?\r\n");
		return;
	}
	else if (!*arg && FIGHTING(ch)) {
		vict = FIGHTING(ch);
		send_to_char(ch, "You focus on the one your are fighting.\r\n");
		if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
			send_to_char(ch, "You can't sense them.\r\n");
			return;
		}
		if (!read_sense_memory(ch, vict)) {
			send_to_char(ch, "You will remember their ki signal from now on.\r\n");
			sense_memory_write(ch, vict);
		}
		act("You look at $N@n intently for a moment.", TRUE, ch, 0, vict, TO_CHAR);
		act("$n looks at you intently for a moment.", TRUE, ch, 0, vict, TO_VICT);
		act("$n looks at $N@n intently for a moment.", TRUE, ch, 0, vict, TO_NOTVICT);
		if (!IS_ANDROID(vict)) {
			if (GET_ALIGNMENT(vict) > 50 && GET_ALIGNMENT(vict) < 200) {
				send_to_char(ch, "You sense slightly pure and good ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) > 200 && GET_ALIGNMENT(vict) < 500) {
				send_to_char(ch, "You sense a pure and good ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) >= 500) {
				send_to_char(ch, "You sense an extremely pure and good ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) < -50 && GET_ALIGNMENT(vict) > -200) {
				send_to_char(ch, "You sense slightly sour and evil ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) < -200 && GET_ALIGNMENT(vict) > -500) {
				send_to_char(ch, "You sense a sour and evil ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) <= -500) {
				send_to_char(ch, "You sense an extremely evil ki from them.\r\n");
			}
			else if (GET_ALIGNMENT(vict) > -50 && GET_ALIGNMENT(vict) < 50) {
				send_to_char(ch, "You sense slightly mild indefinable ki from them.\r\n");
			}
			if (GET_HIT(vict) > GET_HIT(ch) * 50) {
				send_to_char(ch, "Their power is so huge it boggles your mind and crushes your spirit to fight!\n");
			}
			else if (GET_HIT(vict) > GET_HIT(ch) * 25) {
				send_to_char(ch, "Their power is so much larger than you that you would die like an insect.\n");
			}
			else if (GET_HIT(vict) > GET_HIT(ch) * 10) {
				send_to_char(ch, "Their power is many times larger than your own.\n");
			}
			else if (GET_HIT(vict) > GET_HIT(ch) * 5) {
				send_to_char(ch, "Their power is a great deal larger than your own.\n");
			}
			else if (GET_HIT(vict) > GET_HIT(ch) * 2) {
				send_to_char(ch, "Their power is more than twice as large as your own.\n");
			}
			else if (GET_HIT(vict) > GET_HIT(ch)) {
				send_to_char(ch, "Their power is about twice as large as your own.\n");
			}
			else if (GET_HIT(vict) == GET_HIT(ch)) {
				send_to_char(ch, "Their power is exactly as strong as you.\n");
			}
			else if (GET_HIT(vict) >= GET_HIT(ch) * 0.75) {
				send_to_char(ch, "Their power is about a quarter of your own or larger.\n");
			}
			else if (GET_HIT(vict) >= GET_HIT(ch) * 0.5) {
				send_to_char(ch, "Their power is about half of your own or larger.\n");
			}
			else if (GET_HIT(vict) >= GET_HIT(ch) * 0.25) {
				send_to_char(ch, "Their power is about a quarter of your own or larger.\n");
			}
			else if (GET_HIT(vict) >= GET_HIT(ch) * 0.1) {
				send_to_char(ch, "Their power is about a tenth of your own or larger.\n");
			}
			else if (GET_HIT(vict) >= GET_HIT(ch) * 0.01) {
				send_to_char(ch, "Their power is less than a tenth of your own.\n");
			}
			else if (GET_HIT(vict) < GET_HIT(ch) * 0.01) {
				send_to_char(ch, "Their power is less than 1 percent of your own. What a weakling...\n");
			}
		}
		else {
			send_to_char(ch, "You can't sense their powerlevel as they are a machine.\r\n");
		}
		return;
	}

	/* Scanning the entire planet. */
	if (!strcasecmp(arg, "scan")) {
		for (i = descriptor_list; i; i = i->next) {
			if (STATE(i) != CON_PLAYING) {
				continue;
			}
			else if (IS_ANDROID(i->character)) {
				continue;
			}
			else if (i->character == ch) {
				continue;
			}
			else if (GET_HIT(i->character) < (GET_HIT(ch) * 0.001) + 1) {
				continue;
			}
			else if (planet_check(ch, i->character)) {
				if (readIntro(ch, i->character) == 1) {
					send_to_char(ch, "@D[@Y%d@D] @CYou sense @c%s@C with ", (count + 1), get_i_name(ch, i->character));
				}
				else {
					send_to_char(ch, "@D[@Y%d@D] @CYou sense ", (count + 1));
				}
				/* How strong is the one we sense? */
				if (GET_HIT(i->character) > GET_HIT(ch) * 50) {
					send_to_char(ch, "a power so huge it boggles your mind and crushes your spirit to fight!\n");
				}
				else if (GET_HIT(i->character) > GET_HIT(ch) * 25) {
					send_to_char(ch, "a power so much larger than you that you would die like an insect.\n");
				}
				else if (GET_HIT(i->character) > GET_HIT(ch) * 10) {
					send_to_char(ch, "a power that is many times larger than your own.\n");
				}
				else if (GET_HIT(i->character) > GET_HIT(ch) * 5) {
					send_to_char(ch, "a power that is a great deal larger than your own.\n");
				}
				else if (GET_HIT(i->character) > GET_HIT(ch) * 2) {
					send_to_char(ch, "a power that is more than twice as large as your own.\n");
				}
				else if (GET_HIT(i->character) > GET_HIT(ch)) {
					send_to_char(ch, "a power that is about twice as large as your own.\n");
				}
				else if (GET_HIT(i->character) == GET_HIT(ch)) {
					send_to_char(ch, "a power that is exactly as strong as you.\n");
				}
				else if (GET_HIT(i->character) >= GET_HIT(ch) * 0.75) {
					send_to_char(ch, "a power that is about a quarter of your own or larger.\n");
				}
				else if (GET_HIT(i->character) >= GET_HIT(ch) * 0.5) {
					send_to_char(ch, "a power that is about half of your own or larger.\n");
				}
				else if (GET_HIT(i->character) >= GET_HIT(ch) * 0.25) {
					send_to_char(ch, "a power that is about a quarter of your own or larger.\n");
				}
				else if (GET_HIT(i->character) >= GET_HIT(ch) * 0.1) {
					send_to_char(ch, "a power that is about a tenth of your own or larger.\n");
				}
				else if (GET_HIT(i->character) >= GET_HIT(ch) * 0.01) {
					send_to_char(ch, "a power that is less than a tenth of your own.\n");
				}
				else if (GET_HIT(i->character) < GET_HIT(ch) * 0.01) {
					send_to_char(ch, "a power that is less than 1 percent of your own. What a weakling...\n");
				}

				/* What's their alignment? */

				if (GET_ALIGNMENT(i->character) >= 500) {
					send_to_char(ch, "@wYou sense an extremely pure and good ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) > 200) {
					send_to_char(ch, "@wYou sense a pure and good ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) > 50) {
					send_to_char(ch, "@wYou sense slightly pure and good ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) > -50) {
					send_to_char(ch, "@wYou sense a slightly mild indefinable ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) > -200) {
					send_to_char(ch, "@wYou sense a slightly sour and evil ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) > -500) {
					send_to_char(ch, "@wYou sense a sour and evil ki from them.@n\n");
				}
				else if (GET_ALIGNMENT(i->character) <= -500) {
					send_to_char(ch, "@wYou sense an extremely evil ki from them.@n\n");
				}

				char *blah = sense_location(i->character);
				send_to_char(ch, "@wLastly you sense that they are at... @C%s@n\n", blah);
				++count;
				free(blah);
			}
		}
		if (count == 0) {
			send_to_char(ch, "You sense that there is no one important around.@n\n");
		}
		return;
	}

	/* The person can't see the victim. */

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
		send_to_char(ch, "No one is around by that name.\r\n");
		return;
	}

	/* We can't track the victim. */
	if (AFF_FLAGGED(vict, AFF_NOTRACK) || IS_ANDROID(vict)) {
		send_to_char(ch, "You can't sense them.\r\n");
		return;
	}

	if (GET_HIT(vict) < (GET_HIT(ch) * 0.001) + 1) {
		if (IN_ROOM(ch) == IN_ROOM(vict)) {
			if (!read_sense_memory(ch, vict)) {
				send_to_char(ch, "Their powerlevel is too weak for you to sense properly, but you will recognise their ki signal from now on.\r\n");
				sense_memory_write(ch, vict);
			}
			else {
				send_to_char(ch, "Their powerlevel is too weak for you to sense properly.\r\n");
			}
		}
		else {
			send_to_char(ch, "Their powerlevel is too weak for you to sense properly.\r\n");
		}
		return;
	}

	if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_YARDRAT)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_YARDRAT))))
	{
		send_to_char(ch, "@WSense@D: @YYardrat@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_EARTH))))
	{
		send_to_char(ch, "@WSense@D: @GEarth@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_VEGETA))))
	{
		send_to_char(ch, "@WSense@D: @YVegeta@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_NAMEK))))
	{
		send_to_char(ch, "@WSense@D: @gNamek@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_FRIGID))))
	{
		send_to_char(ch, "@WSense@D: @CFrigid@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_AETHER))))
	{
		send_to_char(ch, "@WSense@D: @mAetherh@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_KONACK)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_KONACK))))
	{
		send_to_char(ch, "@WSense@D: @MKonack@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_KANASSA)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_KANASSA))))
	{
		send_to_char(ch, "@WSense@D: @cKanassa@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARLIA)) && (ROOM_FLAGGED(IN_ROOM(vict), ROOM_ARLIA))))
	{
		send_to_char(ch, "@WSense@D: @yArlia@n\r\n");
		if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
		{
			char *blah = sense_location(vict);
			send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
			free(blah);
		}
	}
	else if ((GET_SKILL_BASE(ch, SKILL_SENSE) == 100) && (!(PLANET_ZENITH(IN_ROOM(ch))) && (PLANET_ZENITH(IN_ROOM(vict)))))
   {
	send_to_char(ch, "@WSense@D: @CZenith@n\r\n");
	if (vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))
	{
	 char *blah = sense_location(vict);
	 send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
	 free(blah);
	}
   } 
	else 
	{



		if (GET_SKILL(ch, SKILL_SENSE) < rand_number(1, 101)) {
			int tries = 10;
			/* Find a random direction. :) */
			do {
				dir = rand_number(0, NUM_OF_DIRS - 1);
			} while (!CAN_GO(ch, dir) && --tries);
			send_to_char(ch, "You sense them %s faintly from here, but are unsure....\r\n", dirs[dir]);
			improve_skill(ch, SKILL_SENSE, 1);
			improve_skill(ch, SKILL_SENSE, 1);
			improve_skill(ch, SKILL_SENSE, 1);
			return;
		}

		/* They passed the skill check. */
		dir = find_first_step(IN_ROOM(ch), IN_ROOM(vict));

		switch (dir) {
		case BFS_ERROR:
			send_to_char(ch, "Hmm.. something seems to be wrong.\r\n");
			break;
		case BFS_ALREADY_THERE:
			act("You look at $N@n intently for a moment.", TRUE, ch, 0, vict, TO_CHAR);
			act("$n looks at you intently for a moment.", TRUE, ch, 0, vict, TO_VICT);
			act("$n looks at $N intently for a moment.", TRUE, ch, 0, vict, TO_NOTVICT);
			if (!IS_ANDROID(vict)) {
				if (GET_ALIGNMENT(vict) > 50 && GET_ALIGNMENT(vict) < 200) {
					send_to_char(ch, "You sense slightly pure and good ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) > 200 && GET_ALIGNMENT(vict) < 500) {
					send_to_char(ch, "You sense a pure and good ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) >= 500) {
					send_to_char(ch, "You sense an extremely pure and good ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) < -50 && GET_ALIGNMENT(vict) > -200) {
					send_to_char(ch, "You sense slightly sour and evil ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) < -200 && GET_ALIGNMENT(vict) > -500) {
					send_to_char(ch, "You sense a sour and evil ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) <= -500) {
					send_to_char(ch, "You sense an extremely evil ki from them.\r\n");
				}
				else if (GET_ALIGNMENT(vict) > -50 && GET_ALIGNMENT(vict) < 50) {
					send_to_char(ch, "You sense slightly mild indefinable ki from them.\r\n");
				}
			}
			if (!IS_ANDROID(vict)) {
				if (GET_HIT(vict) > GET_HIT(ch) * 50) {
					send_to_char(ch, "Their power is so huge it boggles your mind and crushes your spirit to fight!\n");
				}
				else if (GET_HIT(vict) > GET_HIT(ch) * 25) {
					send_to_char(ch, "Their power is so much larger than you that you would die like an insect.\n");
				}
				else if (GET_HIT(vict) > GET_HIT(ch) * 10) {
					send_to_char(ch, "Their power is many times larger than your own.\n");
				}
				else if (GET_HIT(vict) > GET_HIT(ch) * 5) {
					send_to_char(ch, "Their power is a great deal larger than your own.\n");
				}
				else if (GET_HIT(vict) > GET_HIT(ch) * 2) {
					send_to_char(ch, "Their power is more than twice as large as your own.\n");
				}
				else if (GET_HIT(vict) > GET_HIT(ch)) {
					send_to_char(ch, "Their power is about twice as large as your own.\n");
				}
				else if (GET_HIT(vict) == GET_HIT(ch)) {
					send_to_char(ch, "Their power is exactly as strong as you.\n");
				}
				else if (GET_HIT(vict) >= GET_HIT(ch) * 0.75) {
					send_to_char(ch, "Their power is about a quarter of your own or larger.\n");
				}
				else if (GET_HIT(vict) >= GET_HIT(ch) * 0.5) {
					send_to_char(ch, "Their power is about half of your own or larger.\n");
				}
				else if (GET_HIT(vict) >= GET_HIT(ch) * 0.25) {
					send_to_char(ch, "Their power is about a quarter of your own or larger.\n");
				}
				else if (GET_HIT(vict) >= GET_HIT(ch) * 0.1) {
					send_to_char(ch, "Their power is about a tenth of your own or larger.\n");
				}
				else if (GET_HIT(vict) >= GET_HIT(ch) * 0.01) {
					send_to_char(ch, "Their power is less than a tenth of your own.\n");
				}
				else if (GET_HIT(vict) < GET_HIT(ch) * 0.01) {
					send_to_char(ch, "Their power is less than 1 percent of your own. What a weakling...\n");
				}
				if (!read_sense_memory(ch, vict)) {
					send_to_char(ch, "You will remember their ki signal from now on.\r\n");
					sense_memory_write(ch, vict);
				}
			}
			else {
				send_to_char(ch, "You can't sense their powerlevel as they are a machine.\r\n");
			}
			break;
		case BFS_TO_FAR:
			send_to_char(ch, "You are too far to sense %s accurately from here.\r\n", HMHR(vict));
			break;
		case BFS_NO_PATH:
			send_to_char(ch, "You can't sense %s from here.\r\n", HMHR(vict));
			break;
		default:	/* Success! */
			if ((GET_SKILL_BASE(ch, SKILL_SENSE) >= 75)) {
				char *blah = sense_location(vict);
				send_to_char(ch, "You sense them %s from here!\r\n", dirs[dir]);
				send_to_char(ch, "@WSense@D: @Y%s@n\r\n", blah);
				free(blah);
			}
			else {
				send_to_char(ch, "You sense them %s from here!\r\n", dirs[dir]);
				break;
			}
			WAIT_STATE(ch, PULSE_2SEC);
			improve_skill(ch, SKILL_SENSE, 1);
			improve_skill(ch, SKILL_SENSE, 1);
			improve_skill(ch, SKILL_SENSE, 1);
		}
	}
}

