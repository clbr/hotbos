#!/bin/sh

awk 'prev !~ /swap/ || prev != $0; {prev = $0}'
