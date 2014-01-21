#!/bin/sh

# First round
#awk 'prev !~ /swap/ || prev != $0; {prev = $0}'

# Second round
sed -e 's@Fragments: @@' -e 's@[[:digit:]]$@&, 0@' -e 's@Fragmentation caused a swap@swap@' |
	awk 1 RS="\nswap" ORS="s" | sed 's@0s@1@'
