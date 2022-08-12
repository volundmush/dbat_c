/*************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "act.movement.h"
#include "dg_comm.h"
#include "vehicles.h"
#include "oasis_copy.h"
#include "handler.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "fight.h"
#include "spells.h"
#include "oasis.h"
#include "guild.h"
#include "dg_scripts.h"
#include "local_limits.h"
#include "house.h"
#include "constants.h"

/* local functions */
static void handle_fall(struct char_data *ch);
static int check_swim(struct char_data *ch);
static void disp_locations(struct char_data *ch);
static int has_boat(struct char_data *ch);
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
static int has_key(struct char_data *ch, obj_vnum key);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, struct obj_data *obj);
static int has_flight(struct char_data *ch);
static int do_simple_enter(struct char_data *ch, struct obj_data *obj, int need_specials_check);
static int perform_enter_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check);
static int do_simple_leave(struct char_data *ch, struct obj_data *obj, int need_specials_check);
static int perform_leave_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check);

/* This handles teleporting players with instant transmission or skills like it. */
void handle_teleport(struct char_data *ch, struct char_data *tar, int location)
{
 int success = FALSE;

 if (location != 0) { /* Teleport to a particular room */
  char_from_room(ch);
  char_to_room(ch, real_room(location));
  success = TRUE;
 } else if (tar != NULL) { /* Teleport to a particular character */
  char_from_room(ch);
  char_to_room(ch, IN_ROOM(tar)); 
  success = TRUE;
 }

 if (success == TRUE) { /* We have made it. */
  act("@w$n@w appears in an instant out of nowhere!@n", TRUE, ch, 0, 0, TO_ROOM);
  if (DRAGGING(ch) && !IS_NPC(DRAGGING(ch))) {
   char_from_room(DRAGGING(ch));
   char_to_room(DRAGGING(ch), IN_ROOM(ch));
   act("@w$n@w appears in an instant out of nowhere being dragged by $N!@n", TRUE, DRAGGING(ch), 0, ch, TO_NOTVICT);
  } if (GRAPPLING(ch) && !IS_NPC(GRAPPLING(ch))) {
   char_from_room(GRAPPLING(ch));
   char_to_room(GRAPPLING(ch), IN_ROOM(ch));
   act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", TRUE, GRAPPLING(ch), 0, ch, TO_NOTVICT);
  } if (CARRYING(ch)) {
   char_from_room(CARRYING(ch));
   char_to_room(CARRYING(ch), IN_ROOM(ch));
   act("@w$n@w appears in an instant out of nowhere being carried by $N!@n", TRUE, CARRYING(ch), 0, ch, TO_NOTVICT);
  } if (GRAPPLED(ch) && !IS_NPC(GRAPPLED(ch))) {
   char_from_room(GRAPPLED(ch));
   char_to_room(GRAPPLED(ch), IN_ROOM(ch));
   act("@w$n@w appears in an instant out of nowhere being grappled by $N!@n", TRUE, GRAPPLED(ch), 0, ch, TO_NOTVICT);
  } if (DRAGGING(ch) && IS_NPC(DRAGGING(ch))) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   DRAGGED(DRAGGING(ch)) = NULL;
   DRAGGING(ch) = NULL;
  } if (GRAPPLING(ch) && IS_NPC(GRAPPLING(ch))) {
   GRAPTYPE(GRAPPLING(ch)) = -1;
   GRAPPLED(GRAPPLING(ch)) = NULL;
   GRAPPLING(ch) = NULL;
   GRAPTYPE(ch) = -1;
  } if (GRAPPLED(ch) && IS_NPC(GRAPPLED(ch))) {
   GRAPTYPE(GRAPPLED(ch)) = -1;
   GRAPPLING(GRAPPLED(ch)) = NULL;
   GRAPPLED(ch) = NULL;
   GRAPTYPE(ch) = -1;
  }
 } else { /* Wut... */
  log("ERROR: handle_teleport called without a destination.");
  return;
 }

}

/* Let's carry someone! Why not? - Iovan */
ACMD(do_carry)
{

 if (IS_NPC(ch))
  return;
 
 struct char_data *vict = NULL;
 char arg[MAX_INPUT_LENGTH];

 if (DRAGGING(ch)) {
  send_to_char(ch, "You are busy dragging someone at the moment.\r\n");
  return;
 }

 if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
 }

 if (CARRYING(ch)) { /* Already carrying someone. Put them down. Simple and clean. */
  if (GET_ALIGNMENT(ch) > 50) {
   carry_drop(ch, 0);
  } else {
   carry_drop(ch, 1);
  }
  return;
 } else { /* So not carrying already. */
  one_argument(argument, arg);

  if (!*arg) {
   send_to_char(ch, "You want to carry who?\r\n");
   return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
   send_to_char(ch, "That person isn't here.\r\n");
   return;
  }

  if (IS_NPC(vict)) {
   send_to_char(ch, "There's no point in carrying them.\r\n");
   return;
  }

  if (CARRIED_BY(vict) != NULL) {
   send_to_char(ch, "Someone is already carrying them!\r\n");
   return;
  }  

  if (GET_POS(vict) > POS_SLEEPING) {
   send_to_char(ch, "They are not unconcious.\r\n");
   return;
  }
   
  if (GET_PC_WEIGHT(vict) + IS_CARRYING_W(vict) > CAN_CARRY_W(ch)) {
   act("@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", TRUE, ch, 0, vict, TO_NOTVICT);
   WAIT_STATE(ch, PULSE_1SEC);
   return;
  } else { /* Let's carry that mofo! */
   act("@WYou pick up @C$N@W and put $M over your shoulder.@n", TRUE, ch, 0, vict, TO_CHAR);
   act("@C$n@W picks up $c$N@W and puts $M over $s shoulder.@n", TRUE, ch, 0, vict, TO_NOTVICT);
   if (SITS(vict)) {
    struct obj_data *chair = SITS(vict);
    SITTING(chair) = NULL;
    SITS(vict) = NULL;
   }
   CARRYING(ch) = vict;
   CARRIED_BY(vict) = ch;
   WAIT_STATE(ch, PULSE_1SEC);
   return;
  }

 } /* End new carry target. */
}

/* Handles dropping someone you are carrying. */
void carry_drop(struct char_data *ch, int type)
{


  struct char_data *vict = NULL;
  
  vict = CARRYING(ch);
  
  switch (type) {
   case 0: /* Awww we were gentle >.> */
    act("@WYou gently set @C$N@W down on the ground.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n @Wgently sets you down on the ground.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n @Wgently sets @c$N@W down on the ground.@n", TRUE, ch, 0, vict, TO_NOTVICT);
   break;
   case 1: /* We're not super nice. */
    act("@WYou set @C$N@W hastily onto the ground.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n @Wsets you hastily onto the ground.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n @Wsets @c$N@W hastily onto the ground.@n", TRUE, ch, 0, vict, TO_NOTVICT);
   break;
   case 2: /* Uh oh we dropped them from being hit! */
    act("@WYou have @C$N@W knocked out of your arms and onto the ground!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@WYou are knocked out of @C$n's@W arms and onto the ground!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n @Whas @c$N@W knocked out of $s arms and onto the ground!@n", TRUE, ch, 0, vict, TO_NOTVICT);
   break;
   case 3: /* Uh oh they are being extracted! */
    act("@WYou stop carrying @C$N@W for some reason.@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@C$n @Wstops carrying you for some reason.@n", TRUE, ch, 0, vict, TO_VICT);
    act("@C$n @Wstops carrying @c$N@W for some reason.@n", TRUE, ch, 0, vict, TO_NOTVICT);
   break;
  }
  CARRYING(ch) = NULL;
  CARRIED_BY(vict) = NULL;
}

