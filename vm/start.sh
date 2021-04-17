#!/bin/sh -e
script=$(readlink -f "$0")
dir=$(dirname "$script")

# start qemu
exec qemu-system-x86_64 \
  -enable-kvm \
  -cpu host \
  -m 1G \
  -machine q35 \
  -drive if=pflash,format=raw,readonly,file="$TCGBOOT_OVMF_CODE" \
  -drive if=pflash,format=raw,file="$dir/bios/OVMF_VARS.fd" \
  -drive if=virtio,format=raw,file="$dir/disk1.img" \
  -nographic \
  "$@"
