/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __COMM_H__
#define __COMM_H__

#include "structs.h"


#if CIRCLE_GNU_LIBC_MEMORY_TRACK
# include <mcheck.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

/* mccp defines */
#define COMPRESS2 86

#define NUM_RESERVED_DESCS	8
#define COPYOVER_FILE "copyover.dat"

/* comm.c */
extern int arena_watch(struct char_data *ch);
extern void send_to_eaves(const char *messg, struct char_data *tch, ...);
extern size_t	send_to_char(struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
extern void send_to_all(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
extern void send_to_room(room_rnum room, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
extern void send_to_outdoor(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
extern void send_to_moon(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
extern void send_to_planet(int type, int planet, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
extern void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
extern void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to);
extern char *act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type);
extern void close_socket(struct descriptor_data *d);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_GMOTE	5
#define TO_SLEEP        (2 << 7)	/* to char, even if sleeping */
#define DG_NO_TRIG      (2 << 8)	/* don't check act trigger   */
#define TO_SNEAKRESIST  (2 << 9)	/* resisted sneaking roll    */
#define TO_HIDERESIST   (2 << 10)	/* resisted hiding roll      */

/* I/O functions */
extern void write_to_q(const char *txt, struct txt_q *queue, int aliased);
extern int write_to_descriptor(socklen_t desc, const char *txt);
extern size_t write_to_output(struct descriptor_data *d, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
extern size_t vwrite_to_output(struct descriptor_data *d, const char *format, va_list args);
extern void string_add(struct descriptor_data *d, char *str);
extern void string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH	22
#define PAGE_WIDTH	79
extern void page_string(struct descriptor_data *d, char *str, int keep_internal);


extern void show_help(struct descriptor_data *t, const char *entry);

/* variables */
extern unsigned long pulse;
extern FILE *logfile;
extern int circle_shutdown, circle_reboot;
extern socklen_t mother_desc;
extern uint16_t port;
extern int buf_switches, buf_largecount, buf_overflows;
extern int no_specials, scheck;
extern bool fCopyOver;
extern char *last_act_message;
extern int to_sleeping;

// functions
extern void free_user(struct descriptor_data *d);
extern void reread_wizlists(int sig);
extern void unrestrict_game(int sig);
extern void reap(int sig);
extern void checkpointing(int sig);
extern void hupsig(int sig);
extern ssize_t perform_socket_read(socklen_t desc, char *read_point, size_t space_left);
extern ssize_t perform_socket_write(socklen_t desc, const char *txt, size_t length);
extern void circle_sleep(struct timeval *timeout);
extern int get_from_q(struct txt_q *queue, char *dest, int *aliased);
extern void init_game(uint16_t port);
extern void signal_setup(void);
extern void game_loop(socklen_t mother_desc);
extern socklen_t init_socket(uint16_t port);
extern int new_descriptor(socklen_t s);
extern int get_max_players(void);
extern int process_output(struct descriptor_data *t);
extern int process_input(struct descriptor_data *t);
extern void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
extern void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
extern void flush_queues(struct descriptor_data *d);
extern void nonblock(socklen_t s);
extern int perform_subst(struct descriptor_data *t, char *orig, char *subst);
extern void record_usage(void);
extern char *make_prompt(struct descriptor_data *point);
extern void check_idle_passwords(void);
extern void check_idle_menu(void);
extern void heartbeat(int heart_pulse);
extern struct in_addr *get_bind_addr(void);

extern int set_sendbuf(socklen_t s);
extern void free_bufpool(void);
extern void setup_log(const char *filename, int fd);
extern int open_logfile(const char *filename, FILE *stderr_fp);
extern void init_descriptor (struct descriptor_data *newd, int desc);

#endif