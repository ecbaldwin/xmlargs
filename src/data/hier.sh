#!/bin/sh

for file in $(ls $srcdir/data/nodes/$name/? 2>/dev/null); do
  if ! test -f results/run/$(basename $file); then
    exit 1
  fi
done

touch results/run/$name

exit 0
