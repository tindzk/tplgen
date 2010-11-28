#!/bin/sh

mkdir -p Build/{tplgen,Jivai}

../Jivai-Utils/jutils.bin        \
	build                        \
	output=tplgen.bin            \
	main=Source/Main.c           \
	manifest=Manifest.h          \
	include=../Jivai/src         \
	map=Source:Build/tplgen      \
	map=../Jivai/src:Build/Jivai \
	optimlevel=0                 \
	dbgsym=yes                   \
	inclhdr=../Jivai/config.h    \
	link=@bfd