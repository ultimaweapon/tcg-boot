pub use self::alloc::*;
pub use self::boot::*;
pub use self::console::*;
pub use self::debug::*;
pub use self::device::*;
pub use self::event::*;
pub use self::fs::*;
pub use self::guid::*;
pub use self::header::*;
pub use self::image::*;
pub use self::memory::*;
pub use self::path::*;
pub use self::proto::*;
pub use self::runtime::*;
pub use self::status::*;
pub use self::string::*;
pub use self::system::*;
pub use self::time::*;

use core::cell::RefCell;
use core::fmt::Write;

mod alloc;
mod boot;
mod console;
mod debug;
mod device;
mod event;
mod fs;
mod guid;
mod header;
mod image;
mod memory;
mod path;
mod proto;
mod runtime;
mod status;
mod string;
mod system;
mod time;

/// Initializes the ZFI.
///
/// This must be called before using any ZFI API. Usually you should call this right away as the
/// first thing in the `efi_main`. See project README for an example.
///
/// # Safety
/// Calling this function more than once is undefined behavior.
pub unsafe fn init(
    im: &'static Image,
    st: &'static SystemTable,
    debug_writer: Option<fn() -> ::alloc::boxed::Box<dyn Write>>,
) {
    // Initialize foundation.
    SystemTable::set_current(st);
    Image::set_current(im);

    // Check EFI version.
    if st.hdr().revision() < TableRevision::new(1, 1) {
        panic!("UEFI version is too old to run {}", im.proto().file_path());
    }

    // Initialize debug log.
    if let Some(f) = debug_writer {
        self::debug::DEBUG_WRITER = Some(RefCell::new(f()));
    }
}
