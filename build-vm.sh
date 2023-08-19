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

# find esp partition
offset=$(parted -sm "$image" unit B print | awk 'NR >= 3 { split($0, i, ":"); if (i[5] == "fat32") print i[2]; }')

if test x"$offset" = x""; then
  echo "no esp partition in $image" >&2;
  exit 1
fi

esp="${image}@@${offset%?}"

# copy a loader and related files to the disk image
mcopy -o -i "$esp" "$loader" ::EFI/boot/bootx64.efi
mcopy -o -i "$esp" "$config" ::EFI/boot/bootx64.efi.conf
