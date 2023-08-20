use crate::{Event, Status, SystemTable};
use alloc::vec::Vec;

/// Prints to the standard output, with a newline.
#[macro_export]
macro_rules! println {
    ($($arg:tt)*) => {{
        use $crate::print;
        use ::alloc::string::String;
        use ::core::fmt::Write;

        let mut buf = String::new();
        write!(buf, $($arg)*).unwrap();
        write!(buf, "\r\n").unwrap();
        print(buf);
    }};
}

/// Prints to the standard error, with a newline.
#[macro_export]
macro_rules! eprintln {
    ($($arg:tt)*) => {{
        use $crate::eprint;
        use ::alloc::string::String;
        use ::core::fmt::Write;

        let mut buf = String::new();
        write!(buf, $($arg)*).unwrap();
        write!(buf, "\r\n").unwrap();
        eprint(buf);
    }};
}

/// Wait for a key stroke.
pub fn pause() {
    let st = SystemTable::current();
    let bs = st.boot_services();
    let stdin = st.stdin();

    bs.wait_for_event(&[stdin.key_event()]).unwrap();
}

/// Prints to the standard output.
pub fn print<S: AsRef<str>>(s: S) {
    print_to(s, SystemTable::current().stdout());
}

/// Prints to the standard error.
pub fn eprint<S: AsRef<str>>(s: S) {
    print_to(s, SystemTable::current().stderr());
}

fn print_to<S: AsRef<str>>(s: S, d: &SimpleTextOutput) {
    // Get UTF-16.
    let mut s: Vec<u16> = s.as_ref().encode_utf16().collect();

    s.push(0); // null-terminate.

    // SAFETY: This is safe because s has null-terminated by the above statement.
    unsafe { d.write(s.as_ptr()).unwrap() };
}

/// Represents an `EFI_SIMPLE_TEXT_INPUT_PROTOCOL`.
#[repr(C)]
pub struct SimpleTextInput {
    reset: fn(),
    read_key_stroke: fn(),
    wait_for_key: Event,
}

impl SimpleTextInput {
    pub fn key_event(&self) -> Event {
        self.wait_for_key
    }
}

/// Represents an `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL`.
#[repr(C)]
pub struct SimpleTextOutput {
    reset: fn(),
    output_string: unsafe extern "efiapi" fn(&Self, s: *const u16) -> Status,
}

impl SimpleTextOutput {
    /// # Safety
    /// `s` must be null-terminated.
    pub unsafe fn write(&self, s: *const u16) -> Result<(), Status> {
        let s = (self.output_string)(self, s);

        if s != Status::SUCCESS {
            Err(s)
        } else {
            Ok(())
        }
    }
}
