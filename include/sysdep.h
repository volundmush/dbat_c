#ifndef __SYSDEP_H__
#define __SYSDEP_H__

//#include "conf.h"
#include "typestubs.h"

#define CIRCLE_GNU_LIBC_MEMORY_TRACK	0	/* 0 = off, 1 = on */

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
//#include <limits.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stddef.h>
#include <dirent.h>

#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include "stringutils.h"
#endif

/* Basic system dependencies *******************************************/
#if CIRCLE_GNU_LIBC_MEMORY_TRACK && !defined(HAVE_MCHECK_H)
#error "Cannot use GNU C library memory tracking without <mcheck.h>"
#endif

#if defined(__cplusplus)	/* C++ */
#define cpp_extern	extern
#else				/* C */
#define cpp_extern	/* Nothing */
#endif

#define HAS_RLIMIT

#define CIRCLE_UNSIGNED_INDEX 0	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE	uint64_t
#define NOTHING	((IDXTYPE)~0)
#else
#define IDXTYPE	int64_t
#define NOTHING	(-1)	/* nil reference for objects	*/
#endif

#define NOWHERE NOTHING
#define NOBODY NOTHING
#define NOFLAG NOTHING

#define I64T "lld"
#define SZT "lld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE vnum;
typedef vnum room_vnum;
typedef vnum obj_vnum;
typedef vnum mob_vnum;
typedef vnum zone_vnum;
typedef vnum shop_vnum;
typedef vnum trig_vnum;
typedef vnum guild_vnum;

/* Various real (array-reference) number types. */
typedef vnum room_rnum;
typedef vnum obj_rnum;
typedef vnum mob_rnum;
typedef vnum zone_rnum;
typedef vnum shop_rnum;
typedef vnum trig_rnum;
typedef vnum guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef uint32_t bitvector_t;

#define FALSE 0
#define TRUE 1

#define NO 0
#define YES 1

#define ACMD(name) void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)
typedef int(*SpecialFunc)(struct char_data *ch, void *me, int cmd, char *argument);
#define SPECIAL(name) int (name)(struct char_data *ch, void *me, int cmd, char *argument)

#endif
