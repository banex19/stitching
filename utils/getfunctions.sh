#!/bin/bash

if [ -z "$1" ]
  then
    echo "Usage: getfunctions.sh </path/to/root/src"
    exit
fi

outFile="$1/internalFunctions.cache"

echo -n "" > "$outFile"

echo "Output will be stored in: $outFile"

for i in `find $1 -name '*.cpp' -o -name '*.c' -o -name '*.cc'` ; do 
#for i in `find $1  -name '*.h' ` ; do 
	echo $i ; 

	clang -Xclang -ast-dump -fsyntax-only "$i" | sed -r "s/\x1B\[(([0-9]+)(;[0-9]+)*)?[m,K,H,f,J]//g" \
	| grep "FunctionDecl" | grep -v "extern" | grep -v "implicit" | grep " [a-zA-Z_1-9]* '" -o  | grep -o ".* " | tr -d "[:blank:]" \
	>> "$outFile"
 done