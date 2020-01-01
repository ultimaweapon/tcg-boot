#!/bin/sh
set -e

# get arguments
linker=$1
objcopy=$2
output=$3
arch=$4

if test x$linker = x ||
   test x$objcopy = x ||
   test x$output = x ||
   test x$arch = x; then
  echo "usage: $0 LINKER OBJCOPY OUTPUT ARCH [ARG]..." >&2
  exit 1
fi

i=0; while test $i -lt 4; do
  shift
  i=$(( $i + 1 ))
done

intermediate="${output%.efi}.so"

# invoke linker
$linker -nostdlib \
        -shared \
        -Wl,-Bsymbolic \
        -T /usr/lib/elf_${arch}_efi.lds \
        -o $intermediate \
        /usr/lib/crt0-efi-${arch}.o \
        "$@"

# convert binary
$objcopy -j .text \
         -j .sdata \
         -j .data \
         -j .dynamic \
         -j .dynsym \
         -j .rel \
		     -j .rela \
         -j '.rel.*' \
         -j '.rela.*' \
         -j '.rel*' \
         -j '.rela*' \
		     -j .reloc \
         --target efi-app-$arch \
         $intermediate \
         $output
