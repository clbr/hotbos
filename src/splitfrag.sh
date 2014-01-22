#!/bin/sh -e

rm -rf split
mkdir -p split

for file in *gz; do
	echo Handling $file..

	edges=`zcat $file | grep -n -e -----`

	line64=`echo "$edges" | grep "size 64" | cut -d: -f1`
	line128=`echo "$edges" | grep "size 128" | cut -d: -f1`
	line256=`echo "$edges" | grep "size 256" | cut -d: -f1`
	line384=`echo "$edges" | grep "size 384" | cut -d: -f1`
	line512=`echo "$edges" | grep "size 512" | cut -d: -f1`
	line1024=`echo "$edges" | grep "size 1024" | cut -d: -f1`
	line1536=`echo "$edges" | grep "size 1536" | cut -d: -f1`
	line2048=`echo "$edges" | grep "size 2048" | cut -d: -f1`
	line4096=`echo "$edges" | grep "size 4096" | cut -d: -f1`

	d64=$((line128-line64))
	d128=$((line256-line128))
	d256=$((line384-line256))
	d384=$((line512-line384))
	d512=$((line1024-line512))
	d1024=$((line1536-line1024))
	d1536=$((line2048-line1536))
	d2048=$((line4096-line2048))
	d4096=100000000

	prefix=${file%gz}

	zcat $file | grep "size 64" -A $d64 | gzip -9 > split/${prefix}64
	zcat $file | grep "size 128" -A $d128 | gzip -9 > split/${prefix}128
	zcat $file | grep "size 256" -A $d256 | gzip -9 > split/${prefix}256
	zcat $file | grep "size 384" -A $d384 | gzip -9 > split/${prefix}384
	zcat $file | grep "size 512" -A $d512 | gzip -9 > split/${prefix}512
	zcat $file | grep "size 1024" -A $d1024 | gzip -9 > split/${prefix}1024
	zcat $file | grep "size 1536" -A $d1536 | gzip -9 > split/${prefix}1536
	zcat $file | grep "size 2048" -A $d2048 | gzip -9 > split/${prefix}2048
	zcat $file | grep "size 4096" -A $d4096 | gzip -9 > split/${prefix}4096
done
