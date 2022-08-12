/*************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "comm.h"
#include "utils.h"
#include "config.h"
#include "maputils.h"
#include "ban.h"
#include "weather.h"
#include "act.wizard.h"
#include "act.misc.h"
#include "house.h"
#include "act.other.h"
#include "dg_comm.h"
#include "handler.h"
#include "dg_scripts.h"
#include "act.item.h"
#include "interpreter.h"
#include "random.h"
#include "act.informative.h"
#include "dg_event.h"
#include "mobact.h"
#include "magic.h"
#include "imc.h"
#include "objsave.h"
#include "genolc.h"
#include "class.h"
#include "combat.h"
#include "modify.h"
#include "fight.h"
#include "local_limits.h"
#include "clan.h"
#include "mail.h"
#include "races.h"
#include "constants.h"
#include "screen.h"

/* externs */


int passcomm(struct char_data *ch, char *comm);


/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics_passed = 0;		/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
struct timeval null_time;	/* zero-valued time structure */
int8_t reread_wizlist;		/* signal: SIGUSR1 */
int8_t emergency_unban;		/* signal: SIGUSR2 */
FILE *logfile = NULL;		/* Where to send the log messages. */
const char *text_overflow = "**OVERFLOW**\r\n";
int dg_act_check;               /* toggle for act_trigger */
unsigned long pulse = 0;        /* number of pulses since game start */
bool fCopyOver;          /* Are we booting in copyover mode? */
uint16_t port;
socklen_t mother_desc;
char *last_act_message = NULL;


void *z_alloc(void *opaque, uInt items, uInt size)
{
    return calloc(items, size);
}

void z_free(void *opaque, void *address)
{
    return free(address);
}

/***********************************************************************
*  main game loop and related stuff                                    *
***********************************************************************/
int enter_player_game(struct descriptor_data *d);

/* first compression neg. string */
const char compress_offer[4] =
{
  (char) IAC,
  (char) WILL,
  (char) COMPRESS2,
  (char) 0,
};


/* Reload players after a copyover */
void copyover_recover()
{
  struct descriptor_data *d;
  FILE *fp;
  char host[1024];
  int desc, player_i;
  bool fOld;
  char name[MAX_INPUT_LENGTH];
  char username[100];
  int saved_loadroom = NOWHERE;
  int set_loadroom = NOWHERE;
	
  log ("Copyover recovery initiated");
  PCOUNTDAY = time(0) + 60;
  fp = fopen (COPYOVER_FILE, "r");
	
  if (!fp) {
    perror ("copyover_recover:fopen");
    log ("Copyover file not found. Exitting.\n\r");
    exit (1);
  }

  unlink (COPYOVER_FILE); /* In case it crashes - doesn't prevent reading */ 
  for (;;) {
    fOld = TRUE;
    fscanf (fp, "%d %s %s %d %s\n", &desc, name, host, &saved_loadroom, username);
    if (desc == -1)
      break;

    /* Write something, and check if it goes error-free */		
    if (write_to_descriptor (desc, "\n\rFolding initiated...\n\r", NULL) < 0) {
      close (desc); /* nope */
      continue;
    }
		
    /* create a new descriptor */
    CREATE (d, struct descriptor_data, 1);
    memset ((char *) d, 0, sizeof (struct descriptor_data));
    init_descriptor (d,desc); /* set up various stuff */
		
    strcpy(d->host, host);
    d->next = descriptor_list;
    descriptor_list = d;

    d->connected = CON_CLOSE;
	
    /* Now, find the pfile */
		
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;

    if ((player_i = load_char(name, d->character)) >= 0) {
     GET_PFILEPOS(d->character) = player_i;
     if (!PLR_FLAGGED(d->character, PLR_DELETED)) {
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
        userLoad(d, username);
     }
     /*else
       fOld = FALSE;*/
    }
    else
     fOld = FALSE;
		
    if (!fOld) /* Player file not found?! */ {
       write_to_descriptor (desc, "\n\rSomehow, your character was lost during the folding. Sorry.\n\r", NULL);
       close_socket(d);
     } else {
       write_to_descriptor (desc, "\n\rFolding complete.\n\r", NULL);
        if (CONFIG_ENABLE_COMPRESSION && !PRF_FLAGGED(d->character, PRF_NOCOMPRESS)) {
            d->comp->state = 1; /* indicates waiting for comp negotiation */
            write_to_output(d, "%s", compress_offer);
        }
       set_loadroom = GET_LOADROOM(d->character);
         GET_LOADROOM(d->character) = saved_loadroom;
       enter_player_game(d);
       GET_LOADROOM(d->character) = set_loadroom;
       d->connected = CON_PLAYING;
       look_at_room(IN_ROOM(d->character), d->character, 0);
       if (AFF_FLAGGED(d->character, AFF_HAYASA)) {
        GET_SPEEDBOOST(d->character) = GET_SPEEDCALC(d->character) * 0.5;
       }
     }
  }
  fclose (fp);
}

/* Init sockets, run game, and cleanup sockets */
void init_game(uint16_t cmport)
{

  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);

  circle_srandom(time(0));

  log("Finding player limit.");
  max_players = get_max_players();

  if (!fCopyOver) { /* If copyover mother_desc is already set up */
  log("Opening mother connection.");
  mother_desc = init_socket(cmport);
  }


  event_init();

  /* set up hash table for find_char() */
  init_lookup_table();

  boot_db();

  if (CONFIG_IMC_ENABLED) {
    imc_startup(FALSE, -1, FALSE); // FALSE arg, so the autoconnect setting can govern it.
  }

       FILE *mapfile;
       int rowcounter, colcounter;
       int vnum_read;

    log("Signal trapping.");
    signal_setup();

  log("Loading Space Map. ");
  
  //Load the map vnums from a file into an array
  mapfile = fopen("../lib/surface.map", "r");
  
  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      fscanf(mapfile, "%d", &vnum_read);
      mapnums[rowcounter][colcounter] = real_room(vnum_read);
    }
  }
  
  fclose(mapfile);

  /* Load the toplist */
  topLoad();

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  if (fCopyOver) /* reload players */
    copyover_recover();

  log("Entering game loop.");

  game_loop(mother_desc);

  Crash_save_all();

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  close(mother_desc);

  if (CONFIG_IMC_ENABLED) {
    imc_shutdown(FALSE);
  }

  if (circle_reboot != 2)
    save_all();

  log("Saving current MUD time.");
  save_mud_time(&time_info);


  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
socklen_t init_socket(uint16_t cmport)
{
  socklen_t s;
  struct sockaddr_in sa;
  int opt;

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("SYSERR: Error creating socket");
        exit(1);
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
        perror("SYSERR: setsockopt REUSEADDR");
        exit(1);
    }

  set_sendbuf(s);

    {
        struct linger ld;

        ld.l_onoff = 0;
        ld.l_linger = 0;
        if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0)
            perror("SYSERR: setsockopt SO_LINGER");	/* Not fatal I suppose. */
    }

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(cmport);
  sa.sin_addr = *(get_bind_addr());

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    close(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return (s);
}


