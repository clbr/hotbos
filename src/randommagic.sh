#!/bin/sh -f

IFS="
"

for line in `cat emptymagic.h`; do

	if `echo $line | grep -q define`; then
		echo $line | grep -q MAGIC && echo "$line" && continue
		echo -n "`echo $line | cut -d"." -f-1`."
		echo $RANDOM
	else
		echo "$line"
	fi
done
