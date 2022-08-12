//
// Created by basti on 10/22/2021.
//

#include "comm.h"
#include "utils.h"
#include "mail.h"
#include "boards.h"
#include "act.informative.h"
#include "act.social.h"
#include "dg_scripts.h"
#include "constants.h"
#include "ban.h"
#include "genolc.h"

int main(int argc, char **argv)
{
    int pos = 1;
    const char *dir;

#ifdef MEMORY_DEBUG
    zmalloc_init();
#endif

#if CIRCLE_GNU_LIBC_MEMORY_TRACK
    mtrace();	/* This must come before any use of malloc(). */
#endif

    /****************************************************************************/
    /** Load the game configuration.                                           **/
    /** We must load BEFORE we use any of the constants stored in constants.c. **/
    /** Otherwise, there will be no variables set to set the rest of the vars  **/
    /** to, which will mean trouble --> Mythran                                **/
    /****************************************************************************/
    CONFIG_CONFFILE = NULL;
    while ((pos < argc) && (*(argv[pos]) == '-')) {
        if (*(argv[pos] + 1) == 'f') {
            if (*(argv[pos] + 2))
                CONFIG_CONFFILE = argv[pos] + 2;
            else if (++pos < argc)
                CONFIG_CONFFILE = argv[pos];
            else {
                puts("SYSERR: File name to read from expected after option -f.");
                exit(1);
            }
        }
        pos++;
    }
    pos = 1;

    if (!CONFIG_CONFFILE)
        CONFIG_CONFFILE = strdup(CONFIG_FILE);

    load_config();

    port = CONFIG_DFLT_PORT;
    dir = CONFIG_DFLT_DIR;

    while ((pos < argc) && (*(argv[pos]) == '-')) {
        switch (*(argv[pos] + 1)) {
            case 'f':
                if (! *(argv[pos] + 2))
                    ++pos;
                break;
            case 'o':
                if (*(argv[pos] + 2))
                    CONFIG_LOGNAME = argv[pos] + 2;
                else if (++pos < argc)
                    CONFIG_LOGNAME = argv[pos];
                else {
                    puts("SYSERR: File name to log to expected after option -o.");
                    exit(1);
                }
                break;
            case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
                fCopyOver = TRUE;
                mother_desc = atoi(argv[pos]+2);
                break;
            case 'd':
                if (*(argv[pos] + 2))
                    dir = argv[pos] + 2;
                else if (++pos < argc)
                    dir = argv[pos];
                else {
                    puts("SYSERR: Directory arg expected after option -d.");
                    exit(1);
                }
                break;
            case 'm':
                mini_mud = 1;
                no_rent_check = 1;
                puts("Running in minimized mode & with no rent check.");
                break;
            case 'c':
                scheck = 1;
                puts("Syntax check mode enabled.");
                break;
            case 'q':
                no_rent_check = 1;
                puts("Quick boot mode -- rent check supressed.");
                break;
            case 'r':
                circle_restrict = 1;
                puts("Restricting game -- no new players allowed.");
                break;
            case 's':
                no_specials = 1;
                puts("Suppressing assignment of special routines.");
                break;
            case 'x':
                xap_objs = 1;
                log("Loading player objects from secondary (ascii) files.");
                break;
            case 'h':
                /* From: Anil Mahajan <amahajan@proxicom.com> */
                printf("Usage: %s [-c] [-m] [-x] [-q] [-r] [-s] [-d pathname] [port #]\n"
                       "  -c             Enable syntax check mode.\n"
                       "  -d <directory> Specify library directory (defaults to 'lib').\n"
                       "  -f<file>       Use <file> for configuration.\n"
                       "  -h             Print this command line argument help.\n"
                       "  -m             Start in mini-MUD mode.\n"
                       "  -o <file>      Write log to <file> instead of stderr.\n"
                       "  -q             Quick boot (doesn't scan rent for object limits)\n"
                       "  -r             Restrict MUD -- no new players allowed.\n"
                       "  -s             Suppress special procedure assignments.\n"
                       " Note:         These arguments are 'CaSe SeNsItIvE!!!'\n"
                       "  -x             Load using secondary (ascii) files.\n",
                       argv[0]
                );
                exit(0);
            default:
                printf("SYSERR: Unknown option -%c in argument string.\n", *(argv[pos] + 1));
                break;
        }
        pos++;
    }

    if (pos < argc) {
        if (!isdigit(*argv[pos])) {
            printf("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
            exit(1);
        } else if ((port = atoi(argv[pos])) <= 1024) {
            printf("SYSERR: Illegal port number %d.\n", port);
            exit(1);
        }
    }

    /* All arguments have been parsed, try to open log file. */
    setup_log(CONFIG_LOGNAME, STDERR_FILENO);

    /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
    log("Using %s for configuration.", CONFIG_CONFFILE);
    log("%s", circlemud_version);
    log("%s", oasisolc_version);
    log("%s", DG_SCRIPT_VERSION);
    log("%s", ascii_pfiles_version);
    log("%s", CWG_VERSION);
    xap_objs = 1;
    if (chdir(dir) < 0) {
        perror("SYSERR: Fatal error changing to data directory");
        exit(1);
    }
    log("Using %s as data directory.", dir);

    if (scheck)
        boot_world();
    else {
        log("Running game on port %d.", port);
        init_game(port);
    }

    log("Clearing game world.");
    destroy_db();

    if (!scheck) {
        log("Clearing other memory.");
        free_bufpool();             /* comm.c */
        free_player_index();	/* players.c */
        clear_free_list();		/* mail.c */
        free_mail_index();          /* mail.c */
        free_text_files();		/* db.c */
        clear_boards();             /* boards.c */
        free(cmd_sort_info);	/* act.informative.c */
        free_command_list();        /* act.informative.c */
        free_social_messages();	/* act.social.c */
        free_help_table();		/* db.c */
        Free_Invalid_List();	/* ban.c */
        free_strings(&config_info, OASIS_CFG); /* oasis_delete.c */
        free_disabled();    /* interpreter.c */
        free_save_list();		/* genolc.c */
    }

    if (last_act_message)
        free(last_act_message);

    /* probably should free the entire config here.. */
    free(CONFIG_CONFFILE);

    log("Done.");

#ifdef MEMORY_DEBUG
    zmalloc_check();
#endif

    return (0);
}
