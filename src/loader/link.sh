#!/bin/sh
set -e

# get arguments
linker=$1
output=$2
arch=$3

if [ "$linker" = "" ] || [ "$output" = "" ] || [ "$arch" = "" ]; then
  echo "usage: $0 LINKER OUTPUT ARCH [ARG]..." >&2
  exit 1
fi

shift
shift
shift

intermediate="${output%.*}.so"

# invoke linker
$linker -nostdlib \
        -shared \
        -Wl,-Bsymbolic \
        -T /usr/lib/elf_${arch}_efi.lds \
        -o $intermediate \
        /usr/lib/crt0-efi-${arch}.o \
        "$@"

# convert binary
objcopy -j .text \
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
