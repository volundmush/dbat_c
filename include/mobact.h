//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_MOBACT_H
#define CIRCLE_MOBACT_H

#include "structs.h"


// functions
extern void clearMemory(struct char_data *ch);
extern void remember(struct char_data *ch, struct char_data *victim);
extern void mobile_activity(void);
extern void forget(struct char_data *ch, struct char_data *victim);
extern void mob_taunt(struct char_data *ch);

#endif //CIRCLE_MOBACT_H
