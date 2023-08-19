#!/bin/sh -e
args="-r"
loader="target/x86_64-unknown-uefi/release/tcg-boot.efi"
config="vm/tcg.conf"
image="vm/disk1.img"

if test x"$1" = x"debug"; then
  args=""
  loader="target/x86_64-unknown-uefi/debug/tcg-boot.efi"
fi

# build
cargo build --target x86_64-unknown-uefi $args

# copy a loader and related files to the disk image
esp=$(./vm/esp-part.sh "$image")

mcopy -o -i "$esp" "$loader" ::EFI/boot/bootx64.efi
mcopy -o -i "$esp" "$config" ::EFI/boot/bootx64.efi.conf
