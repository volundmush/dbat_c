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
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <crypt.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <zlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stddef.h>
#include <arpa/telnet.h>
#include <dirent.h>
#include <linux/limits.h>

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

#define CIRCLE_UNSIGNED_INDEX 1	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE	uint16_t
#define NOWHERE	((IDXTYPE)~0)
#define NOTHING	((IDXTYPE)~0)
#define NOBODY	((IDXTYPE)~0)
#define NOFLAG  ((IDXTYPE)~0)
#else
#define IDXTYPE	int16_t
#define NOWHERE	(-1)	/* nil reference for rooms	*/
#define NOTHING	(-1)	/* nil reference for objects	*/
#define NOBODY		(-1)	/* nil reference for mobiles	*/
#endif

#define I64T "ld"
#define SZT "ld"
#define TMT "ld"

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE trig_vnum;
typedef IDXTYPE guild_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE trig_rnum;
typedef IDXTYPE guild_rnum;

/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 */
typedef uint32_t bitvector_t;

#define FALSE 0
#define TRUE 1


#define ACMD(name) void (name)(struct char_data *ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int (name)(struct char_data *ch, void *me, int cmd, char *argument)

#endif
