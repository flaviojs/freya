#!/bin/bash
# $Id: freya_ais.sh 473 2005-04-07 19:48:02Z Yor $
# Automatic Installation Script [AIS]
# Idea & concept : MagicalTux <MagicalTux@FF.st>
#

# Various default config

# REQUIRED_BINARIES contains list of binaries required to be in $PATH
# in order to be able to compile Freya
REQUIRED_BINARIES="svn gcc make sed"

# Default installation path . If it is not quoted, sh will automatically
# expend it to $HOME
INSTALL_TO_DEFAULT=~

# Default version to compile. Not used yet :)
VERSION_DEFAULT=txt

# Shall we compile a "debug" version ?
DEBUG=yes

### END OF CONFIG

# Force LANG to C to allow reliable compile-bug reports
# (we do not need error messages in [put any strange language here])
LANG=C

echo "--------------------------------------------------------------"
echo "              (c)2004-2006 Freya team presents :              "
echo "                 ___   ___    ___   _  _   __                 "
echo "                (  _) (  ,)  (  _) ( \/ ) (  )                "
echo "                (  _)  )  \   ) _)  \  /  /__\                "
echo "                (_)   (_)\_) (___) (__/  (_)(_)               "
echo "                    http://www.ro-freya.net                   "
echo "--------------------------------------------------------------"

echo "Automatic Installation Script [AIS] made by MagicalTux <MagicalTux@FF.st>"
echo -n "Checking system..."
for bin in $REQUIRED_BINARIES; do
	echo -n "$bin "
	if [ `which $bin` = "" ]; then
		echo "missing !"
		echo "Please install $bin on your system and retry !"
		exit 1
	fi
done
echo "ok"
echo "I'm ready to download and install freya on your system."
echo -n "What version do you want to compile ? [T]xt/[S]ql/[C]ancel"

VERSION="none"

while [ "$VERSION" = "none" ]; do
	read -n1 -r -s ANSWER
	if [ "$ANSWER" = "C" ]; then ANSWER="c"; fi
	if [ "$ANSWER" = "S" ]; then ANSWER="s"; fi
	if [ "$ANSWER" = "T" ]; then ANSWER="t"; fi
	if [ "$ANSWER" = "c" ]; then
		echo
		echo "Installation cancelled by user request."
		exit
	fi
	if [ "$ANSWER" = "t" ]; then
		echo
		echo "Configuring installation for TEXT mode."
		VERSION="txt"
	fi
	if [ "$ANSWER" = "s" ]; then
		echo
		echo "Configuring installation for SQL mode."
		VERSION="sql"
	fi
done

# Ok, let's choose a path to download it
echo "Choose a directory to install Freya or press enter for \`$INSTALL_TO_DEFAULT'"
echo "Please note that a directory named \`freya' will be created"
read INSTALL_TO
if [ -z "$INSTALL_TO" ]; then
	INSTALL_TO="$INSTALL_TO_DEFAULT"
fi

# let's start !!
if [ -d "$INSTALL_TO" ]; then
	if [ -w "$INSTALL_TO" ]; then
		echo "Installation prefix is \`$INSTALL_TO'"
	else
		echo "Error : the directory \`$INSTALL_TO' is not writable !"
		exit 1
	fi
else
	echo "Error : the directory \`$INSTALL_TO' does not exists !"
	exit 1
fi

cd $INSTALL_TO
mkdir freya 2>/dev/null
cd freya

echo -n "Downloading/updating SVN tree..."
svn co http://svn.ff.st/freya/freya freya_src >svn.log
if [ "$?" != "0" ]; then
	echo "error"
	echo "SVN returned an error code. Please check svn.log for more informations."
	exit 1
fi
echo "ok"

# Let's maintain sourcecode in another directory...
echo -n "Symlinking freya source tree..."
cd freya_src
for foo in *; do
	if [ "$foo" != "save" ]; then
		rm -f "../$foo"
		ln -s "freya_src/$foo" ../
	else
		# for the saves, let's copy the dir ONLY if it does not exist !
		if [ -d "../save" ]; then
			# run any saves-update script here if required...
			echo -n "(keeping saves) "
		else
			# recursive copy
			echo -n "(copying new save dir) "
			cp -R save ../
		fi
	fi
done
echo "ok"

echo -n "Cleaning up sourcecode..."
make clean >make.log 2>make-errors.log
echo "done"

echo -n "Building sourcecode (it may take a while)..."
make $VERSION >>make.log 2>>make-errors.log
RC=$?
if [ "$RC" != "0" ]; then
	echo "error"
	echo "Warning ! I could not compile Freya !!"
	if [ "$VERSION" = "sql" ]; then
		echo "You probably don't have the MySQL header files."
		echo "Please check the Freya forum for help"
	fi
	echo "You may scan make.log and make-errors.log to find out why, or just"
	echo "wait for someone to fix the source."
else
	echo "ok"
	echo "Your Freya is now built :"
	for foo in login-server* char-server* map-server* ladmin*; do
		if [ "$DEBUG" = "no" ]; then
			strip "$foo"
		fi
		rm -f "../$foo"
		ln -s "freya_src/${foo}" ../
		ls -l "$foo"
	done
	echo "Enjoy!"
fi
