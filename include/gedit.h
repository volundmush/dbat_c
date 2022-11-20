//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_GEDIT_H
#define CIRCLE_GEDIT_H

#include "structs.h"
#include "guild.h"

extern void gedit_setup_new(struct descriptor_data *d);

extern void gedit_setup_existing(struct descriptor_data *d, int rgm_num);

extern void gedit_parse(struct descriptor_data *d, char *arg);

extern void gedit_disp_menu(struct descriptor_data *d);

extern void gedit_no_train_menu(struct descriptor_data *d);

extern void gedit_save_internally(struct descriptor_data *d);

extern void gedit_save_to_disk(int num);

extern void copy_guild(struct guild_data *tgm, struct guild_data *fgm);

extern void free_guild_strings(struct guild_data *guild);

extern void free_guild(struct guild_data *guild);

extern void gedit_modify_string(char **str, char *new);

#endif //CIRCLE_GEDIT_H