int land_location(struct char_data *ch, char *arg)
{

 if (GET_ROOM_VNUM(IN_ROOM(ch)) == 50) { // Above Earth
  if (!strcasecmp(arg, "Nexus City")) {
   return (300);
  } else if (!strcasecmp(arg, "South Ocean")) {
   return (800);
  } else if (!strcasecmp(arg, "Nexus Field")) {
   return (1150);
  } else if (!strcasecmp(arg, "Cherry Blossom Mountain")) {
   return (1180);
  } else if (!strcasecmp(arg, "Sandy Desert")) {
   return (1287);
  } else if (!strcasecmp(arg, "Northern Plains")) {
   return (1428);
  } else if (!strcasecmp(arg, "Korin's Tower")) {
   return (1456);
  } else if (!strcasecmp(arg, "Kami's Lookout")) {
   return (1506);
  } else if (!strcasecmp(arg, "Shadow Forest")) {
   return (1636);
  } else if (!strcasecmp(arg, "Decrepit Area")) {
   return (1710);
  } else if (!strcasecmp(arg, "West City")) {
   return (19510);
  } else if (!strcasecmp(arg, "Hercule Beach")) {
   return (2141);
  } else if (!strcasecmp(arg, "Satan City")) {
   return (13020);
  } else {
   send_to_char(ch, "You don't know where that made up place is, but decided to land anyway.");
   return (300);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 51) { // Above Frigid
  if (!strcasecmp(arg, "Ice Crown City")) {
   return (4264);
  } else if (!strcasecmp(arg, "Ice Highway")) {
   return (4300);
  } else if (!strcasecmp(arg, "Topica Snowfield")) {
   return (4351);
  } else if (!strcasecmp(arg, "Glug's Volcano")) {
   return (4400);
  } else if (!strcasecmp(arg, "Platonic Sea")) {
   return (4600);
  } else if (!strcasecmp(arg, "Slave City")) {
   return (4800);
  } else if (!strcasecmp(arg, "Acturian Woods")) {
   return (5100);
  } else if (!strcasecmp(arg, "Desolate Demesne")) {
   return (5150);
  } else if (!strcasecmp(arg, "Chateau Ishran")) {
   return (5165);
  } else if (!strcasecmp(arg, "Wyrm Spine Mountain")) {
   return (5200);
  } else if (!strcasecmp(arg, "Cloud Ruler Temple")) {
   return (5500);
  } else if (!strcasecmp(arg, "Koltoan Mine")) {
   return (4944);
  } else {
   send_to_char(ch, "You don't know where that made up place is, but decided to land anyway.");
   return (4264);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 52) { // Above Konack
  if (!strcasecmp(arg, "Tiranoc City")) {
   return (8006);
  } else if (!strcasecmp(arg, "Great Oroist Temple")) {
   return (8300);
  } else if (!strcasecmp(arg, "Elzthuan Forest")) {
   return (8400);
  } else if (!strcasecmp(arg, "Mazori Farm")) {
   return (8447);
  } else if (!strcasecmp(arg, "Dres")) {
   return (8500);
  } else if (!strcasecmp(arg, "Colvian Farm")) {
   return (8600);
  } else if (!strcasecmp(arg, "St Alucia")) {
   return (8700);
  } else if (!strcasecmp(arg, "Meridius Memorial")) {
   return (8800);
  } else if (!strcasecmp(arg, "Desert of Illusion")) {
   return (8900);
  } else if (!strcasecmp(arg, "Plains of Confusion")) {
   return (8954);
  } else if (!strcasecmp(arg, "Turlon Fair")) {
   return (9200);
  } else if (!strcasecmp(arg, "Wetlands")) {
   return (9700);
  } else if (!strcasecmp(arg, "Kerberos")) {
   return (9855);
  } else if (!strcasecmp(arg, "Shaeras Mansion")) {
   return (9864);
  } else if (!strcasecmp(arg, "Slavinus Ravine")) {
   return (9900);
  } else if (!strcasecmp(arg, "Furian Citadel")) {
   return (9949);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (8006);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 53) { // Above Vegeta
  if (!strcasecmp(arg, "Vegetos City")) {
   return (2226);
  } else if (!strcasecmp(arg, "Blood Dunes")) {
   return (2600);
  } else if (!strcasecmp(arg, "Ancestral Mountains")) {
   return (2616);
  } else if (!strcasecmp(arg, "Destopa Swamp")) {
   return (2709);
  } else if (!strcasecmp(arg, "Pride forest")) {
   return (2800);
  } else if (!strcasecmp(arg, "Pride Tower")) {
   return (2899);
  } else if (!strcasecmp(arg, "Ruby Cave")) {
   return (2615);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (2226);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 54) { // Above Namek
  if (!strcasecmp(arg, "Senzu Village")) {
   return (11600);
  } else if (!strcasecmp(arg, "Guru's House")) {
   return (10182);
  } else if (!strcasecmp(arg, "Crystalline Cave")) {
   return (10474);
  } else if (!strcasecmp(arg, "Elder Village")) {
   return (13300);
  } else if (!strcasecmp(arg, "Frieza's Ship")) {
   return (10203);
  } else if (!strcasecmp(arg, "Kakureta Village")) {
   return (10922);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (11600);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 55) { // Above Aether
  if (!strcasecmp(arg, "Haven City")) {
   return (12010);
  } else if (!strcasecmp(arg, "Serenity Lake")) {
   return (12103);
  } else if (!strcasecmp(arg, "Kaiju Forest")) {
   return (12300);
  } else if (!strcasecmp(arg, "Ortusian Temple")) {
   return (12400);
  } else if (!strcasecmp(arg, "Silent Glade")) {
   return (12480);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (12010);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 56) { // Above Yardrat
  if (!strcasecmp(arg, "Yardra City")) {
   return (14008);
  } else if (!strcasecmp(arg, "Jade Forest")) {
   return (14100);
  } else if (!strcasecmp(arg, "Jade Cliffs")) {
   return (14200);
  } else if (!strcasecmp(arg, "Mount Valaria")) {
   return (14300);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (14008);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 198) { // Above Cerria
  if (!strcasecmp(arg, "Cerria Colony")) {
   return (17531);
  } else if (!strcasecmp(arg, "Crystalline Forest")) {
   return (7950);
  } else if (!strcasecmp(arg, "Fistarl Volcano")) {
   return (17420);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (17531);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 57) { // Above Zennith
  if (!strcasecmp(arg, "Utatlan City")) {
   return (3412);
  } else if (!strcasecmp(arg, "Zenith Jungle")) {
   return (3520);
  } else if (!strcasecmp(arg, "Ancient Castle")) {
   return (19600);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (3412);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 58) { // Above Kanassa
  if (!strcasecmp(arg, "Aquis City")) {
   return (14904);
  } else if (!strcasecmp(arg, "Yunkai Pirate Base")) {
   return (15655);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (14904);
  }
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 59) { // Above Arlia
  if (!strcasecmp(arg, "Janacre")) {
   return (16009);
  } else if (!strcasecmp(arg, "Arlian Wasteland")) {
   return (16544);
  } else if (!strcasecmp(arg, "Arlia Mine")) {
   return (16600);
  } else {
   send_to_char(ch, "you don't know where that made up place is, but decided to land anyway.");
   return (16009);
  }
 } else {
  send_to_char(ch, "You are not above a planet!\r\n");
  return (-1);
 }
}

/* This shows the player what locations the planet has to land at. */
static void disp_locations(struct char_data *ch)
{
 if (GET_ROOM_VNUM(IN_ROOM(ch)) == 50) { // Above Earth
  send_to_char(ch, "@D------------------[ @GEarth@D ]------------------@c\n");
  send_to_char(ch, "Nexus City, South Ocean, Nexus field, Cherry Blossom Mountain,\n");
  send_to_char(ch, "Sandy Desert, Northern Plains, Korin's Tower, Kami's Lookout,\n");
  send_to_char(ch, "Shadow Forest, Decrepit Area, West City, Hercule Beach, Satan City.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 51) { // Above Frigid
  send_to_char(ch, "@D------------------[ @CFrigid@D ]------------------@c\n");
  send_to_char(ch, "Ice Crown City, Ice Highway, Topica Snowfield, Glug's Volcano,\n");
  send_to_char(ch, "Platonic Sea, Slave City, Acturian Woods, Desolate Demesne,\n");
  send_to_char(ch, "Chateau Ishran, Wyrm Spine Mountain, Cloud Ruler Temple, Koltoan mine.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 52) { // Above Konack
  send_to_char(ch, "@D------------------[ @MKonack@D ]------------------@c\n");
  send_to_char(ch, "Great Oroist Temple, Elzthuan Forest, Mazori Farm, Dres,\n");
  send_to_char(ch, "Colvian Farm, St Alucia, Meridius Memorial, Desert of Illusion,\n");
  send_to_char(ch, "Plains of Confusion, Turlon Fair, Wetlands, Kerberos,\n");
  send_to_char(ch, "Shaeras Mansion, Slavinus Ravine, Furian Citadel.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 53) { // Above Vegeta
  send_to_char(ch, "@D------------------[ @YVegeta@D ]------------------@c\n");
  send_to_char(ch, "Vegetos City, Blood Dunes, Ancestral Mountains, Destopa Swamp,\n");
  send_to_char(ch, "Pride Forest, Pride tower, Ruby Cave.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 198) { // Above Cerria
  send_to_char(ch, "@D------------------[ @MCerria@D ]------------------@c\n");
  send_to_char(ch, "Cerria Colony, Fistarl Volcano, Crystalline Forest.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 54) { // Above Namek
  send_to_char(ch, "@D------------------[ @gNamek@D ]------------------@c\n");
  send_to_char(ch, "Senzu Village, Guru's House, Crystalline Cave, Elder Village,\n");
  send_to_char(ch, "Frieza's Ship, Kakureta Village.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 55) { // Above Aether
  send_to_char(ch, "@D------------------[ @BAether@D ]-----------------@c\n");
  send_to_char(ch, "Haven City, Serenity Lake, Kaiju Forest, Ortusian Temple,\n");
  send_to_char(ch, "Silent Glade.\n");
  send_to_char(ch, "@D--------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 56) { // Above Yardrat
  send_to_char(ch, "@D-----------------[ @mYardrat@D ]-----------------@c\n");
  send_to_char(ch, "Yardra City, Jade Forest, Jade Cliffs, Mount Valaria.\n");
  send_to_char(ch, "@D-------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 57) { // Above Zennith
  send_to_char(ch, "@D-----------------[ @CZennith@D ]-----------------@c\n");
  send_to_char(ch, "Utatlan City, Zenith Jungle, Ancient Castle.\n");
  send_to_char(ch, "@D-------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 58) { // Above Kanassa
  send_to_char(ch, "@D-----------------[ @CKanassa@D ]-----------------@c\n");
  send_to_char(ch, "Aquis City, Yunkai Pirate Base.\n");
  send_to_char(ch, "@D-------------------------------------------@n\n");
 } else if (GET_ROOM_VNUM(IN_ROOM(ch)) == 59) { // Above Arlia
  send_to_char(ch, "@D------------------[ @MArlia@D ]------------------@c\n");
  send_to_char(ch, "Janacre, Arlian Wasteland, Arlia Mine.\n");
  send_to_char(ch, "@D---------------------------------------------@n\n");
 } else {
  send_to_char(ch, "You are not above a planet!\r\n");
 }
}

ACMD(do_land)
{

 int above_planet = TRUE, inroom = GET_ROOM_VNUM(IN_ROOM(ch));
 skip_spaces(&argument);

 if (inroom != 50 && inroom != 198 && inroom != 51 && inroom != 52 && inroom != 53 && inroom != 54 && inroom != 55 && inroom != 56 && inroom != 57 && inroom != 58 && inroom != 59) {
  above_planet = FALSE;
 }

 if (!*argument) {
  if (above_planet == TRUE) {
   send_to_char(ch, "Land where?\n");
   disp_locations(ch);
   return;
  } else {
   send_to_char(ch, "You are not even in the lower atmosphere of a planet!\r\n");
   return;
  }
 }

 int landing = land_location(ch, argument);

 if (landing != -1) {
  int was_in = GET_ROOM_VNUM(IN_ROOM(ch));
  send_to_char(ch, "You descend through the upper atmosphere, and coming down through the clouds you land quickly on the ground below.\r\n");
  char_from_room(ch);
  char_to_room(ch, real_room(landing));
  char *blah = sense_location(ch);
  char sendback[MAX_INPUT_LENGTH];
  char_from_room(ch);
  char_to_room(ch, real_room(was_in));
  sprintf(sendback, "@C$n@Y flies down through the atmosphere toward @G%s@Y!@n", blah);
  act(sendback, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(landing));
  int zone = 0;
  if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
   fly_zone(zone, "can be seen landing from space nearby!@n\r\n", ch);
  }
  send_to_sense(1, "landing on the planet", ch);
  send_to_scouter("A powerlevel signal has been detected landing on the planet", ch, 0, 1);
  act("$n comes down from high above in the sky and quickly lands on the ground.", TRUE, ch, 0, 0, TO_ROOM);
  return;
 }
}


/* simple function to determine if char can walk on water */
static int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

/*
  if (ROOM_IDENTITY(IN_ROOM(ch)) == DEAD_SEA)
    return (1);
*/

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE) || GET_ADMLEVEL(ch) > 4)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  return (0);
}

/* simple function to determine if char can fly */
static int has_flight(struct char_data *ch)
{
  struct obj_data *obj;

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (AFF_FLAGGED(ch, AFF_FLYING) && GET_MANA(ch) >= (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) && !IS_ANDROID(ch) && !IS_NPC(ch)) {
    return (1);
  }
  if (AFF_FLAGGED(ch, AFF_FLYING) && GET_MANA(ch) < (GET_LEVEL(ch) + (GET_MAX_MANA(ch) / (GET_LEVEL(ch) * 30))) && !IS_ANDROID(ch) && !IS_NPC(ch)) {
    act("@WYou crash to the ground, too tired to fly anymore!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n@W crashes to the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    handle_fall(ch);
    return (0);
  }
  if (AFF_FLAGGED(ch, AFF_FLYING) && IS_ANDROID(ch)) {
    return (1);
  }
  if (AFF_FLAGGED(ch, AFF_FLYING) && IS_NPC(ch)) {
    return (1);
  }

  /* non-wearable flying items in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1); 

  /* anything worn as wings will do */
  return (0);
}
  
/* simple function to determine if char can breathe non-o2 */
int has_o2(struct char_data *ch)
{
  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERBREATH))
    return (1);
  
  if (IS_KANASSAN(ch) || IS_ANDROID(ch) || IS_ICER(ch) || IS_MAJIN(ch))
    return (1);

  return (0);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
  char buf2[MAX_STRING_LENGTH];
  char buf3[MAX_STRING_LENGTH];
  room_rnum was_in = IN_ROOM(ch);
  int need_movement;
  struct room_data *rm;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, throwaway))
    return (0);

  /* blocked by a leave trigger ? */
  if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  int willfall = FALSE;
  /* if this room or the one we're going to needs flight, check for it */
  if ((SECT(IN_ROOM(ch)) == SECT_FLYING) || (SECT(EXIT(ch, dir)->to_room) == SECT_FLYING)) {
    if (!has_flight(ch)) {
     if (dir != 4) {
      willfall = TRUE;
     } else {
      send_to_char(ch, "You need to fly to go there!\r\n");
      return (0);
     }
    }
  }

  if (((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) || (SECT(EXIT(ch, dir)->to_room) == SECT_WATER_NOSWIM)) && IS_HUMANOID(ch)) {
    if (IS_KANASSAN(ch) && !has_flight(ch)) {
      act("@CYou swim swiftly.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C swims swiftly.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (IS_ICER(ch) && !has_flight(ch)) {
      act("@CYou swim swiftly.@n", TRUE, ch, 0, 0, TO_CHAR);
      act("@c$n@C swims swiftly.@n", TRUE, ch, 0, 0, TO_ROOM);
    } else if (!IS_KANASSAN(ch) && !IS_ICER(ch) && !has_flight(ch)) {
      if (!check_swim(ch)) {
       return (0);
      } else {
       act("@CYou swim through the cold water.@n", TRUE, ch, 0, 0, TO_CHAR);
       act("@c$n@C swim through the cold water.@n", TRUE, ch, 0, 0, TO_ROOM);
       WAIT_STATE(ch, PULSE_1SEC);
     }
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) {
   if (!IS_ANDROID(ch)) {
    if (!check_swim(ch)) {
     return (0);
    }
   }
  }

  if (ROOM_EFFECT(EXIT(ch, dir)->to_room) == 6 && !IS_HUMANOID(ch) && IS_NPC(ch)) {
   return (0);
  }

  if (IS_NPC(ch) && ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_NOMOB) && !ch->master) {
    return (0);
  }

  if (SUNKEN(IN_ROOM(ch)) ||
      SUNKEN(EXIT(ch, dir)->to_room)) {
    if (!has_o2(ch) && ((group_bonus(ch, 2) != 10 && GET_MANA(ch) < GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 && GET_MANA(ch) < GET_MAX_MANA(ch) / 800))) {
      if (GET_HIT(ch) >= GET_MAX_HIT(ch) / 20) {
       send_to_char(ch, "@RYou struggle to breath!@n\r\n");
       GET_HIT(ch) -= GET_MAX_HIT(ch) / 20;
      }
      if (GET_HIT(ch) < GET_MAX_HIT(ch) / 20) {
       send_to_char(ch, "@rYou drown!@n\r\n");
       die(ch, NULL);
       return (0);
      }
    }
    if (!has_o2(ch) && ((group_bonus(ch, 2) != 10 && GET_MANA(ch) >= GET_MAX_MANA(ch) / 200) || (group_bonus(ch, 2) == 10 && GET_MANA(ch) >= GET_MAX_MANA(ch) / 800))) {
      send_to_char(ch, "@CYou hold your breath!@n\r\n");
      if (group_bonus(ch, 2) == 10) {
       GET_MANA(ch) -= GET_MAX_MANA(ch) / 800;
      } else {
       GET_MANA(ch) -= GET_MAX_MANA(ch) / 200;
      }
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  if (ROOM_GRAVITY(IN_ROOM(ch)) > 10) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  if (GET_LEVEL(ch) <= 1) {
   need_movement = 0;
  }
  /* Stealth increases your move cost, less if you are good at it */
  if (AFF_FLAGGED(ch, AFF_HIDE))
    need_movement *= ((roll_skill(ch, SKILL_HIDE) > 15) ? 2 : 4);

  if (AFF_FLAGGED(ch, AFF_SNEAK))
    need_movement *= ((roll_skill(ch, SKILL_MOVE_SILENTLY) > 15) ? 1.2 : 2);

  int flight_cost = 0;

  if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch)) {
   if (!GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS)) {
    flight_cost = GET_MAX_MANA(ch) / 100;
   } else if (GET_SKILL(ch, SKILL_CONCENTRATION) && !GET_SKILL(ch, SKILL_FOCUS)) {
    flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_CONCENTRATION) * 2);
   } else if (!GET_SKILL(ch, SKILL_CONCENTRATION) && GET_SKILL(ch, SKILL_FOCUS)) {
    flight_cost = GET_MAX_MANA(ch) / (GET_SKILL(ch, SKILL_FOCUS) * 3);
   } else {
    flight_cost = GET_MAX_MANA(ch) / ((GET_SKILL(ch, SKILL_CONCENTRATION) * 2) + (GET_SKILL(ch, SKILL_FOCUS) * 3));
   }
  }

  if (AFF_FLAGGED(ch, AFF_FLYING) && (GET_MANA(ch) < flight_cost) && !IS_ANDROID(ch)) {
    GET_MANA(ch) = 0;
    act("@WYou crash to the ground, too tired to fly anymore!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n@W crashes to the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
  } else if (AFF_FLAGGED(ch, AFF_FLYING) && !IS_ANDROID(ch)) {
    GET_MANA(ch) -= flight_cost;
  }

  if (GET_MOVE(ch) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
    if (need_specials_check && ch->master) {
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    } else {
      send_to_char(ch, "You are too exhausted.\r\n");
    }

    return (0);
  }

  /* Check if the character needs a skill check to go that way. */
  if (EXIT(ch, dir)->dcskill != 0) {
    if (EXIT(ch, dir)->dcmove > roll_skill(ch, EXIT(ch, dir)->dcskill)) {
      send_to_char(ch, "Your skill in %s isn't enough to move that way!\r\n", spell_info[EXIT(ch, dir)->dcskill].name);
      /* A failed skill check still spends the movement points! */
      if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FLYING))
        GET_MOVE(ch) -= need_movement;
      return (0);
    } else {
      send_to_char(ch, "Your skill in %s aids in your movement.\r\n", spell_info[EXIT(ch, dir)->dcskill].name);
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(EXIT(ch, dir)->to_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL) &&
      (num_pc_in_room(&(world[EXIT(ch, dir)->to_room])) >= CONFIG_TUNNEL_SIZE)) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM) &&
	GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

  /******* Zone flag checks *******/

  rm = &world[EXIT(ch, dir)->to_room];

  if (!IS_NPC(ch) && (GET_ADMLEVEL(ch) < ADMLVL_IMMORT) &&
    (GET_LEVEL(ch) < ZONE_MINLVL(rm->zone)) && (ZONE_MINLVL(rm->zone) > 0)) {
    send_to_char(ch, "Sorry, you are too low a level to enter this zone.\r\n");
    return(0);
  }

  if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && (GET_LEVEL(ch) > ZONE_MAXLVL(rm->zone)) &&
    (ZONE_MAXLVL(rm->zone) > 0)) {
    send_to_char(ch, "Sorry, you are too high a level to enter this zone.\r\n");
    return(0);
  }

  if ((GET_ADMLEVEL(ch) < ADMLVL_IMMORT) && ZONE_FLAGGED(rm->zone, ZONE_CLOSED)){
     send_to_char(ch, "This zone is currently closed to mortals.\r\n");
     return (0);
  }

  if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GRGOD)
       && ZONE_FLAGGED(rm->zone, ZONE_NOIMMORT)){
       send_to_char(ch, "This zone is closed to all.\r\n");
       return (0);
  }

  /* No low level immortal scouting */
  if ((GET_ADMLEVEL(ch) >= ADMLVL_IMMORT && GET_ADMLEVEL(ch) < ADMLVL_GOD) &&
       !can_edit_zone(ch, rm->zone) && ZONE_FLAGGED(rm->zone, ZONE_QUEST)) {
       send_to_char(ch, "This is a Quest zone.\r\n");
       return (0);
  }

  /* Now we know we're allowed to go into the room. */
  if (!ADM_FLAGGED(ch, ADM_WALKANYWHERE) && !IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_FLYING)) {
    GET_MOVE(ch) -= need_movement;
  }

  if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch)) {
  sprintf(buf2, "$n sneaks %s.", dirs[dir]);
  if (GET_SKILL(ch, SKILL_MOVE_SILENTLY)) {
   improve_skill(ch, SKILL_MOVE_SILENTLY, 0);
  } else if (slot_count(ch) + 1 > GET_SLOTS(ch)) {
   send_to_char(ch, "@RYour skill slots are full. You can not learn Move Silently.\r\n");
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
  } else {
   send_to_char(ch, "@GYou learn the very basics of moving silently.@n\r\n");
   SET_SKILL(ch, SKILL_MOVE_SILENTLY, rand_number(5, 10));
   act(buf2, TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
    if (GET_DEX(ch) < rand_number(1, 30)) {
     WAIT_STATE(ch, PULSE_1SEC);
    }
   }
  }

  if (!AFF_FLAGGED(ch, AFF_SNEAK) && !AFF_FLAGGED(ch, AFF_FLYING)) {
  sprintf(buf2, "$n leaves %s.", dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }
  if (!AFF_FLAGGED(ch, AFF_SNEAK) && AFF_FLAGGED(ch, AFF_FLYING)) {
  sprintf(buf2, "$n flies %s.", dirs[dir]);
  act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  was_in = IN_ROOM(ch);
  if (DRAGGING(ch)) {
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  SET_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
  char_from_room(ch);  
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);
  if((world[IN_ROOM(ch)].zone != world[was_in].zone) && !IS_NPC(ch) && !IS_ANDROID(ch)) {
   send_to_sense(0, "You sense someone", ch);
   sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range@n.", add_commas(GET_HIT(ch)));
   send_to_scouter(buf3, ch, 0, 0);
  }
  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) || !enter_wtrigger(&world[IN_ROOM(ch)], ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
    return 0;
  }

    snprintf(buf2, sizeof(buf2), "%s%s",
            ((dir == UP) || (dir == DOWN) ? "" : "the "),
            ( dir == UP  ? "below" :
             (dir == DOWN) ? "above" : dirs[rev_dir[dir]]));
  act("$n arrives from $T.", TRUE, ch, 0, buf2, TO_ROOM | TO_SNEAKRESIST);
  if (FIGHTING(ch)) {
   if (SECT(world[was_in].dir_option[dir]->to_room) != SECT_FLYING && SECT(world[was_in].dir_option[dir]->to_room) != SECT_WATER_NOSWIM && ROOM_EFFECT(world[was_in].dir_option[dir]->to_room) == 0) {
    roll_pursue(FIGHTING(ch), ch);
   }
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PURSUIT);
  }
  if (DRAGGING(ch)) {
   act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   char_from_room(DRAGGING(ch));
   char_to_room(DRAGGING(ch), IN_ROOM(ch));
   if (SITS(DRAGGING(ch))) {
    obj_from_room(SITS(DRAGGING(ch)));
    obj_to_room(SITS(DRAGGING(ch)), IN_ROOM(ch));
   }
   if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) && !AFF_FLAGGED(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(DRAGGING(ch), "You feel your sleeping body being moved.\r\n");
    if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
     set_fighting(DRAGGING(ch), ch);
    }
   }
  }
  if (CARRYING(ch)) {
   act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
   char_from_room(CARRYING(ch));
   char_to_room(CARRYING(ch), IN_ROOM(ch));
   if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) && !AFF_FLAGGED(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(CARRYING(ch), "You feel your sleeping body being moved.\r\n");
   }
  }

  if (ch->desc != NULL) {
    look_at_room(IN_ROOM(ch), ch, 0);
    if (AFF_FLAGGED(ch, AFF_SNEAK) && !IS_NPC(ch) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) < rand_number(1, 101)) {
     send_to_char(ch, "@wYou make a noise as you arrive and are no longer sneaking!@n\r\n");
     act("@c$n@w makes a noise revealing $s sneaking!@n", TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
     reveal_hiding(ch, 0);
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SNEAK);
    }
  }

  if (ROOM_EFFECT(IN_ROOM(ch)) == 6 || ROOM_EFFECT(was_in) == 6) {
    if (!IS_DEMON(ch) && !AFF_FLAGGED(ch, AFF_FLYING) && group_bonus(ch, 2) != 14) {
       act("@rYour legs are burned by the lava!@n", TRUE, ch, 0, 0, TO_CHAR);
       act("@R$n@r's legs are burned by the lava!@n", TRUE, ch, 0, 0, TO_ROOM);
       if (IS_NPC(ch) && IS_HUMANOID(ch) && rand_number(1, 2) == 2) {
        do_fly(ch, 0, 0, 0);
       }
       if (GET_SUPP(ch) >= GET_MAX_HIT(ch) / 20) {
        GET_SUPP(ch) -= GET_MAX_HIT(ch) / 20;
       } else {
       GET_SUPPRESS(ch) = 0;
       GET_HIT(ch) -= (GET_MAX_HIT(ch) / 20) - GET_SUPP(ch);
       GET_SUPP(ch) = 0;
       if (GET_HIT(ch) < 0) {
        act("@rYou have burned to death!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@R$n@r has burned to death!@n", TRUE, ch, 0, 0, TO_ROOM);
        die(ch, NULL);
       }
      }
    }
    if (DRAGGING(ch) && !IS_DEMON(DRAGGING(ch))) {
     act("@R$N@r gets burned!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
     act("@R$N@r gets burned!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
     GET_HIT(DRAGGING(ch)) -= GET_MAX_HIT(DRAGGING(ch)) / 20;
       if (GET_HIT(DRAGGING(ch)) < 0) {
        act("@rYou have burned to death!@n", TRUE, DRAGGING(ch), 0, 0, TO_CHAR);
        act("@R$n@r has burned to death!@n", TRUE, DRAGGING(ch), 0, 0, TO_ROOM);
        die(DRAGGING(ch), NULL);
       }
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_TIMED_DT) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE))
     timed_dt(NULL);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return (0);
  }

  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    look_at_room(IN_ROOM(ch), ch, 0);
  } else greet_memory_mtrigger(ch);
  if (willfall == TRUE) {
   handle_fall(ch);
   if (DRAGGING(ch)) {
    handle_fall(DRAGGING(ch));
   }
  }
  return (1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
   send_to_char(ch, "You are grappling with someone!\r\n");
   return (0);
  }

  if (ABSORBING(ch) || ABSORBBY(ch)) {
   send_to_char(ch, "You are struggling with someone!\r\n");
   return (0);
  }

  if (!AFF_FLAGGED(ch, AFF_SNEAK) || (AFF_FLAGGED(ch, AFF_SNEAK) && GET_SKILL(ch, SKILL_MOVE_SILENTLY) < axion_dice(0))) {
   reveal_hiding(ch, 0);
  }

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return (0);
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE || (EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) && (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) {
    if (EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else
      send_to_char(ch, "It seems to be closed.\r\n");
  } else if (GET_ROOM_VNUM(EXIT(ch, dir)->to_room) == 0 || GET_ROOM_VNUM(EXIT(ch, dir)->to_room) == 1) {
     send_to_char(ch, "Report this direction, it is illegal.\r\n");
  } else {

    struct obj_data *wall;
    for (wall = world[IN_ROOM(ch)].contents; wall;wall=wall->next_content) {
     if(GET_OBJ_VNUM(wall) == 79) {
      if (GET_OBJ_COST(wall) == dir) {
       send_to_char(ch, "That direction has a glacial wall blocking it.\r\n");
       return (0);
      }
     }
    }

    if (!ch->followers)
      return (do_simple_move(ch, dir, need_specials_check));

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((IN_ROOM(k->follower) == was_in) &&
	  (GET_POS(k->follower) >= POS_STANDING) &&
           (!AFF_FLAGGED(ch, AFF_ZANZOKEN) ||
            (AFF_FLAGGED(ch, AFF_GROUP) && AFF_FLAGGED(k->follower, AFF_GROUP)))) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
      else if ((IN_ROOM(k->follower) == was_in) &&
          (GET_POS(k->follower) >= POS_STANDING) &&
           (AFF_FLAGGED(ch, AFF_ZANZOKEN) && AFF_FLAGGED(k->follower, AFF_ZANZOKEN)) &&
            (!AFF_FLAGGED(ch, AFF_GROUP) || !AFF_FLAGGED(k->follower, AFF_GROUP))) {
        act("$N tries to zanzoken and escape, but your zanzoken matches $S!\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
        act("$N tries to zanzoken and escape, but $n's zanzoken matches $S!\r\n", FALSE, k->follower, 0, ch, TO_NOTVICT);
        act("You zanzoken to try and escape, but $n's zanzoken matches yours!\r\n", FALSE, k->follower, 0, ch, TO_VICT);
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
        REMOVE_BIT_AR(AFF_FLAGS(k->follower), AFF_ZANZOKEN);
        perform_move(k->follower, dir, 1);
      }
      else if ((IN_ROOM(k->follower) == was_in) &&
          (GET_POS(k->follower) >= POS_STANDING) &&
           (AFF_FLAGGED(ch, AFF_ZANZOKEN) && !AFF_FLAGGED(k->follower, AFF_ZANZOKEN))) {
        act("You try to follow $N, but $E disappears in a flash of movement!\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
        act("$n tries to follow $N, but $E disappears in a flash of movement!\r\n", FALSE, k->follower, 0, ch, TO_NOTVICT);
        act("$n tries to follow you, but you manage to zanzoken away!\r\n", FALSE, k->follower, 0, ch, TO_VICT);
        REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_ZANZOKEN);
      }
    }
    return (1);
  }
  return (0);
}

