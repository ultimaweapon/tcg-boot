use alloc::borrow::ToOwned;
use alloc::string::String;
use alloc::vec::Vec;
use core::borrow::Borrow;
use core::fmt::{Display, Formatter};
use core::mem::transmute;
use core::slice::{from_raw_parts, IterMut};

/// A borrowed EFI string. The string is always have NUL at the end.
#[repr(transparent)]
pub struct EfiStr([u16]);

impl EfiStr {
    pub const EMPTY: &Self = unsafe { Self::new_unchecked(&[0]) };

    /// # Safety
    /// `data` must be NUL-terminated and must not have any NULs in the middle.
    pub const unsafe fn new_unchecked(data: &[u16]) -> &Self {
        // SAFETY: This is safe because EfiStr is #[repr(transparent)].
        &*(data as *const [u16] as *const Self)
    }

    /// # Safety
    /// `ptr` must be valid and NUL-terminated.
    pub unsafe fn from_ptr<'a>(ptr: *const u16) -> &'a Self {
        let mut len = 0;

        while *ptr.add(len) != 0 {
            len += 1;
        }

        Self::new_unchecked(from_raw_parts(ptr, len + 1))
    }

    pub const fn as_ptr(&self) -> *const u16 {
        self.0.as_ptr()
    }

    pub const fn len(&self) -> usize {
        self.0.len() - 1
    }

    pub const fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

impl AsRef<EfiStr> for EfiStr {
    fn as_ref(&self) -> &EfiStr {
        self
    }
}

impl AsRef<[u8]> for EfiStr {
    fn as_ref(&self) -> &[u8] {
        let ptr = self.0.as_ptr().cast();
        let len = self.0.len() * 2;

        // SAFETY: This is safe because any alignment of u16 is a valid alignment fo u8.
        unsafe { from_raw_parts(ptr, len) }
    }
}

impl AsRef<[u16]> for EfiStr {
    fn as_ref(&self) -> &[u16] {
        &self.0
    }
}

impl ToOwned for EfiStr {
    type Owned = EfiString;

    fn to_owned(&self) -> Self::Owned {
        EfiString(self.0.to_vec())
    }
}

impl Display for EfiStr {
    fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
        let d = &self.0[..(self.0.len() - 1)]; // Exclude NUL.
        let s = String::from_utf16_lossy(d);

        f.write_str(&s)
    }
}

/// An owned version of [`EfiStr`]. The string is always have NUL at the end.
pub struct EfiString(Vec<u16>);

impl EfiString {
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, NulError> {
        // Trim NUL terminated if available.
        let bytes = match bytes.iter().position(|&b| b == 0) {
            Some(v) => {
                if v != (bytes.len() - 1) {
                    return Err(NulError { position: v });
                }

                &bytes[..v]
            }
            None => bytes,
        };

        // Map to u16 slice.
        let mut data = Vec::with_capacity(bytes.len() + 1);

        for &b in bytes {
            data.push(b as u16);
        }

        data.push(0);

        Ok(Self(data))
    }

    pub fn push_str<S: AsRef<str>>(&mut self, v: S) {
        self.0.pop();
        self.0.extend(v.as_ref().encode_utf16());
        self.0.push(0);
    }
}

impl AsRef<EfiStr> for EfiString {
    fn as_ref(&self) -> &EfiStr {
        self.borrow()
    }
}

impl<'a> IntoIterator for &'a mut EfiString {
    type Item = &'a mut EfiChar;
    type IntoIter = IterMut<'a, EfiChar>;

    fn into_iter(self) -> Self::IntoIter {
        let len = self.0.len() - 1; // Exclude NUL.

        // SAFETY: This is safe because EfiChar is #[repr(transparent)].
        unsafe { transmute(self.0[..len].iter_mut()) }
    }
}

impl Borrow<EfiStr> for EfiString {
    fn borrow(&self) -> &EfiStr {
        // SAFETY: This is safe because the data is null-terminated.
        unsafe { EfiStr::new_unchecked(&self.0) }
    }
}

impl Display for EfiString {
    fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
        self.as_ref().fmt(f)
    }
}

/// A non-NUL character in the EFI string.
#[repr(transparent)]
pub struct EfiChar(u16);

impl EfiChar {
    /// # Safety
    /// `b` must not be zero.
    pub const unsafe fn from_u8_unchecked(b: u8) -> Self {
        Self(b as u16)
    }
}

impl PartialEq<u8> for EfiChar {
    fn eq(&self, other: &u8) -> bool {
        self.0 == (*other).into()
    }
}

/// Represents an error when NUL are presents in the EFI string.
#[derive(Debug)]
pub struct NulError {
    position: usize,
}