int get_max_players(void)
{

    int max_descs = 0;
    const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
    {
        struct rlimit limit;

        /* find the limit of file descs */
        method = "rlimit";
        if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("SYSERR: calling getrlimit");
            exit(1);
        }

        /* set the current to the maximum */
        limit.rlim_cur = limit.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
            perror("SYSERR: calling setrlimit");
            exit(1);
        }
        if (limit.rlim_max == RLIM_INFINITY)
            max_descs = CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS;
        else
            max_descs = MIN(CONFIG_MAX_PLAYING + NUM_RESERVED_DESCS, limit.rlim_max);
    }

    /* now calculate max _players_ based on max descs */
    max_descs = MIN(CONFIG_MAX_PLAYING, max_descs - NUM_RESERVED_DESCS);

    if (max_descs <= 0) {
        log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
            max_descs, method);
        exit(1);
    }
    log("   Setting player limit to %d using %s.", max_descs, method);
    return (max_descs);
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(socklen_t cmmother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, opt_time, process_time, temp_time;
  struct timeval before_sleep, now, timeout;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int missed_pulses, maxdesc, aliased, top_desc;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  FD_ZERO(&null_set);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
       if (CONFIG_IMC_ENABLED) {
         top_desc = this_imcmud != NULL ? MAX( cmmother_desc, this_imcmud->desc ) : cmmother_desc;
       } else {
         top_desc = cmmother_desc;
       }
      if (!CONFIG_IMC_ENABLED) {
       log("No connections.  Going to sleep.");
      }
      FD_ZERO(&input_set);
      FD_SET(cmmother_desc, &input_set);

       if (CONFIG_IMC_ENABLED) {
         if ( this_imcmud != NULL && this_imcmud->desc != -1 )
            FD_SET(this_imcmud->desc, &input_set);
       }
      if (select(top_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("SYSERR: Select coma");
      } else
         if (!CONFIG_IMC_ENABLED) {
          log("New connection.  Waking up.");
         }
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(cmmother_desc, &input_set);

    maxdesc = cmmother_desc;
    for (d = descriptor_list; d; d = d->next) {
        if (d->descriptor > maxdesc)
            maxdesc = d->descriptor;
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */
    
    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    timediff(&process_time, &before_sleep, &last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
      missed_pulses = 0;
    } else {
      missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
      missed_pulses += process_time.tv_usec / OPT_USEC;
      process_time.tv_sec = 0;
      process_time.tv_usec = process_time.tv_usec % OPT_USEC;
    }

    /* Calculate the time we should wake up */
    timediff(&temp_time, &opt_time, &process_time);
    timeadd(&last_time, &before_sleep, &temp_time);

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timediff(&timeout, &last_time, &now);

    /* Go to sleep */
    do {
      circle_sleep(&timeout);
      gettimeofday(&now, (struct timezone *) 0);
      timediff(&timeout, &last_time, &now);
    } while (timeout.tv_usec || timeout.tv_sec);

    /* Poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("SYSERR: Select poll");
      return;
    }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(cmmother_desc, &input_set))
      new_descriptor(cmmother_desc);

    /* Kick out the freaky folks in the exception set and marked for close */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
        close_socket(d);
    }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      /*
       * Not combined to retain --(d->wait) behavior. -gg 2/20/98
       * If no wait state, no subtraction.  If there is a wait
       * state then 1 is subtracted. Therefore we don't go less
       * than 0 ever and don't require an 'if' bracket. -gg 2/27/99
       */

      if (d->character) {
        GET_WAIT_STATE(d->character) -= (GET_WAIT_STATE(d->character) > 0);

        if (GET_WAIT_STATE(d->character)) {
          continue;
        }
      }

      if (!get_from_q(&d->input, comm, &aliased))
           continue;

      if (d->character) {
	/* Reset the idle timer & pull char back from void if necessary */
	d->character->timer = 0;
	if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE) {
	  if (IN_ROOM(d->character) != NOWHERE)
	    char_from_room(d->character);
	  char_to_room(d->character, GET_WAS_IN(d->character));
	  GET_WAS_IN(d->character) = NOWHERE;
	  act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	}
        GET_WAIT_STATE(d->character) = 1;
      }
      d->has_prompt = FALSE;

      if (d->showstr_count) /* Reading something w/ pager */
	show_string(d, comm);
      else if (d->str)		/* Writing boards, mail, etc. */
	string_add(d, comm);
      else if (STATE(d) != CON_PLAYING) /* In menus, etc. */
	nanny(d, comm);
      else {			/* else: we're playing normally. */
	if (aliased)		/* To prevent recursive aliases. */
	  d->has_prompt = TRUE;	/* To get newline before next cmd output. */
	else if (perform_alias(d, comm, sizeof(comm)))    /* Run it through aliasing system */
	  get_from_q(&d->input, comm, &aliased);
	command_interpreter(d->character, comm); /* Send it to interpreter */
      }
    }

    /* Send queued output out to the operating system (ultimately to user). */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
	/* Output for this player is ready */
	if (process_output(d) < 0) {
        close_socket(d);
	  log("ERROR: Tried to send output to dead socket!");
        }
	else
	  d->has_prompt = 1;
      }
    }

    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next) {
      if (!d->has_prompt) {
        write_to_output(d, "@n");
        /*write_to_descriptor(d->descriptor, make_prompt(d), d->comp);*/
        d->has_prompt = TRUE;
      }
    }

    /* Kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
          close_socket(d);
    }

    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > 30 RL_SEC) {
      log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 RL_SEC;
    }

    if (CONFIG_IMC_ENABLED) {
      imc_loop();
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Check for any signals we may have received. */
    if (reread_wizlist) {
      reread_wizlist = FALSE;
      mudlog(CMP, ADMLVL_IMMORT, TRUE, "Signal received - rereading wizlists.");
      reboot_wizlists();
    }
    if (emergency_unban) {
      emergency_unban = FALSE;
      mudlog(BRF, ADMLVL_IMMORT, TRUE, "Received SIGUSR2 - completely unrestricting game (emergent)");
      ban_list = NULL;
      circle_restrict = 0;
      num_invalid = 0;
    }

      /* Update tics for deadlock protection */
      tics_passed++;
  }
}


void heartbeat(int heart_pulse)
{
  static int mins_since_crashsave = 0;

  event_process();

  if (!(heart_pulse % PULSE_DG_SCRIPT))
    script_trigger_check();

  if (!(heart_pulse % PULSE_ZONE))
    zone_update();

  if (!(heart_pulse % PULSE_IDLEPWD))		/* 15 seconds */
    check_idle_passwords();

  if (!(heart_pulse % (PULSE_1SEC * 60)))           /* 15 seconds */
    check_idle_menu();

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {           /* 1 second */
    dball_load();
  }
  if (!(heart_pulse % (PULSE_2SEC))) {
    base_update();
    fish_update();
  }

  if (!(heart_pulse % (PULSE_1SEC * 15))) {
   handle_songs();
  }

  if (!(heart_pulse % (PULSE_1SEC)))
    wishSYS();

  if (!(heart_pulse % PULSE_MOBILE))
    mobile_activity();

  if (!(heart_pulse % PULSE_AUCTION))
    check_auction();

  if (!(heart_pulse % (PULSE_IDLEPWD / 15))) {
    fight_stack();
  }
  if (!(heart_pulse % ((PULSE_IDLEPWD / 15) * 2))) {
    if (rand_number(1, 2) == 2) {
     homing_update();
    }
    huge_update();
    broken_update();
    /*update_mob_absorb();*/
  }

  if (!(heart_pulse % (1 * PASSES_PER_SEC))) { /* EVERY second */ 
    copyover_check(); 
  }

  if (!(heart_pulse % PULSE_VIOLENCE)) {
    affect_update_violence();
  }

  if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    weather_and_time(1);
    check_time_triggers();
    affect_update();
  }
  if (!(heart_pulse % ((SECS_PER_MUD_HOUR / 3) * PASSES_PER_SEC))) {
    point_update();
  }

  if (CONFIG_AUTO_SAVE && !(heart_pulse % PULSE_AUTOSAVE)) {	/* 1 minute */
      clan_update();
    if (++mins_since_crashsave >= CONFIG_AUTOSAVE_TIME) {
      mins_since_crashsave = 0;
      Crash_save_all();
      House_save_all();
    }
  }

  if (!(heart_pulse % PULSE_USAGE))
    record_usage();

  if (!(heart_pulse % PULSE_TIMESAVE))
    save_mud_time(&time_info);

  if (!(heart_pulse % (30 * PASSES_PER_SEC))) {
    timed_dt(NULL);
   }

  /* Every pulse! Don't want them to stink the place up... */
  extract_pending_chars();
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 */
void timediff(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  if (a->tv_sec < b->tv_sec)
    *rslt = null_time;
  else if (a->tv_sec == b->tv_sec) {
    if (a->tv_usec < b->tv_usec)
      *rslt = null_time;
    else {
      rslt->tv_sec = 0;
      rslt->tv_usec = a->tv_usec - b->tv_usec;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt->tv_sec = a->tv_sec - b->tv_sec;
    if (a->tv_usec < b->tv_usec) {
      rslt->tv_usec = a->tv_usec + 1000000 - b->tv_usec;
      rslt->tv_sec--;
    } else
      rslt->tv_usec = a->tv_usec - b->tv_usec;
  }
}

/*
 * Add 2 time values.
 *
 * Patch sent by "d. hall" <dhall@OOI.NET> to fix 'static' usage.
 */
void timeadd(struct timeval *rslt, struct timeval *a, struct timeval *b)
{
  rslt->tv_sec = a->tv_sec + b->tv_sec;
  rslt->tv_usec = a->tv_usec + b->tv_usec;

  while (rslt->tv_usec >= 1000000) {
    rslt->tv_usec -= 1000000;
    rslt->tv_sec++;
  }
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (IS_PLAYING(d))
      sockets_playing++;
  }

  log("nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);
}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  write_to_output(d, "%s", off_string);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) 0
  };

  write_to_output(d, "%s", on_string);
}


