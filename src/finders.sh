#!/bin/sh -e

out=repre
> $out

for i in `seq 10 10 90` `seq 100 100 5000`; do
	echo Testing ${i}k

	num=$((i * 1000))
	total=`src/quicktrainer --max-entries=$i | grep Total`

	echo $i $total >> $out
done
