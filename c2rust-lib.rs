#![allow(dead_code)]
#![allow(mutable_transmutes)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(unused_assignments)]
#![allow(unused_mut)]
#![feature(c_variadic)]
#![feature(extern_types)]
#![feature(label_break_value)]
#![feature(register_tool)]
#![register_tool(c2rust)]


extern crate f128;#[macro_use]
extern crate num_traits;
extern crate libc;
pub mod apps {
pub mod autowiz;
pub mod circle;
pub mod delobjs;
pub mod play2to3;
pub mod shopconv;
pub mod showplay;
pub mod sign;
pub mod split;
pub mod wld2html;
} // mod apps
pub mod src {
pub mod act_attack;
pub mod act_comm;
pub mod act_informative;
pub mod act_item;
pub mod act_misc;
pub mod act_movement;
pub mod act_offensive;
pub mod act_other;
pub mod act_social;
pub mod act_wizard;
pub mod aedit;
pub mod alias;
pub mod assedit;
pub mod assemblies;
pub mod ban;
pub mod boards;
pub mod bsd_snprintf;
pub mod cedit;
pub mod clan;
pub mod class;
pub mod combat;
pub mod comm;
pub mod config;
pub mod constants;
pub mod context_help;
pub mod db;
pub mod dg_comm;
pub mod dg_db_scripts;
pub mod dg_event;
pub mod dg_handler;
pub mod dg_misc;
pub mod dg_mobcmd;
pub mod dg_objcmd;
pub mod dg_olc;
pub mod dg_scripts;
pub mod dg_triggers;
pub mod dg_variables;
pub mod dg_wldcmd;
pub mod feats;
pub mod fight;
pub mod gedit;
pub mod gengld;
pub mod genmob;
pub mod genobj;
pub mod genolc;
pub mod genshp;
pub mod genwld;
pub mod genzon;
pub mod graph;
pub mod guild;
pub mod handler;
pub mod hedit;
pub mod house;
pub mod hsedit;
pub mod htree;
pub mod imc;
pub mod improved_edit;
pub mod interpreter;
pub mod local_limits;
pub mod magic;
pub mod mail;
pub mod maputils;
pub mod medit;
pub mod mobact;
pub mod modify;
pub mod oasis;
pub mod oasis_copy;
pub mod oasis_delete;
pub mod oasis_list;
pub mod obj_edit;
pub mod objsave;
pub mod oedit;
pub mod olc;
pub mod players;
pub mod races;
pub mod random;
pub mod redit;
pub mod reset;
pub mod sedit;
pub mod sha256;
pub mod shop;
pub mod spec_assign;
pub mod spec_procs;
pub mod spell_parser;
pub mod spells;
pub mod statedit;
pub mod stringutils;
pub mod tedit;
pub mod utils;
pub mod vehicles;
pub mod weather;
pub mod zedit;
pub mod zmalloc;
} // mod src
