# ZFI – Zero-cost and safe interface to UEFI firmware

ZFI is a Rust crate for writing a UEFI application with the following goals:

- Provides the APIs that are almost identical to the UEFI specifications.
- Most APIs are zero-cost abstraction over UEFI API.
- Safe.
- Work on stable Rust.

## Example

```rust
#![no_std]
#![no_main]

use zfi::{eprintln, Image, pause, println, Status, SystemTable};

extern crate alloc;

#[no_mangle]
extern "efiapi" fn efi_main(image: &'static Image, st: &'static SystemTable) -> Status {
    // This is the only place you need to use unsafe. This must be doing immediately and the call
    // order must be exactly like this.
    unsafe {
        SystemTable::set_current(st);
        Image::set_current(image);
    }

    // Any EFI_HANDLE will be represents by a reference to a Rust type (e.g. image here is a type of
    // Image). Each type that represents EFI_HANDLE provides the methods to access any protocols it
    // is capable for (e.g. you can do image.proto() here to get a EFI_LOADED_IMAGE_PROTOCOL from it
    // ). You can download the UEFI specifications for free here: https://uefi.org/specifications
    println!("Hello, world!");
    pause();

    Status::SUCCESS
}

#[cfg(not(test))]
#[panic_handler]
fn panic_handler(info: &core::panic::PanicInfo) -> ! {
    eprintln!("{info}");
    loop {}
}

#[cfg(not(test))]
#[global_allocator]
static ALLOCATOR: zfi::PoolAllocator = zfi:PoolAllocator;
```

To build the above example you need to add a UEFI target to Rust:

```sh
rustup target add x86_64-unknown-uefi
```

Then build with the following command:

```sh
cargo build --target x86_64-unknown-uefi
```

You can grab the EFI file in `target/x86_64-unknown-uefi/debug` and boot it on a compatible machine.

## License

MIT
