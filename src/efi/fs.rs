use super::{EfiStr, Guid, Owned, Status, Time};
use alloc::alloc::{alloc, dealloc, handle_alloc_error};
use bitflags::bitflags;
use core::alloc::Layout;
use core::mem::{size_of, transmute};
use core::ptr::null;

/// Represents an `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL`.
#[repr(C)]
pub struct SimpleFileSystem {
    revision: u64,
    open_volume: unsafe extern "efiapi" fn(&Self, root: *mut *const File) -> Status,
}

impl SimpleFileSystem {
    pub const ID: Guid = Guid::new(
        0x0964e5b22,
        0x6459,
        0x11d2,
        [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
    );

    /// Opens the root directory on a volume.
    pub fn open(&self) -> Result<Owned<File>, Status> {
        let mut root = null();
        let status = unsafe { (self.open_volume)(self, &mut root) };

        if status != Status::SUCCESS {
            Err(status)
        } else {
            Ok(unsafe { Owned::new(root, File::dtor) })
        }
    }
}

/// Represents an `EFI_FILE_PROTOCOL`.
#[repr(C)]
pub struct File {
    revision: u64,
    open: unsafe extern "efiapi" fn(
        &Self,
        out: *mut *const Self,
        name: *const u16,
        modes: FileModes,
        attrs: FileAttributes,
    ) -> Status,
    close: unsafe extern "efiapi" fn(*const Self) -> Status,
    delete: fn(),
    read: unsafe extern "efiapi" fn(&Self, size: *mut usize, buf: *mut u8) -> Status,
    write: fn(),
    get_position: fn(),
    set_position: extern "efiapi" fn(&Self, u64) -> Status,
    get_info:
        unsafe extern "efiapi" fn(&Self, ty: *const Guid, len: *mut usize, buf: *mut u8) -> Status,
}

impl File {
    /// Opens a new file relative to the current file's location.
    pub fn open<N: AsRef<EfiStr>>(
        &self,
        name: N,
        modes: FileModes,
        attrs: FileAttributes,
    ) -> Result<Owned<Self>, Status> {
        let mut out = null();
        let status = unsafe { (self.open)(self, &mut out, name.as_ref().as_ptr(), modes, attrs) };

        if status != Status::SUCCESS {
            Err(status)
        } else {
            Ok(unsafe { Owned::new(out, Self::dtor) })
        }
    }

    /// Reads data from the file.
    pub fn read(&self, buf: &mut [u8]) -> Result<usize, Status> {
        let mut len = buf.len();
        let status = unsafe { (self.read)(&self, &mut len, buf.as_mut_ptr()) };

        if status != Status::SUCCESS {
            Err(status)
        } else {
            Ok(len)
        }
    }

    /// Sets a file's current position.
    pub fn set_position(&self, position: u64) -> Result<(), Status> {
        let status = (self.set_position)(self, position);

        if status != Status::SUCCESS {
            Err(status)
        } else {
            Ok(())
        }
    }

    pub fn info(&self) -> Result<Owned<FileInfo>, Status> {
        static ID: Guid = Guid::new(
            0x09576e92,
            0x6d3f,
            0x11d2,
            [0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b],
        );

        // Get the initial memory layout.
        let mut layout = Layout::new::<FileInfo>()
            .extend(Layout::array::<u16>(1).unwrap())
            .unwrap()
            .0
            .pad_to_align();

        // Try until buffer is enought.
        let info: Owned<FileInfo> = loop {
            // Allocate a buffer.
            let mut len = layout.size();
            let info = unsafe { alloc(layout) };

            if info.is_null() {
                handle_alloc_error(layout);
            }

            // Get info.
            let status = unsafe { (self.get_info)(self, &ID, &mut len, info) };

            if status == Status::SUCCESS {
                break unsafe { Owned::new(info as _, move |i| dealloc(transmute(i), layout)) };
            }

            // Check if we need to try again.
            unsafe { dealloc(info, layout) };

            if status != Status::BUFFER_TOO_SMALL {
                return Err(status);
            }

            // Update memory layout and try again.
            layout = Layout::new::<FileInfo>()
                .extend(Layout::array::<u16>((len - size_of::<FileInfo>()) / 2).unwrap())
                .unwrap()
                .0
                .pad_to_align();
        };

        Ok(info)
    }

    fn dtor(f: *const Self) {
        unsafe { assert_eq!(((*f).close)(f), Status::SUCCESS) };
    }
}

bitflags! {
    /// Flags to control how [`File::open()`] open the file.
    #[repr(transparent)]
    pub struct FileModes: u64 {
        const READ = 0x0000000000000001;
    }
}

bitflags! {
    /// Attributes of the file to create.
    #[repr(transparent)]
    pub struct FileAttributes: u64 {}
}

/// Represents an `EFI_FILE_INFO`.
#[repr(C)]
pub struct FileInfo {
    size: u64,
    file_size: u64,
    physical_size: u64,
    create_time: Time,
    last_access_time: Time,
    modification_time: Time,
    attribute: u64,
    file_name: [u16; 0],
}

impl FileInfo {
    pub fn file_size(&self) -> u64 {
        self.file_size
    }

    pub fn file_name(&self) -> &EfiStr {
        unsafe { EfiStr::from_ptr(self.file_name.as_ptr()) }
    }
}
