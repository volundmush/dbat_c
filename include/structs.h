/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and constants                *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef CIRCLE_STRUCTS_H
#define CIRCLE_STRUCTS_H

#include "sysdep.h"

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030100 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */

/* CWG Version String */
#define CWG_VERSION "CWG Rasputin - 3.5.31"
#define DBAT_VERSION "DBAT - version 2.4"

/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */





#define SG_MIN		2 /* Skill gain check must be less than this
			     number in order to be successful. 
			     IE: 1% of a skill gain */

/* Ocarina Songs */
#define SONG_SAFETY             1
#define SONG_SHIELDING          2
#define SONG_SHADOW_STITCH      3
#define SONG_TELEPORT_EARTH     4
#define SONG_TELEPORT_KONACK    5
#define SONG_TELEPORT_ARLIA     6
#define SONG_TELEPORT_NAMEK     7
#define SONG_TELEPORT_VEGETA    8
#define SONG_TELEPORT_FRIGID    9
#define SONG_TELEPORT_AETHER   10
#define SONG_TELEPORT_KANASSA  11

/* Fighting Preferences */
#define PREFERENCE_THROWING     1
#define PREFERENCE_H2H          2
#define PREFERENCE_KI           3
#define PREFERENCE_WEAPON       4

/* Ingredient vnums for recipes */
#define RCP_TOMATO      17212
#define RCP_POTATO      17213
#define RCP_ONION       17215
#define RCP_CUCUMBER    17217
#define RCP_CHILIPEPPER 17219
#define RCP_FOUSTAFI    17221
#define RCP_CARROT      17223
#define RCP_GREENBEAN   17225
#define RCP_NORMAL_MEAT 1612
#define RCP_GOOD_MEAT   6500
#define RCP_BLACKBASS   1000
#define RCP_SILVERTROUT 1001
#define RCP_STRIPEDBASS 1002
#define RCP_BLUECATFISH 1003
#define RCP_FLOUNDER    1004
#define RCP_SILVEREEL   1005
#define RCP_COBIA       1006
#define RCP_TAMBOR      1007
#define RCP_NARRI       1008
#define RCP_VALBISH     1009
#define RCP_GUSBLAT     1010
#define RCP_REPEEIL     1011
#define RCP_GRAVELREBOI 1012
#define RCP_VOOSPIKE    1013
#define RCP_SHADOWFISH  1014
#define RCP_SHADEEEL    1015
#define RCP_BROWNMUSH   1608
#define RCP_GARLIC      1131
#define RCP_RICE        1590
#define RCP_FLOUR       1591
#define RCP_LETTUCE     17227
#define RCP_APPLEPLUM   8001
#define RCP_FROZENBERRY 4901
#define RCP_CARAMBOLA   3416

/* Meal recipes */
#define RECIPE_TOMATO_SOUP       1
#define RECIPE_POTATO_SOUP       2
#define RECIPE_VEGETABLE_SOUP    3
#define RECIPE_MEAT_STEW         4
#define RECIPE_STEAK             5
#define RECIPE_ROAST             6
#define RECIPE_CHILI_SOUP        7
#define RECIPE_GRILLED_NORMFISH  8
#define RECIPE_GRILLED_GOODFISH  9
#define RECIPE_GRILLED_GREATFISH 10
#define RECIPE_GRILLED_BESTFISH  11
#define RECIPE_COOKED_RICE       12
#define RECIPE_SUSHI             13
#define RECIPE_BREAD             14
#define RECIPE_SALAD             15
#define RECIPE_APPLEPLUM         16
#define RECIPE_FBERRY_MUFFIN     17
#define RECIPE_CARAMBOLA_BREAD   18

/* Completed Meal Object Vnums */
#define MEAL_START               1220
#define MEAL_TOMATO_SOUP         1220
#define MEAL_STEAK               1221
#define MEAL_POTATO_SOUP         1222
#define MEAL_VEGETABLE_SOUP      1223
#define MEAL_MEAT_STEW           1224
#define MEAL_ROAST               1225
#define MEAL_CHILI_SOUP          1226
#define MEAL_NORM_FISH           1227
#define MEAL_GOOD_FISH           1228
#define MEAL_GREAT_FISH          1229
#define MEAL_BEST_FISH           1230
#define MEAL_COOKED_RICE         1231
#define MEAL_SUSHI               1232
#define MEAL_BREAD               1233
#define MEAL_SALAD               1234
#define MEAL_APPLEPLUM           1235
#define MEAL_FBERRY_MUFFIN       1236
#define MEAL_CARAMBOLA_BREAD     1237


#define MEAL_LAST                1234

/* Fishing Defines */
#define FISH_NOFISH             0
#define FISH_BITE               1
#define FISH_HOOKED             2
#define FISH_REELING            3


/* Shadow Dragon Defines */

#define SHADOW_DRAGON1_VNUM 81
#define SHADOW_DRAGON2_VNUM 82
#define SHADOW_DRAGON3_VNUM 83
#define SHADOW_DRAGON4_VNUM 84
#define SHADOW_DRAGON5_VNUM 85
#define SHADOW_DRAGON6_VNUM 86
#define SHADOW_DRAGON7_VNUM 87

/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NORTHWEST      6
#define NORTHEAST      7
#define SOUTHEAST      8
#define SOUTHWEST      9
#define INDIR         10
#define OUTDIR        11

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		0   /* Dark			*/
#define ROOM_DEATH		1   /* Death trap		*/
#define ROOM_NOMOB		2   /* MOBs not allowed		*/
#define ROOM_INDOORS	3   /* Indoors			*/
#define ROOM_PEACEFUL	4   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF	5   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK	6   /* Track won't go through	*/
#define ROOM_NOINSTANT	7   /* IT not allowed		*/
#define ROOM_TUNNEL		8   /* room for only 1 pers	*/
#define ROOM_PRIVATE	9   /* Can't teleport in		*/
#define ROOM_GODROOM	10  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		11  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH 12  /* (R) House needs saving	*/
#define ROOM_ATRIUM		13  /* (R) The door to a house	*/
#define ROOM_OLC		14  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK	15  /* (R) breath-first srch mrk	*/
#define ROOM_VEHICLE    16  /* Requires a vehicle to pass       */
#define ROOM_UNDERGROUND        17  /* Room is below ground      */
#define ROOM_CURRENT     	18  /* Room move with random currents	*/
#define ROOM_TIMED_DT     	19  /* Room has a timed death trap  	*/
#define ROOM_EARTH              20  /* Room is on Earth */
#define ROOM_VEGETA             21  /* Room is on Vegeta */
#define ROOM_FRIGID             22  /* Room is on Frigid */
#define ROOM_KONACK             23  /* Room is on Konack */
#define ROOM_NAMEK              24  /* Room is on Namek */
#define ROOM_NEO                25  /* Room is on Neo */
#define ROOM_AL                 26  /* Room is on AL */
#define ROOM_SPACE              27  /* Room is on Space */
#define ROOM_HELL               28  /* Room is Punishment Hell*/
#define ROOM_REGEN              29  /* Better regen */
#define ROOM_RHELL              30  /* Room is HELLLLLLL */
#define ROOM_GRAVITYX10         31  /* For rooms that have 10x grav */
#define ROOM_AETHER		32  /* Room is on Aether */
#define ROOM_HBTC               33  /* Room is extra special training area */
#define ROOM_PAST               34  /* Inside the pendulum room */
#define ROOM_CBANK              35  /* This room is a clan bank */
#define ROOM_SHIP               36  /* This room is a private ship room */
#define ROOM_YARDRAT            37  /* This room is on planet Yardrat   */
#define ROOM_KANASSA            38  /* This room is on planet Kanassa   */
#define ROOM_ARLIA              39  /* This room is on planet Arlia     */
#define ROOM_AURA               40  /* This room has an aura around it  */
#define ROOM_EORBIT             41  /* Earth Orbit                      */
#define ROOM_FORBIT             42  /* Frigid Orbit                     */
#define ROOM_KORBIT             43  /* Konack Orbit                     */
#define ROOM_NORBIT             44  /* Namek  Orbit                     */
#define ROOM_VORBIT             45  /* Vegeta Orbit                     */
#define ROOM_AORBIT             46  /* Aether Orbit                     */
#define ROOM_YORBIT             47  /* Yardrat Orbit                    */
#define ROOM_KANORB             48  /* Kanassa Orbit                    */
#define ROOM_ARLORB             49  /* Arlia Orbit                      */
#define ROOM_NEBULA             50  /* Nebulae                          */
#define ROOM_ASTERO             51  /* Asteroid                         */
#define ROOM_WORMHO             52  /* Wormhole                         */
#define ROOM_STATION            53  /* Space Station                    */
#define ROOM_STAR               54  /* Is a star                        */
#define ROOM_CERRIA             55  /* This room is on planet Cerria    */
#define ROOM_CORBIT             56  /* This room is in Cerria's Orbit   */
#define ROOM_BEDROOM            57  /* +25% regen                       */
#define ROOM_WORKOUT            58  /* Workout Room                     */
#define ROOM_GARDEN1            59  /* 8 plant garden                   */
#define ROOM_GARDEN2            60  /* 20 plant garden                  */
#define ROOM_FERTILE1           61
#define ROOM_FERTILE2           62
#define ROOM_FISHING            63
#define ROOM_FISHFRESH          64
#define ROOM_CANREMODEL         65

#define NUM_ROOM_FLAGS          66

   /* Zone info: Used in zone_data.zone_flags */
#define ZONE_CLOSED		0
#define ZONE_NOIMMORT		1
#define ZONE_QUEST		2
#define ZONE_DBALLS		3
#define ZONE_SPARE2		4
#define ZONE_SPARE3		5
#define ZONE_SPARE4		6
#define ZONE_SPARE5		7
#define ZONE_SPARE6		8
#define ZONE_SPARE7		9
#define ZONE_SPARE8		10
#define ZONE_SPARE9		11
#define ZONE_SPARE10		12
#define ZONE_SPARE11		13
#define ZONE_SPARE12		14
#define ZONE_SPARE13		15
#define ZONE_SPARE14		16
#define ZONE_SPARE15		17
#define ZONE_SPARE16		18
#define ZONE_SPARE17		19
#define ZONE_SPARE18		20
#define ZONE_SPARE19		21
#define ZONE_SPARE20		22
#define ZONE_SPARE21		23
#define ZONE_SPARE22		24
#define ZONE_SPARE23		25
#define ZONE_SPARE24		26
#define ZONE_SPARE25		27
#define ZONE_SPARE26		28
#define ZONE_SPARE27		29
#define ZONE_SPARE28		30
#define ZONE_SPARE29		31
#define ZONE_SPARE30		32
#define ZONE_SPARE31		33
#define ZONE_SPARE32		34
#define ZONE_SPARE33		35


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/
#define EX_SECRET		(1 << 4)   /* The door is hidden        */

#define NUM_EXIT_FLAGS 5

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/
#define SECT_SHOP            10            /* Shop                      */
#define SECT_IMPORTANT       11            /* Important Rooms           */
#define SECT_DESERT          12            /* A desert                  */
#define SECT_SPACE           13            /* This is a space room      */
#define SECT_LAVA            14            /* This room always has lava */

#define NUM_ROOM_SECTORS     15


/* char and mob-related defines *****************************************/

/* PC classes */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define CLASS_UNDEFINED	        -1
#define CLASS_ROSHI             0
#define CLASS_PICCOLO           1
#define CLASS_KRANE             2
#define CLASS_NAIL              3
#define CLASS_BARDOCK           4
#define CLASS_GINYU             5
#define CLASS_FRIEZA            6
#define CLASS_TAPION            7
#define CLASS_ANDSIX            8
#define CLASS_DABURA            9
#define CLASS_KABITO            10
#define CLASS_JINTO             11
#define CLASS_TSUNA             12
#define CLASS_KURZAK            13
#define CLASS_ASSASSIN          14
#define CLASS_BLACKGUARD        15
#define CLASS_DRAGON_DISCIPLE   16
#define CLASS_DUELIST           17
#define CLASS_DWARVEN_DEFENDER  18
#define CLASS_ELDRITCH_KNIGHT   19
#define CLASS_HIEROPHANT        20
#define CLASS_HORIZON_WALKER    21
#define CLASS_LOREMASTER        22
#define CLASS_MYSTIC_THEURGE    23
#define CLASS_SHADOWDANCER      24
#define CLASS_THAUMATURGIST     25
#define CLASS_NPC_EXPERT	26
#define CLASS_NPC_ADEPT		27
#define CLASS_NPC_COMMONER	28
#define CLASS_NPC_ARISTOCRAT	29
#define CLASS_NPC_WARRIOR	30

#define MAX_SENSEI              15 /* Used by Sensei Style */


#define NUM_CLASSES             31
#define NUM_NPC_CLASSES 	4
#define NUM_PRESTIGE_CLASSES	15
#define NUM_BASIC_CLASSES	(14)


/* Gauntlet crap */
#define GAUNTLET_ZONE  24    /* The gauntlet zone vnum */ 
#define GAUNTLET_START 2403  /* The waiting room at the start of the gauntlet */ 
#define GAUNTLET_END   2404  /* The treasure room at the end of the gauntlet  */