char *make_prompt(struct descriptor_data *d)
{
  static char prompt[MAX_PROMPT_LENGTH];
  struct obj_data *chair = NULL;
  int flagged = FALSE;

  /* Note, prompt is truncated at MAX_PROMPT_LENGTH chars (structs.h) */

  if (d->showstr_count) {
    snprintf(prompt, sizeof(prompt),
	    "\r\n[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]",
	    d->showstr_page, d->showstr_count);
  } else if (d->str) {
     if (STATE(d) == CON_EXDESC) {
     strcpy(prompt, "Enter Description(/h for editor help)> ");
     }
     else if (PLR_FLAGGED(d->character, PLR_WRITING) && !PLR_FLAGGED(d->character, PLR_MAILING)) {
     strcpy(prompt, "Enter Message(/h for editor help)> ");
     }
     else if (PLR_FLAGGED(d->character, PLR_MAILING)) {
     strcpy(prompt, "Enter Mail Message(/h for editor help)> ");
     }
     else {
     strcpy(prompt, "Enter Message> ");
     }
    }
  else if (STATE(d) == CON_PLAYING && !IS_NPC(d->character)) {
    int count;
    size_t len = 0;

    *prompt = '\0';

    if (GET_INVIS_LEV(d->character) && len < sizeof(prompt)) {
      count = snprintf(prompt + len, sizeof(prompt) - len, "i%d ", GET_INVIS_LEV(d->character));
      if (count >= 0)
        len += count;
    }
    /* show only when below 25% */
    if (PRF_FLAGGED(d->character, PRF_DISPAUTO) && GET_LEVEL(d->character) >= 500 && len < sizeof(prompt)) {
      struct char_data *ch = d->character;
      if (GET_HIT(ch) << 2 < GET_MAX_HIT(ch) ) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "PL: %"I64T" ", GET_HIT(ch));
        if (count >= 0)
          len += count;
      }
      if (GET_MOVE(ch) << 2 < GET_MAX_MOVE(ch) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "STA: %"I64T" ", GET_MOVE(ch));
        if (count >= 0)
          len += count;
      }
      if (GET_KI(ch) << 2 < GET_MAX_KI(ch) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "KI: %"I64T" ", GET_KI(ch));
        if (count >= 0)
          len += count;
      }
    } else { /* not auto prompt */
      if (len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@w");
        if (count >= 0)
          len += count;
      }
      if (PLR_FLAGGED(d->character, PLR_SELFD) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RSELF-D@r: @w%s@D]@n", PLR_FLAGGED(d->character, PLR_SELFD2) ? "READY" : "PREP");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (IS_HALFBREED(d->character) && !PLR_FLAGGED(d->character, PLR_FURY) && PRF_FLAGGED(d->character, PRF_FURY)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @r%d@D]@w", GET_FURY(d->character));
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (IS_HALFBREED(d->character) && PLR_FLAGGED(d->character, PLR_FURY) && PRF_FLAGGED(d->character, PRF_FURY)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mFury@W: @rENGAGED@D]@w");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (has_mail(GET_IDNUM(d->character)) && !PRF_FLAGGED(d->character, PRF_NMWARN) && (GET_ADMLEVEL(d->character) > 0) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (GET_KAIOKEN(d->character) > 0 && GET_ADMLEVEL(d->character) > 0) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", GET_KAIOKEN(d->character));
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (GET_SONG(d->character) > 0) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "%s - ", song_types[GET_SONG(d->character)]);
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (GET_KAIOKEN(d->character) > 0 && GET_ADMLEVEL(d->character) <= 0) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "KAIOKEN X%d - ", GET_KAIOKEN(d->character));
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (has_mail(GET_IDNUM(d->character)) && (GET_ADMLEVEL(d->character) <= 0) && !PRF_FLAGGED(d->character, PRF_NMWARN) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "CHECK MAIL - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (d->snooping && d->snooping->character != NULL && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Snooping: (%s) - ", GET_NAME(d->snooping->character));
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (DRAGGING(d->character) && DRAGGING(d->character) != NULL && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Dragging: (%s) - ", GET_NAME(DRAGGING(d->character)));
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_BUILDWALK) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "BUILDWALKING - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (AFF_FLAGGED(d->character, AFF_FLYING) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "FLYING - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (AFF_FLAGGED(d->character, AFF_HIDE) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "HIDING - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (PLR_FLAGGED(d->character, PLR_SPAR) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "SPARRING - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (PLR_FLAGGED(d->character, PLR_NOSHOUT) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "MUTED - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 51 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Bash) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 52 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Headbutt) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 56 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Tailwhip) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 0 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Punch) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 1 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Kick) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 2 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Elbow) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 3 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Knee) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 4 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Roundhouse) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 5 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Uppercut) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 6 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Slam) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (COMBO(d->character) == 8 && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "Combo (Heeldrop) - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_AFK) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "AFK - ");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (PLR_FLAGGED(d->character, PLR_FISHING) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "FISHING -");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (flagged == TRUE && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@n\n");
        if (count >= 0)
          len += count;
      }
      if ((SITS(d->character) && PLR_FLAGGED(d->character, PLR_HEALT)) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        chair = SITS(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "@c<@CFloating inside a healing tank@c>@n\r\n");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if ((SITS(d->character) && GET_POS(d->character) == POS_SITTING) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        chair = SITS(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "Sitting on: %s\r\n", chair->short_description);
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if ((SITS(d->character) && GET_POS(d->character) == POS_RESTING) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        chair = SITS(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "Resting on: %s\r\n", chair->short_description);
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if ((SITS(d->character) && GET_POS(d->character) == POS_SLEEPING) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        chair = SITS(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "Sleeping on: %s\r\n", chair->short_description);
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (AFF_FLAGGED(d->character, AFF_POSITION) && len < sizeof(prompt) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        chair = SITS(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "(Best Position)\r\n");
        flagged = TRUE;
        if (count >= 0)
          len += count;
      }
      if (GET_CHARGE(d->character) < GET_MAX_MANA(d->character) * .01 && GET_CHARGE(d->character) > 0) {
       GET_CHARGE(d->character) = 0;
      }
    if (GET_CHARGE(d->character) > 0) {
     int64_t charge = GET_CHARGE(d->character);
     if (!PRF_FLAGGED(d->character, PRF_NODEC) && !PRF_FLAGGED(d->character, PRF_DISPERC)) {
      if (charge >= GET_MAX_MANA(d->character)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@D<@RMAX@D>@G===@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .95) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========-@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .9) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=========@g-@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .85) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========-@g-@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .80) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G========@g--@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .75) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======-@g--@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .70) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=======@g---@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .65) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======-@g---@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .60) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G======@g----@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .55) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====-@g----@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .50) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=====@g-----@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .45) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====-@g-----@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .40) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G====@g------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .35) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===-@g------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .30) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G===@g-------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .25) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==-@g-------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .20) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G==@g--------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .15) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=-@g--------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G=@g---------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge >= GET_MAX_MANA(d->character) * .05) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@G-@g---------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else if (charge < GET_MAX_MANA(d->character) * .05) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@g----------@D]@n\n");
        if (count >= 0)
          len += count;
       }
      else {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@CCharge @D[@g----------@D]@n\n");
        if (count >= 0)
          len += count;
      }
     }
     if (PRF_FLAGGED(d->character, PRF_DISPERC) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
      if (GET_CHARGE(d->character) > 0) {
        int64_t perc = (GET_CHARGE(d->character) * 100) / GET_MAX_MANA(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@BCharge@Y: @C%"I64T"%s@D]@n\n", perc, "%");
        if (count >= 0)
          len += count;
      }
     }
      if (PRF_FLAGGED(d->character, PRF_NODEC)) {
      if (charge > 0) {
        int64_t perc = (charge * 100) / GET_MAX_MANA(d->character);
        count = snprintf(prompt + len, sizeof(prompt) - len, "Ki is charged to %"I64T" percent.\n", perc);
        if (count >= 0)
          len += count;
       }
      }
      }
      if (AFF_FLAGGED(d->character, AFF_FIRESHIELD)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D(@rF@RI@YR@rE@RS@YH@rI@RE@YL@rD@D)@n\n");
        if (count >= 0)
          len += count;
      }
      if (AFF_FLAGGED(d->character, AFF_SANCTUARY)) {
       if (PRF_FLAGGED(d->character, PRF_DISPERC) && !PRF_FLAGGED(d->character, PRF_NODEC)) {
        if (GET_BARRIER(d->character) > 0) {
         int64_t perc = (GET_BARRIER(d->character) * 100) / GET_MAX_MANA(d->character);
         count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GBarrier@Y: @B%"I64T"%s@D]@n\n", perc, "%");
         if (count >= 0)
          len += count;
        }
       }

      if(!PRF_FLAGGED(d->character, PRF_NODEC) && !PRF_FLAGGED(d->character, PRF_DISPERC)) {
       if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .75) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==MAX==@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .70) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=======@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .65) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======-@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .60) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C======@c-@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .55) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====-@c-@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .50) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=====@c--@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .45) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====-@c--@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .40) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C====@c---@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .35) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===-@c---@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .30) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C===@c----@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .25) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==-@c----@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .20) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C==@c-----@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .15) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=-@c-----@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C=@c------@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) >= GET_MAX_MANA(d->character) * .05) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C-@c------@D]@n\n");
        if (count >= 0)
          len += count;
       }
       else if (GET_BARRIER(d->character) < GET_MAX_MANA(d->character) * .05) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@BBarrier @D[@C--Low-@D]@n\n");
        if (count >= 0)
          len += count;
       }
       }
       if(PRF_FLAGGED(d->character, PRF_NODEC)) {
        if (GET_BARRIER(d->character) > 0) {
         int64_t perc = (GET_BARRIER(d->character) * 100) / GET_MAX_MANA(d->character);
         count = snprintf(prompt + len, sizeof(prompt) - len, "A barrier charged to %"I64T" percent surrounds you.@n\n", perc);
         if (count >= 0)
          len += count;
         }
       }
      }
      if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
       if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt) && GET_HIT(d->character) >= gear_pl(d->character) && GET_HIT(d->character) < GET_MAX_HIT(d->character)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @m%s@D]@n", add_commas(GET_HIT(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt) && GET_HIT(d->character) > GET_MAX_HIT(d->character)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @G%s@D]@n", add_commas(GET_HIT(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt) && GET_HIT(d->character) > (gear_pl(d->character)) / 2) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @c%s@D]@n", add_commas(GET_HIT(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt) && GET_HIT(d->character) > (gear_pl(d->character)) / 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @y%s@D]@n", add_commas(GET_HIT(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPHP) && len < sizeof(prompt) && GET_HIT(d->character) <= (gear_pl(d->character))/ 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @r%s@D]@n", add_commas(GET_HIT(d->character)));
        if (count >= 0)
          len += count;
       }
      } else if (PRF_FLAGGED(d->character, PRF_DISPHP)) {
       int64_t power = GET_HIT(d->character), maxpower = (GET_MAX_HIT(d->character));
       int perc = 0;
       if (power <= 0) {
        power = 1;
       } if (maxpower <= 0) {
        maxpower = 1;
       }
       perc = (power * 100) / maxpower;
       if (perc > 100) {
        if (power >= gear_pl(d->character) && power < GET_MAX_HIT(d->character)) {
         count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @m%d%s@D]@n", perc, "@w%");
        } else if (power > GET_MAX_HIT(d->character)) {
         count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @G%d%s@D]@n", perc, "@w%");
        } else {
         count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @g%d%s@D]@n", perc, "@w%");
        }
         if (count >= 0)
           len += count;
       } else if (perc >= 70) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @c%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 51) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @Y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 20) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@RPL@n@Y: @r%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       }
      }
      if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
       if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) && GET_MANA(d->character) > GET_MAX_MANA(d->character) / 2) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @c%s@D]@n", add_commas(GET_MANA(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) && GET_MANA(d->character) > GET_MAX_MANA(d->character) / 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @y%s@D]@n", add_commas(GET_MANA(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPKI) && len < sizeof(prompt) && GET_MANA(d->character) <= GET_MAX_MANA(d->character) / 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@Y: @r%s@D]@n", add_commas(GET_MANA(d->character)));
        if (count >= 0)
          len += count;
       }
      } else if (PRF_FLAGGED(d->character, PRF_DISPKI)) {
       int64_t power = GET_MANA(d->character), maxpower = GET_MAX_MANA(d->character);
       int perc = 0;
       if (power <= 0) {
        power = 1;
       } if (maxpower <= 0) {
        maxpower = 1;
       }
       perc = (power * 100) / maxpower;
       if (perc > 100) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @G%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 70) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @c%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 51) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @Y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 20) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@CKI@n@Y: @r%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       }
      }
      if (!PRF_FLAGGED(d->character, PRF_DISPERC)) {
       if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) && GET_MOVE(d->character) > GET_MAX_MOVE(d->character) / 2) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @c%s@D]@n", add_commas(GET_MOVE(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) && GET_MOVE(d->character) > GET_MAX_MOVE(d->character) / 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @y%s@D]@n", add_commas(GET_MOVE(d->character)));
        if (count >= 0)
          len += count;
       }
       else if (PRF_FLAGGED(d->character, PRF_DISPMOVE) && len < sizeof(prompt) && GET_MOVE(d->character) <= GET_MAX_MOVE(d->character) / 10) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@Y: @r%s@D]@n", add_commas(GET_MOVE(d->character)));
        if (count >= 0)
          len += count;
       }
      } else if (PRF_FLAGGED(d->character, PRF_DISPMOVE)) {
       int64_t power = GET_MOVE(d->character), maxpower = GET_MAX_MOVE(d->character);
       int perc = 0;
       if (power <= 0) {
        power = 1;
       } if (maxpower <= 0) {
        maxpower = 1;
       }
       perc = (power * 100) / maxpower;
       if (perc > 100) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @G%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 70) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @c%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 51) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @Y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else if (perc >= 20) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @y%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       } else {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@GSTA@n@Y: @r%d%s@D]@n", perc, "@w%");
        if (count >= 0)
          len += count;
       }
      }
      if (PRF_FLAGGED(d->character, PRF_DISPTNL) && len < sizeof(prompt) && GET_LEVEL(d->character) < 100) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@yTNL@Y: @W%s@D]@n", add_commas(level_exp(d->character, GET_LEVEL(d->character) + 1) - GET_EXP(d->character)));
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_DISTIME) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@W%2d %s@D]@n", (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12), time_info.hours >= 12 ? "PM" : "AM");
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_DISGOLD) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@YZen@y: @W%s@D]@n", add_commas(GET_GOLD(d->character)));
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_DISPRAC) && len < sizeof(prompt)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mPS@y: @W%s@D]@n", add_commas(GET_PRACTICES(d->character, GET_CLASS(d->character))));
        if (count >= 0)
          len += count;
      }
      if (PRF_FLAGGED(d->character, PRF_DISHUTH) && len < sizeof(prompt)) {
        int hun = GET_COND(d->character, HUNGER), thir = GET_COND(d->character, THIRST);
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n@D[@mHung@y:");
         if (count >= 0)
          len += count;
         if (hun >= 48) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WFull@D]@n");
         } else if (hun >= 40) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Full@D]@n");
         } else if (hun >= 30) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WNeed Snack@D]@n");
         } else if (hun >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WHungry@D]@n");
         } else if (hun >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WVery Hungry@D]@n");
         } else if (hun >= 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WAlmost Starving@D]@n");
         } else if (hun >= 5) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WNear Starving@D]@n");
         } else if (hun >= 0) {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WStarving@D]@n");
         } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, " @WN/A@D]@n");
         }
         if (count >= 0)
          len += count;
         if (thir >= 48) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WQuenched@D]@n");
         } else if (thir >= 40) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Sip@D]@n");
         } else if (thir >= 30) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNeed Drink@D]@n");
         } else if (thir >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WThirsty@D]@n");
         } else if (thir >= 20) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WVery Thirsty@D]@n");
         } else if (thir >= 10) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WAlmost Dehydrated@D]@n");
         } else if (thir >= 5) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WNear Dehydration@D]@n");
         } else if (thir >= 0) {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WDehydrated@D]@n");
         } else {
          count = snprintf(prompt + len, sizeof(prompt) - len, "@D[@mThir@y: @WN/A@D]@n");
         }
        if (count >= 0)
          len += count;
      }
      if (len < sizeof(prompt) && has_group(d->character) && !PRF_FLAGGED(d->character, PRF_GHEALTH)) {
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n%s", report_party_health(d->character));
        if (d->character->temp_prompt)
          free(d->character->temp_prompt);
        if (count >= 0)
          len += count;
      }
      if (len < sizeof(prompt))
        count = snprintf(prompt + len, sizeof(prompt) - len, "\n"); /* strncat: OK */
    }
    if (len < sizeof(prompt) && len < 5)
      strncat(prompt, ">\n", sizeof(prompt) - len - 1);	/* strncat: OK */

  } else if (STATE(d) == CON_PLAYING && IS_NPC(d->character))
    snprintf(prompt, sizeof(prompt), "%s>\n", CAP(GET_NAME(d->character)));
  else
    *prompt = '\0';

  return (prompt);
}


