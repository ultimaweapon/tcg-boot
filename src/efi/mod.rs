pub use self::alloc::*;
pub use self::boot::*;
pub use self::console::*;
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

mod alloc;
mod boot;
mod console;
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

/// # Safety
/// This must be called as the first thing in the `efi_main`.
pub unsafe fn init(im: &'static Image, st: &'static SystemTable) {
    SystemTable::set_current(st);
    Image::set_current(im);

    if st.hdr().revision() < TableRevision::new(1, 1) {
        panic!("UEFI version is too old to run {}", im.proto().file_path());
    }
}
