/// Represents an `EFI_EVENT`.
#[repr(transparent)]
#[derive(Clone, Copy)]
pub struct Event(usize);
