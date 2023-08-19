#!/bin/sh -e
image=$1
offset=$(parted -sm "$image" unit B print | awk 'NR >= 3 { split($0, i, ":"); if (i[5] == "fat32") print i[2]; }')

if test x"$offset" = x""; then
  echo "no esp partition in $image" >&2;
  exit 1
fi

echo "${image}@@${offset%?}"
