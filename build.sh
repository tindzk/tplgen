#!/bin/sh

mkdir -p build/{Jivai,tplgen}

../Jivai-Utils/jutils.bin        \
	build                        \
	output=tplgen.bin            \
	main=src/Main.c              \
	include=../Jivai/src         \
	map=src:build/tplgen         \
	map=../Jivai/src:build/Jivai \
	optimlevel=0                 \
	dbgsym=yes                   \
	inclhdr=config.h             \
	link=@bfd
