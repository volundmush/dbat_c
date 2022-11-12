//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_DG_COMM_H
#define CIRCLE_DG_COMM_H

#include "structs.h"

// functions
extern void send_to_imm(char *messg, ...);
extern void fly_zone(zone_rnum zone, char *messg, struct char_data *ch);
extern void send_to_scouter(char *messg, struct char_data *ch, int num, int type);
extern void send_to_sense(int type, char *messg, struct char_data *ch);
extern void send_to_worlds(struct char_data *ch);

#endif //CIRCLE_DG_COMM_H
