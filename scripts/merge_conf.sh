#!/bin/sh
# merge_conf.sh
# Param:
#  $1 : old config file
#  $2 : new config file
#
# This program will merge two Nezumi config files, getting comments and options
# from the new config file, but getting option values from the old config file.
#
# Written by MagicalTux <MagicalTux@ookoo.org>
#
# $Id: merge_conf.sh 28 2006-01-17 21:09:11Z MagicalTux $

cat "$2" | while read line; do
	foo=`echo "$line" | sed -e 's/\/\/.*//'`
	if [ x"$foo" = x ]; then
		# empty line or comment
		echo "$line"
		continue
	fi
	option_name=`echo "$line" | sed -e 's/:.*//'`
	option_new_value=`echo "$line" | sed -e 's/.*: *//'`
	old_line=`cat "$1" | grep "^$option_name *:"`
	option_old_value=`echo "$old_line" | sed -e 's/.*: *//'`

	if [ x"$old_line" = x ]; then
		echo "$option_name: $option_new_value"
	else
		echo "$option_name: $option_old_value"
	fi
done


