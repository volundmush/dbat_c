use dbat::*;


#[link(name = "circlemud")]
extern "C" {
    // This matches the signature you renamed in apps/circle.c
    fn circle_main(argc: i32, argv: *const *const i8) -> i32;
}

fn main() {
    // Typically, you'd pass command-line args. A minimal approach might be:
    let args: Vec<std::ffi::CString> = std::env::args()
        .map(|arg| std::ffi::CString::new(arg).unwrap())
        .collect();

    let mut raw_args: Vec<*const i8> =
        args.iter().map(|s| s.as_ptr()).collect();
    raw_args.push(std::ptr::null());

    unsafe {
        circle_main(args.len() as i32, raw_args.as_ptr());
    }
}