ACMD(do_move)
{
  if (IS_NPC(ch)) {
   perform_move(ch, subcmd - 1, 0);
   return;
  }
  if (PLR_FLAGGED(ch, PLR_SELFD)) {
   send_to_char(ch, "You are preparing to blow up!\r\n");
   return;
  }
  if (AFF_FLAGGED(ch, AFF_LIQUEFIED)) {
   send_to_char(ch, "You are liquefied right now!\r\n");
   return;
  }
  if (GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .51) {
   send_to_char(ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
   return;
  }
  else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .5 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .51) && GET_SKILL(ch, SKILL_CONCENTRATION) < 100) {
   send_to_char(ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
   return;
  }
  else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .4 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .5) && GET_SKILL(ch, SKILL_CONCENTRATION) < 80) {
   send_to_char(ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
   return;
  }
  else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .3 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .4) && GET_SKILL(ch,SKILL_CONCENTRATION) < 70) {
   send_to_char(ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
   return;
  }
  else if ((GET_CHARGE(ch) >= GET_MAX_MANA(ch) * .2 && GET_CHARGE(ch) < GET_MAX_MANA(ch) * .3) && GET_SKILL(ch, SKILL_CONCENTRATION) < 60) {
   send_to_char(ch, "You have too much ki charged. You can't concentrate on keeping it charged while also traveling.\r\n");
   return;
  }

  if (GET_COND(ch, DRUNK) > 4 && (rand_number(1, 9) + GET_COND(ch, DRUNK)) >= rand_number(14, 20)) {
   send_to_char(ch, "You wobble around and then fall on your ass.\r\n");
   act("@C$n@W wobbles around before falling on $s ass@n.", TRUE, ch, 0, 0, TO_ROOM);
   GET_POS(ch) = POS_SITTING;
   return;
  }

  if (FIGHTING(ch) && !IS_NPC(ch)) {
   char blah[MAX_INPUT_LENGTH];
   sprintf(blah, "%s", dirs[subcmd - 1]);
   do_flee(ch, blah, 0, 0);
   return;
  }

  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   struct obj_data *vehicle = NULL, *controls = NULL;
   int noship = FALSE;
   if (!(controls = find_control(ch)) && GET_ADMLEVEL(ch) < 1) {
    noship = TRUE;
   }
   else if (!(vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(controls, 0))) ) {
    noship = TRUE;
   }
   if (noship == TRUE) {
    send_to_char(ch, "Your ship controls are not here or your ship was not found, report to Iovan!\r\n");
    return;
   } else if (controls != NULL && vehicle != NULL) {
    if (GET_FUEL(controls) <= 0) {
     send_to_char(ch, "The ship is out of fuel!\r\n");
     return;
    }
    drive_in_direction(ch, vehicle, subcmd - 1);  
    if (GET_OBJ_VAL(controls, 1) == 1) {
     WAIT_STATE(ch, PULSE_2SEC);
    }
    else if (GET_OBJ_VAL(controls, 1) == 2) {
     WAIT_STATE(ch, PULSE_1SEC);
    }
    controls = NULL;
    vehicle = NULL;
    return;
   }
   return;
  }
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }
  if (!IS_NPC(ch)) {
     int fail = FALSE;
     struct obj_data *obj, *next_obj;
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (KICHARGE(obj) > 0 && USER(obj) == ch) {
       fail = TRUE;
      }
    }
    if (fail == TRUE) {
     send_to_char(ch, "You are too busy controlling your attack!\r\n");
     return;
    }
  }

  if (!IS_NPC(ch) && GET_LIMBCOND(ch, 1) <= 0 && GET_LIMBCOND(ch, 2) <= 0 && GET_LIMBCOND(ch, 3) <= 0 && GET_LIMBCOND(ch, 4) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
   send_to_char(ch, "Unless you fly, you can't get far with no limbs.\r\n");
   return;
  }
  if (GRAPPLING(ch) || GRAPPLED(ch)) {
   send_to_char(ch, "You are grappling with someone!\r\n");
   return;
  }
  if (ABSORBING(ch)) {
   send_to_char(ch, "You are busy absorbing from %s!\r\n", GET_NAME(ABSORBING(ch)));
   return;
  }
  if (ABSORBBY(ch)) {
   if (axion_dice(0) < GET_SKILL(ABSORBBY(ch), SKILL_ABSORB)) {
   send_to_char(ch, "You are being held by %s, they are absorbing you!\r\n", GET_NAME(ABSORBBY(ch)));
   send_to_char(ABSORBBY(ch), "%s struggles in your grasp!\r\n", GET_NAME(ch));
   WAIT_STATE(ch, PULSE_2SEC);
   return;
   }
   else {
    act("@c$N@W manages to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_NOTVICT);
    act("@WYou manage to break loose of @C$n's@W hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_VICT);
    act("@c$N@W manages to break loose of your hold!@n", TRUE, ABSORBBY(ch), 0, ch, TO_CHAR);
    ABSORBING(ABSORBBY(ch)) = NULL;
    ABSORBBY(ch) = NULL;
   }
  }
   if (!block_calc(ch)) {
    return;
   }
   if (GET_EAVESDROP(ch) > 0) {
    send_to_char(ch, "You stop eavesdropping.\r\n");
    GET_EAVESDROP(ch) = real_room(0);
   } 
 if (!IS_NPC(ch)) {
  if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
     REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
     ARENA_IDNUM(ch) = -1;
  } if (GET_ROOM_VNUM(IN_ROOM(ch)) != NOWHERE && GET_ROOM_VNUM(IN_ROOM(ch)) != 0 && GET_ROOM_VNUM(IN_ROOM(ch)) != 1) {
     GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && GET_MAX_HIT(ch) <= 10000 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_1SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 20 && GET_MAX_HIT(ch) <= 30000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_2SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 30 && GET_MAX_HIT(ch) <= 100000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 40 && GET_MAX_HIT(ch) <= 200000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 50 && GET_MAX_HIT(ch) <= 300000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 100 && GET_MAX_HIT(ch) <= 500000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 200 && GET_MAX_HIT(ch) <= 1000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 300 && GET_MAX_HIT(ch) <= 8000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 400 && GET_MAX_HIT(ch) <= 15000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_3SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 500 && GET_MAX_HIT(ch) <= 25000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_4SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 1000 && GET_MAX_HIT(ch) <= 35000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_5SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 5000 && GET_MAX_HIT(ch) <= 100000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_5SEC);
  } if (ROOM_GRAVITY(IN_ROOM(ch)) == 10000 && GET_MAX_HIT(ch) <= 200000000) {
     send_to_char(ch, "The gravity slows you down some.\r\n");
     WAIT_STATE(ch, PULSE_5SEC);
  } if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE) && GET_ADMLEVEL(ch) < 1) {
     send_to_char(ch, "You struggle to cross the vast distance.\r\n");
     WAIT_STATE(ch, PULSE_6SEC);
  } else if ((GET_LIMBCOND(ch, 3) <= 0 && GET_LIMBCOND(ch, 4) <= 0) && GET_LIMBCOND(ch, 1) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
   act("@wYou slowly pull yourself along with your arm...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w slowly pulls $mself along with one arm...@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_LIMBCOND(ch, 2) < 50) {
    send_to_char(ch, "@RYour left arm is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 2) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 1) <= 0) {
     act("@RYour left arm falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R left arm falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
   }
   WAIT_STATE(ch, PULSE_5SEC);
  } else if ((GET_LIMBCOND(ch, 3) <= 0 && GET_LIMBCOND(ch, 4) <= 0) && GET_LIMBCOND(ch, 2) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
   act("@wYou slowly pull yourself along with your arm...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w slowly pulls $mself along with one arm...@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_LIMBCOND(ch, 1) < 50) {
    send_to_char(ch, "@RYour right arm is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 1) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 1) <= 0) {
     act("@RYour right arm falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R right arm falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
   }
   WAIT_STATE(ch, PULSE_5SEC);
  } else if ((GET_LIMBCOND(ch, 3) <= 0 && GET_LIMBCOND(ch, 4) <= 0) && !AFF_FLAGGED(ch, AFF_FLYING)) {
   act("@wYou slowly pull yourself along with your arms...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w slowly pulls $mself along with one arms...@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_LIMBCOND(ch, 2) < 50) {
    send_to_char(ch, "@RYour left arm is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 2) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 2) <= 0) {
     act("@RYour left arm falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R left arm falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
   }
   if (GET_LIMBCOND(ch, 1) < 50) {
    send_to_char(ch, "@RYour right arm is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 1) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 1) <= 0) {
     act("@RYour right arm falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R right arm falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
   }
   WAIT_STATE(ch, PULSE_3SEC);
  } else if (GET_LIMBCOND(ch, 3) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
   act("@wYou hop on one leg...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w hops on one leg...@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_LIMBCOND(ch, 4) < 50) {
    send_to_char(ch, "@RYour left leg is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 4) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 4) <= 0) {
     act("@RYour left leg falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R left leg falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }     
   }
   WAIT_STATE(ch, PULSE_2SEC);
  } else if (GET_LIMBCOND(ch, 4) <= 0 && !AFF_FLAGGED(ch, AFF_FLYING)) {
   act("@wYou hop on one leg...@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w hops on one leg...@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_LIMBCOND(ch, 3) < 50) {
    send_to_char(ch, "@RYour right leg is damaged by the forced use!@n\r\n");
    GET_LIMBCOND(ch, 3) -= rand_number(1, 5);
    if (GET_LIMBCOND(ch, 3) <= 0) {
     act("@RYour right leg falls apart!@n", TRUE, ch, 0, 0, TO_CHAR);
     act("@r$n's@R right leg falls apart!@n", TRUE, ch, 0, 0, TO_ROOM);
    }
   }
   WAIT_STATE(ch, PULSE_2SEC);
  } else if (GET_POS(ch) == POS_RESTING) {
   act("@wYou crawl on your hands and knees.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w crawls on $s hands and knees.@n", TRUE, ch, 0, 0, TO_ROOM);
   if (SITS(ch)) {
    struct obj_data *chair = SITS(ch);
    SITTING(chair) = NULL;
    SITS(ch) = NULL;
   }
   WAIT_STATE(ch, PULSE_3SEC);
  } else if (GET_POS(ch) == POS_SITTING) {
   act("@wYou shuffle on your hands and knees.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@w shuffles on $s hands and knees.@n", TRUE, ch, 0, 0, TO_ROOM);
   if (SITS(ch)) {
    struct obj_data *chair = SITS(ch);
    SITTING(chair) = NULL;
    SITS(ch) = NULL;
   }
   WAIT_STATE(ch, PULSE_2SEC);
  } else if (GET_POS(ch) < POS_RESTING) {
    send_to_char(ch, "You are in no condition to move! Try standing...\r\n");
    return;
   }
  }
  perform_move(ch, subcmd - 1, 0);
  if (GET_RDISPLAY(ch)) {
     if (GET_RDISPLAY(ch) != "Empty") {
		GET_RDISPLAY(ch) = "Empty";
	}
  }
}

static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) < 0 &&
        (door = search_block(dir, abbr_dirs, FALSE)) < 0) {	/* Partial Match */
      send_to_char(ch, "That's not a direction.\r\n");
      return (-1);
    }
    if (EXIT(ch, door)) {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword) {
	if (is_name(type, EXIT(ch, door)->keyword))
	  return (door);
	else {
	  send_to_char(ch, "I see no %s there.\r\n", type);
	  return (-1);
        }
      } else
	return (door);
    } else {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->keyword)
	  if (is_name(type, EXIT(ch, door)->keyword))
	    return (door);

    send_to_char(ch, "There doesn't seem to be %s %s that could be manipulated in that way here.\r\n", AN(type), type);
    return (-1);
  }
}

static int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;
  int i;
 
  if (key == 1) {
   return (1);
  }

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      if (GET_OBJ_VNUM(GET_EQ(ch, i)) == key)
        return (1);

  return (0);
}

#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

const char *cmd_door[NUM_DOOR_CMD] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

static const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_CLOSED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, VAL_CONTAINER_FLAGS), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH];
  size_t len;
  int num = 0;
  room_rnum other_room = NOWHERE;
  struct room_direction_data *back = NULL;
  struct obj_data *hatch = NULL, *obj2 = NULL, *next_obj, *vehicle = NULL;

    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH) {
     vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(obj, VAL_HATCH_DEST));
    }
    else if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
      if (real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE) {
      num = IN_ROOM(ch);
      char_from_room(ch);
      char_to_room(ch, real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)));
      }
      for (obj2 = world[IN_ROOM(ch)].contents; obj2; obj2 = next_obj) {
       next_obj = obj2->next_content;
       if (GET_OBJ_TYPE(obj2) == ITEM_HATCH) {
        hatch = obj2;
       }
      }
      obj2 = NULL;
  }

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE)) {
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != IN_ROOM(ch))
	back = NULL;
  }
  switch (scmd) {
  case SCMD_OPEN:
    if (obj) {
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
     OPEN_DOOR(IN_ROOM(ch), vehicle, door);
     if (GET_OBJ_VNUM(obj) > 19199) {
      send_to_room(IN_ROOM(ch), "@wThe ship hatch opens slowly and settles onto the ground outside.\r\n");
      send_to_room(IN_ROOM(vehicle), "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
      if (ROOM_FLAGGED(IN_ROOM(vehicle), ROOM_SPACE)) {
       send_to_room(IN_ROOM(ch), "@wA great vortex forms as air begins to get sucked out into the void!\r\n");
      }
     } else {
      act("@wYou open @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@w opens @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_room(IN_ROOM(vehicle), "@wThe door to %s@w is opened from the other side.\r\n", vehicle->short_description);
     }
     vehicle = NULL;
    }
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
     OPEN_DOOR(IN_ROOM(ch), hatch, door);
     char_from_room(ch);
     char_to_room(ch, num);
     if (GET_OBJ_VNUM(obj) > 19199) {
      send_to_room(num, "@wThe ship hatch opens slowly and settles onto the ground.\r\n");
      send_to_room(IN_ROOM(hatch), "@wThe ship hatch opens slowly.\r\n");
      if (ROOM_FLAGGED(IN_ROOM(obj), ROOM_SPACE)) {
       send_to_room(num, "@wThe air starts getting sucked out into space as the hatch opens!\r\n");
      }
     } else {
      act("@wYou open @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@w opens @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_room(IN_ROOM(hatch), "@wThe door is opened from the other side.\r\n");
     }
     hatch = NULL;
      }
     }
     OPEN_DOOR(IN_ROOM(ch), obj, door);
    if (back) {
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    }
    if (!obj) {
    send_to_char(ch, "You open the %s that leads %s.\r\n", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door", dirs[door]);
    }
    else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH) {
    send_to_char(ch, "You open %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_CLOSE:
    if (obj) {
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
     CLOSE_DOOR(IN_ROOM(ch), vehicle, door);
     if (GET_OBJ_VNUM(obj) > 19199) {
      send_to_room(IN_ROOM(ch), "@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
      send_to_room(IN_ROOM(vehicle), "@wThe ship hatch slowly closes, sealing the ship.\r\n");
      if (ROOM_FLAGGED(IN_ROOM(vehicle), ROOM_SPACE)) {
       send_to_room(IN_ROOM(ch), "@wThe air stops getting sucked out into space as the hatch seals!\r\n");
      }
     } else {
      act("@wYou close @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@w closes @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_room(IN_ROOM(vehicle), "@wThe door to %s@w is closed from the other side.\r\n", vehicle->short_description);
     }
     vehicle = NULL;
    }
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
     CLOSE_DOOR(IN_ROOM(ch), hatch, door);
     char_from_room(ch);
     char_to_room(ch, num);
     if (GET_OBJ_VNUM(obj) > 19199) {
      send_to_room(num, "@wThe ship hatch slowly closes, sealing the ship.\r\n");
      send_to_room(IN_ROOM(hatch), "@wThe ship hatch slowly closes, sealing the ship from the outside.\r\n");
      if (ROOM_FLAGGED(IN_ROOM(obj), ROOM_SPACE)) {
       send_to_room(num, "@wAir stops getting sucked out into space as the hatch seals!\r\n");
      }
     } else {
      act("@wYou close @c$p@w.", TRUE, ch, obj, 0, TO_CHAR);
      act("@C$n@w closes @c$p@w.", TRUE, ch, obj, 0, TO_ROOM);
      send_to_room(IN_ROOM(hatch), "@wThe door to %s@w is closed from the other side.\r\n", hatch->short_description);
     }
     hatch = NULL;
     }
    }
    CLOSE_DOOR(IN_ROOM(ch), obj, door);
    if (back) {
      CLOSE_DOOR(other_room, obj, rev_dir[door]);
    }
    if (!obj) {
    send_to_char(ch, "You close the %s that leads %s.\r\n", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door", dirs[door]);
    }
    else if (GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH) {
    send_to_char(ch, "You close %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_LOCK:
    if (obj) {
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
     LOCK_DOOR(IN_ROOM(ch), vehicle, door);
     vehicle = NULL;
    }
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
     LOCK_DOOR(IN_ROOM(ch), hatch, door);
     char_from_room(ch);
     char_to_room(ch, num);
     hatch = NULL;
     }
    }
    LOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back) {
      LOCK_DOOR(other_room, obj, rev_dir[door]);
     }
    if (!obj) {
    send_to_char(ch, "You lock the %s that leads %s.\r\n", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door", dirs[door]);
    }
    else {
    send_to_char(ch, "You lock %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_UNLOCK:
    if (obj) {
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_HATCH && (vehicle)) {
     UNLOCK_DOOR(IN_ROOM(ch), vehicle, door);
     vehicle = NULL;
    }
    if ((obj) && GET_OBJ_TYPE(obj) == ITEM_VEHICLE && (hatch)) {
     UNLOCK_DOOR(IN_ROOM(ch), hatch, door);
     char_from_room(ch);
     char_to_room(ch, num);
     hatch = NULL;
     }
    }
    UNLOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back) {
      UNLOCK_DOOR(other_room, obj, rev_dir[door]);
     }
    if (!obj) {
    send_to_char(ch, "You unlock the %s that leads %s.\r\n", EXIT(ch, door)->keyword ? EXIT(ch, door)->keyword : "door", dirs[door]);
    }
    else {
    send_to_char(ch, "You unlock %s.\r\n", obj->short_description);
    }
    break;

  case SCMD_PICK:
    TOGGLE_LOCK(IN_ROOM(ch), obj, door);
    if (back)
      TOGGLE_LOCK(other_room, obj, rev_dir[door]);
    send_to_char(ch, "The lock quickly yields to your skills.\r\n");
    len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    break;
  }

  /* Notify the room. */
  char dbuf[100];
  if (!obj) {
   sprintf(dbuf, "%s", dirs[door]);
  }
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s%s%s.",
	obj ? "" : "the ", obj ? "$p" : EXIT(ch, door)->keyword ? "$F" : "door", obj ? "" : " that leads ", obj ? "" : dbuf);
  if (!obj || IN_ROOM(obj) != NOWHERE)
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE)) {
      send_to_room(EXIT(ch, door)->to_room, "The %s that leads %s is %s%s from the other side.\r\n",
		back->keyword ? fname(back->keyword) : "door", dbuf, cmd_door[scmd],
		scmd == SCMD_CLOSE ? "d" : "ed");
  }
  else if (back && (scmd == SCMD_LOCK || scmd == SCMD_UNLOCK)) {
      send_to_room(EXIT(ch, door)->to_room, "The %s that leads %s is %sed from the other side.\r\n",
                back->keyword ? fname(back->keyword) : "door", dbuf, cmd_door[scmd]);
  }
  *dbuf = '\0';
}

