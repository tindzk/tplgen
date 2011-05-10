#!/bin/sh

mkdir -p Build/Jivai

../../tplgen/tplgen.bin \
	name=Template       \
	dir=Templates       \
	ext=.html           \
	itf=no              \
	out=Source/Template || exit 1

../../Depend/Depend.bin             \
	build                           \
	output=Main.bin                 \
	main=Source/Main.c              \
	manifest=Manifest.h             \
	include=../..                   \
	include=../../Jivai/src         \
	map=Source:Build                \
	map=../../Jivai/src:Build/Jivai \
	optimlevel=0                    \
	dbgsym=yes                      \
	inclhdr=Config.h                \
	link=@bfd                       \
	link=@dl
