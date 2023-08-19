use super::{AllocateType, MemoryType, Status, SystemTable, PAGE_SIZE};
use alloc::boxed::Box;
use core::alloc::{GlobalAlloc, Layout};
use core::mem::size_of;
use core::ops::{Deref, DerefMut};
use core::ptr::{null_mut, read_unaligned, write_unaligned};

/// A shortcut to [`super::BootServices::allocate_pages()`].
pub fn allocate_pages(
    at: AllocateType,
    mt: MemoryType,
    pages: usize,
    addr: u64,
) -> Result<Pages, Status> {
    SystemTable::current()
        .boot_services()
        .allocate_pages(at, mt, pages, addr)
}

/// Encapsulate a pointer to one or more memory pages.
pub struct Pages {
    ptr: *mut u8,
    len: usize, // In bytes.
}

impl Pages {
    /// # Safety
    /// `ptr` must be valid and was allocated with [`super::BootServices::allocate_pages()`].
    pub unsafe fn new(ptr: *mut u8, len: usize) -> Self {
        Self { ptr, len }
    }

    pub fn addr(&self) -> usize {
        self.ptr as _
    }
}

impl Drop for Pages {
    fn drop(&mut self) {
        unsafe {
            SystemTable::current()
                .boot_services()
                .free_pages(self.ptr, self.len / PAGE_SIZE)
                .unwrap()
        };
    }
}

impl Deref for Pages {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        unsafe { core::slice::from_raw_parts(self.ptr, self.len) }
    }
}

impl DerefMut for Pages {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { core::slice::from_raw_parts_mut(self.ptr, self.len) }
    }
}

/// Encapsulate an object that need to be free by some mechanism.
pub struct Owned<T> {
    ptr: *mut T,
    dtor: Option<Box<dyn FnOnce(*mut T)>>,
}

impl<T> Owned<T> {
    /// # Safety
    /// `ptr` must be valid.
    pub unsafe fn new<D>(ptr: *mut T, dtor: D) -> Self
    where
        D: FnOnce(*mut T) + 'static,
    {
        Self {
            ptr,
            dtor: Some(Box::new(dtor)),
        }
    }
}

impl<T> Drop for Owned<T> {
    fn drop(&mut self) {
        self.dtor.take().unwrap()(self.ptr);
    }
}

impl<T> Deref for Owned<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.ptr }
    }
}

impl<T> DerefMut for Owned<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.ptr }
    }
}

impl<T> AsRef<T> for Owned<T> {
    fn as_ref(&self) -> &T {
        &self
    }
}

impl<T> AsMut<T> for Owned<T> {
    fn as_mut(&mut self) -> &mut T {
        self
    }
}

/// An implementation of [`GlobalAlloc`] using EFI memory pool.
pub struct PoolAllocator;

unsafe impl GlobalAlloc for PoolAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        // Calculate allocation size to include a spare room for adjusting alignment.
        let mut size = if layout.align() <= 8 {
            layout.size()
        } else {
            layout.size() + (layout.align() - 8)
        };

        // We will store how many bytes that we have shifted in the beginning at the end.
        size += size_of::<usize>();

        // Do allocation.
        let mem = SystemTable::current()
            .boot_services()
            .allocate_pool(MemoryType::LoaderData, size)
            .unwrap_or(null_mut());

        if mem.is_null() {
            return null_mut();
        }

        // Get number of bytes to shift so the alignment is correct.
        let misaligned = (mem as usize) % layout.align();
        let adjust = if misaligned == 0 {
            0
        } else {
            layout.align() - misaligned
        };

        // Store how many bytes have been shifted.
        let mem = mem.add(adjust);

        write_unaligned(mem.add(layout.size()) as *mut usize, adjust);

        mem
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // Get original address before alignment.
        let adjusted = read_unaligned(ptr.add(layout.size()) as *const usize);
        let ptr = ptr.sub(adjusted);

        // Free the memory.
        SystemTable::current().boot_services().free_pool(ptr);
    }
}
