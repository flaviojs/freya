#!/bin/sh

dir=`dirname "$0"`
if [ x"$dir" != x ]; then cd "$dir"; fi

if [ $# = 0 ]; then
	libs=`cat *.dep 2>/dev/null`
	final_echo=yes
else
	libs="$1"
	final_echo=no
fi

for lib in $libs; do
	case "$lib" in
		*.la)
			. "$lib"
			lib="$libdir/$old_library $dependency_libs"
			./get_deps.sh "$lib"
			continue
		#
	esac
	echo -n "$lib "
done

if [ "$final_echo" = yes ]; then echo; fi