static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int dclock, int scmd, struct obj_data *hatch)
{
  int skill_lvl, found = FALSE;
  struct obj_data *obj, *next_obj;

  for (obj = ch->carrying; obj; obj = next_obj) {
       next_obj = obj->next_content;
   if (GET_OBJ_VNUM(obj) == 18 && (!OBJ_FLAGGED(obj, ITEM_BROKEN) && !OBJ_FLAGGED(obj, ITEM_FORGED))) {
    found = TRUE;
   }
  }

  if (scmd != SCMD_PICK)
    return (1);

  /* PICKING_LOCKS is not an untrained skill */
  if (!GET_SKILL(ch, SKILL_OPEN_LOCK)) {
    send_to_char(ch, "You have no idea how!\r\n");
    return (0);
  }
  if (found == FALSE) {
    send_to_char(ch, "You need a lock picking kit.\r\n");
    return (0);
  }
  if (hatch != NULL && (GET_OBJ_TYPE(hatch) == ITEM_HATCH || GET_OBJ_TYPE(hatch) == ITEM_VEHICLE)) {
     send_to_char(ch, "No picking ship hatches.\r\n");
     hatch = NULL;
     return (0);
  }
  skill_lvl = roll_skill(ch, SKILL_OPEN_LOCK);
  if (dclock == 0) {
   dclock = rand_number(1, 101);
  }

  if (keynum == NOTHING) {
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  }
  else if (pickproof) {
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
    act("@c$n@w puts a set of lockpick tools away.@n" , TRUE, ch, 0, 0, TO_ROOM);
  /* The -2 is here because that is a penality for not having a set of
   * thieves' tools. If the player has them, that modifier will be accounted
   * for in roll_skill, and negate (or surpass) this. 
   */
  }
  else if (GET_MOVE(ch) < GET_MAX_MOVE(ch) / 30) {
   send_to_char(ch, "You don't have the stamina to try, it takes percision to pick locks."
                    "Not shaking tired hands.\r\n");
  }
  else if (dclock > (skill_lvl - 2)) {
    send_to_char(ch, "You failed to pick the lock...\r\n");
    act("@c$n@w puts a set of lockpick tools away.@n" , TRUE, ch, 0, 0, TO_ROOM);
    if (GET_MOVE(ch) > GET_MOVE(ch) / 30) {
     GET_MOVE(ch) -= GET_MOVE(ch) / 30;
    }
    else {
     GET_MOVE(ch) = 0;
    }
  }
  else {
    if (GET_MOVE(ch) > GET_MOVE(ch) / 30) {
     GET_MOVE(ch) -= GET_MOVE(ch) / 30;
    }
    else {
     GET_MOVE(ch) = 0;
    }
    return (1);
  }

  return (0);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_VEHICLE)   && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_HATCH)     && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_WINDOW)    && \
                        OBJVAL_FLAGGED(obj, CONT_CLOSEABLE))   || \
                        ((GET_OBJ_TYPE(obj) == ITEM_PORTAL)    && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_SECRET(ch, obj, door) ((obj) ? \
                        (OBJVAL_FLAGGED(obj, CONT_SECRET)) : \
                        (EXIT_FLAGGED(EXIT(ch, door), EX_SECRET)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, VAL_KEY_KEYCODE)) : \
					(EXIT(ch, door)->key))
