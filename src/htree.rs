use std::{
    collections::HashMap,
    ffi::c_int,
    sync::Mutex
};

use lazy_static::lazy_static;

lazy_static! {
    static ref HTREE: Mutex<HashMap<c_int, HashMap<c_int, c_int>>> = Default::default();
}

#[no_mangle]
pub extern "C" fn htree_add(tree_type: c_int, vnum: c_int, rnum: c_int) {
    let mut map = HTREE.lock().unwrap();
    let inner = map.entry(tree_type).or_insert_with(HashMap::new);
    inner.insert(vnum, rnum);
}

#[no_mangle]
pub extern "C" fn htree_find(tree_type: c_int, vnum: c_int) -> c_int {
    // return a -1 if the key is not found.
    let map = HTREE.lock().unwrap();
    map.get(&tree_type).and_then(|inner| inner.get(&vnum)).copied().unwrap_or(-1)
}

#[no_mangle]
pub extern "C" fn htree_del(tree_type: c_int, vnum: c_int) {
    let mut map = HTREE.lock().unwrap();
    if let Some(inner) = map.get_mut(&tree_type) {
        inner.remove(&vnum);
    }
}