/*
 * NOTE: 'txt' must be at most MAX_INPUT_LENGTH big.
 */
void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  newt->text = strdup(txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}


/*
 * NOTE: 'dest' must be at least MAX_INPUT_LENGTH big.
 */
int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return (0);

  strcpy(dest, queue->head->text);	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
  *aliased = queue->head->aliased;

  tmp = queue->head;
  queue->head = queue->head->next;
  free(tmp->text);
  free(tmp);

  return (1);
}


/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (d->input.head) {
    struct txt_block *tmp = d->input.head;
    d->input.head = d->input.head->next;
    free(tmp->text);
    free(tmp);
  }
}


/* Add a new string to a player's output queue. For outside use. */
size_t write_to_output(struct descriptor_data *t, const char *txt, ...)
{
  va_list args;
  size_t left;

  va_start(args, txt);
  left = vwrite_to_output(t, txt, args);
  va_end(args);

  return left;
}

#define COLOR_ON(ch) (COLOR_LEV(ch) > 0)

/* Color replacement arrays. Orig. Renx -- 011100, now modified */
char *ANSI[] = {
  "@",
  AA_NORMAL,
  AA_NORMAL ANSISEPSTR AF_BLACK,
  AA_NORMAL ANSISEPSTR AF_BLUE,
  AA_NORMAL ANSISEPSTR AF_GREEN,
  AA_NORMAL ANSISEPSTR AF_CYAN,
  AA_NORMAL ANSISEPSTR AF_RED,
  AA_NORMAL ANSISEPSTR AF_MAGENTA,
  AA_NORMAL ANSISEPSTR AF_YELLOW,
  AA_NORMAL ANSISEPSTR AF_WHITE,
  AA_BOLD ANSISEPSTR AF_BLACK,
  AA_BOLD ANSISEPSTR AF_BLUE,
  AA_BOLD ANSISEPSTR AF_GREEN,
  AA_BOLD ANSISEPSTR AF_CYAN,
  AA_BOLD ANSISEPSTR AF_RED,
  AA_BOLD ANSISEPSTR AF_MAGENTA,
  AA_BOLD ANSISEPSTR AF_YELLOW,
  AA_BOLD ANSISEPSTR AF_WHITE,
  AB_BLACK,
  AB_BLUE,
  AB_GREEN,
  AB_CYAN,
  AB_RED,
  AB_MAGENTA,
  AB_YELLOW,
  AB_WHITE,
  AA_BLINK,
  AA_UNDERLINE,
  AA_BOLD,
  AA_REVERSE,
  "!"
};

const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @

  @[num] - user color choice num, [] are required
*/
const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