/* Death Types for producing corpses with depth */
#define DTYPE_NORMAL      0    /* Default Death Type */
#define DTYPE_HEAD        1    /* Lost their head    */
#define DTYPE_HALF        2    /* Blown in half      */
#define DTYPE_VAPOR       3    /* Vaporized by attack*/
#define DTYPE_PULP        4    /* Beat to a pulp     */

/* Character Creation Styles */
/* Let's define bonuses/negatives */
#define BONUS_THRIFTY           0
#define BONUS_PRODIGY           1
#define BONUS_QUICK_STUDY       2
#define BONUS_DIEHARD           3
#define BONUS_BRAWLER           4
#define BONUS_DESTROYER         5
#define BONUS_HARDWORKER        6
#define BONUS_HEALER            7
#define BONUS_LOYAL             8
#define BONUS_BRAWNY            9
#define BONUS_SCHOLARLY        10
#define BONUS_SAGE             11
#define BONUS_AGILE            12
#define BONUS_QUICK            13
#define BONUS_STURDY           14
#define BONUS_THICKSKIN        15
#define BONUS_RECIPE           16
#define BONUS_FIREPROOF        17
#define BONUS_POWERHIT         18
#define BONUS_HEALTHY          19
#define BONUS_INSOMNIAC        20
#define BONUS_EVASIVE          21
#define BONUS_WALL             22
#define BONUS_ACCURATE         23
#define BONUS_LEECH            24
#define BONUS_GMEMORY          25
#define BONUS_SOFT             26
#define BONUS_LATE             27
#define BONUS_IMPULSE          28
#define BONUS_SICKLY           29
#define BONUS_PUNCHINGBAG      30
#define BONUS_PUSHOVER         31
#define BONUS_POORDEPTH        32
#define BONUS_THINSKIN         33
#define BONUS_FIREPRONE        34
#define BONUS_INTOLERANT       35
#define BONUS_COWARD           36
#define BONUS_ARROGANT         37
#define BONUS_UNFOCUSED        38
#define BONUS_SLACKER          39
#define BONUS_SLOW_LEARNER     40
#define BONUS_MASOCHISTIC      41
#define BONUS_MUTE             42
#define BONUS_WIMP             43
#define BONUS_DULL             44
#define BONUS_FOOLISH          45
#define BONUS_CLUMSY           46
#define BONUS_SLOW             47
#define BONUS_FRAIL            48
#define BONUS_SADISTIC         49
#define BONUS_LONER            50
#define BONUS_BMEMORY          51

#define MAX_BONUSES            52

/* Distinguishing Feature */
#define DISTFEA_EYE             0
#define DISTFEA_HAIR            1
#define DISTFEA_SKIN            2
#define DISTFEA_HEIGHT          3
#define DISTFEA_WEIGHT          4

/* Custom Aura */
#define AURA_WHITE              0
#define AURA_BLUE               1
#define AURA_RED                2
#define AURA_GREEN              3
#define AURA_PINK               4
#define AURA_PURPLE             5
#define AURA_YELLOW             6
#define AURA_BLACK              7
#define AURA_ORANGE             8

/* Eye Color */
#define EYE_UNDEFINED           -1
#define EYE_BLUE                0
#define EYE_BLACK               1
#define EYE_GREEN               2
#define EYE_BROWN               3
#define EYE_RED                 4
#define EYE_AQUA                5
#define EYE_PINK                6
#define EYE_PURPLE              7
#define EYE_CRIMSON             8
#define EYE_GOLD                9
#define EYE_AMBER               10
#define EYE_EMERALD             11

/*Hair Length */
#define HAIRL_UNDEFINED         -1
#define HAIRL_BALD              0
#define HAIRL_SHORT             1
#define HAIRL_MEDIUM            2
#define HAIRL_LONG              3
#define HAIRL_RLONG             4


/*Hair Color */
#define HAIRC_UNDEFINED         -1
#define HAIRC_NONE              0
#define HAIRC_BLACK             1
#define HAIRC_BROWN             2
#define HAIRC_BLONDE            3
#define HAIRC_GREY              4
#define HAIRC_RED               5
#define HAIRC_ORANGE            6
#define HAIRC_GREEN             7
#define HAIRC_BLUE              8
#define HAIRC_PINK              9
#define HAIRC_PURPLE            10
#define HAIRC_SILVER            11
#define HAIRC_CRIMSON           12
#define HAIRC_WHITE             13

/* Hair Style */
#define HAIRS_UNDEFINED         -1
#define HAIRS_NONE              0
#define HAIRS_PLAIN             1
#define HAIRS_MOHAWK            2
#define HAIRS_SPIKY             3
#define HAIRS_CURLY             4
#define HAIRS_UNEVEN            5
#define HAIRS_PONYTAIL          6
#define HAIRS_AFRO              7
#define HAIRS_FADE              8
#define HAIRS_CREW              9
#define HAIRS_FEATHERED         10
#define HAIRS_DRED              11


/* Skin Color */
#define SKIN_UNDEFINED          -1
#define SKIN_WHITE              0
#define SKIN_BLACK              1
#define SKIN_GREEN              2
#define SKIN_ORANGE             3
#define SKIN_YELLOW             4
#define SKIN_RED                5
#define SKIN_GREY               6
#define SKIN_BLUE               7
#define SKIN_AQUA               8
#define SKIN_PINK               9
#define SKIN_PURPLE             10
#define SKIN_TAN                11

/* Annual Sign Phase */

#define PHASE_PURITY            0
#define PHASE_BRAVERY           1
#define PHASE_HATRED            2
#define PHASE_DOMINANCE         3
#define PHASE_GUARDIAN          4
#define PHASE_LOVE              5
#define PHASE_STRENGTH          6

/* Races */
#define RACE_UNDEFINED		-1
#define RACE_HUMAN		0
#define RACE_SAIYAN		1
#define RACE_ICER		2
#define RACE_KONATSU		3
#define RACE_NAMEK		4
#define RACE_MUTANT		5
#define RACE_KANASSAN		6
#define RACE_HALFBREED		7
#define RACE_BIO		8
#define RACE_ANDROID		9
#define RACE_DEMON		10
#define RACE_MAJIN		11
#define RACE_KAI		12
#define RACE_TRUFFLE		13
#define RACE_GOBLIN		14
#define RACE_ANIMAL		15
#define RACE_ORC		16
#define RACE_SNAKE		17
#define RACE_TROLL		18
#define RACE_MINOTAUR		19
#define RACE_KOBOLD		20
#define RACE_LIZARDFOLK		21
#define RACE_WARHOST		22
#define RACE_FAERIE		23

#define NUM_RACES		24

#define ALIGN_SAINT              0
#define ALIGN_VALIANT            1
#define ALIGN_HERO               2
#define ALIGN_DOGOOD             3
#define ALIGN_NEUTRAL            4
#define ALIGN_CROOK              5
#define ALIGN_VILLAIN            6
#define ALIGN_TERRIBLE           7
#define ALIGN_HORRIBLE           8

#define NUM_ALIGNS               9

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define SIZE_UNDEFINED	(-1)
#define SIZE_FINE	0
#define SIZE_DIMINUTIVE	1
#define SIZE_TINY	2
#define SIZE_SMALL	3
#define SIZE_MEDIUM	4
#define SIZE_LARGE	5
#define SIZE_HUGE	6
#define SIZE_GARGANTUAN	7
#define SIZE_COLOSSAL	8

#define NUM_SIZES         9

#define WIELD_NONE        0
#define WIELD_LIGHT       1
#define WIELD_ONEHAND     2
#define WIELD_TWOHAND     3

/* Number of weapon types */
#define MAX_WEAPON_TYPES            26

/* Critical hit types */
#define CRIT_X2		0
#define CRIT_X3		1
#define CRIT_X4		2

#define MAX_CRIT_TYPE	CRIT_X4
#define NUM_CRIT_TYPES 3

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

#define NUM_SEX       3

/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/

#define NUM_POSITIONS  9 

/* AUCTIONING STATES */
#define AUC_NULL_STATE		0   /* not doing anything */
#define AUC_OFFERING		1   /* object has been offfered */
#define AUC_GOING_ONCE		2	/* object is going once! */
#define AUC_GOING_TWICE		3	/* object is going twice! */
#define AUC_LAST_CALL		4	/* last call for the object! */
#define AUC_SOLD		5
/* AUCTION CANCEL STATES */
#define AUC_NORMAL_CANCEL	6	/* normal cancellation of auction */
#define AUC_QUIT_CANCEL		7	/* auction canclled because player quit */
#define AUC_WIZ_CANCEL		8	/* auction cancelled by a god */
/* OTHER JUNK */
#define AUC_STAT		9
#define AUC_BID			10

/* Player flags: used by char_data.act */
#define PLR_KILLER	0   /* Player is a player-killer        */
#define PLR_THIEF	1   /* Player is a player-thief         */
#define PLR_FROZEN	2   /* Player is frozen                 */
#define PLR_DONTSET	3   /* Don't EVER set (ISNPC bit) 	*/
#define PLR_WRITING	4   /* Player writing (board/mail/olc)  */
#define PLR_MAILING	5   /* Player is writing mail           */
#define PLR_CRASH	6   /* Player needs to be crash-saved   */
#define PLR_SITEOK	7   /* Player has been site-cleared     */
#define PLR_NOSHOUT	8   /* Player not allowed to shout/goss */
#define PLR_NOTITLE	9   /* Player not allowed to set title  */
#define PLR_DELETED	10  /* Player deleted - space reusable  */
#define PLR_LOADROOM	11  /* Player uses nonstandard loadroom */
#define PLR_NOWIZLIST	12  /* Player shouldn't be on wizlist  	*/
#define PLR_NODELETE	13  /* Player shouldn't be deleted     	*/
#define PLR_INVSTART	14  /* Player should enter game wizinvis*/
#define PLR_CRYO	15  /* Player is cryo-saved (purge prog)*/
#define PLR_NOTDEADYET	16  /* (R) Player being extracted.     	*/
#define PLR_AGEMID_G	17  /* Player has had pos of middle age	*/
#define PLR_AGEMID_B	18  /* Player has had neg of middle age	*/
#define PLR_AGEOLD_G	19  /* Player has had pos of old age	*/
#define PLR_AGEOLD_B	20  /* Player has had neg of old age	*/
#define PLR_AGEVEN_G	21  /* Player has had pos of venerable age	*/
#define PLR_AGEVEN_B	22  /* Player has had neg of venerable age	*/
#define PLR_OLDAGE	23  /* Player is dead of old age	*/
#define PLR_RARM        24  /* Player has a right arm           */
#define PLR_LARM        25  /* Player has a left arm            */
#define PLR_RLEG        26  /* Player has a right leg           */
#define PLR_LLEG        27  /* Player has a left leg            */
#define PLR_HEAD        28  /* Player has a head                */
#define PLR_STAIL       29  /* Player has a saiyan tail         */
#define PLR_TAIL        30  /* Player has a non-saiyan tail     */
#define PLR_PILOTING    31  /* Player is sitting in the pilots chair */
#define PLR_SKILLP      32  /* Player made a good choice in CC  */
#define PLR_SPAR        33  /* Player is in a spar stance       */
#define PLR_CHARGE      34  /* Player is charging               */
#define PLR_TRANS1      35  /* Transformation 1                 */
#define PLR_TRANS2      36  /* Transformation 2                 */
#define PLR_TRANS3      37  /* Transformation 3                 */
#define PLR_TRANS4      38  /* Transformation 4                 */
#define PLR_TRANS5      39  /* Transformation 5                 */
#define PLR_TRANS6      40  /* Transformation 6                 */
#define PLR_ABSORB      41  /* Absorb model                     */
#define PLR_REPAIR      42  /* Repair model                     */
#define PLR_SENSEM      43  /* Sense-Powersense model           */
#define PLR_POWERUP     44  /* Powering Up                      */
#define PLR_KNOCKED     45  /* Knocked OUT                      */
#define PLR_CRARM       46  /* Cybernetic Right Arm             */
#define PLR_CLARM       47  /* Cybernetic Left Arm              */
#define PLR_CRLEG       48  /* Cybernetic Right Leg             */
#define PLR_CLLEG       49  /* Cybernetic Left Leg              */
#define PLR_FPSSJ       50  /* Full Power Super Saiyan          */
#define PLR_IMMORTAL    51  /* The player is immortal           */
#define PLR_EYEC        52  /* The player has their eyes closed */
#define PLR_DISGUISED   53  /* The player is disguised          */
#define PLR_BANDAGED    54  /* THe player has been bandaged     */
#define PLR_PR          55  /* Has had their potential released */
#define PLR_HEALT       56  /* Is inside a healing tank         */
#define PLR_FURY        57  /* Is in fury mode                  */
#define PLR_POSE        58  /* Ginyu Pose Effect                */
#define PLR_OOZARU      59
#define PLR_ABSORBED    60
#define PLR_MULTP       61
#define PLR_PDEATH      62
#define PLR_THANDW      63
#define PLR_SELFD       64
#define PLR_SELFD2      65
#define PLR_SPIRAL      66
#define PLR_BIOGR       67
#define PLR_LSSJ        68
#define PLR_REPLEARN    69
#define PLR_FORGET      70
#define PLR_TRANSMISSION 71
#define PLR_FISHING     72
#define PLR_GOOP        73
#define PLR_MULTIHIT    74
#define PLR_AURALIGHT   75
#define PLR_RDISPLAY    76
#define PLR_STOLEN      77
#define PLR_TAILHIDE    78  /* Hides tail for S & HB            */
#define PLR_NOGROW      79  /* Halt Growth for S & HB           */

