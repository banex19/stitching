#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi

before='/'
after='_'
filename=$(basename $1)
filename=$(echo "$filename" | sed "s/\..*//")
out_file="/lib/cache/$filename.cache"
nm -D $1 | cut -d' ' -f 1,3 > "$out_file"
#objdump $1 -d | grep "<[^@-]*>:" | grep -v "plt" | sed -e 's/<\|>\|://g' > "$out_file"
#echo -e "" >> "$out_file"

