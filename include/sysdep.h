#pragma once

// #include "conf.h"
#include "typestubs.h"

#define CIRCLE_GNU_LIBC_MEMORY_TRACK 0 /* 0 = off, 1 = on */

#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <assert.h>
#include <crypt.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>

#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include "stringutils.h"
#endif

/* Basic system dependencies *******************************************/
#if CIRCLE_GNU_LIBC_MEMORY_TRACK && !defined(HAVE_MCHECK_H)
#error "Cannot use GNU C library memory tracking without <mcheck.h>"
#endif

#ifdef __cplusplus /* C++ */
#define cpp_extern extern
#else /* C */
#define cpp_extern /* Nothing */
#endif

#define HAS_RLIMIT

#define CIRCLE_UNSIGNED_INDEX 0 /* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
#define IDXTYPE uint16_t
#define NOTHING ((IDXTYPE) ~0)
#else
#define IDXTYPE int16_t
#define NOTHING (-1) /* nil reference for objects	*/
#endif
#define NOBODY NOTHING
#define NOWHERE NOTHING
#define NOFLAG NOTHING

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

#define ACMD(name) void(name)(struct char_data * ch, char *argument, int cmd, int subcmd)
#define SPECIAL(name) int(name)(struct char_data * ch, void *me, int cmd, char *argument)