#define NEW_STRING_LENGTH (size_t)(dest_char-save_pos)
size_t proc_colors(char *txt, size_t maxlen, int parse, char **choices)
{
  extern char *default_color_choices[NUM_COLOR+1];
  char *dest_char, *source_char, *color_char, *save_pos, *replacement = NULL;
  int i, temp_color;
  size_t wanted;

  if (!txt || !strchr(txt, '@')) /* skip out if no color codes     */
    return strlen(txt);

  source_char = txt;
  CREATE(dest_char, char, maxlen);
  save_pos = dest_char;
  for( ; *source_char && (NEW_STRING_LENGTH < maxlen); ) {
    /* no color code - just copy */
    if (*source_char != '@') {
      *dest_char++ = *source_char++;
      continue;
    }

    /* if we get here we have a color code */

    source_char++; /* source_char now points to the code */

    /* look for a random color code picks a random number between 1 and 14 */
    if (*source_char == 'x') {
      temp_color = (rand() % 14);
      *source_char = RANDOM_COLORS[temp_color];
    }

    if (*source_char == '\0') { /* string was terminated with color code - just put it in */
      *dest_char++ = '@';
      /* source_char will now point to '\0' in the for() check */
      continue;
    }

    if (!parse) { /* not parsing, just skip the code, unless it's @@ */
      if (*source_char == '@') {
        *dest_char++ = '@';
      }
      if (*source_char == '[') { /* Multi-character code */
        source_char++;
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
      source_char++; /* skip to next (non-colorcode) char */
      continue;
    }

    /* parse the color code */
    if (*source_char == '[') { /* User configurable color */
      source_char++;
      if (*source_char) {
        i = atoi(source_char);
        if (i < 0 || i >= NUM_COLOR)
          i = COLOR_NORMAL;
        replacement = default_color_choices[i];
        if (choices && choices[i])
          replacement = choices[i];
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
    } else if (*source_char == 'n') {
        replacement = default_color_choices[COLOR_NORMAL];
        if (choices && choices[COLOR_NORMAL])
          replacement = choices[COLOR_NORMAL];
    } else {
      for (i = 0; CCODE[i] != '!'; i++) { /* do we find it ? */
        if ((*source_char) == CCODE[i]) {           /* if so :*/
          replacement = ANSI[i];
          break;
        }
      }
    }
    if (replacement) {
      if ( NEW_STRING_LENGTH + strlen(replacement) + strlen(ANSISTART) + 1 < maxlen)  { /* only substitute if there's room for the whole code */
        if (isdigit(replacement[0]))
          for(color_char = ANSISTART ; *color_char ; )
            *dest_char++ = *color_char++;
        for(color_char = replacement ; *color_char ; )
          *dest_char++ = *color_char++;
        if (isdigit(replacement[0]))
          *dest_char++ = ANSIEND;
      }
      replacement = NULL;
    }
   /* If we couldn't find any correct color code, or we found it and
    * substituted above, let's just process the next character.
    * - Welcor
    */
    source_char++;

  } /* for loop */

  /* make sure output is NULL - terminated */
  *dest_char = '\0';

  wanted = strlen(source_char); /* see if we wanted more space */
  strncpy(txt, save_pos, maxlen-1);
  free(save_pos); /* plug memory leak */

  return NEW_STRING_LENGTH+wanted;
}
#undef NEW_STRING_LENGTH

/* Add a new string to a player's output queue. */
size_t vwrite_to_output(struct descriptor_data *t, const char *format, va_list args)
{
  static char txt[MAX_STRING_LENGTH];
  size_t wantsize;
  int size;

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufspace == 0)
    return (0);

  wantsize = size = vsnprintf(txt, sizeof(txt), format, args);
  if (t->character)
    wantsize = size = proc_colors(txt, sizeof(txt), COLOR_ON(t->character), COLOR_CHOICES(t->character));
  /* If exceeding the size of the buffer, truncate it for the overflow message */
  if (size < 0 || wantsize >= sizeof(txt)) {
    size = sizeof(txt) - 1;
    strcpy(txt + size - strlen(text_overflow), text_overflow);  /* strcpy: OK */
  }

  /*
   * If the text is too big to fit into even a large buffer, truncate
   * the new text to make it fit.  (This will switch to the overflow
   * state automatically because t->bufspace will end up 0.)
   */
  if (size + t->bufptr + 1 > LARGE_BUFSIZE) {
    size = LARGE_BUFSIZE - t->bufptr - 1;
    txt[size] = '\0';
    GET_OVERFLOW(t->character) = TRUE;
    buf_overflows++;
  }

  /*
   * If we have enough space, just write to buffer and that's it! If the
   * text just barely fits, then it's switched to a large buffer instead.
   */
  if (t->bufspace > size) {
    strcpy(t->output + t->bufptr, txt); /* strcpy: OK (size checked above) */
    t->bufspace -= size;
    t->bufptr += size;
    return (t->bufspace);
  }

  buf_switches++;

  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {                      /* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);     /* strcpy: OK (size checked previously) */
  t->output = t->large_outbuf->text;    /* make big buffer primary */
  strcat(t->output, txt);       /* strcat: OK (size checked) */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);

  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;

  return (t->bufspace);
}

void free_bufpool(void)
{
  struct txt_block *tmp;

  while (bufpool) {
    tmp = bufpool->next;
    if (bufpool->text)
      free(bufpool->text);
    free(bufpool);
    bufpool = tmp;
  }
}

/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *) &bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (CONFIG_DFLT_IP == NULL) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!inet_aton(CONFIG_DFLT_IP, &bind_addr)) {
      log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",
          CONFIG_DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    log("Binding to all IP interfaces on this host.");
  else
    log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return (&bind_addr);
}

/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socklen_t s)
{
    int opt = MAX_SOCK_BUF;

    if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
        perror("SYSERR: setsockopt SNDBUF");
        return (-1);
    }

  return (0);
}

/* Initialize a descriptor */
void init_descriptor (struct descriptor_data *newd, int desc)
{
  static int last_desc = 0; /* last descriptor number */

  newd->descriptor = desc;
  newd->idle_tics = 0;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;
  newd->has_prompt = 1;  /* prompt is part of greetings */
  STATE(newd) = CON_GET_USER;
  CREATE(newd->history, char *, HISTORY_SIZE);
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  CREATE(newd->comp, struct compr, 1);
  newd->comp->state = 0; /* we start in normal mode */
    newd->comp->stream = NULL;
}

void set_color(struct descriptor_data *d)
{
   if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
  SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR);
  write_to_output(d, GREETANSI);
  write_to_output(d, "\r\n@w                  Welcome to Dragonball Advent Truth\r\n");
  write_to_output(d, "@D                 ---(@CPeak Logon Count Today@W: @w%4d@D)---@n\r\n", PCOUNT);
  write_to_output(d, "@D                 ---(@CHighest Logon Count   @W: @w%4d@D)---@n\r\n", HIGHPCOUNT);
  write_to_output(d, "@D                 ---(@CTotal Era %d Characters@W: @w%4s@D)---@n\r\n", CURRENT_ERA, add_commas(ERAPLAYERS));
  write_to_output(d, "\r\n@cEnter your desired username or the username you have already made.\n@CEnter Username:@n\r\n");
  d->user = strdup("Empty");
  d->pass = strdup("Empty");
  d->email = strdup("Empty");
  d->tmp1 = strdup("Empty");
  d->tmp2 = strdup("Empty");
  d->tmp3 = strdup("Empty");
  d->tmp4 = strdup("Empty");
  d->tmp5 = strdup("Empty");
  return;
}

int new_descriptor(socklen_t s)
{
  socklen_t desc;
  int sockets_connected = 0;
  socklen_t i;
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from = NULL;

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) == INVALID_SOCKET) {
    perror("SYSERR: accept");
    return (-1);
  }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
      close(desc);
    return (0);
  }

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= CONFIG_MAX_PLAYING) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n", NULL);
      close(desc);
    return (0);
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);

  /* find the sitename */
  if (CONFIG_NS_IS_SLOW ||
      !(from = gethostbyaddr((char *) &peer.sin_addr,
				      sizeof(peer.sin_addr), AF_INET))) {

    /* resolution failed */

    /* find the numeric site address */
    strncpy(newd->host, (char *)inet_ntoa(peer.sin_addr), HOST_LENGTH);	/* strncpy: OK (n->host:HOST_LENGTH+1) */
    newd->host[HOST_LENGTH] = '\0';
  } else {
    strncpy(newd->host, from->h_name, HOST_LENGTH);	/* strncpy: OK (n->host:HOST_LENGTH+1) */
    newd->host[HOST_LENGTH] = '\0';
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
      close(desc);
    mudlog(CMP, ADMLVL_GOD, TRUE, "Connection attempt denied from [%s]", newd->host);
    free(newd);
    return (0);
  }

  /* initialize descriptor data */
  init_descriptor(newd, desc);

  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;

  set_color(newd);

  return (0);
}


/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 *
 * 32 int8_tGARBAGE_SPACE in MAX_SOCK_BUF used for:
 *	 2 bytes: prepended \r\n
 *	14 bytes: overflow message
 *	 2 bytes: extra \r\n for non-comapct
 *      14 bytes: unused
 */
