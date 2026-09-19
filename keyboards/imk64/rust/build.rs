use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let archive = PathBuf::from(
        env::var_os("QMK_CH582_ARCHIVE")
            .expect("QMK_CH582_ARCHIVE must name the QMK archive built by tools/build.sh"),
    );
    assert!(archive.is_absolute(), "QMK_CH582_ARCHIVE must be absolute");
    assert!(
        archive.is_file(),
        "QMK archive is missing: {}",
        archive.display()
    );

    let archive_dir = archive.parent().expect("QMK archive has no parent");
    let stem = archive
        .file_stem()
        .and_then(|name| name.to_str())
        .and_then(|name| name.strip_prefix("lib"))
        .expect("QMK archive must be named lib<name>.a");
    assert_eq!(archive.extension().and_then(|ext| ext.to_str()), Some("a"));

    let out = PathBuf::from(env::var_os("OUT_DIR").expect("OUT_DIR is missing"));
    fs::write(
        out.join("memory.x"),
        concat!(
            "MEMORY\n{\n",
            // Match hitsmaxft/qmk_port_ch582's imk64 application slot. The
            // preceding 0x1000 bytes belong to the MCUboot image header.
            "  FLASH (rx)  : ORIGIN = 0x00013000, LENGTH = 372K\n",
            "  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K\n",
            "}\n",
            "REGION_ALIAS(\"REGION_TEXT\", FLASH);\n",
            "REGION_ALIAS(\"REGION_RODATA\", FLASH);\n",
            "REGION_ALIAS(\"REGION_DATA\", RAM);\n",
            "REGION_ALIAS(\"REGION_BSS\", RAM);\n",
            "REGION_ALIAS(\"REGION_HEAP\", RAM);\n",
            "REGION_ALIAS(\"REGION_STACK\", RAM);\n",
        ),
    )
    .expect("write CH582 memory layout");

    println!("cargo:rustc-link-search=native={}", archive_dir.display());
    println!("cargo:rustc-link-search={}", out.display());
    println!("cargo:rustc-link-lib=static={stem}");
    println!("cargo:rerun-if-changed={}", archive.display());
    println!("cargo:rerun-if-env-changed=QMK_CH582_ARCHIVE");

    // The branch-aware wrapper supplies the archive for its selected keyboard
    // and keymap. Keep this linker adapter independent of the branch name.
}