#define DOOR_DCLOCK(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK)) : EXIT(ch, door)->dclock)

ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER && GET_OBJ_TYPE(obj) != ITEM_VEHICLE && GET_OBJ_TYPE(obj) != ITEM_HATCH)) {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!DOOR_DCLOCK(ch, obj, door)) {
      if (obj) {
        GET_OBJ_VAL(obj, VAL_DOOR_DCLOCK) = 20;
      }
      else {
        EXIT(ch, door)->dclock = 20;
      }
    }
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && !ADM_FLAGGED(ch, ADM_NOKEYS) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (!obj && ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), DOOR_DCLOCK(ch, obj, door), subcmd, NULL))
      do_doorcmd(ch, obj, door, subcmd);
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), DOOR_DCLOCK(ch, obj, door), subcmd, obj) && obj)
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}

static int do_simple_enter(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST));
  room_rnum was_in = IN_ROOM(ch);
  int need_movement = 0;

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
      IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  if (ROOM_GRAVITY(IN_ROOM(ch)) > 10) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  if (GET_LEVEL(ch) <= 1) {
   need_movement = 0;
  }
  if (GET_MOVE(ch) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(dest_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[dest_room])) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(dest_room, ROOM_GODROOM) &&
	GET_ADMLEVEL(ch) < ADMLVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
    GET_MOVE(ch) -= need_movement;

  act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);

  if (DRAGGING(ch)) {
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) ) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

    if (GET_OBJ_TYPE(obj) == ITEM_PORTAL)
     act("$n arrives from $p.", FALSE, ch, obj, 0, TO_ROOM | TO_SNEAKRESIST);
    else
     act("$n arrives from outside.", FALSE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  if (DRAGGING(ch)) {
   act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) && !AFF_FLAGGED(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(DRAGGING(ch), "You feel your sleeping body being moved.\r\n");
    if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
     set_fighting(DRAGGING(ch), ch);
    }
   }
   char_from_room(DRAGGING(ch));
   char_to_room(DRAGGING(ch), IN_ROOM(ch));
   if (SITS(DRAGGING(ch))) {
    obj_from_room(SITS(DRAGGING(ch)));
    obj_to_room(SITS(DRAGGING(ch)), IN_ROOM(ch));
   }
  }
  if (CARRYING(ch)) {
   act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
   if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) && !AFF_FLAGGED(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(CARRYING(ch), "You feel your sleeping body being moved.\r\n");
   }
   char_from_room(CARRYING(ch));
   char_to_room(CARRYING(ch), IN_ROOM(ch));
   if (SITS(CARRYING(ch))) {
    obj_from_room(SITS(CARRYING(ch)));
    obj_to_room(SITS(CARRYING(ch)), IN_ROOM(ch));
   }
  }

  if (ch->desc != NULL)
    look_at_room(IN_ROOM(ch), ch, 0);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