int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF], *osb = i + 2;
  int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");    /* strcpy: OK (for 'MAX_SOCK_BUF >= 3') */

  /* now, append the 'real' output */
  strcpy(osb, t->output);       /* strcpy: OK (t->output:LARGE_BUFSIZE < osb:MAX_SOCK_BUF-2) */

  /* if we're in the overflow state, notify the user */
  if (t->bufspace == 0)
    strcat(osb, "**OVERFLOW**\r\n");    /* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING && t->character && !IS_NPC(t->character) && !PRF_FLAGGED(t->character, PRF_COMPACT)) 
    strcat(osb, "\r\n");	/* strcpy: OK (osb:MAX_SOCK_BUF-2 reserves space) */

  /* add a prompt */

  strcat(i, make_prompt(t));    /* strcpy: OK (i:MAX_SOCK_BUF reserves space) */
  if (STATE(t) == CON_PLAYING) {
  proc_colors(i, sizeof(i), COLOR_ON(t->character), COLOR_CHOICES(t->character));
  }

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt) {
    t->has_prompt = FALSE;
    result = write_to_descriptor(t->descriptor, i, t->comp);
    if (result >= 2)
      result -= 2;
  } else
    result = write_to_descriptor(t->descriptor, osb, t->comp);

  if (result < 0) {     /* Oops, fatal error. Bye! */
    close_socket(t);
    return (-1);
  } else if (result == 0)       /* Socket buffer full. Try later. */
    return (0);

  /* Handle snooping: prepend "% " and send to snooper. */
  if (t->snoop_by)
    write_to_output(t->snoop_by, "\nvvvvvvvvvvvvv[Snoop]vvvvvvvvvvvvv\n%s\n^^^^^^^^^^^^^[Snoop]^^^^^^^^^^^^^\n", t->output);
  /* The common case: all saved output was handed off to the kernel buffer. */
  if (result >= t->bufptr ) {
    /*
     * if we were using a large buffer, put the large buffer on the buffer pool
     * and switch back to the small one
     */
    if (t->large_outbuf) {
      t->large_outbuf->next = bufpool;
      bufpool = t->large_outbuf;
      t->large_outbuf = NULL;
      t->output = t->small_outbuf;
    }
    /* reset total bufspace back to that of a small buffer */
    t->bufspace = SMALL_BUFSIZE - 1;
    t->bufptr = 0;
    *(t->output) = '\0';

    /*
     * If the overflow message or prompt were partially written, try to save
     * them. There will be enough space for them if this is true.  'result'
     * is effectively unsigned here anyway.
     */
    if ((unsigned int)result < strlen(osb)) {
      size_t savetextlen = strlen(osb + result);

      strcat(t->output, osb + result);
      t->bufptr   -= savetextlen;
      t->bufspace += savetextlen;
    }

  } else {
    /* Not all data in buffer sent.  result < output buffersize. */

    strcpy(t->output, t->output + result);      /* strcpy: OK (overlap) */
    t->bufptr   -= result;
    t->bufspace += result;
  }

  return (result);
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socklen_t desc, const char *txt, size_t length, struct compr *comp)
{
  ssize_t result = 0;

  int compr_result, tmp, cnt, bytes_copied;
  
  /* MCCP! this is where the zlib compression is handled */
  if (comp && comp->state >= 2) { /* compress2 on */
    /* copy data to input buffer */
    /* first check that overflow won't happen */
    /* if it will, we only copy over so much text */
    if (comp->size_in + length > comp->total_in)
      bytes_copied = comp->total_in - comp->size_in;
    else
      bytes_copied = length;
    
    /* now copy what will fit into the buffer */
    strncpy((char *)comp->buff_in + comp->size_in, txt, bytes_copied);
    comp->size_in += bytes_copied;

    /* set up stream input */
    comp->stream->avail_in = comp->size_in;
    comp->stream->next_in = comp->buff_in;
    
    /* lets do it */
    /* deflate all the input - this means flushing our output buffer when it fills */
    do {
      /* set up stream output - the size_out bit is somewhat unnecessary, but makes things safer */
      comp->stream->avail_out = comp->total_out - comp->size_out;
      comp->stream->next_out = comp->buff_out + comp->size_out;
      
      compr_result = deflate(comp->stream, comp->state == 3 ? Z_FINISH : Z_SYNC_FLUSH);
      
      if (compr_result == Z_STREAM_END)
        compr_result = 0;
      else if (compr_result == Z_OK && !(comp->stream->avail_out))
        compr_result = 1;
      else if (compr_result < 0) {  /* uh oh, fatal zlib error */
	result = 0;
	break;
      } else
	compr_result = 0;
    
      /* adjust output state value */
      comp->size_out = comp->total_out - comp->stream->avail_out;

      /* write out compressed data - flush buff_out */
      /* if problems encountered, resort to resending all data by breaking and returning < 1.. */
      tmp = 0;
      while (comp->size_out > 0) {
	result = write(desc, comp->buff_out + tmp, comp->size_out);
	if (result < 1) /* unsuccessful write or socket error */
	  goto exitzlibdo; /* yummy, goto. faster than two breaks ! */ 
	comp->size_out -= result;
	tmp += result;
      }
    } while (compr_result);
exitzlibdo:
    
    /* adjust buffers - is this necessary? not with Z_SYNC_FLUSH I think - but just to be safe */
    /* input loses size_in - avail_in bytes */
    tmp = comp->size_in - comp->stream->avail_in;
    for (cnt = tmp; cnt < comp->size_in; cnt++)
	*(comp->buff_in + (cnt - tmp)) = *(comp->buff_in + cnt);

    /* adjust input state value - it is important that this is done after the previous step */
    comp->size_in = comp->stream->avail_in;
    /* the above as taken out because I don't think its necessary.. this is faster too */
    /*comp->size_in = 0;*/

    if (result > 0)
	result = bytes_copied;
  } else 

  result = write(desc, txt, length);

  if (result > 0) {
    /* Write was successful. */
    return (result);
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return (-1);
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

    if (errno == EAGAIN)
        return (0);

  /* Looks like the error was fatal.  Too bad. */
  return (-1);
}

    
/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 * >=0  If all is well and good.
 *  -1  If an error was encountered, so that the player should be cut off.
 */
int write_to_descriptor(socklen_t desc, const char *txt, struct compr *comp)
{
  ssize_t bytes_written;
  size_t total = strlen(txt), write_total = 0;

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total, comp);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
      perror("SYSERR: Write to socket");
      return (-1);
    } else if (bytes_written == 0) {
      /* Temporary failure -- socket buffer full. */
      return (write_total);
    } else {
      txt += bytes_written;
      total -= bytes_written;
      write_total += bytes_written;
    }
  }

  return (write_total);
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socklen_t desc, char *read_point, size_t space_left)
{
  ssize_t ret;

    ret = read(desc, read_point, space_left);

  /* Read was successful. */
  if (ret > 0)
    return (ret);

  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
    log("WARNING: EOF on socket read (connection broken by peer)");
    return (-1);
  }

  /*
   * read returned a value < 0: there was an error
   */

    if (errno == EINTR)
        return (0);

    if (errno == EAGAIN)
        return (0);

    if (errno == ECONNRESET)
        return (-1);
  /*
   * We don't know what happened, cut them off. This qualifies for
   * a SYSERR because we have no idea what happened at this point.
   */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return (-1);
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 *
 * Ever wonder why 'tmp' had '+8' on it?  The crusty old code could write
 * MAX_INPUT_LENGTH+1 bytes to 'tmp' if there was a '$' as the final
 * character in the input buffer.  This would also cause 'space_left' to
 * drop to -1, which wasn't very happy in an unsigned variable.  Argh.
 * So to fix the above, 'tmp' lost the '+8' since it doesn't need it
 * and the code has been changed to reserve space by accepting one less
 * character. (Do you really need 256 characters on a line?)
 * -gg 1/21/2000
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH];

const char compress_start[] =
        {
                (char) IAC,
                (char) SB,
                (char) COMPRESS2,
                (char) IAC,
                (char) SE,
                (char) 0
        };

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      log("WARNING: process_input: about to close connection: input overflow");
      return (-1);
    }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

    if (bytes_read < 0)	/* Error, disconnect them. */
      return (-1);
    else if (bytes_read == 0)	/* Just blocking, no problems. */
      return (0);

    /* check for compression response, if still expecting something */
    /* note: this will bork if the user is giving lots of input when he first connects */
    /* he shouldn't be doing this, and for the sake of efficiency, the read buffer isn't searched */
    /* (ie. it assumes that read_point[0] will be IAC, etc.) */
    if (t->comp->state == 1) {

      if (*read_point == (char)IAC && *(read_point + 1) == (char)DO && *(read_point + 2) == (char)COMPRESS2) {
	/* compression just turned on */
	/* first send plaintext start of the compression stream */
	write_to_descriptor(t->descriptor, compress_start, NULL);
	
	/* init the compression stream */	
	CREATE(t->comp->stream, z_stream, 1);
	t->comp->stream->zalloc = z_alloc;
	t->comp->stream->zfree = z_free;
	t->comp->stream->opaque = Z_NULL;
	deflateInit(t->comp->stream, Z_DEFAULT_COMPRESSION);
        
	/* init the state structure */
	/* first the output component */
	CREATE(t->comp->buff_out, Bytef, SMALL_BUFSIZE);
	t->comp->total_out = SMALL_BUFSIZE;
	t->comp->size_out = 0;
	/* now the input component */
	CREATE(t->comp->buff_in, Bytef, SMALL_BUFSIZE);
	t->comp->total_in = SMALL_BUFSIZE;
	t->comp->size_in = 0;

	/* finally, turn compression on */
	t->comp->state = 2;
	
	bytes_read = 0; /* ignore the compression string - don't process it further */
      } else if (*read_point == (char)IAC && *(read_point + 1) == (char)DONT && *(read_point + 2) == (char)COMPRESS2) {
	t->comp->state = 0;
	
	bytes_read = 0; /* ignore the compression string - don't process it further */
      }
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
  } while (nl_pos == NULL);

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    /* The '> 1' reserves room for a '$ => $$' expansion. */
    for (ptr = read_point; (space_left > 1) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {		/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer, t->comp) < 0)
	return (-1);
    }
    if (t->snoop_by)
      write_to_output(t->snoop_by, "%% %s\r\n", tmp);
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1)))	/* Redo last command. */
      strcpy(tmp, t->last_input);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln = (tmp + 1);
      int starting_pos = t->history_pos,
	  cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
	if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
	  strcpy(tmp, t->history[cnt]);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
	  strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
          write_to_output(t, "%s\r\n", tmp);
	  break;
	}
        if (cnt == 0)	/* At top, loop to bottom. */
	  cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
    } else {
      strcpy(t->last_input, tmp);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */
      if (t->history[t->history_pos])
	free(t->history[t->history_pos]);	/* Clear the old line. */
      t->history[t->history_pos] = strdup(tmp);	/* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
	t->history_pos = 0;
    }
   /* the '--' command flushes the queue - Jamdog - 9th May 2007 */ 
   if (masadv(tmp, t->character)) {
     /* Unfinished color cycling code, do not touch. */
   }
   if ( (*tmp == '-') && (*(tmp+1) == '-') && !(*(tmp+2)) ) 
   { 
     write_to_output(t, "All queued commands cancelled.\r\n"); 
     flush_queues(t);  /* Flush the command queue */ 
     /* No need to process the -- command any further, so quit back out */ 
   }
    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return (1);
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    write_to_output(t, "Invalid substitution.\r\n");
    return (1);
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    write_to_output(t, "Invalid substitution.\r\n");
    return (1);
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, strpos - orig);	/* strncpy: OK (newsub:MAX_INPUT_LENGTH+5 > orig:MAX_INPUT_LENGTH) */
  newsub[strpos - orig] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, MAX_INPUT_LENGTH - strlen(newsub) - 1);	/* strncpy: OK */

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), MAX_INPUT_LENGTH - strlen(newsub) - 1);	/* strncpy: OK */

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub);	/* strcpy: OK (by mutual MAX_INPUT_LENGTH) */

  return (0);
}

void free_user(struct descriptor_data *d)
{
  if (d->user_freed == 1) {
   return;
  }

   if (d->user == NULL) {
    send_to_imm("ERROR: free_user called but no user to free!");
    return;
   }
   d->user_freed = 1;

   if (!strcasecmp(d->user, "Empty"))
    return;

   log("Freeing User: %s", d->user);

   /* Free up all the user data as needed */
   if (d->user) {
    free(d->user);
   }
   if (d->pass) {
    free(d->pass);
   }
   if (d->email) {
    free(d->email);
   }
   if (d->tmp1) {
    free(d->tmp1);
   }
   if (d->tmp2) {
    free(d->tmp2);
   }
   if (d->tmp3) {
    free(d->tmp3);
   }
   if (d->tmp4) {
    free(d->tmp4);
   }
   if (d->tmp5) {
    free(d->tmp5);
   }
}