#define NUM_PLR_FLAGS 80

/* Mob Personalty */
#define MAX_PERSONALITIES 5

/* Mobile flags: used by char_data.act */
#define MOB_SPEC		0  /* Mob has a callable spec-proc   	*/
#define MOB_SENTINEL		1  /* Mob should not move            	*/
#define MOB_NOSCAVENGER		2  /* Mob won't pick up items from rooms*/
#define MOB_ISNPC		3  /* (R) Automatically set on all Mobs */
#define MOB_AWARE		4  /* Mob can't be backstabbed          */
#define MOB_AGGRESSIVE		5  /* Mob auto-attacks everybody nearby	*/
#define MOB_STAY_ZONE		6  /* Mob shouldn't wander out of zone  */
#define MOB_WIMPY		7  /* Mob flees if severely injured  	*/
#define MOB_AGGR_EVIL		8  /* Auto-attack any evil PC's		*/
#define MOB_AGGR_GOOD		9  /* Auto-attack any good PC's      	*/
#define MOB_AGGR_NEUTRAL	10 /* Auto-attack any neutral PC's   	*/
#define MOB_MEMORY		11 /* remember attackers if attacked    */
#define MOB_HELPER		12 /* attack PCs fighting other NPCs    */
#define MOB_NOCHARM		13 /* Mob can't be charmed         	*/
#define MOB_NOSUMMON		14 /* Mob can't be summoned             */
#define MOB_NOSLEEP		15 /* Mob can't be slept           	*/
#define MOB_AUTOBALANCE		16 /* Mob stats autobalance		*/
#define MOB_NOBLIND		17 /* Mob can't be blinded         	*/
#define MOB_NOKILL		18 /* Mob can't be killed               */
#define MOB_NOTDEADYET		19 /* (R) Mob being extracted.          */
#define MOB_MOUNTABLE		20 /* Mob is mountable.			*/
#define MOB_RARM                21 /* Player has a right arm            */
#define MOB_LARM                22 /* Player has a left arm             */
#define MOB_RLEG                23 /* Player has a right leg            */
#define MOB_LLEG                24 /* Player has a left leg             */
#define MOB_HEAD                25 /* Player has a head                 */
#define MOB_JUSTDESC            26 /* Mob doesn't use auto desc         */
#define MOB_HUSK                27 /* Is an extracted Husk              */
#define MOB_SPAR                28 /* This is mob sparring              */
#define MOB_DUMMY               29 /* This mob will not fight back      */
#define MOB_ABSORB              30 /* Absorb type android               */
#define MOB_REPAIR              31 /* Repair type android               */
#define MOB_NOPOISON            32 /* No poison                         */
#define MOB_KNOWKAIO            33 /* Knows kaioken                     */
#define MOB_POWERUP             34 /* Is powering up                    */

#define NUM_MOB_FLAGS 35

/*  flags: used by char_data.player_specials.pref */
#define PRF_BRIEF	0  /* Room descs won't normally be shown	*/
#define PRF_COMPACT	1  /* No extra CRLF pair before prompts		*/
#define PRF_DEAF	2  /* Can't hear shouts              		*/
#define PRF_NOTELL	3  /* Can't receive tells		    	*/
#define PRF_DISPHP	4  /* Display hit points in prompt  		*/
#define PRF_DISPMANA	5  /* Display mana points in prompt    		*/
#define PRF_DISPMOVE	6  /* Display move points in prompt 		*/
#define PRF_AUTOEXIT	7  /* Display exits in a room          		*/
#define PRF_NOHASSLE	8  /* Aggr mobs won't attack           		*/
#define PRF_QUEST	9  /* On quest					*/
#define PRF_SUMMONABLE	10 /* Can be summoned				*/
#define PRF_NOREPEAT	11 /* No repetition of comm commands		*/
#define PRF_HOLYLIGHT	12 /* Can see in dark				*/
#define PRF_COLOR	13 /* Color					*/
#define PRF_SPARE	14 /* Used to be second color bit		*/
#define PRF_NOWIZ	15 /* Can't hear wizline			*/
#define PRF_LOG1	16 /* On-line System Log (low bit)		*/
#define PRF_LOG2	17 /* On-line System Log (high bit)		*/
#define PRF_NOAUCT	18 /* Can't hear auction channel		*/
#define PRF_NOGOSS	19 /* Can't hear gossip channel			*/
#define PRF_NOGRATZ	20 /* Can't hear grats channel			*/
#define PRF_ROOMFLAGS	21 /* Can see room flags (ROOM_x)		*/
#define PRF_DISPAUTO	22 /* Show prompt HP, MP, MV when < 30%.	*/
#define PRF_CLS         23 /* Clear screen in OasisOLC 			*/
#define PRF_BUILDWALK   24 /* Build new rooms when walking		*/
#define PRF_AFK         25 /* Player is AFK				*/
#define PRF_AUTOLOOT    26 /* Loot everything from a corpse		*/
#define PRF_AUTOGOLD    27 /* Loot gold from a corpse			*/
#define PRF_AUTOSPLIT   28 /* Split gold with group			*/
#define PRF_FULL_EXIT   29 /* Shows full autoexit details		*/
#define PRF_AUTOSAC     30 /* Sacrifice a corpse 			*/
#define PRF_AUTOMEM     31 /* Memorize spells				*/
#define PRF_VIEWORDER   32 /* if you want to see the newest first 	*/
#define PRF_NOCOMPRESS  33 /* If you want to force MCCP2 off          	*/
#define PRF_AUTOASSIST  34 /* Auto-assist toggle                      	*/
#define PRF_DISPKI	35 /* Display ki points in prompt 		*/
#define PRF_DISPEXP	36 /* Display exp points in prompt 		*/
#define PRF_DISPTNL	37 /* Display TNL exp points in prompt 		*/
#define PRF_TEST        38 /* Sets triggers safety off for imms         */
#define PRF_HIDE        39 /* Hide on who from other mortals            */
#define PRF_NMWARN      40 /* No mail warning                           */
#define PRF_HINTS       41 /* Receives hints                            */
#define PRF_FURY        42 /* Sees fury meter                           */
#define PRF_NODEC       43
#define PRF_NOEQSEE     44
#define PRF_NOMUSIC     45
#define PRF_LKEEP       46
#define PRF_DISTIME     47 /* Part of Prompt Options */
#define PRF_DISGOLD     48 /* Part of Prompt Options */
#define PRF_DISPRAC     49 /* Part of Prompt Options */
#define PRF_NOPARRY     50
#define PRF_DISHUTH     51 /* Part of Prompt Options */
#define PRF_DISPERC     52 /* Part of Prompt Options */
#define PRF_CARVE       53
#define PRF_ARENAWATCH  54
#define PRF_NOGIVE      55
#define PRF_INSTRUCT    56
#define PRF_GHEALTH     57
#define PRF_IHEALTH     58
#define PRF_ENERGIZE    59

#define NUM_PRF_FLAGS   60

/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF        0       /* Autoexit off                     */
#define EXIT_NORMAL     1       /* Brief display (stock behaviour)  */
#define EXIT_NA         2       /* Not implemented - do not use     */
#define EXIT_COMPLETE   3       /* Full display                     */

#define _exitlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch),PRF_AUTOEXIT) ? 1 : 0 ) + (PRF_FLAGGED((ch),PRF_FULL_EXIT) ? 2 : 0 ) : 0 )
#define EXIT_LEV(ch) (_exitlevel(ch))


/* Affect bits: used in char_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_DONTUSE           0    /* DON'T USE! 		*/
#define AFF_BLIND             1    /* (R) Char is blind         */
#define AFF_INVISIBLE         2    /* Char is invisible         */
#define AFF_DETECT_ALIGN      3    /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      4    /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      5    /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        6    /* Char can sense hidden life*/
#define AFF_WATERWALK         7    /* Char can walk on water    */
#define AFF_SANCTUARY         8    /* Char protected by sanct.  */
#define AFF_GROUP             9    /* (R) Char is grouped       */
#define AFF_CURSE             10   /* Char is cursed            */
#define AFF_INFRAVISION       11   /* Char can see in dark      */
#define AFF_POISON            12   /* (R) Char is poisoned      */
#define AFF_WEAKENED_STATE    13   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      14   /* Char protected from good  */
#define AFF_SLEEP             15   /* (R) Char magically asleep */
#define AFF_NOTRACK           16   /* Char can't be tracked     */
#define AFF_UNDEAD            17   /* Char is undead 		*/
#define AFF_PARALYZE          18   /* Char is paralized		*/
#define AFF_SNEAK             19   /* Char can move quietly     */
#define AFF_HIDE              20   /* Char is hidden            */
#define AFF_UNUSED20          21   /* Room for future expansion */
#define AFF_CHARM             22   /* Char is charmed         	*/
#define AFF_FLYING            23   /* Char is flying         	*/
#define AFF_WATERBREATH       24   /* Char can breath non O2    */
#define AFF_ANGELIC           25   /* Char is an angelic being  */
#define AFF_ETHEREAL          26   /* Char is ethereal          */
#define AFF_MAGICONLY         27   /* Char only hurt by magic   */
#define AFF_NEXTPARTIAL       28   /* Next action cannot be full*/
#define AFF_NEXTNOACTION      29   /* Next action cannot attack (took full action between rounds) */
#define AFF_STUNNED           30   /* Char is stunned		*/
#define AFF_TAMED             31   /* Char has been tamed	*/
#define AFF_CDEATH            32   /* Char is undergoing creeping death */
#define AFF_SPIRIT            33   /* Char has no body          */
#define AFF_STONESKIN         34   /* Char has temporary DR     */
#define AFF_SUMMONED          35   /* Char is summoned (i.e. transient */
#define AFF_CELESTIAL         36   /* Char is celestial         */
#define AFF_FIENDISH          37   /* Char is fiendish          */
#define AFF_FIRE_SHIELD       38   /* Char has fire shield      */
#define AFF_LOW_LIGHT         39   /* Char has low light eyes   */
#define AFF_ZANZOKEN          40   /* Char is ready to zanzoken */
#define AFF_KNOCKED           41   /* Char is knocked OUT!      */
#define AFF_MIGHT             42   /* Strength +3               */
#define AFF_FLEX              43   /* Agility +3                */
#define AFF_GENIUS            44   /* Intelligence +3           */
#define AFF_BLESS             45   /* Bless for better regen    */
#define AFF_BURNT             46   /* Disintergrated corpse     */
#define AFF_BURNED            47   /* Burned by honoo or similar skill */
#define AFF_MBREAK            48   /* Can't charge while flagged */
#define AFF_HASS              49   /* Does double punch damage  */
#define AFF_FUTURE            50   /* Future Sight */
#define AFF_PARA              51   /* Real Paralyze */
#define AFF_INFUSE            52   /* Ki infused attacks */
#define AFF_ENLIGHTEN         53   /* Enlighten */
#define AFF_FROZEN            54   /* They got frozededed */
#define AFF_FIRESHIELD        55   /* They have a blazing personality */
#define AFF_ENSNARED          56   /* They have silk ensnaring their arms! */
#define AFF_HAYASA            57   /* They are speedy!                */
#define AFF_PURSUIT           58   /* Being followed */
#define AFF_WITHER            59   /* Their body is withered */
#define AFF_HYDROZAP          60   /* Custom Skill Kanso Suru */
#define AFF_POSITION          61   /* Better combat position */
#define AFF_SHOCKED           62   /* Psychic Shock          */
#define AFF_METAMORPH         63   /* Metamorphisis, Demon's Ripoff Custom Skill */
#define AFF_HEALGLOW          64   /* Healing Glow */
#define AFF_EARMOR            65   /* Ethereal Armor */
#define AFF_ECHAINS           66   /* Ethereal Chains */
#define AFF_WUNJO             67   /* Wunjo rune */
#define AFF_POTENT            68   /* Purisaz rune */
#define AFF_ASHED             69   /* Leaves ash */
#define AFF_PUKED             70
#define AFF_LIQUEFIED         71
#define AFF_SHELL             72
#define AFF_IMMUNITY          73
#define AFF_SPIRITCONTROL     74

