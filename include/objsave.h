//
// Created by basti on 10/21/2021.
//

#ifndef CIRCLE_OBJSAVE_H
#define CIRCLE_OBJSAVE_H

#include "structs.h"


/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5

// functions
extern void Crash_extract_norent_eq(struct char_data *ch);
extern void Crash_rentsave(struct char_data *ch, int cost);
extern int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
extern void update_obj_file(void);
extern int cp(struct char_data *ch);
extern void delete_inv_backup(struct char_data *ch);
extern int Crash_delete_file(char *name);
extern int Crash_delete_crashfile(struct char_data *ch);
extern int Crash_clean_file(char *name);
extern void Crash_listrent(struct char_data *ch, char *name);
extern int Crash_load(struct char_data *ch);
extern void Crash_crashsave(struct char_data *ch);
extern void Crash_idlesave(struct char_data *ch);
extern void Crash_save_all(void);
extern int Crash_load_xapobjs(struct char_data *ch);

extern SPECIAL(receptionist);

#endif //CIRCLE_OBJSAVE_H