static int perform_enter_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = IN_ROOM(ch);
  int could_move = FALSE;
  struct follow_type *k;

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
   send_to_char(ch, "You are grappling with someone!\r\n");
   return (0);
  }

  if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE ||
      GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
      send_to_char(ch, "But it's closed!\r\n");
    } else if ((GET_OBJ_VAL(obj, VAL_PORTAL_DEST)            != NOWHERE) &&
               (real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)) != NOWHERE)) {
      if (GET_OBJ_VAL(obj, VAL_PORTAL_DEST) >= 45000 && GET_OBJ_VAL(obj, VAL_PORTAL_DEST) <= 45099) {
        struct char_data *tch, *next_v;
        int filled = FALSE;
        for (tch = world[real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))].people; tch; tch = next_v) {
          next_v = tch->next_in_room;
          if (tch) {
           filled = TRUE;
          }
        }
        if (filled == TRUE) {
         send_to_char(ch, "Only one person can fit in there at a time.\r\n");
         return (0);
        }
      }
      if ((could_move = do_simple_enter(ch, obj, need_specials_check)))
        for (k = ch->followers; k; k = k->next)
          if ((IN_ROOM(k->follower) == was_in) &&
              (GET_POS(k->follower) >= POS_STANDING)) {
	    act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	    perform_enter_obj(k->follower, obj, 1);
          }
    } else {
       send_to_char(ch,
           "It doesn't look like you can enter it at the moment.\r\n");
    }
  } else {
    send_to_char(ch, "You can't enter that!\r\n");
  }
  return could_move;
}

ACMD(do_enter)
{
  struct obj_data *obj = NULL;
  char buf[MAX_INPUT_LENGTH];
  int door, move_dir = -1;

  one_argument(argument, buf);

  if (*buf) { /* an argument was supplied, search for door keyword */
    /* Is the object in the room? */
    obj = get_obj_in_list_vis(ch,buf, NULL, world[IN_ROOM(ch)].contents);
    /* Is the object in the character's inventory? */
    if (!obj)
      obj = get_obj_in_list_vis(ch,buf, NULL, ch->carrying);
    /* Is the character carrying the object? */
    if (!obj)
      obj = get_obj_in_equip_vis(ch, buf, NULL, ch->equipment);
    /* We have an object to enter */
    if (obj)
      perform_enter_obj(ch, obj, 0);
    /* Is there a door to enter? */
    else {
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (EXIT(ch, door))
          if (EXIT(ch, door)->keyword)
            if (isname(buf, EXIT(ch, door)->keyword))
              move_dir = door;
      /* Did we find what they wanted to enter. */
      if (move_dir > -1)
        perform_move(ch, move_dir, 1);
      else
        send_to_char(ch, "There is no %s here.\r\n", buf);
    }
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS)) {
    send_to_char(ch, "You are already indoors.\r\n");
  } else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->to_room != NOWHERE)
          if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
              ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS))
            move_dir = door;
    if (move_dir > -1)
      perform_move(ch, move_dir, 1);
    else
      send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}

static int do_simple_leave(struct char_data *ch, struct obj_data *obj, int need_specials_check)

