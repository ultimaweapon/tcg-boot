#!/bin/sh
set -e

# get arguments
efi=$1
kernel=$2
image=$3
sectors=$4

if test x"$efi" = x"" ||
   test x"$kernel" = x"" ||
   test x"$image" = x""; then
  echo "usage: $0 EFI KERNEL IMAGE [SECTORS]" >&2
  exit 1
fi

if test x"$sectors" = x""; then
  # TODO: calculate required sectors from total file's size
  sectors=93750
fi

if test $sectors -lt 93750; then
  echo "minimum sectors count is 93750" >&2
  exit 1
fi

# create disk image
dd if=/dev/zero of=$image bs=512 count=$sectors 2> /dev/null

# partition disk image
parted -s $image mktable gpt
parted -s $image mkpart EFI fat32 2048s 100%

part1="${image}@@1048576"

# format partition
size=`parted -sm $image unit s print | awk 'NR == 3 { split($0, i, ":"); print int(512*i[4]/(1024*1024)) }'`

mformat -i $part1 -s 32 -h 64 -t $size -F

# copy efi and related files to disk image
base=`dirname $efi`
conf="$base/default.conf"

mmd -i $part1 ::EFI ::EFI/boot
mcopy -i $part1 $efi ::EFI/boot
mcopy -i $part1 $conf ::EFI/boot/`basename $efi`.conf
mcopy -i $part1 $kernel ::vmlinuz
