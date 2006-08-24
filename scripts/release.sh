#!/bin/sh
# This script will erase any .svn directories in order to make this 
# copy of the SVN a release copy.

if [ ! -f configure ]; then
	cd ..
	if [ ! -f configure ]; then
		echo "Please run this script from the Freya SVN !"
		exit 1
	fi
fi

# first, get current SVN number of this directory...
SVN=`cat .svn/entries | grep revision | head -n 1 | sed -e 's/^[^"]*"//;s/"[^"]*\$//'`

echo -n "Building release for SVN#$SVN ..."

# edit configure file to include the release number
cat configure | sed 's/^#*RELEASED_SVN=.*$/RELEASED_SVN='"$SVN"'/' >configure.tmp
# this allows to keep modes on file "configure"
cat configure.tmp >configure
rm -f configure.tmp

# remove all ".svn" directories. This will make a lot of output on stderr, so keep it silent
find . -name .svn -type d -exec rm -fr {} \; >/dev/null 2>&1

# cleanup Makefile content
cat <<EOF >Makefile
#!make
# Empty Makefile ...

ifeq (\$(MAKE), )
	MAKE=make
endif

.PHONY: txt txt-win32 sql-win32 sql

all:
	./configure --choosesql --with-make

txt:
	./configure --with-make

txt-win32:
	./configure --with-win32 --with-make

sql-win32:
	./configure --with-win32 --with-mysql --with-make

sql:
	./configure --with-mysql --with-make

EOF

echo "done"