#define NUM_AFF_FLAGS 75

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0	/* Playing - Nominal state		*/
#define CON_CLOSE	 1	/* User disconnect, remove character.	*/
#define CON_GET_NAME	 2	/* By what name ..?			*/
#define CON_NAME_CNFRM	 3	/* Did I get that right, x?		*/
#define CON_PASSWORD	 4	/* Password:				*/
#define CON_NEWPASSWD	 5	/* Give me a password for x		*/
#define CON_CNFPASSWD	 6	/* Please retype password:		*/
#define CON_QSEX	 7	/* Sex?					*/
#define CON_QCLASS	 8	/* Class?				*/
#define CON_RMOTD	 9	/* PRESS RETURN after MOTD		*/
#define CON_MENU	 10	/* Your choice: (main menu)		*/
#define CON_EXDESC	 11	/* Enter a new description:		*/
#define CON_CHPWD_GETOLD 12	/* Changing passwd: get old		*/
#define CON_CHPWD_GETNEW 13	/* Changing passwd: get new		*/
#define CON_CHPWD_VRFY   14	/* Verify new password			*/
#define CON_DELCNF1	 15	/* Delete confirmation 1		*/
#define CON_DELCNF2	 16	/* Delete confirmation 2		*/
#define CON_DISCONNECT	 17	/* In-game link loss (leave character)	*/
#define CON_OEDIT	 18	/* OLC mode - object editor		*/
#define CON_REDIT	 19	/* OLC mode - room editor		*/
#define CON_ZEDIT	 20	/* OLC mode - zone info editor		*/
#define CON_MEDIT	 21	/* OLC mode - mobile editor		*/
#define CON_SEDIT	 22	/* OLC mode - shop editor		*/
#define CON_TEDIT	 23	/* OLC mode - text editor		*/
#define CON_CEDIT	 24	/* OLC mode - config editor		*/
#define CON_QRACE        25     /* Race? 				*/
#define CON_ASSEDIT      26     /* OLC mode - Assemblies                */
#define CON_AEDIT        27	/* OLC mode - social (action) edit      */
#define CON_TRIGEDIT     28	/* OLC mode - trigger edit              */
#define CON_RACE_HELP    29	/* Race Help 				*/
#define CON_CLASS_HELP   30	/* Class Help 				*/
#define CON_QANSI	 31	/* Ask for ANSI support     */
#define CON_GEDIT	 32	/* OLC mode - guild editor 		*/
#define CON_QROLLSTATS	 33	/* Reroll stats 			*/
#define CON_IEDIT        34	/* OLC mode - individual edit		*/
#define CON_LEVELUP	 35	/* Level up menu			*/
#define CON_QSTATS 	 36	/* Assign starting stats        	*/
#define CON_HAIRL        37     /* Choose your hair length        */
#define CON_HAIRS        38     /* Choose your hair style         */
#define CON_HAIRC        39     /* Choose your hair color         */
#define CON_SKIN         40     /* Choose your skin color         */
#define CON_EYE          41     /* Choose your eye color          */
#define CON_Q1           42     /* Make a life choice!            */
#define CON_Q2           43     /* Make a second life choice!     */
#define CON_Q3           44     /* Make a third life choice!      */
#define CON_Q4           45     /* Make a fourth life choice!     */
#define CON_Q5           46     /* Make a fifth life choice!      */
#define CON_Q6           47     /* Make a sixth life choice!      */
#define CON_Q7           48     /* Make a seventh life choice!    */
#define CON_Q8           49     /* Make an eighth life choice!    */
#define CON_Q9           50     /* Make a ninth life choice!      */
#define CON_QX           51     /* Make a tenth life choice!      */
#define CON_HSEDIT       52     /* House Olc                      */
#define CON_ALPHA        53     /* Alpha Password                 */
#define CON_ALPHA2       54     /* Alpha Password For Newb        */
#define CON_ANDROID      55
#define CON_HEDIT        56     /* OLC mode - help edit           */
#define CON_GET_USER     57
#define CON_GET_EMAIL    58
#define CON_UMENU        59
#define CON_USER_CONF    60
#define CON_DISTFEA      61
#define CON_HW           62
#define CON_AURA         63
#define CON_BONUS        64
#define CON_NEGATIVE     65
#define CON_NEWSEDIT     66
#define CON_RACIAL       67
#define CON_POBJ         68
#define CON_ALIGN        69
#define CON_SKILLS       70
#define CON_USER_TITLE   71
#define CON_GENOME       72

#define NUM_CON_TYPES 73

/* Colors that the player can define */
#define COLOR_NORMAL			0
#define COLOR_ROOMNAME			1
#define COLOR_ROOMOBJS			2
#define COLOR_ROOMPEOPLE		3
#define COLOR_HITYOU			4
#define COLOR_YOUHIT			5
#define COLOR_OTHERHIT			6
#define COLOR_CRITICAL			7
#define COLOR_HOLLER			8
#define COLOR_SHOUT			9
#define COLOR_GOSSIP			10
#define COLOR_AUCTION			11
#define COLOR_CONGRAT			12
#define COLOR_TELL			13
#define COLOR_YOUSAY			14
#define COLOR_ROOMSAY			15

#define NUM_COLOR			16

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_UNUSED0    0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_UNUSED1   11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD1    16
#define WEAR_WIELD2    17
#define WEAR_BACKPACK  18
#define WEAR_EAR_R     19
#define WEAR_EAR_L     20
#define WEAR_SH        21
#define WEAR_EYE       22

#define NUM_WEARS      23	/* This must be the # of eq positions!! */

#define SPELL_LEVEL_0     0 
#define SPELL_LEVEL_1     1
#define SPELL_LEVEL_2     2
#define SPELL_LEVEL_3     3
#define SPELL_LEVEL_4     4
#define SPELL_LEVEL_5     5
#define SPELL_LEVEL_6     6
#define SPELL_LEVEL_7     7
#define SPELL_LEVEL_8     8
#define SPELL_LEVEL_9     9

#define MAX_SPELL_LEVEL   10                    /* how many spell levels */
#define MAX_MEM          (MAX_SPELL_LEVEL * 10) /* how many total spells */

#define DOMAIN_UNDEFINED	-1
#define DOMAIN_AIR		0
#define DOMAIN_ANIMAL		1
#define DOMAIN_CHAOS		2
#define DOMAIN_DEATH		3
#define DOMAIN_DESTRUCTION	4
#define DOMAIN_EARTH		5
#define DOMAIN_EVIL		6
#define DOMAIN_FIRE		7
#define DOMAIN_GOOD		8
#define DOMAIN_HEALING		9
#define DOMAIN_KNOWLEDGE	10
#define DOMAIN_LAW		11
#define DOMAIN_LUCK		12
#define DOMAIN_MAGIC		13
#define DOMAIN_PLANT		14
#define DOMAIN_PROTECTION	15
#define DOMAIN_STRENGTH		16
#define DOMAIN_SUN		17
#define DOMAIN_TRAVEL		18
#define DOMAIN_TRICKERY		19
#define DOMAIN_UNIVERSAL	20
#define DOMAIN_WAR		22
#define DOMAIN_WATER		23
#define DOMAIN_ARTIFACE         24
#define DOMAIN_CHARM            25
#define DOMAIN_COMMUNITY        26
#define DOMAIN_CREATION         27
#define DOMAIN_DARKNESS         28
#define DOMAIN_GLORY            29
#define DOMAIN_LIBERATION       30
#define DOMAIN_MADNESS          31
#define DOMAIN_NOBILITY         32
#define DOMAIN_REPOSE           33
#define DOMAIN_RUNE             34
#define DOMAIN_SCALYKIND        35
#define DOMAIN_WEATHER          36

#define NUM_DOMAINS		37

#define SCHOOL_UNDEFINED	-1
#define SCHOOL_ABJURATION	0
#define SCHOOL_CONJURATION	1
#define SCHOOL_DIVINATION	2
#define SCHOOL_ENCHANTMENT	3
#define SCHOOL_EVOCATION	4
#define SCHOOL_ILLUSION		5
#define SCHOOL_NECROMANCY	6
#define SCHOOL_TRANSMUTATION	7
#define SCHOOL_UNIVERSAL	8

#define NUM_SCHOOLS		10

#define DEITY_UNDEFINED			-1

#define NUM_DEITIES			0

/* Combat feats that apply to a specific weapon type */
#define CFEAT_IMPROVED_CRITICAL			0
#define CFEAT_WEAPON_FINESSE			1
#define CFEAT_WEAPON_FOCUS			2
#define CFEAT_WEAPON_SPECIALIZATION		3
#define CFEAT_GREATER_WEAPON_FOCUS		4
#define CFEAT_GREATER_WEAPON_SPECIALIZATION	5

#define CFEAT_MAX				5

/* Spell feats that apply to a specific school of spells */
#define CFEAT_SPELL_FOCUS			0
#define CFEAT_GREATER_SPELL_FOCUS		1

#define SFEAT_MAX				1

/* object-related defines ********************************************/


/* Item types: used by obj_data.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_CAMPFIRE   7		/* Burn things for fun!		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_VEHICLE   24               /* Item is a vehicle            */
#define ITEM_HATCH     25               /* Item is a vehicle hatch      */
#define ITEM_WINDOW    26               /* Item is a vehicle window     */
#define ITEM_CONTROL   27               /* Item is a vehicle control    */
#define ITEM_PORTAL    28               /* Item is a portal	        */
#define ITEM_SPELLBOOK 29               /* Item is a spellbook	        */
#define ITEM_BOARD     30               /* Item is a message board 	*/
#define ITEM_CHAIR     31               /* Is a chair                   */
#define ITEM_BED       32               /* Is a bed                     */
#define ITEM_YUM       33               /* This was good food           */
#define ITEM_PLANT     34               /* This will grow!              */
#define ITEM_FISHPOLE  35               /* FOR FISHING                  */
#define ITEM_FISHBAIT  36               /* DITTO                        */

#define NUM_ITEM_TYPES 37

/* Take/Wear flags: used by obj_data.wear_flags */
#define ITEM_WEAR_TAKE        0  /* Item can be taken         */
#define ITEM_WEAR_FINGER      1  /* Can be worn on finger     */
#define ITEM_WEAR_NECK        2  /* Can be worn around neck   */
#define ITEM_WEAR_BODY        3  /* Can be worn on body       */
#define ITEM_WEAR_HEAD        4  /* Can be worn on head       */
#define ITEM_WEAR_LEGS        5  /* Can be worn on legs       */
#define ITEM_WEAR_FEET        6  /* Can be worn on feet       */
#define ITEM_WEAR_HANDS       7  /* Can be worn on hands      */
#define ITEM_WEAR_ARMS        8  /* Can be worn on arms       */
#define ITEM_WEAR_SHIELD      9  /* Can be used as a shield   */
#define ITEM_WEAR_ABOUT       10 /* Can be worn about body    */
#define ITEM_WEAR_WAIST       11 /* Can be worn around waist  */
#define ITEM_WEAR_WRIST       12 /* Can be worn on wrist      */
#define ITEM_WEAR_WIELD       13 /* Can be wielded            */
#define ITEM_WEAR_HOLD        14 /* Can be held               */
#define ITEM_WEAR_PACK        15 /* Can be worn as a backpack */
#define ITEM_WEAR_EAR         16 /* Can be worn as an earring */
#define ITEM_WEAR_SH          17 /* Can be worn as wings      */
#define ITEM_WEAR_EYE         18 /* Can be worn as a mask     */

#define NUM_ITEM_WEARS 19

