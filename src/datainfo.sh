#!/bin/sh

export LANG=C

for file in ../data/*.bin; do

	echo ${file##*/}

	first=`./statinfo $file fo | head -n1`
	ms=`./statinfo $file fo | tail -n1 | awk '{print $5}'`
	min=`calc $ms / 60000`
	min=`printf %.1f $min`

	creates=`./statinfo $file fo | grep create | wc -l`
	cpus=`./statinfo $file fo | grep cpu | wc -l`
	reads=`./statinfo $file fo | grep read | wc -l`
	writes=`./statinfo $file fo | grep write | wc -l`
	destroys=`./statinfo $file fo | grep destroy | wc -l`

	s=`calc $ms / 1000`

	createss=`calc $creates / $s`
	cpuss=`calc $cpus / $s`
	readss=`calc $reads / $s`
	writess=`calc $writes / $s`
	destroyss=`calc $destroys / $s`

	echo "${first%\ found}, runtime $ms ms (~$min minutes)"
	echo $creates creates, $cpus cpu ops, $reads reads, $writes writes, $destroys destroys

	echo $createss creates/s, $cpuss cpu ops/s, $readss reads/s, $writess writes/s, $destroyss destroys/s

	echo
done
