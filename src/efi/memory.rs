use super::{MemoryDescriptor, Status, SystemTable};
use alloc::vec::Vec;

pub const PAGE_SIZE: usize = 4096; // Getting from EFI_BOOT_SERVICES.FreePages() specs.

/// Gets how many pages required for a specified number of bytes.
pub fn page_count(bytes: usize) -> usize {
    (bytes / PAGE_SIZE) + if bytes % PAGE_SIZE == 0 { 0 } else { 1 }
}

/// Just a shortcut to [`super::BootServices::get_memory_map()`]. Do not discard the returned map if
/// you want the key to use with [`super::BootServices::exit_boot_services()`].
pub fn get_memory_map() -> Result<(Vec<MemoryDescriptor>, usize), Status> {
    SystemTable::current().boot_services().get_memory_map()
}