/* Extra object flags: used by obj_data.extra_flags */
#define ITEM_GLOW            0  /* Item is glowing              */
#define ITEM_HUM             1  /* Item is humming              */
#define ITEM_NORENT          2  /* Item cannot be rented        */
#define ITEM_NODONATE        3  /* Item cannot be donated       */
#define ITEM_NOINVIS         4  /* Item cannot be made invis    */
#define ITEM_INVISIBLE       5  /* Item is invisible            */
#define ITEM_MAGIC           6  /* Item is magical              */
#define ITEM_NODROP          7  /* Item is cursed: can't drop   */
#define ITEM_BLESS           8  /* Item is blessed              */
#define ITEM_ANTI_GOOD       9  /* Not usable by good people    */
#define ITEM_ANTI_EVIL       10 /* Not usable by evil people    */
#define ITEM_ANTI_NEUTRAL    11 /* Not usable by neutral people */
#define ITEM_ANTI_WIZARD     12 /* Not usable by mages          */
#define ITEM_ANTI_CLERIC     13 /* Not usable by clerics        */
#define ITEM_ANTI_ROGUE      14 /* Not usable by thieves        */
#define ITEM_ANTI_FIGHTER    15 /* Not usable by warriors       */
#define ITEM_NOSELL          16 /* Shopkeepers won't touch it   */
#define ITEM_ANTI_DRUID      17 /* Not usable by druids         */
#define ITEM_2H              18 /* Requires two free hands      */
#define ITEM_ANTI_BARD       19 /* Not usable by bards          */
#define ITEM_ANTI_RANGER     20 /* Not usable by rangers        */
#define ITEM_ANTI_PALADIN    21 /* Not usable by paladins       */
#define ITEM_ANTI_HUMAN      22 /* Not usable by humans         */
#define ITEM_ANTI_ICER       23 /* Not usable by dwarves        */
#define ITEM_ANTI_SAIYAN     24 /* Not usable by elves          */
#define ITEM_ANTI_KONATSU    25 /* Not usable by gnomes         */
#define ITEM_UNIQUE_SAVE     26	/* unique object save           */
#define ITEM_BROKEN          27 /* Item is broken hands         */
#define ITEM_UNBREAKABLE     28 /* Item is unbreakable          */
#define ITEM_ANTI_MONK       29 /* Not usable by monks          */
#define ITEM_ANTI_BARBARIAN  30 /* Not usable by barbarians     */
#define ITEM_ANTI_SORCERER   31 /* Not usable by sorcerers      */
#define ITEM_DOUBLE          32 /* Double weapon                */
#define ITEM_ONLY_WIZARD     33 /* Only usable by mages         */
#define ITEM_ONLY_CLERIC     34 /* Only usable by clerics       */
#define ITEM_ONLY_ROGUE      35 /* Only usable by thieves       */
#define ITEM_ONLY_FIGHTER    36 /* Only usable by warriors      */
#define ITEM_ONLY_DRUID      37 /* Only usable by druids        */
#define ITEM_ONLY_BARD       38 /* Only usable by bards         */
#define ITEM_ONLY_RANGER     39 /* Only usable by rangers       */
#define ITEM_ONLY_PALADIN    40 /* Only usable by paladins      */
#define ITEM_ONLY_HUMAN      41 /* Only usable by humans        */
#define ITEM_ONLY_ICER       42 /* Only usable by dwarves       */
#define ITEM_ONLY_SAIYAN     43 /* Only usable by elves         */
#define ITEM_ONLY_KONATSU    44 /* Only usable by gnomes        */
#define ITEM_ONLY_MONK       45 /* Only usable by monks         */
#define ITEM_ONLY_BARBARIAN  46 /* Only usable by barbarians    */
#define ITEM_ONLY_SORCERER   47 /* Only usable by sorcerers     */
#define ITEM_ANTI_ARCANE_ARCHER        48 /* Not usable by Aracane Archers          */
#define ITEM_ANTI_ARCANE_TRICKSTER     49 /* Not usable by Aracane tricksters       */
#define ITEM_ANTI_ARCHMAGE             50 /* Not usable by Archmages                */
#define ITEM_ANTI_ASSASSIN             51 /* Not usable by Assassins                */
#define ITEM_ANTI_BLACKGUARD           52 /* Not usable by Blackguards              */
#define ITEM_ANTI_DRAGON_DISCIPLE      53 /* Not usable by Dragon disciples         */
#define ITEM_ANTI_DUELIST              54 /* Not usable by Duelists                 */
#define ITEM_ANTI_DWARVEN_DEFENDER     55 /* Not usable by Dwarven defenders        */
#define ITEM_ANTI_ELDRITCH_KNIGHT      56 /* Not usable by Eldritch knights         */
#define ITEM_ANTI_HIEROPHANT           57 /* Not usable by Hierophants              */ 
#define ITEM_ANTI_HORIZON_WALKER       58 /* Not usable by Horizon walkers          */     
#define ITEM_ANTI_LOREMASTER           59 /* Not usable by Loremasters              */
#define ITEM_ANTI_MYSTIC_THEURGE       60 /* Not usable by Mystic theurges          */
#define ITEM_ANTI_SHADOWDANCER         61 /* Not useable by shadowdancers           */ 
#define ITEM_ANTI_THAUMATURGIST        62 /* Not useable by thaumaturgists          */     
#define ITEM_BSCOUTER                  63
#define ITEM_MSCOUTER                  64
#define ITEM_ASCOUTER                  65
#define ITEM_USCOUTER                  66
#define ITEM_WEAPLVL1                  67
#define ITEM_WEAPLVL2                  68
#define ITEM_WEAPLVL3                  69
#define ITEM_WEAPLVL4                  70
#define ITEM_WEAPLVL5                  71
#define ITEM_CBOARD                    72
#define ITEM_FORGED                    73
#define ITEM_SHEATH                    74
#define ITEM_ONLY_JINTO                75
#define ITEM_BURIED                    76
#define ITEM_SLOT1                     77
#define ITEM_SLOT2                     78
#define ITEM_TOKEN                     79
#define ITEM_SLOT_ONE                  80
#define ITEM_SLOTS_FILLED              81
#define ITEM_RESTRING                  82
#define ITEM_CUSTOM                    83
#define ITEM_PROTECTED                 84
#define ITEM_NORANDOM                  85
#define ITEM_THROW                     86
#define ITEM_HOT                       87
#define ITEM_PURGE                     88
#define ITEM_ICE                       89
#define ITEM_DUPLICATE                 90
#define ITEM_MATURE                    91
#define ITEM_CARDCASE                  92
#define ITEM_NOPICKUP                  93
#define ITEM_NOSTEAL                   94

#define NUM_ITEM_FLAGS 95

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intelligence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_ACCURACY         18	/* Apply to accuracy		*/
#define APPLY_DAMAGE           19	/* Apply to damage 		*/
#define APPLY_REGEN	       20	/* Regen Rate Buffed            */
#define APPLY_TRAIN	       21	/* Skill training rate buffed   */
#define APPLY_LIFEMAX	       22	/* Life Force max buffed        */
#define APPLY_UNUSED3	       23	/* Unused			*/
#define APPLY_UNUSED4	       24	/* Unused			*/
#define APPLY_RACE             25       /* Apply to race                */
#define APPLY_TURN_LEVEL       26       /* Apply to turn undead         */
#define APPLY_SPELL_LVL_0      27       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_1      28       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_2      29       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_3      30       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_4      31       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_5      32       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_6      33       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_7      34       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_8      35       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_9      36       /* Apply to spell cast per day  */
#define APPLY_KI               37	/* Apply to max ki		*/
#define APPLY_FORTITUDE        38	/* Apply to fortitue save	*/
#define APPLY_REFLEX           39	/* Apply to reflex save		*/
#define APPLY_WILL             40	/* Apply to will save		*/
#define APPLY_SKILL            41       /* Apply to a specific skill    */
#define APPLY_FEAT             42       /* Apply to a specific feat     */
#define APPLY_ALLSAVES         43       /* Apply to all 3 save types 	*/
#define APPLY_RESISTANCE       44       /* Apply to resistance	 	*/
#define APPLY_ALL_STATS        45       /* Apply to all attributes	*/

#define NUM_APPLIES 46

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/

#define NUM_CONT_FLAGS 4

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15

#define NUM_LIQ_TYPES 16

#define MATERIAL_BONE           0
#define MATERIAL_CERAMIC        1
#define MATERIAL_COPPER         2
#define MATERIAL_DIAMOND        3
#define MATERIAL_GOLD           4
#define MATERIAL_IRON           5
#define MATERIAL_LEATHER        6
#define MATERIAL_MITHRIL        7
#define MATERIAL_OBSIDIAN       8
#define MATERIAL_STEEL          9
#define MATERIAL_STONE          10
#define MATERIAL_SILVER         11
#define MATERIAL_WOOD           12
#define MATERIAL_GLASS          13
#define MATERIAL_ORGANIC        14
#define MATERIAL_CURRENCY       15
#define MATERIAL_PAPER          16
#define MATERIAL_COTTON         17
#define MATERIAL_SATIN          18
#define MATERIAL_SILK           19
#define MATERIAL_BURLAP         20
#define MATERIAL_VELVET         21
#define MATERIAL_PLATINUM       22
#define MATERIAL_ADAMANTINE     23
#define MATERIAL_WOOL           24
#define MATERIAL_ONYX           25
#define MATERIAL_IVORY          26
#define MATERIAL_BRASS          27
#define MATERIAL_MARBLE         28
#define MATERIAL_BRONZE         29
#define MATERIAL_KACHIN         30
#define MATERIAL_RUBY           31
#define MATERIAL_SAPPHIRE       32
#define MATERIAL_EMERALD        33
#define MATERIAL_GEMSTONE       34
#define MATERIAL_GRANITE        35
#define MATERIAL_ENERGY         36
#define MATERIAL_HEMP           37
#define MATERIAL_CRYSTAL        38
#define MATERIAL_EARTH  	39
#define MATERIAL_LIQUID  	40
#define MATERIAL_CLOTH  	41
#define MATERIAL_METAL  	42
#define MATERIAL_WAX	  	43
#define MATERIAL_OTHER	  	44
#define MATERIAL_FOOD	  	45
#define MATERIAL_OIL	  	46

#define NUM_MATERIALS           47

/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define HUNGER       1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* for the 128bits */
#define RF_ARRAY_MAX    4
#define PM_ARRAY_MAX    4
#define PR_ARRAY_MAX    4
#define AF_ARRAY_MAX    4
#define TW_ARRAY_MAX    4
#define EF_ARRAY_MAX    4
#define AD_ARRAY_MAX	4
#define FT_ARRAY_MAX	4
#define ZF_ARRAY_MAX	4


/* History */
#define HIST_ALL       0
#define HIST_SAY       1
#define HIST_GOSSIP    2
#define HIST_WIZNET    3
#define HIST_TELL      4
#define HIST_SHOUT     5
#define HIST_GRATS     6
#define HIST_HOLLER    7
#define HIST_AUCTION   8
#define HIST_SNET      9

#define NUM_HIST      10

/* other #defined constants **********************************************/

/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
#define ADMLVL_NONE		0
#define ADMLVL_IMMORT		1
#define ADMLVL_BUILDER          2
#define ADMLVL_GOD		3
#define ADMLVL_VICE             4
#define ADMLVL_GRGOD		5
#define ADMLVL_IMPL		6

/* First character level that forces epic levels */
#define LVL_EPICSTART		101

/*
 * ADM flags - define admin privs for chars
 */
#define ADM_TELLALL		0	/* Can use 'tell all' to broadcast GOD */
#define ADM_SEEINV		1	/* Sees other chars inventory IMM */
#define ADM_SEESECRET		2	/* Sees secret doors IMM */
#define ADM_KNOWWEATHER		3	/* Knows details of weather GOD */
#define ADM_FULLWHERE		4	/* Full output of 'where' command IMM */
#define ADM_MONEY 		5	/* Char has a bottomless wallet GOD */
#define ADM_EATANYTHING 	6	/* Char can eat anything GOD */
#define ADM_NOPOISON	 	7	/* Char can't be poisoned IMM */
#define ADM_WALKANYWHERE	8	/* Char has unrestricted walking IMM */
#define ADM_NOKEYS		9	/* Char needs no keys for locks GOD */
#define ADM_INSTANTKILL		10	/* "kill" command is instant IMPL */
#define ADM_NOSTEAL		11	/* Char cannot be stolen from IMM */
#define ADM_TRANSALL		12	/* Can use 'trans all' GRGOD */
#define ADM_SWITCHMORTAL	13	/* Can 'switch' to a mortal PC body IMPL */
#define ADM_FORCEMASS		14	/* Can force rooms or all GRGOD */
#define ADM_ALLHOUSES		15	/* Can enter any house GRGOD */
#define ADM_NODAMAGE		16	/* Cannot be damaged IMM */
#define ADM_ALLSHOPS		17	/* Can use all shops GOD */
#define ADM_CEDIT		18	/* Can use cedit IMPL */

#define NUM_ADMFLAGS            19

/* Level of the 'freeze' command */
#define ADMLVL_FREEZE	ADMLVL_GRGOD

#define NUM_OF_DIRS	12	/* number of directions in a room (nsewud) */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC	100000		/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC
#define CD_TICK         * PASSES_PER_SEC

#define PULSE_ZONE	(CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE    (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE  (CONFIG_PULSE_VIOLENCE RL_SEC)
#define PULSE_AUCTION	(15 RL_SEC)
#define PULSE_AUTOSAVE	(CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD	(CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY	(CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE	(CONFIG_PULSE_SANITY * 300 RL_SEC)   /* 5 mins */
#define PULSE_TIMESAVE	(CONFIG_PULSE_TIMESAVE * 900 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT	(CONFIG_PULSE_CURRENT RL_SEC)
#define PULSE_1SEC      (1 RL_SEC)
#define PULSE_2SEC      (2 RL_SEC)
#define PULSE_3SEC      (3 RL_SEC)
#define PULSE_4SEC      (4 RL_SEC)
#define PULSE_5SEC      (5 RL_SEC)
#define PULSE_6SEC      (6 RL_SEC)
#define PULSE_7SEC      (7 RL_SEC)

/* Cool Down Ticks */
#define PULSE_CD1       (1 CD_TICK)
#define PULSE_CD2       (2 CD_TICK)
#define PULSE_CD3       (3 CD_TICK)
#define PULSE_CD4       (4 CD_TICK) /* This and the 3 above are for safety */
#define PULSE_CD5       (5 CD_TICK) /* Punch */
#define PULSE_CD6       (6 CD_TICK)
#define PULSE_CD7       (7 CD_TICK)
#define PULSE_CD8       (8 CD_TICK)
#define PULSE_CD9       (9 CD_TICK)
#define PULSE_CD10      (10 CD_TICK)
#define PULSE_CD11      (11 CD_TICK)
#define PULSE_CD12      (12 CD_TICK)
/* End CD Ticks    */


