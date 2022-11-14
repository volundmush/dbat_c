from yaml import dump

data = {
    "zedit_parse": ["arg"],
    "perform_tell": ["arg"],
    "handle_whisper": ["buf"],
    "overhear": ["buf"],
    "look_in_obj": ["arg"],
    "look_at_target": ["arg"],
    "look_out_window": ["arg"],
    "space_to_minus": ["str"],
    "perform_mortal_where": ["arg"],
    "perform_immort_where": ["arg"],
    "cchoice_to_str": ["col"],
    "str_to_cchoice": ["str", "choice"],
    "add_history": ["str"],
    "find_social": ["name"],
    "Valid_Name": ["newname"],
    "load_zones": ["zonename"],
    "file_to_string_alloc": ["name"],
    "log_zone_error": ["message"],
    "check_bitvector_names": ["whatami", "whatbits"],
    "do_oecho": ["argument"],
    "do_oforce": ["argument"],
    "do_ozoneecho": ["argument"],
    "do_osend": ["argument"],
    "do_orecho": ["argument"],
    "do_otimer": ["argument"],
    "do_otransform": ["argument"],
    "do_dupe": ["argument"],
    "do_opurge": ["argument"],
    "do_ogoto": ["argument"],
    "pick_n_throw": ["buf"],
    "mob_attack": ["buf"],
    "shopping_list": ["arg"],
    "shopping_value": ["arg"],
    "read_shop_message": ["why"],
    "spello": ["name"],
    "arto": ["name"],
    "skillo": ["name"],
    "touch": ["path"],
    "ship_land_location": ["arg"],
    "drive_into_vehicle": ["arg"]
}

def main():
    out = list()

    for k, v in data.items():
        out.append({"name": k, "fields": [{"name": i, "type": "string"} for i in v]})

    ident = {"ident": out}
    print(dump(ident))


if __name__ == "__main__":
    main()