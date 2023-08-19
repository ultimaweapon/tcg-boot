#!/bin/sh -e
script=$(readlink -f "$0")
dir=$(dirname "$script")
vm="$dir/vm"
disk="$vm/disk1.img"
log="$vm/tcg.log"

# start qemu
qemu-system-x86_64 \
  -enable-kvm \
  -cpu host \
  -m 1G \
  -machine q35 \
  -drive if=pflash,format=raw,readonly=on,file="$TCGBOOT_OVMF_CODE" \
  -drive if=pflash,format=raw,file="$vm/bios/OVMF_VARS.fd" \
  -drive if=virtio,format=raw,file="$disk" \
  "$@"

# copy log from the vm
esp=$("$vm/esp-part.sh" "$disk")

mcopy -n -o -i "$esp" "::EFI/boot/bootx64.efi.log" "$log"
cat "$log"