/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (96 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       1024          /* Max length of prompt        */
#define GARBAGE_SPACE		512          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		6020        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE		5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH	64936   
#define MAX_INPUT_LENGTH	2048	/* Max length per *line* of 
input */
#define MAX_RAW_INPUT_LENGTH	4096	/* Max size of *raw* input */
#define MAX_MESSAGES		100
#define MAX_NAME_LENGTH		20
#define MAX_PWD_LENGTH		30
#define MAX_TITLE_LENGTH	120
#define HOST_LENGTH		40
#define EXDSCR_LENGTH		16384
#define MAX_TONGUE		3
#define MAX_SKILLS		200
#define MAX_AFFECT		32
#define MAX_OBJ_AFFECT		6
#define MAX_NOTE_LENGTH		6000	/* arbitrary */
#define SKILL_TABLE_SIZE	1000
#define SPELLBOOK_SIZE		50
#define MAX_FEATS	        750
#define MAX_HELP_KEYWORDS       256
#define MAX_HELP_ENTRY          MAX_STRING_LENGTH
#define NUM_FEATS_DEFINED       252
#define MAX_ARMOR_TYPES         5
#define NUM_CONFIG_SECTIONS     7
#define NUM_CREATION_METHODS    5
#define NUM_ATTACK_TYPES        15
#define NUM_MTRIG_TYPES         20
#define NUM_OTRIG_TYPES         20
#define NUM_WTRIG_TYPES         20
#define NUM_ZONE_FLAGS          36
#define NUM_TRADERS             78
#define NUM_SHOP_FLAGS          3
#define NUM_DOOR_CMD            5
#define MAX_ASSM    	        11
#define NUM_FULLNESS		5
#define NUM_WEEK_DAYS		7
#define NUM_MONTHS		12
#define NUM_CONDITIONS		3
#define NUM_WIELD_NAMES		4

/* define the largest set of commands for a trigger */
#define MAX_CMD_LENGTH          16384 /* 16k should be plenty and then some */


/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */

/**********************************************************************
* Structures                                                          *
**********************************************************************/

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/
#define NUM_OBJ_VAL_POSITIONS 16

#define VAL_ALL_HEALTH                4
#define VAL_ALL_MAXHEALTH             5
#define VAL_ALL_MATERIAL              7
/*
 * Uses for generic object values on specific object types
 * Please use these instead of numbers to prevent overlaps.
 */
#define VAL_LIGHT_TIME                0
#define VAL_LIGHT_UNUSED2             1
#define VAL_LIGHT_HOURS               2
#define VAL_LIGHT_UNUSED4             3
#define VAL_LIGHT_HEALTH              4
#define VAL_LIGHT_MAXHEALTH           5
#define VAL_LIGHT_UNUSED7             6
#define VAL_LIGHT_MATERIAL            7
#define VAL_SCROLL_LEVEL              0
#define VAL_SCROLL_SPELL1             1
#define VAL_SCROLL_SPELL2             2
#define VAL_SCROLL_SPELL3             3
#define VAL_SCROLL_HEALTH             4
#define VAL_SCROLL_MAXHEALTH          5
#define VAL_SCROLL_UNUSED7            6
#define VAL_SCROLL_MATERIAL           7
#define VAL_WAND_LEVEL                0
#define VAL_WAND_MAXCHARGES           1
#define VAL_WAND_CHARGES              2
#define VAL_WAND_SPELL                3
#define VAL_WAND_HEALTH               4
#define VAL_WAND_MAXHEALTH            5
#define VAL_WAND_UNUSED7              6
#define VAL_WAND_MATERIAL             7
#define VAL_STAFF_LEVEL               0
#define VAL_STAFF_MAXCHARGES          1
#define VAL_STAFF_CHARGES             2
#define VAL_STAFF_SPELL               3
#define VAL_STAFF_HEALTH              4
#define VAL_STAFF_MAXHEALTH           5
#define VAL_STAFF_UNUSED7             6
#define VAL_STAFF_MATERIAL            7
#define VAL_WEAPON_SKILL              0
#define VAL_WEAPON_DAMDICE            1
#define VAL_WEAPON_DAMSIZE            2
#define VAL_WEAPON_DAMTYPE            3
#define VAL_WEAPON_HEALTH             4
#define VAL_WEAPON_MAXHEALTH          5
#define VAL_WEAPON_CRITTYPE           6
#define VAL_WEAPON_MATERIAL           7
#define VAL_WEAPON_CRITRANGE          8
#define VAL_FIREWEAPON_UNUSED1        0
#define VAL_FIREWEAPON_UNUSED2        1
#define VAL_FIREWEAPON_UNUSED3        2
#define VAL_FIREWEAPON_UNUSED4        3
#define VAL_FIREWEAPON_HEALTH         4
#define VAL_FIREWEAPON_MAXHEALTH      5
#define VAL_FIREWEAPON_UNUSED7        6
#define VAL_FIREWEAPON_MATERIAL       7
#define VAL_MISSILE_UNUSED1           0
#define VAL_MISSILE_UNUSED2           1
#define VAL_MISSILE_UNUSED3           2
#define VAL_MISSILE_UNUSED4           3
#define VAL_MISSILE_HEALTH            4
#define VAL_MISSILE_MAXHEALTH         5
#define VAL_MISSILE_UNUSED7           6
#define VAL_MISSILE_MATERIAL          7
#define VAL_TREASURE_UNUSED1          0
#define VAL_TREASURE_UNUSED2          1
#define VAL_TREASURE_UNUSED3          2
#define VAL_TREASURE_UNUSED4          3
#define VAL_TREASURE_HEALTH           4
#define VAL_TREASURE_MAXHEALTH        5
#define VAL_TREASURE_UNUSED7          6
#define VAL_TREASURE_MATERIAL         7
#define VAL_ARMOR_APPLYAC             0
#define VAL_ARMOR_SKILL               1
#define VAL_ARMOR_MAXDEXMOD           2
#define VAL_ARMOR_CHECK               3
#define VAL_ARMOR_HEALTH              4
#define VAL_ARMOR_MAXHEALTH           5
#define VAL_ARMOR_SPELLFAIL           6
#define VAL_ARMOR_MATERIAL            7
#define VAL_POTION_LEVEL              0
#define VAL_POTION_SPELL1             1
#define VAL_POTION_SPELL2             2
#define VAL_POTION_SPELL3             3
#define VAL_POTION_HEALTH             4
#define VAL_POTION_MAXHEALTH          5
#define VAL_POTION_UNUSED7            6
#define VAL_POTION_MATERIAL           7
#define VAL_WORN_UNUSED1              0
#define VAL_WORN_UNUSED2              1
#define VAL_WORN_UNUSED3              2
#define VAL_WORN_UNUSED4              3
#define VAL_WORN_HEALTH               4
#define VAL_WORN_MAXHEALTH            5
#define VAL_WORN_UNUSED7              6
#define VAL_WORN_MATERIAL             7
#define VAL_OTHER_UNUSED1             0
#define VAL_OTHER_UNUSED2             1
#define VAL_OTHER_UNUSED3             2
#define VAL_OTHER_UNUSED4             3
#define VAL_OTHER_HEALTH              4
#define VAL_OTHER_MAXHEALTH           5
#define VAL_OTHER_UNUSED7             6
#define VAL_OTHER_MATERIAL            7
#define VAL_TRASH_UNUSED1             0
#define VAL_TRASH_UNUSED2             1
#define VAL_TRASH_UNUSED3             2
#define VAL_TRASH_UNUSED4             3
#define VAL_TRASH_HEALTH              4
#define VAL_TRASH_MAXHEALTH           5
#define VAL_TRASH_UNUSED7             6
#define VAL_TRASH_MATERIAL            7
#define VAL_TRAP_SPELL                0
#define VAL_TRAP_HITPOINTS            1
#define VAL_TRAP_UNUSED3              2
#define VAL_TRAP_UNUSED4              3
#define VAL_TRAP_HEALTH               4
#define VAL_TRAP_MAXHEALTH            5
#define VAL_TRAP_UNUSED7              6
#define VAL_TRAP_MATERIAL             7
#define VAL_CONTAINER_CAPACITY        0
#define VAL_CONTAINER_FLAGS           1
#define VAL_CONTAINER_KEY             2
#define VAL_CONTAINER_CORPSE          3
#define VAL_CONTAINER_HEALTH          4
#define VAL_CONTAINER_MAXHEALTH       5
#define VAL_CONTAINER_UNUSED7         6
#define VAL_CONTAINER_MATERIAL        7
#define VAL_CONTAINER_OWNER           8
#define VAL_NOTE_LANGUAGE             0
#define VAL_NOTE_UNUSED2              1
#define VAL_NOTE_UNUSED3              2
#define VAL_NOTE_UNUSED4              3
#define VAL_NOTE_HEALTH               4
#define VAL_NOTE_MAXHEALTH            5
#define VAL_NOTE_UNUSED7              6
#define VAL_NOTE_MATERIAL             7
#define VAL_DRINKCON_CAPACITY         0
#define VAL_DRINKCON_HOWFULL          1
#define VAL_DRINKCON_LIQUID           2
#define VAL_DRINKCON_POISON           3
#define VAL_DRINKCON_HEALTH           4
#define VAL_DRINKCON_MAXHEALTH        5
#define VAL_DRINKCON_UNUSED7          6
#define VAL_DRINKCON_MATERIAL         7
#define VAL_KEY_UNUSED1               0
#define VAL_KEY_UNUSED2               1
#define VAL_KEY_KEYCODE               2
#define VAL_KEY_UNUSED4               3
#define VAL_KEY_HEALTH                4
#define VAL_KEY_MAXHEALTH             5
#define VAL_KEY_UNUSED7               6
#define VAL_KEY_MATERIAL              7
#define VAL_FOOD_FOODVAL              0
#define VAL_FOOD_UNUSED2              1
#define VAL_FOOD_UNUSED3              2
#define VAL_FOOD_POISON               3
#define VAL_FOOD_HEALTH               4
#define VAL_FOOD_MAXHEALTH            5
#define VAL_FOOD_UNUSED7              6
#define VAL_FOOD_MATERIAL             7
#define VAL_MONEY_SIZE                0
#define VAL_MONEY_UNUSED2             1
#define VAL_MONEY_UNUSED3             2
#define VAL_MONEY_UNUSED4             3
#define VAL_MONEY_HEALTH              4
#define VAL_MONEY_MAXHEALTH           5
#define VAL_MONEY_UNUSED7             6
#define VAL_MONEY_MATERIAL            7
#define VAL_PEN_UNUSED1               0
#define VAL_PEN_UNUSED2               1
#define VAL_PEN_UNUSED3               2
#define VAL_PEN_UNUSED4               3
#define VAL_PEN_HEALTH                4
#define VAL_PEN_MAXHEALTH             5
#define VAL_PEN_UNUSED7               6
#define VAL_PEN_MATERIAL              7
#define VAL_BOAT_UNUSED1              0
#define VAL_BOAT_UNUSED2              1
#define VAL_BOAT_UNUSED3              2
#define VAL_BOAT_UNUSED4              3
#define VAL_BOAT_HEALTH               4
#define VAL_BOAT_MAXHEALTH            5
#define VAL_BOAT_UNUSED7              6
#define VAL_BOAT_MATERIAL             7
#define VAL_FOUNTAIN_CAPACITY         0
#define VAL_FOUNTAIN_HOWFULL          1
#define VAL_FOUNTAIN_LIQUID           2
#define VAL_FOUNTAIN_POISON           3
#define VAL_FOUNTAIN_HEALTH           4
#define VAL_FOUNTAIN_MAXHEALTH        5
#define VAL_FOUNTAIN_UNUSED7          6
#define VAL_FOUNTAIN_MATERIAL         7
#define VAL_VEHICLE_ROOM              0
#define VAL_VEHICLE_UNUSED2           1
#define VAL_VEHICLE_UNUSED3           2
#define VAL_VEHICLE_APPEAR            3
#define VAL_VEHICLE_HEALTH            4
#define VAL_VEHICLE_MAXHEALTH         5
#define VAL_VEHICLE_UNUSED7           6
#define VAL_VEHICLE_MATERIAL          7
#define VAL_HATCH_DEST                0
#define VAL_HATCH_FLAGS               1
#define VAL_HATCH_DCSKILL             2
#define VAL_HATCH_DCMOVE              3
#define VAL_HATCH_HEALTH              4
#define VAL_HATCH_MAXHEALTH           5
#define VAL_HATCH_UNUSED7             6
#define VAL_HATCH_MATERIAL            7
#define VAL_HATCH_DCLOCK              8
#define VAL_HATCH_DCHIDE              9
#define VAL_WINDOW_UNUSED1            0
#define VAL_WINDOW_UNUSED2            1
#define VAL_WINDOW_UNUSED3            2
#define VAL_WINDOW_UNUSED4            3
#define VAL_WINDOW_HEALTH             4
#define VAL_WINDOW_MAXHEALTH          5
#define VAL_WINDOW_UNUSED7            6
#define VAL_WINDOW_MATERIAL           7
#define VAL_CONTROL_UNUSED1           0
#define VAL_CONTROL_UNUSED2           1
#define VAL_CONTROL_UNUSED3           2
#define VAL_CONTROL_UNUSED4           3
#define VAL_CONTROL_HEALTH            4
#define VAL_CONTROL_MAXHEALTH         5
#define VAL_CONTROL_UNUSED7           6
#define VAL_CONTROL_MATERIAL          7
#define VAL_PORTAL_DEST               0
#define VAL_PORTAL_DCSKILL            1
#define VAL_PORTAL_DCMOVE             2
#define VAL_PORTAL_APPEAR             3
#define VAL_PORTAL_HEALTH             4
#define VAL_PORTAL_MAXHEALTH          5
#define VAL_PORTAL_UNUSED7            6
#define VAL_PORTAL_MATERIAL           7
#define VAL_PORTAL_DCLOCK             8
#define VAL_PORTAL_DCHIDE             9
#define VAL_BOARD_READ                0
#define VAL_BOARD_WRITE               1
#define VAL_BOARD_ERASE               2
#define VAL_BOARD_UNUSED4             3
#define VAL_BOARD_HEALTH              4
#define VAL_BOARD_MAXHEALTH           5
#define VAL_BOARD_UNUSED7             6
#define VAL_BOARD_MATERIAL            7
#define VAL_DOOR_DCLOCK               8
#define VAL_DOOR_DCHIDE               9
#define VAL_CORPSE_HEAD               8
#define VAL_CORPSE_RARM               9
#define VAL_CORPSE_LARM              10
#define VAL_CORPSE_RLEG              11
#define VAL_CORPSE_LLEG              12 
#define VAL_GROWTH                    0
#define VAL_MATGOAL                   1
#define VAL_MATURITY                  2
#define VAL_MAXMATURE                 3
#define VAL_WATERLEVEL                6
#define VAL_SOILQ                     8

struct obj_affected_type {
   int location;       /* Which ability to change (APPLY_XXX) */
   int specific;       /* Some locations have parameters      */
   int modifier;       /* How much it changes by              */
};

struct obj_spellbook_spell {
   int spellname;	/* Which spell is written */
   int pages;		/* How many pages does it take up */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/
   room_vnum room_loaded;	/* Room loaded in, for room_max checks	*/

   int  value[NUM_OBJ_VAL_POSITIONS];   /* Values of the item (see list)    */
   int8_t type_flag;      /* Type of item                        */
   int  level;           /* Minimum level of object.            */
   int  wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
   int  extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
   int64_t  weight;         /* Weigt what else                     */
   int  cost;           /* Value when sold (gp.)               */
   int  cost_per_day;   /* Cost to keep pr. real day           */
   int  timer;          /* Timer for object                    */
   int  bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
   int  size;           /* Size class of object                */

   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   int16_t worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   int32_t id;                       /* used by DG triggers              */
   time_t generation;             /* creation time for dupe check     */
   int64_t unique_id;  /* random bits for dupe check       */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */

   struct obj_spellbook_spell *sbinfo;  /* For spellbook info */
   struct char_data *sitting;       /* Who is sitting on me? */
   int scoutfreq;
   time_t lload;
   int healcharge;
   int64_t kicharge;
   int kitype;
   struct char_data *user;
   struct char_data *target;
   int distance;
   int foob;
   int32_t aucter;
   int32_t curBidder;
   time_t aucTime;
   int bid;
   int startbid;
   char *auctname;
   int posttype;
   struct obj_data *posted_to;
   struct obj_data *fellow_wall;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   int16_t exit_info;		/* Exit info			*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
   int dclock;			/* DC to pick the lock			*/
   int dchide;			/* DC to find hidden			*/
   int dcskill;			/* Skill req. to move through exit	*/
   int dcmove;			/* DC for skill to move through exit	*/
   int failsavetype;		/* Saving Throw type on skill fail	*/
   int dcfailsave;		/* DC to save against on fail		*/
   int failroom;		/* Room # to put char in when fail > 5  */
   int totalfailroom;		/* Room # if char fails save < 5	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int room_flags[RF_ARRAY_MAX];   /* DEATH,DARK ... etc */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */

   int8_t light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */

   int timed;                   /* For timed Dt's                     */
   int dmg;                     /* How damaged the room is            */
   int gravity;                 /* What is the level of gravity?      */
   int geffect;			/* Effect of ground destruction       */

};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   int32_t id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   int16_t year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;	/* This represents the characters current age        */
   time_t created;	/* This does not change                              */
   time_t maxage;	/* This represents death by natural causes           */
   time_t logon;	/* Time of the last logon (used to calculate played) */
   time_t played;	/* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  int level;		/* max level for this time limit	*/
  int days;		/* time limit in days			*/
}; 


