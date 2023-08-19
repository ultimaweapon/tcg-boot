#![no_std]
#![no_main]

use self::boot::boot;
use self::config::Config;
use self::efi::{
    pause, DebugFile, FileAttributes, FileModes, Image, PathNode, Status, SystemTable,
};
use alloc::borrow::ToOwned;
use alloc::boxed::Box;
use alloc::vec::Vec;

mod boot;
mod config;
mod efi;

extern crate alloc;

#[no_mangle]
extern "efiapi" fn efi_main(image: &'static Image, st: &'static SystemTable) -> Status {
    // SAFETY: This is safe because we do it as the first thing here.
    unsafe {
        crate::efi::init(
            image,
            st,
            Some(|| Box::new(DebugFile::next_to_image("log").unwrap())),
        )
    };

    let status = main(image);

    if status != Status::SUCCESS {
        println!("Press any key to continue.");
        pause();
    }

    status
}

fn main(image: &'static Image) -> Status {
    // Get config path.
    let image = image.proto();
    let mut path = match image.file_path().read() {
        PathNode::MediaFilePath(v) => v.to_owned(),
    };

    path.push_str(".conf").unwrap();

    // Get filesystem of the volume that contains our image.
    let fs = match image.device().file_system() {
        Some(v) => v,
        None => {
            eprintln!("TCG Loader cannot be booting from a non-EFI system partition.");
            return Status::ABORTED;
        }
    };

    // Open the root directory.
    let root = match fs.open() {
        Ok(v) => v,
        Err(e) => {
            eprintln!(
                "Cannot open the root directory on the volume where the TCG Loader is reside: {e}."
            );
            return Status::ABORTED;
        }
    };

    // Open config file.
    let config = match root.open(&path, FileModes::READ, FileAttributes::empty()) {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot open {path}: {e}.");
            return Status::ABORTED;
        }
    };

    // Load the config.
    let config = match Config::load(config) {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot load {path}: {e}.");
            return Status::ABORTED;
        }
    };

    // Check if kernel has been specified in the configuration.
    if config.kernel().is_empty() {
        eprintln!("No kernel has been configured in {path}.");
        return Status::ABORTED;
    }

    // Open the kernel.
    let kernel = match root.open(config.kernel(), FileModes::READ, FileAttributes::empty()) {
        Ok(v) => v,
        Err(e) => {
            eprintln!("Cannot open Linux kernel from {}: {}.", config.kernel(), e);
            return Status::ABORTED;
        }
    };

    // Check if initrd has been specified in the configuration.
    if config.initrd().is_empty() {
        eprintln!("No initrd has been configured in {path}.");
        return Status::ABORTED;
    }

    // Open all initrd.
    let mut initrds = Vec::with_capacity(config.initrd().len());

    for path in config.initrd() {
        match root.open(path, FileModes::READ, FileAttributes::empty()) {
            Ok(v) => initrds.push(v),
            Err(e) => {
                eprintln!("Cannot open initrd {path}: {e}.");
                return Status::ABORTED;
            }
        }
    }

    // Get command line.
    let cmd_line = match config.command_line() {
        "" => {
            eprintln!("No command line has been configured in {path}.");
            return Status::ABORTED;
        }
        v => v,
    };

    // Boot.
    boot(kernel, initrds, cmd_line)
}

#[cfg(not(test))]
#[panic_handler]
fn panic_handler(info: &core::panic::PanicInfo) -> ! {
    eprintln!("{info}");
    loop {}
}

#[cfg(not(test))]
#[global_allocator]
static ALLOCATOR: efi::PoolAllocator = efi::PoolAllocator;
