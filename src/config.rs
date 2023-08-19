use crate::efi::{EfiChar, EfiStr, EfiString, File, Status};
use alloc::borrow::{Cow, ToOwned};
use alloc::string::String;
use alloc::vec;
use alloc::vec::Vec;
use core::fmt::{Display, Formatter};

/// User-provided configurations for TCG Loader.
pub struct Config {
    kernel: Cow<'static, EfiStr>,
    command_line: String,
    initrd: Vec<EfiString>,
}

impl Config {
    pub fn load(file: &File) -> Result<Self, LoadError> {
        // Get file size.
        let info = match file.info() {
            Ok(v) => v,
            Err(e) => return Err(LoadError::GetFileInfoFailed(e)),
        };

        // Read the file.
        let mut data = vec![0u8; info.file_size().try_into().unwrap()];
        let data = match file.read(&mut data) {
            Ok(v) => &data[..v],
            Err(e) => return Err(LoadError::ReadFileFailed(e)),
        };

        // Parse the file.
        let mut line = 1;
        let mut start = 0;
        let mut key: Option<(usize, usize)> = None;
        let mut conf = Self {
            kernel: Cow::Borrowed(EfiStr::EMPTY),
            command_line: String::new(),
            initrd: Vec::new(),
        };

        for i in 0..data.len() {
            let ch = data[i];

            // Skip until we found a separator.
            match ch {
                b'=' => {
                    if key.is_some() {
                        continue;
                    }
                }
                b'\n' => {}
                _ => continue,
            }

            // Process the part.
            match &key {
                Some((ko, kl)) => {
                    conf.parse(&data[*ko..(*ko + *kl)], &data[start..i], line)?;
                    key = None;
                }
                None => {
                    let len = i - start;

                    if len != 0 {
                        if ch != b'=' {
                            return Err(LoadError::SyntaxError(line));
                        }

                        key = Some((start, len));
                    } else if len == 0 {
                        if ch == b'=' {
                            // Line begin with "=".
                            return Err(LoadError::SyntaxError(line));
                        }
                    }
                }
            }

            // Move to next part.
            if ch == b'\n' {
                line += 1;
            }

            start = i + 1;
        }

        // Parse the last component if the last line does not ended with LF.

        Ok(conf)
    }

    pub fn kernel(&self) -> &EfiStr {
        self.kernel.as_ref()
    }

    pub fn command_line(&self) -> &str {
        self.command_line.as_ref()
    }

    pub fn initrd(&self) -> &[EfiString] {
        self.initrd.as_ref()
    }

    fn parse(&mut self, key: &[u8], val: &[u8], line: usize) -> Result<(), LoadError> {
        match key {
            b"kernel" => self.kernel = Cow::Owned(Self::parse_path(val)),
            b"command_line" => self.command_line = core::str::from_utf8(val).unwrap().to_owned(),
            b"initrd" => self.initrd.push(Self::parse_path(val)),
            v => {
                return Err(LoadError::UnknownKey(
                    String::from_utf8_lossy(v).into_owned(),
                    line,
                ));
            }
        }

        Ok(())
    }

    fn parse_path(val: &[u8]) -> EfiString {
        // Replace any / with \.
        let mut path = EfiString::from_bytes(val).unwrap();

        for ch in &mut path {
            if *ch == b'/' {
                *ch = unsafe { EfiChar::from_u8_unchecked(b'\\') };
            }
        }

        path
    }
}

/// Represents an error when configuration reading is failed.
pub enum LoadError {
    GetFileInfoFailed(Status),
    ReadFileFailed(Status),
    SyntaxError(usize),
    UnknownKey(String, usize),
}

impl Display for LoadError {
    fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
        match self {
            Self::GetFileInfoFailed(e) => write!(f, "cannot get file information -> {e}"),
            Self::ReadFileFailed(e) => write!(f, "cannot read the file -> {e}"),
            Self::SyntaxError(l) => write!(f, "syntax error at line {l}"),
            Self::UnknownKey(k, l) => write!(f, "unknown configuration '{k}' at line {l}"),
        }
    }
}