/* Char's abilities. */
struct abil_data {
   int8_t str;            /* New stats can go over 18 freely, no more /xx */
   int8_t intel;
   int8_t wis;
   int8_t dex;
   int8_t con;
   int8_t cha;
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct player_special_data {
  char *poofin;			/* Description on arrival of a god.     */
  char *poofout;		/* Description upon a god's exit.       */
  struct alias_data *aliases;	/* Character's aliases                  */
  int32_t last_tell;		/* idnum of last tell from              */
  void *last_olc_targ;		/* olc control                          */
  int last_olc_mode;		/* olc control                          */
  char *host;			/* host of last logon                   */
  struct imcchar_data *imcchardata;  /**< IMC2 Data */
  int spell_level[MAX_SPELL_LEVEL];
				/* bonus to number of spells memorized */
  int memcursor;		/* points to the next free slot in spellmem */
  int wimp_level;		/* Below this # of hit points, flee!	*/
  int8_t freeze_level;		/* Level of god who froze char, if any	*/
  int16_t invis_level;		/* level of invisibility		*/
  room_vnum load_room;		/* Which room to place char in		*/
  int pref[PR_ARRAY_MAX];	/* preference flags for PC's.		*/
  uint8_t bad_pws;		/* number of bad password attemps	*/
  int8_t conditions[NUM_CONDITIONS];		/* Drunk, full, thirsty			*/
  int skill_points;		/* Skill points earned from race HD	*/
  int class_skill_points[NUM_CLASSES];
				/* Skill points earned from a class	*/
  struct txt_block *comm_hist[NUM_HIST]; /* Player's communcations history     */
  int olc_zone;			/* Zone where OLC is permitted		*/
  int gauntlet;                 /* Highest Gauntlet Position */
  int speaking;			/* Language currently speaking		*/
  int tlevel;			/* Turning level			*/
  int ability_trains;		/* How many stat points can you train?	*/
  int spellmem[MAX_MEM];	/* Spell slots				*/
  int feat_points;		/* How many general feats you can take	*/
  int epic_feat_points;		/* How many epic feats you can take	*/
  int class_feat_points[NUM_CLASSES];
				/* How many class feats you can take	*/
  int epic_class_feat_points[NUM_CLASSES];
				/* How many epic class feats 		*/
  int domain[NUM_DOMAINS];
  int school[NUM_SCHOOLS];
  int deity;
  int spell_mastery_points;
  char *color_choices[NUM_COLOR]; /* Choices for custom colors		*/
  uint8_t page_length;
  int murder;                   /* Murder of PC's count                 */
  int trainstr;
  int trainint;
  int traincon;
  int trainwis;
  int trainagl;
  int trainspd;

  struct char_data *carrying;
  struct char_data *carried_by;

  int racial_pref;
};


/* this can be used for skills that can be used per-day */
struct memorize_node {
   int		timer;			/* how many ticks till memorized */
   int		spell; 			/* the spell number */
   struct 	memorize_node *next; 	/* link to the next node */
};

struct innate_node {
   int timer;
   int spellnum;
   struct innate_node *next;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;	    /* List of attackers to remember	       */
   int8_t	attack_type;        /* The Attack Type Bitvector for NPC's     */
   int8_t default_pos;        /* Default position for NPC                */
   int8_t damnodice;          /* The number of damage dice's	       */
   int8_t damsizedice;        /* The size of the damage dice's           */
   int newitem;             /* Check if mob has new inv item       */
};


/* An affect structure. */
struct affected_type {
   int16_t type;          /* The type of spell that caused this      */
   int16_t duration;      /* For how long its effects will last      */
   int modifier;         /* This is added to apropriate ability     */
   int location;         /* Tells which ability to change(APPLY_XXX)*/
   int specific;         /* Some locations have parameters          */
   bitvector_t bitvector; /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};

/* Queued spell entry */
struct queued_act {
   int level;
   int spellnum;
};

/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


#define LEVELTYPE_CLASS	1
#define LEVELTYPE_RACE	2

struct level_learn_entry {
  struct level_learn_entry *next;
  int location;
  int specific;
  int8_t value;
};

struct levelup_data {
  struct levelup_data *next;	/* Form a linked list			*/
  struct levelup_data *prev;	/* Form a linked list			*/
  int8_t type;		/* LEVELTYPE_ value			*/
  int8_t spec;		/* Specific class or race		*/
  int8_t level;		/* Level ir HD # for that class or race	*/

  int8_t hp_roll;		/* Straight die-roll value with no mods	*/
  int8_t mana_roll;		/* Straight die-roll value with no mods	*/
  int8_t ki_roll;		/* Straight die-roll value with no mods	*/
  int8_t move_roll;		/* Straight die-roll value with no mods	*/

  int8_t accuracy;		/* Hit accuracy change			*/
  int8_t fort;		/* Fortitude change			*/
  int8_t reflex;		/* Reflex change			*/
  int8_t will;		/* Will change				*/

  int8_t add_skill;		/* Total added skill points		*/
  int8_t add_gen_feats;	/* General feat points			*/
  int8_t add_epic_feats;	/* General epic feat points		*/
  int8_t add_class_feats;	/* Class feat points			*/
  int8_t add_class_epic_feats;/* Epic class feat points		*/

  struct level_learn_entry *skills;	/* Head of linked list		*/
  struct level_learn_entry *feats;	/* Head of linked list		*/
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
  int pfilepos;			/* playerfile pos			*/
  mob_rnum nr;			/* Mob's rnum				*/
  room_rnum in_room;		/* Location (real room number)		*/
  room_rnum was_in_room;	/* location for linkdead people		*/
  int wait;			/* wait for how many loops		*/

  char *name;			/* PC / NPC s name (kill ...  )		*/
  char *short_descr;		/* for NPC 'actions'			*/
  char *long_descr;		/* for 'look'				*/
  char *description;		/* Extra descriptions                   */
  char *title;			/* PC / NPC's title                     */
  int size;			/* Size class of char                   */
  int8_t sex;			/* PC / NPC's sex                       */
  int8_t race;		/* PC / NPC's race                      */
  int8_t hairl;               /* PC hair length                       */
  int8_t hairs;               /* PC hair style                        */
  int8_t hairc;               /* PC hair color                        */
  int8_t skin;                /* PC skin color                        */
  int8_t eye;                 /* PC eye color                         */
  int8_t distfea;             /* PC's Distinguishing Feature          */
  int race_level;		/* PC / NPC's racial level / hit dice   */
  int level_adj;		/* PC level adjustment                  */
  int8_t chclass;		/* Last class taken                     */
  int chclasses[NUM_CLASSES];	/* Ranks in all classes        */
  int epicclasses[NUM_CLASSES];	/* Ranks in all epic classes */
  struct levelup_data *level_info;
				/* Info on gained levels */
  int level;			/* PC / NPC's level                     */
  int admlevel;			/* PC / NPC's admin level               */
  int admflags[AD_ARRAY_MAX];	/* Bitvector for admin privs		*/
  room_vnum hometown;		/* PC Hometown / NPC spawn room         */
  struct time_data time;	/* PC's AGE in days			*/
  uint8_t weight;		/* PC / NPC's weight                    */
  uint8_t height;		/* PC / NPC's height                    */

  struct abil_data real_abils;	/* Abilities without modifiers   */
  struct abil_data aff_abils;	/* Abils with spells/stones/etc  */
  struct player_special_data *player_specials;
				/* PC specials				*/
  struct mob_special_data mob_specials;
				/* NPC specials				*/

  struct affected_type *affected;
				/* affected by what spells		*/
  struct affected_type *affectedv;
				/* affected by what combat spells	*/
  struct queued_act *actq;	/* queued spells / other actions	*/

  struct obj_data *equipment[NUM_WEARS];
				/* Equipment array			*/
  struct obj_data *carrying;	/* Head of list				*/

  struct descriptor_data *desc;	/* NULL for mobiles			*/
  int32_t id;			/* used by DG triggers			*/

  struct trig_proto_list *proto_script;
				/* list of default triggers		*/
  struct script_data *script;	/* script info for the object		*/
  struct script_memory *memory;	/* for mob memory triggers		*/

  struct char_data *next_in_room;
				/* For room->people - list		*/
  struct char_data *next;	/* For either monster or ppl-list	*/
  struct char_data *next_fighting;
				/* For fighting list			*/
  struct char_data *next_affect;/* For affect wearoff			*/
  struct char_data *next_affectv;
				/* For round based affect wearoff	*/

  struct follow_type *followers;/* List of chars followers		*/
  struct char_data *master;	/* Who is char following?		*/
  int32_t master_id;

  struct memorize_node *memorized;
  struct innate_node *innate;

  struct char_data *fighting;	/* Opponent				*/

  int8_t position;		/* Standing, fighting, sleeping, etc.	*/

  int carry_weight;		/* Carried weight			*/
  int8_t carry_items;		/* Number of items carried		*/
  int timer;			/* Timer for update			*/

  struct obj_data *sits;      /* What am I sitting on? */
  struct char_data *blocks;    /* Who am I blocking?    */
  struct char_data *blocked;   /* Who is blocking me?    */
  struct char_data *absorbing; /* Who am I absorbing */
  struct char_data *absorbby;  /* Who is absorbing me */

