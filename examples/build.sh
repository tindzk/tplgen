#!/bin/sh

mkdir -p build/Jivai

../../tplgen/tplgen.bin \
	name=Template       \
	dir=tpl             \
	ext=.html           \
	itf=no              \
	out=src/Template.c

../../Jivai-Utils/jutils.bin        \
	build                           \
	output=Main.bin                 \
	main=src/Main.c                 \
	include=../../Jivai/src         \
	map=src:build                   \
	map=../../Jivai/src:build/Jivai \
	optimlevel=0                    \
	dbgsym=yes                      \
	inclhdr=config.h                \
	link=@bfd
