#!/bin/sh
# create nezumi makefile and remove svn folders

if [ ! -f configure ]; then
	cd ..
	if [ ! -f configure ]; then
		echo "Unable to detect configuration file !"
		echo "Please run this file on nezumi folder !"
		exit 1
	fi
fi

# get and display current svn revision number
SVN=`cat .svn/entries | grep revision | head -n 1 | sed -e 's/^[^"]*"//;s/"[^"]*\$//'`

echo  "Building release for svn revision $SVN ..."
echo  "Modifying configuration file ..."
# add revision number to configuration file
cat configure | sed 's/^#*RELEASED_SVN=.*$/RELEASED_SVN='"$SVN"'/' >configure.tmp
# this allows to keep modes on file "configure"
cat configure.tmp >configure
rm -f configure.tmp

echo    "Cleaning up release ..."
# remove all ".svn" directories. This will make a lot of output on stderr, so keep it silent
find . -name .svn -type d -exec rm -fr {} \; >/dev/null 2>&1

echo    "Creating makefile ..."
# (re)create clean makefile
cat <<EOF >Makefile
#!make
# Nezumi Makefile

ifeq (\$(MAKE), )
	MAKE=make
endif

.PHONY: txt txt-win32 sql sql-win32 sqlite sqlite-win32

all:
	./configure --choosesql --with-make

txt:
	./configure --with-make

txt-win32:
	./configure --with-win32 --with-make

sql:
	./configure --with-mysql --with-make

sql-win32:
	./configure --with-win32 --with-mysql --with-make

sqlite:
	./configure --with-sqlite --with-make

sqlite-win32:
	./configure --with-win32  --with-sqlite --with-make

EOF

echo "Done"