  int8_t feats[MAX_FEATS + 1];	/* Feats (booleans and counters)	*/
  int combat_feats[CFEAT_MAX+1][FT_ARRAY_MAX];
				/* One bitvector array per CFEAT_ type	*/
  int school_feats[SFEAT_MAX+1];/* One bitvector array per CFEAT_ type	*/

  int8_t skills[SKILL_TABLE_SIZE + 1];
				/* array of skills/spells/arts/etc	*/
  int8_t skillmods[SKILL_TABLE_SIZE + 1];
				/* array of skill mods			*/
  int8_t skillperfs[SKILL_TABLE_SIZE + 1];
                                /* array of skill mods                  */

  int alignment;		/* +-1000 for alignment good vs. evil	*/
  int alignment_ethic;		/* +-1000 for alignment law vs. chaos	*/
  int32_t idnum;			/* player's idnum; -1 for mobiles	*/
  int act[PM_ARRAY_MAX];	/* act flag for NPC's; player flag for PC's */

  int affected_by[AF_ARRAY_MAX];/* Bitvector for current affects	*/
  int bodyparts[AF_ARRAY_MAX];  /* Bitvector for current bodyparts      */
  int16_t saving_throw[3];	/* Saving throw				*/
  int16_t apply_saving_throw[3];	/* Saving throw bonuses			*/

  int powerattack;		/* Setting for power attack level	*/
  int combatexpertise;		/* Setting for Combat expertise level   */

  int64_t mana;
  int64_t max_mana;	/* Max mana for PC/NPC			*/
  int64_t hit;
  int64_t max_hit;	/* Max hit for PC/NPC			*/
  int64_t move;
  int64_t max_move;	/* Max move for PC/NPC			*/
  int64_t ki;
  int64_t max_ki;/* Max ki for PC/NPC			*/

  int armor;		/* Internally stored *10		*/
  int16_t shield_bonus;       /* Shield bonus for AC			*/
  int gold;			/* Money carried			*/
  int bank_gold;		/* Gold the char has in a bank account	*/
  int64_t exp;			/* The experience of the player		*/

  int accuracy;			/* Base hit accuracy			*/
  int accuracy_mod;		/* Any bonus or penalty to the accuracy	*/
  int damage_mod;		/* Any bonus or penalty to the damage	*/

  int16_t spellfail;		/* Total spell failure %                 */
  int16_t armorcheck;		/* Total armorcheck penalty with proficiency forgiveness */
  int16_t armorcheckall;	/* Total armorcheck penalty regardless of proficiency */

  /* All below added by Iovan for sure o.o */

  int64_t basepl;
  int64_t baseki;
  int64_t basest;
  int64_t charge;
  int64_t chargeto;
  int64_t barrier;

  char *clan;
  
  room_vnum droom;
  int choice;
  int sleeptime;
  int foodr;
  int altitude;
  int overf;
  int spam;

  room_vnum radar1;
  room_vnum radar2;
  room_vnum radar3;
  int ship;
  room_vnum shipr;
  time_t lastpl;
  time_t lboard[5];

  room_vnum listenroom;
  int crank;
  int kaioken;
  int absorbs;
  int boosts;
  int upgrade;
  time_t lastint;
  int majinize;
  short fury;
  short btime;
  int eavesdir;
  time_t deathtime;
  int rp;
  int64_t suppression;
  int64_t suppressed;
  struct char_data *drag;
  struct char_data *dragged;
  int trp;
  struct char_data *mindlink;
  int lasthit;
  int dcount;
  char *voice;                  /* PC's snet voice */
  int limbs[4];                 /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
  int aura;
  time_t rewtime;
  struct char_data *grappling;
  struct char_data *grappled;
  int grap;
  int genome[2];                /* Bio racial bonus, Genome */
  int combo;
  int lastattack;
  int combhits;
  int ping;
  int starphase;
  int mimic;
  int bonuses[MAX_BONUSES];
  int ccpoints;
  int negcount;
  int cooldown;
  int death_type;

  int64_t moltexp;
  int moltlevel;

  char *loguser;                /* What user was I last saved as?      */
  int arenawatch;
  int64_t majinizer;
  int speedboost;
  int skill_slots;
  int tail_growth;
  int rage_meter;
  char *feature;
  int transclass;
  int transcost[6];
  int armor_last;
  int forgeting;
  int forgetcount;
  int backstabcool;
  int con_cooldown;
  short stupidkiss;
  char *temp_prompt;

  int personality;
  int combine;
  int linker;
  int fishstate;
  int throws;

  struct char_data *defender;
  struct char_data *defending;

  int64_t lifeforce;
  int lifeperc;
  int gooptime;
  int blesslvl;
  struct char_data *poisonby;

  int mobcharge;
  int preference;
  int aggtimer;

  int lifebonus;
  int asb;
  int regen;
  int rbank;
  int con_sdcooldown;

  int limb_condition[4];
  
  char placeholder[2];
	
  char *rdisplay;
  
  short song;
  struct char_data *original;
  short clones;
  int relax_count;
 	int ingestLearned;
};

/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};

struct compr {
    int state; /* 0 - off. 1 - waiting for response. 2 - compress2 on */

    Bytef *buff_out;
    int total_out; /* size of input buffer */
    int size_out; /* size of data in output buffer */

    Bytef *buff_in;
    int total_in; /* size of input buffer */
    int size_in; /* size of data in input buffer */

    z_streamp stream;
};

struct descriptor_data {
   socklen_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   int8_t	bad_pws;	/* number of bad pw attemps this login	*/
   int8_t idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* backup string for modify-str system	*/
   size_t max_str;	        /* maximum size of string in modify-str	*/
   int32_t mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct oasis_olc_data *olc;   /* OLC info                            */
   struct compr *comp;                /* compression info */
   char *user;                   /* What user am I?                     */
   char *email;                  /* User Account Email.                 */
   char *pass;                   /* User Account Password.              */
   char *loadplay;               /* What character am I loading?        */
   int writenew;                 /* What slot am I writing to?          */
   int total;                    /* What Is My Total Character Limit?   */
   int rpp;                      /* What is my total RPP?               */
   char *tmp1;
   char *tmp2;
   char *tmp3;
   char *tmp4;
   char *tmp5;
   int level;
   char *newsbuf;
   /*---------------Player Level Object Editing Variables-------------------*/
   int obj_editval;
   int obj_editflag;
   char *obj_was;
   char *obj_name;
   char *obj_short;
   char *obj_long;
   int obj_type;
   int obj_weapon;
   struct obj_data *obj_point;
   /*---------------Ship Construction Editing Variables---------------------*/
   int shipmenu;
   int shipsize;
   char *ship_name;
   int shipextra[4];
   int shields;
   int armor;
   int drive;
   int shipweap;
   /*-----------------------------------------------------------------------*/
   int user_freed;
   int customfile;
   char *title;
   int rbank;
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};

/* used in the socials */
struct social_messg {
  int act_nr;
  char *command;               /* holds copy of activating command */
  char *sort_as;              /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
  int hide;                   /* ? */
  int min_victim_position;    /* Position of victim */
  int min_char_position;      /* Position of char */
  int min_level_char;          /* Minimum level of socialing char */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;
  char *others_found;
  char *vict_found;

  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found;
  char *others_body_found;
  char *vict_body_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;

  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found;
  char *others_obj_found;
};


struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
   mob_vnum	vnum;	/* virtual number of this mob/obj		*/
   int		number;	/* number of existing units of this mob/obj	*/
   SPECIAL(*func);

   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

struct guild_info_type {
  int pc_class;
  room_vnum guild_room;
  int direction;
};

/*
 * Config structs
 * 
 */
 
 /*
 * The game configuration structure used for configurating the game play 
 * variables.
 */
struct game_data {
  int pk_allowed;         /* Is player killing allowed? 	  */
  int pt_allowed;         /* Is player thieving allowed?	  */
  int level_can_shout;	  /* Level player must be to shout.	  */
  int holler_move_cost;	  /* Cost to holler in move points.	  */
  int tunnel_size;        /* Number of people allowed in a tunnel.*/
  int max_exp_gain;       /* Maximum experience gainable per kill.*/
  int max_exp_loss;       /* Maximum experience losable per death.*/
  int max_npc_corpse_time;/* Num tics before NPC corpses decompose*/
  int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
  int idle_void;          /* Num tics before PC sent to void(idle)*/
  int idle_rent_time;     /* Num tics before PC is autorented.	  */
  int idle_max_level;     /* Level of players immune to idle.     */
  int dts_are_dumps;      /* Should items in dt's be junked?	  */
  int load_into_inventory;/* Objects load in immortals inventory. */
  int track_through_doors;/* Track through doors while closed?    */
  int level_cap;          /* You cannot level to this level       */
  int stack_mobs;	  /* Turn mob stacking on                 */
  int stack_objs;	  /* Turn obj stacking on                 */
  int mob_fighting;       /* Allow mobs to attack other mobs.     */	 
  char *OK;               /* When player receives 'Okay.' text.	  */
  char *NOPERSON;         /* 'No-one by that name here.'	  */
  char *NOEFFECT;         /* 'Nothing seems to happen.'	          */
  int disp_closed_doors;  /* Display closed doors in autoexit?	  */
  int reroll_player;      /* Players can reroll stats on creation */
  int initial_points;	  /* Initial points pool size		  */
  int enable_compression; /* Enable MCCP2 stream compression      */
  int enable_languages;   /* Enable spoken languages              */
  int all_items_unique;   /* Treat all items as unique 		  */
  float exp_multiplier;     /* Experience gain  multiplier	  */
};



/*
 * The rent and crashsave options.
 */
struct crash_save_data {
  int free_rent;          /* Should the MUD allow rent for free?  */
  int max_obj_save;       /* Max items players can rent.          */
  int min_rent_cost;      /* surcharge on top of item costs.	  */
  int auto_save;          /* Does the game automatically save ppl?*/
  int autosave_time;      /* if auto_save=TRUE, how often?        */
  int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
  int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};


/*
 * The room numbers. 
 */
struct room_numbers {
  room_vnum mortal_start_room;	/* vnum of room that mortals enter at.  */
  room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
  room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1;    /* vnum of donation room #1.            */
  room_vnum donation_room_2;    /* vnum of donation room #2.            */
  room_vnum donation_room_3;    /* vnum of donation room #3.	        */
};


/*
 * The game operational constants.
 */
struct game_operation {
  uint16_t DFLT_PORT;      /* The default port to run the game.  */
  char *DFLT_IP;            /* Bind to all interfaces.		  */
  char *DFLT_DIR;           /* The default directory (lib).	  */
  char *LOGNAME;            /* The file to log messages to.	  */
  int max_playing;          /* Maximum number of players allowed. */
  int max_filesize;         /* Maximum size of misc files.	  */
  int max_bad_pws;          /* Maximum number of pword attempts.  */
  int siteok_everyone;	    /* Everyone from all sites are SITEOK.*/
  int nameserver_is_slow;   /* Is the nameserver slow or fast?	  */
  int use_new_socials;      /* Use new or old socials file ?      */
  int auto_save_olc;        /* Does OLC save to disk right away ? */
  char *MENU;               /* The MAIN MENU.			  */
  char *WELC_MESSG;	    /* The welcome message.		  */
  char *START_MESSG;        /* The start msg for new characters.  */
  int imc_enabled; /**< Is connection to IMC allowed ? */
};

/*
 * The Autowizard options.
 */
struct autowiz_data {
  int use_autowiz;        /* Use the autowiz feature?		*/
  int min_wizlist_lev;    /* Minimun level to show on wizlist.	*/
};

/* This is for the tick system.
 *
 */
 
struct tick_data {
  int pulse_violence;
  int pulse_mobile;
  int pulse_zone;
  int pulse_autosave;
  int pulse_idlepwd;
  int pulse_sanity;
  int pulse_usage;
  int pulse_timesave;
  int pulse_current;
};

/*
 * The character advancement (leveling) options.
 */
struct advance_data {
  int allow_multiclass; /* Allow advancement in multiple classes     */
  int allow_prestige;   /* Allow advancement in prestige classes     */
};

/*
 * The new character creation method options.
 */
struct creation_data {
  int method; /* What method to use for new character creation */
};

/*
 * The main configuration structure;
 */
struct config_data {
  char                   *CONFFILE;	/* config file path	 */
  struct game_data       play;		/* play related config   */
  struct crash_save_data csd;		/* rent and save related */
  struct room_numbers    room_nums;	/* room numbers          */
  struct game_operation  operation;	/* basic operation       */
  struct autowiz_data    autowiz;	/* autowiz related stuff */
  struct advance_data    advance;   /* char advancement stuff */
  struct tick_data       ticks;		/* game tick stuff 	 */
  struct creation_data	 creation;	/* char creation method	 */
};

/*
 * Data about character aging
 */
struct aging_data {
  int adult;		/* Adulthood */
  int classdice[3][2];	/* Dice info for starting age based on class age type */
  int middle;		/* Middle age */
  int old;		/* Old age */
  int venerable;	/* Venerable age */
  int maxdice[2];	/* For roll to determine natural death beyond venerable */
};

#ifdef MEMORY_DEBUG
#include "zmalloc.h"
#endif

#endif