void close_socket(struct descriptor_data *d)
{
  struct descriptor_data *temp;

  REMOVE_FROM_LIST(d, descriptor_list, next, temp);
    close(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    write_to_output(d->snoop_by, "Your victim is no longer among us.\r\n");
    d->snoop_by->snooping = NULL;
  }

  if (d->character) {
    /* If we're switched, this resets the mobile taken. */
    d->character->desc = NULL;

    /* Plug memory leak, from Eric Green. */
    if (!IS_NPC(d->character) && PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
      d->str = NULL;
    } else if (d->backstr && !IS_NPC(d->character) && !PLR_FLAGGED(d->character, PLR_WRITING)) {
      free(d->backstr);      /* editing description ... not olc */
      d->backstr = NULL;
    }
    if (IS_PLAYING(d) || STATE(d) == CON_DISCONNECT) {
      struct char_data *link_challenged = d->original ? d->original : d->character;

      /* We are guaranteed to have a person. */
      act("$n has lost $s link.", TRUE, link_challenged, 0, 0, TO_ROOM);
      save_char(link_challenged);
      mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(link_challenged)), TRUE, "Closing link to: %s.", GET_NAME(link_challenged));
    } else {
       free_char(d->character);
    }
  } else
    mudlog(CMP, ADMLVL_IMMORT, TRUE, "Losing descriptor without char.");

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  /* Clear the command history. */
  if (d->history) {
    int cnt;
    for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
      if (d->history[cnt])
	free(d->history[cnt]);
    free(d->history);
  }

  if (d->showstr_head)
    free(d->showstr_head);
  if (d->showstr_count)
    free(d->showstr_vector);
  if (d->obj_name)
    free(d->obj_name);
  if (d->obj_short)
    free(d->obj_short);
  if (d->obj_long)
    free(d->obj_long); 

  free_user(d);

  /*. Kill any OLC stuff .*/
  switch (d->connected) {
    case CON_OEDIT:
    case CON_IEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
    case CON_TEDIT:
    case CON_AEDIT:
    case CON_TRIGEDIT:
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      break;
  }

  /* free compression structures */
    if (d->comp->stream) {
        deflateEnd(d->comp->stream);
        free(d->comp->stream);
        free(d->comp->buff_out);
        free(d->comp->buff_in);
    }
  /* d->comp was still created even if there is no zlib, for comp->state) */
  if (d->comp)
    free(d->comp);  
      
  free(d);
}

void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_EMAIL && STATE(d) != CON_NEWPASSWD)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      write_to_output(d, "\r\nTimed out... goodbye.\r\n");
      STATE(d) = CON_CLOSE;
    }
  }
}

void check_idle_menu(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_MENU && STATE(d) != CON_GET_USER && STATE(d) != CON_UMENU)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      write_to_output(d, "\r\nYou are about to be disconnected due to inactivity in 60 seconds.\r\n");
      continue;
    } else {
      echo_on(d);
      write_to_output(d, "\r\nTimed out... goodbye.\r\n");
      STATE(d) = CON_CLOSE;
    }
  }
}

/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

void nonblock(socklen_t s)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    flags |= O_NONBLOCK;
    if (fcntl(s, F_SETFL, flags) < 0) {
        perror("SYSERR: Fatal error executing nonblock (comm.c)");
        exit(1);
    }
}


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

void reread_wizlists(int sig)
{
    reread_wizlist = TRUE;
}


void unrestrict_game(int sig)
{
    emergency_unban = TRUE;
}

/* clean up our zombie kids to avoid defunct processes */
void reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  signal(SIGCHLD, reap);
}

/* Dying anyway... */
void checkpointing(int sig)
{
#ifndef MEMORY_DEBUG
  if (!tics_passed) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics_passed = 0;
#endif
}


/* Dying anyway... */
void hupsig(int sig)
{
  log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  exit(1);			/* perhaps something more elegant should
				 * substituted */
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

void signal_setup(void)
{
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  signal(SIGHUP, hupsig);
  signal(SIGCHLD, reap);
  signal(SIGINT, hupsig);
  signal(SIGTERM, hupsig);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);
}

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

size_t send_to_char(struct char_data *ch, const char *messg, ...)
{
  if (ch->desc && messg && *messg) {
    size_t left;
    va_list args;

    va_start(args, messg);
    left = vwrite_to_output(ch->desc, messg, args);
    va_end(args);
    return left;
  }
  return 0;
}

int arena_watch(struct char_data *ch)
{

 struct descriptor_data *d;
 int found = FALSE, room = NOWHERE;

 for (d = descriptor_list; d; d = d->next) {
  if (STATE(d) != CON_PLAYING)
   continue;

  if (IN_ARENA(d->character)) {
   if (ARENA_IDNUM(ch) == GET_IDNUM(d->character)) {
    found = TRUE;
    room = GET_ROOM_VNUM(IN_ROOM(d->character));
   }
  }
 }

 if (found == FALSE) {
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
  ARENA_IDNUM(ch) = -1;
  return (NOWHERE);
 } else {
  return (room);
 }
}

void send_to_eaves(const char *messg, struct char_data *tch, ...)
{
 struct descriptor_data *d;



 for (d = descriptor_list; d; d = d->next) {
     if(STATE(d) != CON_PLAYING)
      continue;

     int roll = rand_number(1, 101);
     if (GET_EAVESDROP(d->character) == GET_ROOM_VNUM(IN_ROOM(tch)) && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
      char buf[1000];
      char buf2[1000];
      *buf = '\0';
      sprintf(buf2, "@W%s %s\r\n", PERS(d->character, tch), messg);
      sprintf(buf, "-----Eavesdrop-----\r\n%s-----Eavesdrop-----\r\n", buf2);
      send_to_char(d->character, buf);
     }
  }
}

void send_to_all(const char *messg, ...)
{
  struct descriptor_data *i;
  va_list args;

  if (messg == NULL)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}


void send_to_outdoor(const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !OUTSIDE(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_moon(const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !HAS_MOON(i->character))
      continue;

    va_start(args, messg);
    vwrite_to_output(i, messg, args);
    va_end(args);
  }
}

void send_to_planet(int type, int planet, const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    va_list args;

    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !ROOM_FLAGGED(IN_ROOM(i->character), planet))
      continue;
    else {
     if (type == 0) {
      va_start(args, messg);
      vwrite_to_output(i, messg, args);
      va_end(args);
     } else if (OUTSIDE(i->character) && GET_SKILL(i->character, SKILL_SPOT) >= axion_dice(-5)) {
      va_start(args, messg);
      vwrite_to_output(i, messg, args);
      va_end(args);
     }
    }
  }
}


void send_to_room(room_rnum room, const char *messg, ...)
{
  struct char_data *i;
  va_list args;

  if (messg == NULL)
    return;

  for (i = world[room].people; i; i = i->next_in_room) {
    if (!i->desc)
      continue;

    va_start(args, messg);
    vwrite_to_output(i->desc, messg, args);
    va_end(args);
  }

 struct descriptor_data *d;

 for (d = descriptor_list; d; d = d->next) {
     if(STATE(d) != CON_PLAYING)
      continue;

     if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
      if (arena_watch(d->character) == room) {
       char buf[2000];
       *buf = '\0';
       sprintf(buf, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", messg);
       va_start(args, messg);
       vwrite_to_output(d, buf, args);
       va_end(args);
      }     
     }
     if (GET_EAVESDROP(d->character) > 0) {
       int roll = rand_number(1, 101);
       if (GET_EAVESDROP(d->character) == room && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
        char buf[1000];
        *buf = '\0';
        sprintf(buf, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", messg);
        va_start(args, messg);
        vwrite_to_output(d, buf, args);
        va_end(args);
       }
     }

  }
 
}



const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, struct char_data *to)
{
  const char *i = NULL;
  char lbuf[MAX_STRING_LENGTH], *buf, *j;
  bool uppercasenext = FALSE;
  const struct char_data *dg_victim = NULL;
  const struct obj_data *dg_target = NULL;
  const char *dg_arg = NULL;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
	i = PERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
	dg_victim = (const struct char_data *) vict_obj;
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((const struct char_data *) vict_obj));
	dg_victim = (const struct char_data *) vict_obj;
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((const struct char_data *) vict_obj));
	dg_victim = (const struct char_data *) vict_obj;
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((const struct char_data *) vict_obj));
	dg_victim = (const struct char_data *) vict_obj;
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((const struct obj_data *) vict_obj, to));
	dg_target = (const struct obj_data *) vict_obj;
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((const struct obj_data *) vict_obj, to));
	dg_target = (const struct obj_data *) vict_obj;
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((const struct obj_data *) vict_obj));
	dg_target = (const struct obj_data *) vict_obj;
	break;
      case 'T':
	CHECK_NULL(vict_obj, (const char *) vict_obj);
	dg_arg = (const char *) vict_obj;
	break;
      case 't':
        CHECK_NULL(obj, (char *) obj);
        break;
      case 'F':
	CHECK_NULL(vict_obj, fname((const char *) vict_obj));
	break;
      /* uppercase previous word */
      case 'u':
        for (j=buf; j > lbuf && !isspace((int) *(j-1)); j--);
        if (j != buf)
          *j = UPPER(*j);
        i = "";
        break;
      /* uppercase next word */
      case 'U':
        uppercasenext = TRUE;
        i = "";
        break;
      case '$':
	i = "$";
	break;
      default:
        return;
	break;
      }
      while ((*buf = *(i++)))
        {
        if (uppercasenext && !isspace((int) *buf))
          {
          *buf = UPPER(*buf);
          uppercasenext = FALSE;
          }
	buf++;
        }
      orig++;
    } else if (!(*(buf++) = *(orig++))) {
      break;
    } else if (uppercasenext && !isspace((int) *(buf-1))) {
      *(buf-1) = UPPER(*(buf-1));
      uppercasenext = FALSE;
    }
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
  write_to_output(to->desc, "%s", CAP(lbuf));

  if ((IS_NPC(to) && dg_act_check) && (to != ch))
    act_mtrigger(to, lbuf, ch, dg_victim, obj, dg_target, dg_arg);

  if (last_act_message)
    free(last_act_message);
  last_act_message = strdup(lbuf);

}

