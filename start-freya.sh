#!/bin/sh
# Freya Start Script
# Starts freya, stops it, etc...
# $Id: start-freya.sh 464 2005-10-30 22:27:54Z Yor $
#
# This script uses itself as a daemon to make sure
# no server stops. It will check every 10 secs that
# each server is running by sending a "SIGUSR1" to
# the server.
#

# CONFIG
FREYA_PATH=.
FREYA_BIN="login-server char-server map-server"
ALIVE_TIMEOUT=300

freya_res_log() {
	echo "["`date`"] $@"
}

freya_res_stop() {
	freya_res_log "Ending signal received, stopping daemons..."
	STOP_DAEMON=""
	# reverse order to stop map first
	for foo in $DAEMON; do STOP_DAEMON="$foo $STOP_DAEMON"; done
	NOW=`date +%s`
	for foo in $STOP_DAEMON; do
		D_NAME=`echo "$foo" | cut -d'|' -f1`
		D_START=`echo "$foo" | cut -d'|' -f2`
		D_PID=`echo "$foo" | cut -d'|' -f3`
		if [ x$D_PID = x ]; then continue; fi
		UPTIME=$[ $NOW - $D_START ]
		freya_res_log "Stopping $D_NAME after an uptime of $UPTIME secs"
		if kill -TERM $D_PID 2>/dev/null; then
			sleep 1s
			if kill -KILL $D_PID 2>/dev/null; then
				freya_res_log "Daemon wasn't stopped - killed with SIGKILL"
			fi
		fi
	done
	exit 0
}

freya_res_restart() {
	freya_res_log "Restarting all FREYA servers..."
	STOP_DAEMON=""
}

freya_resident() {
	# Freya Resident System
	cd $FREYA_PATH
	DAEMON=""
	freya_res_log "Freya Resident v1.0 by MagicalTux <MagicalTux@FF.st>"
	trap freya_res_stop SIGINT
	trap freya_res_stop SIGTERM
	trap freya_res_restart SIGUSR2
	
	for foo in $FREYA_BIN; do
		if [ -x $foo ]; then
			DAEMON="$DAEMON ${foo}|0|"
		else
			echo "[ERROR] Missing binary : $foo"
			exit 1
		fi
	done
	# save start time for uptime statistics...
	RESIDENT=`date +%s`
	ALIVE_STATUS=$[ $RESIDENT + $ALIVE_TIMEOUT ]
	# main loop : check daemons every 10 secs
	while true; do
		NEW_DAEMON=""
		NOW=`date +%s`
		DO_ALIVE=no
		
		if [ $NOW -gt $ALIVE_STATUS ]; then
			ALIVE_STATUS=$[ $NOW + $ALIVE_TIMEOUT ]
			DO_ALIVE=""
		fi

		for foo in $DAEMON; do
			D_NAME=`echo "$foo" | cut -d'|' -f1`
			D_START=`echo "$foo" | cut -d'|' -f2`
			D_PID=`echo "$foo" | cut -d'|' -f3`
			UPTIME=$[ $NOW - $D_START ]
			if [ x$D_PID = x ]; then
				# start daemon !
				freya_res_log "Starting $D_NAME ..."
				./$D_NAME >/dev/null 2>&1 &
				D_PID=$!
				D_START=`date +%s`
				UPTIME=0
			else
				# check if it's still running !
				if kill -USR1 $D_PID 2>/dev/null; then
					# ok, good !
					:
				else
					# argh !!
					freya_res_log "Warning: $D_NAME down after $UPTIME secs - restarting !"
					./$D_NAME >/dev/null 2>&1 &
					D_PID=$!
					D_START=$NOW
					UPTIME=0
				fi
			fi
			if [ "x$DO_ALIVE" != xno ]; then
				DO_ALIVE="$DO_ALIVE ${D_NAME}=${UPTIME}s"
			fi
			NEW_DAEMON="$NEW_DAEMON ${D_NAME}|${D_START}|${D_PID}"
		done
		# copy & strip spaces
		DAEMON=`echo $NEW_DAEMON`
		# sleep a bit before re-running the tests....
		if [ "x$DO_ALIVE" != xno ]; then
			freya_res_log "System alive:$DO_ALIVE"
		fi
		sleep 10s
	done
}

case "$1" in
start|stop|restart)
	echo "Not implemented for the moment, come back later!"
	exit 1
	;;
resident)
	freya_resident
	;;
*)
	echo "Usage: $0 ( start | stop | restart | resident )"
	exit 1
	;;
esac
exit 0

