//
// Created by basti on 11/12/2022.
//
#include "item_search.h"
#include "utils.h"
#include "handler.h"

bool item_is_usable(struct obj_data *obj) {
    return !(OBJ_FLAGGED(obj, ITEM_BROKEN) || OBJ_FLAGGED(obj, ITEM_FORGED));
}

struct obj_data *find_obj_in_list_lambda(struct obj_data *head, bool(*f)(struct obj_data *obj)) {
    while (head) {
        if (f(head)) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_vnum(struct obj_data *head, obj_vnum vn) {
    while (head) {
        if (GET_OBJ_VNUM(head) == vn) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_vnum_good(struct obj_data *head, obj_vnum vn) {
    while (head) {
        if (GET_OBJ_VNUM(head) == vn && item_is_usable(head)) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_type(struct obj_data *head, int item_type) {
    while (head) {
        if (GET_OBJ_TYPE(head) == item_type) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_flag(struct obj_data *head, bitvector_t flag) {
    while (head) {
        if (OBJ_FLAGGED(head, flag)) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_name(struct obj_data *head, char *name) {
    while (head) {
        if (isname(name, head->name)) return head;
        head = head->next_content;
    }
    return NULL;
}

struct obj_data *find_obj_in_list_id(struct obj_data *head, long item_id) {
    while (head) {
        if (GET_ID(head) == item_id) return head;
        head = head->next_content;
    }
    return NULL;
}