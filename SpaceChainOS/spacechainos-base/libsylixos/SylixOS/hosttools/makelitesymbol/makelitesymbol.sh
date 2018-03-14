#!/usr/bin/env bash

srcfile=$1
destfile=$2

NM=nm

echo "create $destfile from: $srcfile"

rm -f $destfile 1>/dev/null 2>&1

for i in $srcfile; do 
    # function
    $NM $i | sed -n '/.*/{s/\ [TW]\ /\ /gp}' | sed -n "s/\(^[^ ]*\) \([^:]*\)/\2\ =\ 0x\1;/gp" >>$destfile;
    # obj
    $NM $i | sed -n '/.*/{s/\ [BDRSCVG]\ /\ /gp}' | sed -n "s/\(^[^ ]*\) \([^:]*\)/\2\ =\ 0x\1;/gp" >>$destfile;
done

exit 0
