from yaml import dump

data = {
    "do_oteleport": ["argument"],
	"do_dgoload": ["argument"],
	"do_odamage": ["argument"],
	"do_oasound": ["argument"],
	"do_odoor": ["argument"],
	"do_osetval": ["argument"],
	"do_oat": ["argument"],
	"obj_command_interpreter": ["argument"],
	"eval_expr": ["line", "result"],
	"eval_lhs_op_rhs": ["expr", "result"],
	"process_if": ["cond"],
	"process_wait": ["cmd"],
	"process_set": ["cmd"],
	"process_return": ["cmd"],
	"process_unset": ["cmd"],
	"process_remote": ["cmd"],
	"perform_set_dg_var": ["val_arg"],
	"process_rdelete": ["cmd"],
	"process_global": ["cmd"],
	"process_context": ["cmd"],
	"extract_value": ["cmd"],
	"dg_letter_value": ["cmd"],
	"find_case": ["cond"],
	"fgetline": ["p"],
	"gedit_modify_string": ["new_g"],
	"check_mobile_string": ["dscr"],
	"modify_string": ["new_s"],
	"read_guild_line": ["string"],
	"aff_apply_modify": ["msg"],
	"is_abbrev": ["arg1", "arg2"],
	"lockRead": ["name"],
	"load_HMVS": ["line"],
	"load_BASE": ["line"],
	"get_slide_obj_vis": ["name"],
	"read_line": ["string"]
}

def main():
    out = list()

    for k, v in data.items():
        out.append({"name": k, "fields": [{"name": i, "type": "string"} for i in v]})

    ident = {"ident": out}
    print(dump(ident))


if __name__ == "__main__":
    main()
