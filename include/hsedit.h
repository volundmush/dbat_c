//
// Created by volund on 10/20/21.
//

#ifndef CIRCLE_HSEDIT_H
#define CIRCLE_HSEDIT_H

#include "structs.h"
#include "house.h"

void hsedit_setup_new(struct descriptor_data *d);
void hsedit_setup_existing(struct descriptor_data *d, int real_num);
void hsedit_save_internally(struct descriptor_data *d);
void hsedit_save_to_disk(void);
void hsedit_disp_type_menu(struct descriptor_data *d);
void hsedit_disp_menu(struct descriptor_data * d);
void hsedit_parse(struct descriptor_data * d, char *arg);
void hsedit_disp_flags_menu(struct descriptor_data *d);
void hsedit_disp_val0_menu(struct descriptor_data *d);
void hsedit_disp_val1_menu(struct descriptor_data *d);
void hsedit_disp_val2_menu(struct descriptor_data *d);
void hsedit_disp_val3_menu(struct descriptor_data *d);
void free_house(struct house_control_rec *house);

#endif //CIRCLE_HSEDIT_H
