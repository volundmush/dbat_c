use cc;
use std::path::PathBuf;

fn main() {
    let mut files = Vec::new();
    for entry in glob::glob("c_src/*.c").expect("Failed to read glob pattern") {
        match entry {
            Ok(path) => { files.push(path); }
            Err(e) => { println!("cargo:warning=Globbing error: {}", e); }
        }
    }

    let out_path = PathBuf::from("src").join("bindings.rs");
     bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("include/db.h")
        .derive_default(true)
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings")
        .write_to_file(out_path)
        .expect("Couldn't write bindings!");

    // Use the 'cc' crate to compile your C sources:
    cc::Build::new()
        .include("include")          // if you need -I c_src
        .files(files)
        .warnings(false)
        //.std("c17")
        .flag_if_supported("-g")    // generate debug info
        .compile("circlemud");

    // Instruct Cargo to re-run build.rs if any .c/.h changes:
    println!("cargo:rerun-if-changed=c_src");
    println!("cargo:rerun-if-changed=include");
}