{
  room_rnum was_in = IN_ROOM(ch), dest_room = NOWHERE;
  int need_movement = 0;
  struct obj_data *vehicle = NULL;

  if (GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
   vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(obj, VAL_HATCH_DEST));
  }

  if (vehicle == NULL && GET_OBJ_TYPE(obj) != ITEM_PORTAL) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
  }

  if (GET_OBJ_TYPE(obj) == ITEM_PORTAL && OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
    send_to_char(ch, "But it's closed!\r\n");
    return 0;
  }

  if (vehicle != NULL) {
   if ((dest_room = IN_ROOM(vehicle)) == NOWHERE) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
   }
  }
  if (vehicle == NULL) {
   if ((dest_room = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST))) == NOWHERE) {
    send_to_char(ch, "That doesn't appear to lead anywhere.\r\n");
    return 0;
   }
  }

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master &&
      IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = 1;
  if (ROOM_GRAVITY(IN_ROOM(ch)) > 10) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  else if (ROOM_GRAVITY(IN_ROOM(ch)) == 10 && !IS_BARDOCK(ch) && !IS_NPC(ch)) {
   need_movement = (need_movement + ROOM_GRAVITY(IN_ROOM(ch))) * ROOM_GRAVITY(IN_ROOM(ch));
  }
  if (GET_LEVEL(ch) <= 1) {
   need_movement = 0;
  }
  if (GET_MOVE(ch) < need_movement && !AFF_FLAGGED(ch, AFF_FLYING) && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(dest_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(dest_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[dest_room])) >= CONFIG_TUNNEL_SIZE) {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Now we know we're allowed to go into the room. */
  if (!(IS_NPC(ch) || ADM_FLAGGED(ch, ADM_WALKANYWHERE)) && !AFF_FLAGGED(ch, AFF_FLYING))
    GET_MOVE(ch) -= need_movement;

  act("$n leaves $p.", TRUE, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);

  if (DRAGGING(ch)) {
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
  }
  if (CARRYING(ch)) {
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
  }
  char_from_room(ch);
  char_to_room(ch, dest_room);

  /* move them first, then move them back if they aren't allowed to go. */
  /* see if an entry trigger disallows the move */
  if (!entry_mtrigger(ch) ) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  if (vehicle) {
   act("$n arrives from inside $p.", TRUE, ch, vehicle, 0, TO_ROOM | TO_SNEAKRESIST);
  } else {
   act("$n arrives from inside", TRUE, ch, 0, 0, TO_ROOM | TO_SNEAKRESIST);
  }
  if (DRAGGING(ch)) {
   act("@wYou drag @C$N@w with you.@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@w drags @c$N@w with $m.@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   char_from_room(DRAGGING(ch));
   char_to_room(DRAGGING(ch), IN_ROOM(ch));
   if (SITS(DRAGGING(ch))) {
    obj_from_room(SITS(DRAGGING(ch)));
    obj_to_room(SITS(DRAGGING(ch)), IN_ROOM(ch));
   }
   if (!AFF_FLAGGED(DRAGGING(ch), AFF_KNOCKED) && !AFF_FLAGGED(DRAGGING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(DRAGGING(ch), "You feel your sleeping body being moved.\r\n");
    if (IS_NPC(DRAGGING(ch)) && !FIGHTING(DRAGGING(ch))) {
     set_fighting(DRAGGING(ch), ch);
    }
   }
  }
  if (CARRYING(ch)) {
   act("@wYou carry @C$N@w with you.@n", TRUE, ch, 0, CARRYING(ch), TO_CHAR);
   act("@C$n@w carries @c$N@w with $m.@n", TRUE, ch, 0, CARRYING(ch), TO_ROOM);
   char_from_room(CARRYING(ch));
   char_to_room(CARRYING(ch), IN_ROOM(ch));
   if (SITS(CARRYING(ch))) {
    obj_from_room(SITS(CARRYING(ch)));
    obj_to_room(SITS(CARRYING(ch)), IN_ROOM(ch));
   }
   if (!AFF_FLAGGED(CARRYING(ch), AFF_KNOCKED) && !AFF_FLAGGED(CARRYING(ch), AFF_SLEEP) && rand_number(1, 3)) {
    send_to_char(CARRYING(ch), "You feel your sleeping body being moved.\r\n");
   }
  }

  char buf3[MAX_STRING_LENGTH];
   send_to_sense(0, "You sense someone ", ch);
   sprintf(buf3, "@D[@GBlip@D]@Y %s\r\n@RSomeone has entered your scouter detection range.@n", add_commas(GET_HIT(ch)));
   send_to_scouter(buf3, ch, 0, 0);

  if (ch->desc != NULL) {
    act(obj->action_description, TRUE, ch, obj, 0, TO_CHAR);
    look_at_room(IN_ROOM(ch), ch, 0);
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && !ADM_FLAGGED(ch, ADM_WALKANYWHERE)) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return 0;
  }

  entry_memory_mtrigger(ch);
  greet_memory_mtrigger(ch);

  return 1;
}

static int perform_leave_obj(struct char_data *ch, struct obj_data *obj, int need_specials_check)
{
  room_rnum was_in = IN_ROOM(ch);
  int could_move = FALSE;
  struct follow_type *k;

  if (GRAPPLING(ch) || GRAPPLED(ch)) {
   send_to_char(ch, "You are grappling with someone!\r\n");
   return (0);
  }

  if (OBJVAL_FLAGGED(obj, CONT_CLOSED)) {
     send_to_char(ch, "But the way out is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(obj, VAL_HATCH_DEST) != NOWHERE)
      if ((could_move = do_simple_leave(ch, obj, need_specials_check)))
        for (k = ch->followers; k; k = k->next)
          if ((IN_ROOM(k->follower) == was_in) &&
              (GET_POS(k->follower) >= POS_STANDING)) {
            act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            perform_leave_obj(k->follower, obj, 1);
	  }
  }
  return could_move;
}


ACMD(do_leave)
{
  int door;
  struct obj_data *obj = NULL;

  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }

  for (obj = world[IN_ROOM(ch)].contents; obj ; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if (GET_OBJ_TYPE(obj) ==  ITEM_HATCH || GET_OBJ_TYPE(obj) == ITEM_PORTAL) {
	perform_leave_obj(ch, obj, 0);
        return;
      }

  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}

static void handle_fall(struct char_data *ch)
{
 int room = -1;
 while (EXIT(ch, 5) && SECT(IN_ROOM(ch)) == SECT_FLYING) {
  room = GET_ROOM_VNUM(EXIT(ch, 5)->to_room);
  char_from_room(ch);
  char_to_room(ch, real_room(room));
  if (CARRYING(ch)) {
   char_from_room(CARRYING(ch));
   char_to_room(CARRYING(ch), real_room(room));
  }
  if (!EXIT(ch, 5) || SECT(IN_ROOM(ch)) != SECT_FLYING) {
   act("@r$n slams into the ground!@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_HIT(ch) - GET_MAX_HIT(ch) / 20 <= 0) {
    GET_HIT(ch) = 1;
   } else {
    GET_HIT(ch) -= GET_MAX_HIT(ch) / 20;
   }
   act("@rYou slam into the ground!@n", TRUE, ch, 0, 0, TO_CHAR);
   look_at_room(IN_ROOM(ch), ch, 0);
  } else {
   act("@r$n pummets down toward the ground below!@n", TRUE, ch, 0, 0, TO_ROOM);
  }
 }
 if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM && !CARRIED_BY(ch) && !IS_KANASSAN(ch)) {
  if (GET_MOVE(ch) >= gear_weight(ch)) {
   act("@bYou swim in place.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@b swims in place.@n", TRUE, ch, 0, 0, TO_ROOM);
   GET_MOVE(ch) -= gear_weight(ch);
  } else {
   GET_MOVE(ch) -= gear_weight(ch);
   if (GET_MOVE(ch) < 0) {
    GET_MOVE(ch) = 0;
   }
   act("@RYou are drowning!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n@b gulps water as $e struggles to stay above the water line.@n", TRUE, ch, 0, 0, TO_ROOM);
   if (GET_HIT(ch) - (gear_pl(ch) / 3) <= 0) {
    act("@rYou drown!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@R$n@r drowns!@n", TRUE, ch, 0, 0, TO_ROOM);
    die(ch, NULL);
    GET_HIT(ch) = 1;
   } else {
    GET_HIT(ch) -= (gear_pl(ch) / 3);
   }
  }
 }

}

static int check_swim(struct char_data *ch)
{

 if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SPACE)) {
  if (GET_MANA(ch) >= (GET_MAX_MANA(ch) / 1000) + (gear_weight(ch) / 2)) {
   GET_MANA(ch) -= (GET_MAX_MANA(ch) / 1000) + (gear_weight(ch) / 2);
   return (1);
  } else {
   GET_MANA(ch) = 0;
   send_to_char(ch, "You do not have enough ki to fly through space. You are drifting helplessly.\r\n");
   return (0);
  }
 } else {
  if (GET_MOVE(ch) >= gear_weight(ch) - 1) {
   GET_MOVE(ch) -= gear_weight(ch) - 1;
   return (1);
  } else {
   send_to_char(ch, "You are too tired to swim!\r\n");
   return (0);
  }
 }

}

ACMD(do_fly)
{
 char arg[MAX_INPUT_LENGTH];

 one_argument(argument, arg);

 if (ABSORBING(ch) || ABSORBBY(ch)) {
  send_to_char(ch, "You can't fly, you are struggling with someone right now!");
  return;
 }
 if (GRAPPLING(ch) || GRAPPLED(ch)) {
  send_to_char(ch, "You can't fly, you are struggling with someone right now!");
  return;
 }
 if (!IS_NPC(ch)) {
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
    send_to_char(ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
    send_to_char(ch, "You are busy piloting a ship!\r\n");
    return;
  }
 }
 
 if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_FOCUS) < 30 && !IS_ANDROID(ch)) {
   send_to_char(ch, "You do not have enough focus to hold yourself aloft.\r\n");
   send_to_char(ch, "@wOOC@D: @WYou need the skill Focus at @m30@W.@n\r\n");
   return;
 }

 if (!*arg) {
 if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) != SECT_FLYING && SECT(IN_ROOM(ch)) != SECT_SPACE) {
   act("@WYou slowly settle down to the ground.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@W$n slowly settles down to the ground.@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 0;
   return;
 }

 if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) == SECT_FLYING) {
   act("@WYou begin to plummet to the ground!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@W$n starts to pummet to the ground below!@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 0;
   handle_fall(ch);
   return;
 }
 if (AFF_FLAGGED(ch, AFF_FLYING) && SECT(IN_ROOM(ch)) == SECT_SPACE) {
   act("@WYou let yourself drift aimlessly through space.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@W$n starts to drift slowly.!@n", TRUE, ch, 0, 0, TO_ROOM);
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 0;
   return;
 }
 if (GET_MANA(ch) < GET_MAX_MANA(ch) / 100 && !IS_ANDROID(ch)) {
  send_to_char(ch, "You do not have the ki to fly.");
  return;
 }
 else {
   reveal_hiding(ch, 0);
   act("@WYou slowly take off into the sky.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@W$n slowly takes off into the sky.@n", TRUE, ch, 0, 0, TO_ROOM);
   if (SITS(ch)) {
    SITTING(SITS(ch)) = NULL;
    SITS(ch) = NULL;
   }
   if (GET_POS(ch) < POS_STANDING) {
    GET_POS(ch) = POS_STANDING;
   }
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 1;
   GET_MANA(ch) -= GET_MAX_MANA(ch) / 100;
 }
 }
 if (!strcasecmp("high", arg)) {
  if (GET_MANA(ch) < GET_MAX_MANA(ch) / 100 && !IS_ANDROID(ch)) {
   send_to_char(ch, "You do not have the ki to fly.");
   return;
  }
  else {
   reveal_hiding(ch, 0);
   act("@WYou rocket high into the sky.@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@W$n rockets high into the sky.@n", TRUE, ch, 0, 0, TO_ROOM);
   if (SITS(ch)) {
    SITTING(SITS(ch)) = NULL;
    SITS(ch) = NULL;
   }
   if (GET_POS(ch) < POS_STANDING) {
    GET_POS(ch) = POS_STANDING;
   }
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 2;
   GET_MANA(ch) -= GET_MAX_MANA(ch) / 100;
  }
 }
 if (!strcasecmp("space", arg)) {
  if (!OUTSIDE(ch)) {
   send_to_char(ch, "You are not outside!");
   return;
  }
  if (GET_MANA(ch) < GET_MAX_MANA(ch) / 10 && !IS_ANDROID(ch)) {
   send_to_char(ch, "You do not have the ki to fly to space.");
   return;
  }
  if (FIGHTING(ch)) {
   send_to_char(ch, "You are too busy fighting!");
   return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_EARTH)) {
   reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(50));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_CERRIA)) {
   reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(198));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_VEGETA)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(53));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KANASSA)) {
   if (GET_ROOM_VNUM(IN_ROOM(ch)) == 14904) {
          reveal_hiding(ch, 0);
     GET_ALT(ch) = 2;
     SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
     if (!block_calc(ch)) {
      return;
     }
     GET_ALT(ch) = 0;
     REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
    act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR); 
    act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, real_room(58));
    act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
    if (!IS_ANDROID(ch)) {
     GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
    }
    WAIT_STATE(ch, PULSE_3SEC);
   } else {
    send_to_char(ch, "You can only fly off the planet from the launchpad of Aquis.\r\n");
   }
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FRIGID)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(51));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_KONACK)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(52));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_NAMEK)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(54));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_AETHER)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(55));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_YARDRAT)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(56));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARLIA)) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(59));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else if (PLANET_ZENITH(IN_ROOM(ch))) {
         reveal_hiding(ch, 0);
   GET_ALT(ch) = 2;
   SET_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
   if (!block_calc(ch)) {
    return;
   }
   GET_ALT(ch) = 0;
   REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_FLYING);
    int zone = 0;
    if ((zone = real_zone_by_thing(GET_ROOM_VNUM(IN_ROOM(ch)))) != NOWHERE) {
     fly_zone(zone, "can be seen blasting off into space!@n\r\n", ch);
    }
    send_to_sense(1, "leaving the planet", ch);
    send_to_scouter("A powerlevel signal has left the planet", ch, 0, 2);
   act("@CYou blast off from the ground and rocket through the air. Your speed increases until you manage to reach the brink of space!@n", TRUE, ch, 0, 0, TO_CHAR);
   act("@C$n blasts off from the ground and rockets through the air. You quickly lose sight of $m as $e continues upward!@n", TRUE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, real_room(57));
   act("@C$n blasts up from the atmosphere below and then comes to a stop.@n", TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(ch, "@mOOC: Use the command 'land' to return to the planet from here.@n\r\n");
   if (!IS_ANDROID(ch)) {
    GET_MANA(ch) -= GET_MAX_MANA(ch) / 10;
   }
   WAIT_STATE(ch, PULSE_3SEC);
   return;
  } else {
   send_to_char(ch, "You are not on a planet.\r\n");
   return;
  }
 }
}

