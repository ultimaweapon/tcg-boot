#![no_std]
#![no_main]
#![feature(abi_efiapi)]

extern crate uefi;
extern crate uefi_services;

use uefi::{Handle, Status};
use uefi::prelude::entry;
use uefi::table::{Boot, SystemTable};

#[entry]
fn efi_main(inst: Handle, sys: SystemTable<Boot>) -> Status {
    Status::SUCCESS
}
