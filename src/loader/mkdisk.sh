#!/bin/sh
set -e

# get arguments
image=$1
sectors=$2
efi=$3

if test x"$image" = x"" || test x"$sectors" = x"" || test x"$efi" = x""; then
  echo "usage: $0 IMAGE SECTORS EFI" >&2
  exit 1
elif test $sectors -lt 93750; then
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

# copy efi to disk image
mmd -i $part1 ::EFI ::EFI/boot
mcopy -i $part1 $efi ::EFI/boot