ACMD(do_stand)
{
  struct obj_data *chair;
  if (AFF_FLAGGED(ch, AFF_KNOCKED)) {
   send_to_char(ch, "You are knocked out cold for right now!\r\n");
   return;
  }
  if (!IS_NPC(ch) && GET_LIMBCOND(ch, 3) <= 0 && GET_LIMBCOND(ch, 4) <= 0) {
   send_to_char(ch, "With what legs will you be standing up on?\r\n");
   return;
  }
  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;
  case POS_SITTING:
          reveal_hiding(ch, 0);
    send_to_char(ch, "You stand up.\r\n");
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    if (SITS(ch)) {
     if (CAN_WEAR(SITS(ch), ITEM_WEAR_TAKE) && GET_OBJ_TYPE(SITS(ch)) != ITEM_CHAIR && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(SITS(ch)) <= CAN_CARRY_W(ch)) {
      obj_from_room(SITS(ch));
      obj_to_char(SITS(ch), ch);
      act("You pick up $p.", TRUE, ch, SITS(ch), 0, TO_CHAR);
      act("$n picks up $p.", TRUE, ch, SITS(ch), 0, TO_ROOM);
     }
     chair = SITS(ch);
     SITTING(chair) = NULL;
     SITS(ch) = NULL;
    }
    /* May be sitting for some reason and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and stand up.\r\n");
    act("$n stops resting, and clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    if (SITS(ch)) {
     if (CAN_WEAR(SITS(ch), ITEM_WEAR_TAKE) && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(SITS(ch)) <= CAN_CARRY_W(ch)) {
      obj_from_room(SITS(ch));
      obj_to_char(SITS(ch), ch);
      act("You pick up $p.", TRUE, ch, SITS(ch), 0, TO_CHAR);
      act("$n picks up $p.", TRUE, ch, SITS(ch), 0, TO_ROOM);
     }
     chair = SITS(ch);
     SITTING(chair) = NULL;
     SITS(ch) = NULL;
    }
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}

ACMD(do_sit)
{
  struct obj_data *chair = NULL;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
  }
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }

  if (DRAGGING(ch)) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   DRAGGED(DRAGGING(ch)) = NULL;
   DRAGGING(ch) = NULL;
  }
  if (CARRYING(ch)) {
   send_to_char(ch, "You are busy carrying someone!\r\n");
   return;
  }

  if (AFF_FLAGGED(ch, AFF_FLYING)) {
   do_fly(ch, 0, 0, 0);
  }

  if (!*arg) {
  switch (GET_POS(ch)) {
  case POS_STANDING:
          reveal_hiding(ch, 0);
    send_to_char(ch, "You sit down.\r\n");
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
 }
 else {
  if (SITS(ch)) {
   send_to_char(ch, "You are already on something!\r\n");
   return;
  }
  if (!(chair = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
   send_to_char(ch, "That isn't here.\r\n");
   return;
  }
  if (GET_OBJ_VNUM(chair) == 65) {
   send_to_char(ch, "You can't get on that!\r\n");
   return;
  }
  if (SITTING(chair)) {
   send_to_char(ch, "Someone is already on that one!\r\n");
   return;
  }
  if (GET_OBJ_TYPE(chair) != ITEM_CHAIR && GET_OBJ_TYPE(chair) != ITEM_BED) {
   send_to_char(ch, "You can't sit on that!\r\n");
   return;
  }
  if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
   send_to_char(ch, "You are too large for it!\r\n");
   return;
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
          reveal_hiding(ch, 0);
    act("You sit down on $p.", FALSE, ch, chair, 0, TO_CHAR);
    act("$n sits down on $p.", FALSE, ch, chair, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    SITS(ch) = chair;
    SITTING(chair) = ch;
    break;
  case POS_SITTING:
    send_to_char(ch, "You should stand up first.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You should stand up first.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
 }
}

ACMD(do_rest)
{
  struct obj_data *chair = NULL;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
  }
  if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
   send_to_char(ch, "You can't rest here!\r\n");
   return;
  }
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }

  if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
   if (GET_SKILL(ch, SKILL_BARRIER)) {
    send_to_char(ch, "You have a barrier around you and can't rest.\r\n");
    return;
   } else {
    GET_BARRIER(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SANCTUARY);
   }
  }
  if (FIGHTING(ch)) {
   send_to_char(ch, "You are a bit busy at the moment!\r\n");
   return;
  }
  if (GET_KAIOKEN(ch) > 0) {
   send_to_char(ch, "You are utilizing kaioken and can't rest!\r\n");
   return;
  }

  if (DRAGGING(ch)) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   DRAGGED(DRAGGING(ch)) = NULL;
   DRAGGING(ch) = NULL;
  }

  if (CARRYING(ch)) {
   send_to_char(ch, "You are carrying someone!\r\n");
   return;
  }

  if (AFF_FLAGGED(ch, AFF_FLYING)) {
   do_fly(ch, 0, 0, 0);
  }

  if (!*arg) {
   if (SITS(ch)) {
    chair = SITS(ch);
    if (GET_OBJ_TYPE(chair) != ITEM_BED) {
     send_to_char(ch, "You can't lay on that!\r\n");
     return;
    }
   }
  switch (GET_POS(ch)) {
  case POS_STANDING:
          reveal_hiding(ch, 0);
    send_to_char(ch, "You lay down and rest your tired bones.\r\n");
    act("$n lays down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You rest your tired bones.\r\n");
    act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  }
 }
 else {
  if (SITS(ch)) {
   send_to_char(ch, "You are already on something!\r\n");
   return;
  }
  if (!(chair = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
   send_to_char(ch, "That isn't here.\r\n");
   return;
  }
  if (GET_OBJ_VNUM(chair) == 65) {
   send_to_char(ch, "You can't get on that!\r\n");
   return;
  }
  if (SITTING(chair)) {
   send_to_char(ch, "Someone is already on that one!\r\n");
   return;
  }
  if (GET_OBJ_TYPE(chair) != ITEM_BED) {
   send_to_char(ch, "You can't lay on that!\r\n");
   return;
  }
  if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
   send_to_char(ch, "You are too large for it!\r\n");
   return;
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
          reveal_hiding(ch, 0);
    act("You lay down and rest on $p.", TRUE, ch, chair, 0, TO_CHAR);
    act("$n lays down and rests on $p.", TRUE, ch, chair, 0, TO_ROOM);
    SITS(ch) = chair;
    SITTING(chair) = ch;
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You should get up first.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  }
 }
}

ACMD(do_sleep)
{
  struct obj_data *chair = NULL;
  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  if (!IS_NPC(ch)) {
   if (PRF_FLAGGED(ch, PRF_ARENAWATCH)) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ARENAWATCH);
    ARENA_IDNUM(ch) = -1;
    send_to_char(ch, "You stop watching the arena action.\r\n");
   }
  }

  if (GET_BONUS(ch, BONUS_INSOMNIAC)) {
   send_to_char(ch, "You don't feel the least bit tired.\r\n");
   return;
  }

  if (SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) {
   send_to_char(ch, "You can't rest here!\r\n");
   return;
  }

  if (PLR_FLAGGED(ch, PLR_PILOTING)) {
   send_to_char(ch, "You are busy piloting a ship!\r\n");
   return;
  }
  if (FIGHTING(ch)) {
   send_to_char(ch, "You are a bit busy at the moment!\r\n");
   return;
  }
  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP)) {
   send_to_char(ch, "You are busy powering up!\r\n");
   return;
  }
  if (AFF_FLAGGED(ch, AFF_SANCTUARY)) {
   if (GET_SKILL(ch, SKILL_BARRIER) > 0) {
    send_to_char(ch, "You have a barrier around you and can't sleep.\r\n");
    return;
   } else {
    GET_BARRIER(ch) = 0;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_SANCTUARY);
   }
  }
  if (GET_KAIOKEN(ch) > 0) {
   send_to_char(ch, "You are utilizing kaioken and can't sleep!\r\n");
   return;
  }
  if (GET_SLEEPT(ch) > 0) {
   send_to_char(ch, "You aren't sleepy enough.\r\n");
   return;
  }
  if (DRAGGING(ch)) {
   act("@WYou stop dragging @C$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_CHAR);
   act("@C$n@W stops dragging @c$N@W!@n", TRUE, ch, 0, DRAGGING(ch), TO_ROOM);
   DRAGGED(DRAGGING(ch)) = NULL;
   DRAGGING(ch) = NULL;
  }
  if (CARRYING(ch)) {
   send_to_char(ch, "You are carrying someone!\r\n");
   return;
  }

  if (AFF_FLAGGED(ch, AFF_FLYING)) {
   do_fly(ch, 0, 0, 0);
  }

  if (!*arg) {
  if (SITS(ch)) {
   chair = SITS(ch);
   if (GET_OBJ_TYPE(chair) != ITEM_BED) {
    send_to_char(ch, "You can't sleep on %s.\r\n", chair->short_description);
    return;
   }
  }
  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
          reveal_hiding(ch, 0);
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    /* Fury Mode Loss for halfbreeds */

    if (PLR_FLAGGED(ch, PLR_FURY)) {
     send_to_char(ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
     REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_FURY);
    }

    /* Fury Mode Loss for halfbreeds */

    if (GET_STUPIDKISS(ch) > 0) {
     GET_STUPIDKISS(ch) = 0;
     send_to_char(ch, "You forget about that stupid kiss.\r\n");
    }
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
        TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
 }
  else {
  if (SITS(ch)) {
   send_to_char(ch, "You are already on something!\r\n");
   return;
  }
  if (!(chair = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
   send_to_char(ch, "That isn't here.\r\n");
   return;
  }
  if (GET_OBJ_VNUM(chair) == 65) {
   send_to_char(ch, "You can't get on that!\r\n");
   return;
  }
  if (SITTING(chair)) {
   send_to_char(ch, "Someone is already on that one!\r\n");
   return;
  }
  if (GET_OBJ_TYPE(chair) != ITEM_BED) {
   send_to_char(ch, "You can't sleep on that!\r\n");
   return;
  }
  if (GET_OBJ_SIZE(chair) + 1 < get_size(ch)) {
   send_to_char(ch, "You are too large for it!\r\n");
   return;
  }
  switch (GET_POS(ch)) {
  case POS_RESTING:
  case POS_SITTING:
    send_to_char(ch, "You need to get up first!\r\n");
    break;
  case POS_STANDING:
          reveal_hiding(ch, 0);
    act("You lay down on $p and sleep.", FALSE, ch, chair, 0, TO_CHAR);
    act("$n lays down on $p and sleeps.", FALSE, ch, chair, 0, TO_ROOM);
    /* Fury Mode Loss for halfbreeds */

    if (PLR_FLAGGED(ch, PLR_FURY)) {
     send_to_char(ch, "Your fury subsides for now. Next time try to take advantage of it before you calm down.\r\n");
     REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_FURY);
    }

    /* Fury Mode Loss for halfbreeds */

    if (GET_STUPIDKISS(ch) > 0) {
     GET_STUPIDKISS(ch) = 0;
     send_to_char(ch, "You forget about that stupid kiss.\r\n");
    }
    SITS(ch) = chair;
    SITTING(chair) = ch;
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
        TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
 }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  
  if (AFF_FLAGGED(ch, AFF_KNOCKED)) {
   send_to_char(ch, "You are knocked out cold for right now!\r\n");
   return;
  }

  if (GET_BONUS(ch, BONUS_LATE) && GET_POS(ch) == POS_SLEEPING) {
   send_to_char(ch, "Nah you're enjoying sleeping too much.\r\n");
   return;
  }

  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_KNOCKED))
     send_to_char(ch, "They are knocked out cold for right now!\r\n");
    else if (GET_BONUS(ch, BONUS_LATE))
     send_to_char(ch, "They say 'Yeah yeah...' and then roll back over.\r\n");
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
      if (DRAGGED(vict)) {
       act("@WYou stop dragging @C$N@W!@n", TRUE, DRAGGED(vict), 0, vict, TO_CHAR);
       act("@C$n@W stops dragging @c$N@W!@n", TRUE, DRAGGED(vict), 0, vict, TO_ROOM);
       DRAGGING(DRAGGED(vict)) = NULL;
       DRAGGED(vict) = NULL;
      }
      if (CARRIED_BY(vict)) {
       if (GET_ALIGNMENT(CARRIED_BY(vict)) > 50) {
        carry_drop(CARRIED_BY(vict), 0);
       } else {
        carry_drop(CARRIED_BY(vict), 1);
       }
      }
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else {
    send_to_char(ch, "You awaken, and sit up.\r\n");
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    if (DRAGGED(ch)) {
     act("@WYou stop dragging @C$N@W!@n", TRUE, DRAGGED(ch), 0, ch, TO_CHAR);
     act("@C$n@W stops dragging you!@n", TRUE, DRAGGED(ch), 0, ch, TO_VICT);
     act("@C$n@W stops dragging @c$N@W!@n", TRUE, DRAGGED(ch), 0, ch, TO_NOTVICT);
     DRAGGING(DRAGGED(ch)) = NULL;
     DRAGGED(ch) = NULL;
    }
    if (CARRIED_BY(ch)) {
     if (GET_ALIGNMENT(CARRIED_BY(ch)) > 50) {
      carry_drop(CARRIED_BY(ch), 0);
     } else {
      carry_drop(CARRIED_BY(ch), 1);
     }
    }
    GET_POS(ch) = POS_SITTING;
  }
}

ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (PLR_FLAGGED(ch, PLR_HEALT)) {
   send_to_char(ch, "You are inside a healing tank!\r\n");
   return;
  }

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  } else {
    send_to_char(ch, "Whom do you wish to follow?\r\n");
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char(ch, "You are already following yourself.\r\n");
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
            reveal_hiding(ch, 0);
      add_follower(ch, leader);
    }
  }
}
