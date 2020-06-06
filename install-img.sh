#!/bin/sh -e
image=$1
loader=$2
config=$3

if test x"$image" = x"" || test x"$loader" = x"" || test x"$config" = x""; then
  echo "usage: $0 IMAGE LOADER CONFIG" >&2
  exit 1
fi

# find esp partition
offset=$(parted -sm "$image" unit B print | awk 'NR >= 3 { split($0, i, ":"); if (i[5] == "fat32") print i[2]; }')

if test x"$offset" = x""; then
  echo "no esp partition in $image" >&2;
  exit 1
fi

esp="${image}@@${offset%?}"

# copy a loader and related files to the disk image
mcopy -o -i "$esp" "$loader" ::EFI/boot
mcopy -o -i "$esp" "$config" ::EFI/boot/$(basename "$loader").conf
