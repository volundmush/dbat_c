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
int arena_watch(struct char_data *ch);
void send_to_eaves(const char *messg, struct char_data *tch, ...);
size_t	send_to_char(struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_all(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void	send_to_room(room_rnum room, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_outdoor(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void    send_to_moon(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void    send_to_planet(int type, int planet, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
void	send_to_range(room_vnum start, room_vnum finish, const char *messg, ...) __attribute__ ((format (printf, 3, 4)));
void	perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to);
char	*act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, int type);
void close_socket(struct descriptor_data *d);

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
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
int	write_to_descriptor(socklen_t desc, const char *txt, struct compr *comp);
size_t	write_to_output(struct descriptor_data *d, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
size_t	vwrite_to_output(struct descriptor_data *d, const char *format, va_list args);
void	string_add(struct descriptor_data *d, char *str);
void	string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH	22
#define PAGE_WIDTH	79
void	page_string(struct descriptor_data *d, char *str, int keep_internal);


void   show_help(struct descriptor_data *t, const char *entry);

/* variables */
extern unsigned long pulse;
extern FILE *logfile;
extern int circle_shutdown, circle_reboot;
extern socklen_t mother_desc;
extern uint16_t port;
extern int buf_switches, buf_largecount, buf_overflows;
extern int no_specials, scheck;
extern const char compress_offer[4];
extern bool fCopyOver;
extern char *last_act_message;

// functions
void free_user(struct descriptor_data *d);
void reread_wizlists(int sig);
void unrestrict_game(int sig);
void reap(int sig);
void checkpointing(int sig);
void hupsig(int sig);
ssize_t perform_socket_read(socklen_t desc, char *read_point, size_t space_left);
ssize_t perform_socket_write(socklen_t desc, const char *txt, size_t length, struct compr *comp);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void circle_sleep(struct timeval *timeout);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(uint16_t port);
void signal_setup(void);
void game_loop(socklen_t mother_desc);
socklen_t init_socket(uint16_t port);
int new_descriptor(socklen_t s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);
void timeadd(struct timeval *sum, struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(socklen_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
void check_idle_menu(void);
void heartbeat(int heart_pulse);
struct in_addr *get_bind_addr(void);

int set_sendbuf(socklen_t s);
void free_bufpool(void);
void setup_log(const char *filename, int fd);
int open_logfile(const char *filename, FILE *stderr_fp);
void init_descriptor (struct descriptor_data *newd, int desc);

#endif