char *act(const char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, const void *vict_obj, int type)
{
  struct char_data *to;
  int to_sleeping, res_sneak, res_hide, dcval = 0, resskill = 0;

  if (!str || !*str)
    return NULL;

  /* Warning: the following TO_SLEEP code is a hack. I wanted to be able to tell
   * act to deliver a message regardless of sleep without adding an additional 
   * argument.  TO_SLEEP is 128 (a single bit high up).  It's ONLY legal to 
   * combine TO_SLEEP with one other TO_x command.  It's not legal to combine 
   * TO_x's with each other otherwise. TO_SLEEP only works because its value 
   * "happens to be" a single bit; do not change it to something else.  In 
   * short, it is a hack.  The same applies to TO_*RESIST.  */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if ((res_sneak = (type & TO_SNEAKRESIST)))
    type &= ~TO_SNEAKRESIST;

  if ((res_hide = (type & TO_HIDERESIST)))
    type &= ~TO_HIDERESIST;

  if (res_sneak && AFF_FLAGGED(ch,  AFF_SNEAK)) {
    dcval = roll_skill(ch, SKILL_MOVE_SILENTLY); /* How difficult to counter? */
    if (GET_SKILL(ch, SKILL_BALANCE))
     dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
    if (IS_MUTANT(ch) && (GET_GENOME(ch, 0) == 5 || GET_GENOME(ch, 1) == 5)) {
     dcval += 10;
    }
    resskill = SKILL_SPOT;             /* Skill used to resist      */
  } else if (res_hide && AFF_FLAGGED(ch, AFF_HIDE)) {
    dcval = roll_skill(ch, SKILL_HIDE);
    if (GET_SKILL(ch, SKILL_BALANCE))
     dcval += GET_SKILL(ch, SKILL_BALANCE) / 10;
    resskill = SKILL_SPOT;
  }

  /* this is a hack as well - DG_NO_TRIG is 256 -- Welcor */
  /* If the bit is set, unset dg_act_check, thus the ! below */
  if (!(dg_act_check = !IS_SET(type, DG_NO_TRIG)))
    REMOVE_BIT(type, DG_NO_TRIG); 

  if (type == TO_CHAR) {
    if (ch && SENDOK(ch) && (!resskill || (roll_skill(ch, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, ch);
      return last_act_message;
    }
    return NULL;
  }

  if (type == TO_VICT) {
    if ((to = (struct char_data *) vict_obj) != NULL && SENDOK(to) && (!resskill || (roll_skill(to, resskill) >= dcval))) {
      perform_act(str, ch, obj, vict_obj, to);
      return last_act_message;
    }
    return NULL;
  }

  if (type == TO_GMOTE) { 
    struct descriptor_data *i; 
    char buf[MAX_STRING_LENGTH];
    for (i = descriptor_list; i; i = i->next) { 
      if (!i->connected && i->character && 
         !PRF_FLAGGED(i->character, PRF_NOGOSS) && 
         !PLR_FLAGGED(i->character, PLR_WRITING) && 
         !ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) { 

        sprintf(buf, "@y%s@n", str);
        perform_act(buf, ch, obj, vict_obj, i->character);
        char buf2[MAX_STRING_LENGTH];
        sprintf(buf2, "%s\r\n", buf);
        add_history(i->character, buf2, HIST_GOSSIP);
      } 
    } 
    return last_act_message;
  } 

  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (ch && IN_ROOM(ch) != NOWHERE)
    to = world[IN_ROOM(ch)].people;
  else if (obj && IN_ROOM(obj) != NOWHERE)
    to = world[IN_ROOM(obj)].people;
  else {
    return NULL;
  }

  if ((type & TO_ROOM)) {
   struct descriptor_data *d;

   for (d = descriptor_list; d; d = d->next) {
     if(STATE(d) != CON_PLAYING)
      continue;

     if (ch != NULL) {
      if (IN_ARENA(ch)) {
       if (PRF_FLAGGED(d->character, PRF_ARENAWATCH)) {
        if (arena_watch(d->character) == GET_ROOM_VNUM(IN_ROOM(ch))) {
          char buf3[2000];
          *buf3 = '\0';
          sprintf(buf3, "@c-----@CArena@c-----@n\r\n%s\r\n@c-----@CArena@c-----@n\r\n", str);
          perform_act(buf3, ch, obj, vict_obj, d->character);
        }
       }
      }
     } if (GET_EAVESDROP(d->character) > 0) {
       int roll = rand_number(1, 101);
       if (!resskill || (roll_skill(d->character, resskill) >= dcval)) {
        if (ch != NULL && GET_EAVESDROP(d->character) == GET_ROOM_VNUM(IN_ROOM(ch)) && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
         char buf3[1000];
         *buf3 = '\0';
         sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
         perform_act(buf3, ch, obj, vict_obj, d->character);
        }
        else if (obj != NULL && GET_EAVESDROP(d->character) == GET_ROOM_VNUM(IN_ROOM(obj)) && GET_SKILL(d->character, SKILL_EAVESDROP) > roll) {
         char buf3[1000];
         *buf3 = '\0';
         sprintf(buf3, "-----Eavesdrop-----\r\n%s\r\n-----Eavesdrop-----\r\n", str);
         perform_act(buf3, ch, obj, vict_obj, d->character);
        }
       }
     }
   }
  }

  for (; to; to = to->next_in_room) {
    if (!SENDOK(to) || (to == ch))
      continue;
    if (hide_invisible && ch && !CAN_SEE(to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
    if (resskill && roll_skill(to, resskill) < dcval)
      continue;
    perform_act(str, ch, obj, vict_obj, to);
  }
  return last_act_message;
}


/* Prefer the file over the descriptor. */
void setup_log(const char *filename, int fd)
{
  FILE *s_fp;

    s_fp = stderr;

  if (filename == NULL || *filename == '\0') {
    /* No filename, set us up with the descriptor we just opened. */
    logfile = s_fp;
    puts("Using file descriptor for logging.");
    return;
  }

  /* We honor the default filename first. */
  if (open_logfile(filename, s_fp))
    return;

  /* Well, that failed but we want it logged to a file so try a default. */
  if (open_logfile("log/syslog", s_fp))
    return;

  /* Ok, one last shot at a file. */
  if (open_logfile("syslog", s_fp))
    return;

  /* Erp, that didn't work either, just die. */
  puts("SYSERR: Couldn't open anything to log to, giving up.");
  exit(1);
}

int open_logfile(const char *filename, FILE *stderr_fp)
{
  if (stderr_fp)	/* freopen() the descriptor. */
    logfile = freopen(filename, "w", stderr_fp);
  else
    logfile = fopen(filename, "w");

  if (logfile) {
    printf("Using log file '%s'%s.\n",
		filename, stderr_fp ? " with redirection" : "");
    return (TRUE);
  }

  printf("SYSERR: Error opening file '%s': %s\n", filename, strerror(errno));
  return (FALSE);
}

/*
 * This may not be pretty but it keeps game_loop() neater than if it was inline.
 */


void circle_sleep(struct timeval *timeout)
{
  if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, timeout) < 0) {
    if (errno != EINTR) {
      perror("SYSERR: Select sleep");
      exit(1);
    }
  }
}

void show_help(struct descriptor_data *t, const char *entry)
{
  int chk, bot, top, mid, minlen;
  char buf[MAX_STRING_LENGTH];

  if (!help_table) return;

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(entry);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      return;
    }
    else if (!(chk = strncasecmp(entry, help_table[mid].keywords, minlen))) {
      while ((mid > 0) &&
       (!(chk = strncasecmp(entry, help_table[mid - 1].keywords, minlen))))
       mid--;
      write_to_output(t, "\r\n");
      snprintf(buf, sizeof(buf), "%s\r\n[ PRESS RETURN TO CONTINUE ]",
       help_table[mid].entry);
      page_string(t, buf, 0);
      return;
    } else {
      if (chk > 0) bot = mid + 1;
      else top = mid - 1;
    }
  }
}

/* Thx to Jamie Nelson of 4D for this contribution */
void send_to_range(room_vnum start, room_vnum finish, const char *messg, ...)
{
  struct char_data *i;
  va_list args;
  int j;

  if (start > finish) {
    log("send_to_range passed start room value greater then finish.");
    return;
  }
  if (messg == NULL)
    return;

  for (j = 0; j < top_of_world; j++) {
    if (GET_ROOM_VNUM(j) >= start && GET_ROOM_VNUM(j) <= finish) {
      for (i = world[j].people; i; i = i->next_in_room) {
        if (!i->desc)
          continue;

        va_start(args, messg);
        vwrite_to_output(i->desc, messg, args);
        va_end(args);
      }
    }
  }
}

int passcomm(struct char_data *ch, char *comm)
{

 if (!strcasecmp(comm, "score")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "sco")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "ooc")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "newbie")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "newb")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "look")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "lo")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "l")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "status")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "stat")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "sta")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "tell")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "reply")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "say")) {
  return TRUE;
 }
 else if (!strcasecmp(comm, "osay")) {
  return TRUE;
 }
 else {
  return FALSE;
 }

}
