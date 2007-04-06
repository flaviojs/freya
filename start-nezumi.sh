#!/bin/sh
# Nezumi Start Script
# Starts nezumi, stops it, etc...
# $Id: start-nezumi.sh 630 2006-06-05 07:58:18Z MagicalTux $
#
# This script uses itself as a daemon to make sure
# no server stops. It will check every 10 secs that
# each server is running by sending a "SIGUSR1" to
# the server.
#

# Change this value to reflect your installation path if the resident can't locate Nezumi
NEZUMI_PATH=.

# Check our directory
if [ ! -f "$NEZUMI_PATH/login-server" ]; then
	# login-server isn't with us, try to locate it in the path found in $0
	POSSIBLE_PATH=`dirname "$0"`
	if [ ! -f "$POSSIBLE_PATH/login_server" ]; then
		echo "Server not found, make sure that $0 is ran within Nezumi directory."
		exit 1
	fi
	NEZUMI_PATH="$POSSIBLE_PATH"
fi

# CONFIG
NEZUMI_BIN="login-server char-server map-server"
UPDATE_STATUS_TIMER=60
PIDFILE_PATH="$NEZUMI_PATH/save/agent.pid"
STFILE_PATH="$NEZUMI_PATH/save/agent_status.txt"
LOGFILE_PATH="$NEZUMI_PATH/log/agent.log"
DAEMON_STOP_TIMEOUT="2s"

nezumi_start() {
	if [ -f "$PIDFILE_PATH" ]; then
		if kill -0 `cat "$PIDFILE_PATH"` 2>/dev/null; then
			echo "Error: resident seems to be already running. Erase $PIDFILE_PATH if it's not the case."
			exit 1
		fi
		# stale pidfile
		rm -f "$PIDFILE_PATH"
	fi
	echo -n "Starting Nezumi resident..."
	nezumi_resident &
	PID="$!"
	echo "$PID" >"$PIDFILE_PATH"
	echo " started (pid: $PID)"
}

nezumi_check() {
	if [ -f "$PIDFILE_PATH" ]; then
		if kill -0 `cat "$PIDFILE_PATH"` 2>/dev/null; then
			# already running, nothing to do
			exit 0
		fi
		# stale pidfile
		rm -f "$PIDFILE_PATH"
	fi
	# start nezumi resident...
	nezumi_resident &
	PID="$!"
	echo "$PID" >"$PIDFILE_PATH"
}

nezumi_restart() {
	if [ -f "$PIDFILE_PATH" ]; then
		if kill -USR2 `cat "$PIDFILE_PATH"` 2>/dev/null; then
			echo "Restart signal sent. Server is restarting..."
			exit
		fi
	fi
	echo "Failed to send Restart signal. Try to start the server before restarting it..."
	exit 1
}

nezumi_stop() {
	if [ -f "$PIDFILE_PATH" ]; then
		PID=`cat "$PIDFILE_PATH"`
		if kill -TERM $PID 2>/dev/null; then
			echo -n "Please wait while stopping Nezumi..."
			TIMEOUT=15
			while true; do
				sleep 1s
				if kill -0 $PID 2>/dev/null; then
					# still running -> wait
					if [ $TIMEOUT -lt 1 ]; then
						echo "FAILED"
						echo "Stopping of Nezumi timed out. Please check ps"
						exit 1
					fi
				else
					break
				fi
				TIMEOUT=$[ $TIMEOUT - 1 ]
			done
			echo "OK"
			exit
		fi
	fi
	echo "Could not stop Nezumi. It's most likely that Nezumi isn't running currently."
	exit 1
}

nezumi_res_log() {
	echo "["`date`"] $@" >>"$LOGFILE_PATH"
}

nezumi_status() {
	DAEMON=`cat "$STFILE_PATH"`
	NOW=`date +%s`
	echo -e "Name\t\tState\tPID\tUptime"
	for foo in $DAEMON; do
		D_NAME=`echo "$foo" | cut -d'|' -f1`
		D_START=`echo "$foo" | cut -d'|' -f2`
		D_PID=`echo "$foo" | cut -d'|' -f3`
		if kill -0 $D_PID 2>/dev/null; then
			D_RUNNING="up"
		else
			D_RUNNING="down"
		fi
		UPTIME=$[ $NOW - $D_START ]
		echo -e "$D_NAME\t$D_RUNNING\t$D_PID\t${UPTIME}s"
	done
}

nezumi_res_stop() {
	nezumi_res_log "Ending signal received, stopping daemons..."
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
		nezumi_res_log "Stopping $D_NAME after an uptime of $UPTIME secs"
		if kill -TERM $D_PID 2>/dev/null; then
			sleep "$DAEMON_STOP_TIMEOUT"
			if kill -KILL $D_PID 2>/dev/null; then
				nezumi_res_log "Daemon wasn't stopped - killed with SIGKILL"
			fi
		fi
	done
	rm -f "$PIDFILE_PATH"
	exit 0
}

nezumi_res_restart() {
	nezumi_res_log "Restarting all NEZUMI servers..."
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
		nezumi_res_log "Stopping $D_NAME after an uptime of $UPTIME secs"
		if kill -TERM $D_PID 2>/dev/null; then
			sleep "$DAEMON_STOP_TIMEOUT"
			if kill -KILL $D_PID 2>/dev/null; then
				nezumi_res_log "Daemon wasn't stopped - killed with SIGKILL"
			fi
		fi
	done
	# reset DAEMON to avoid warning messages...
	DAEMON=""
	for foo in $NEZUMI_BIN; do
		DAEMON="$DAEMON ${foo}|0|"
	done
}

nezumi_resident() {
	# Nezumi Resident System
	cd $NEZUMI_PATH
	DAEMON=""
	# close stdin, stdout and stderr
	exec 0</dev/null
	exec 1>/dev/null
	exec 2>/dev/null
	nezumi_res_log "Nezumi Resident v1.0 by MagicalTux <MagicalTux@ooKoo.org>"
	trap nezumi_res_stop SIGINT
	trap nezumi_res_stop SIGTERM
	trap nezumi_res_restart SIGUSR2
	
	for foo in $NEZUMI_BIN; do
		if [ -x $foo ]; then
			DAEMON="$DAEMON ${foo}|0|"
		else
			echo "[ERROR] Missing binary : $foo"
			exit 1
		fi
	done
	# save start time for uptime statistics...
	RESIDENT=`date +%s`
	ALIVE_STATUS=$[ $RESIDENT - 60 ]
	# main loop : check daemons every 10 secs
	while true; do
		NEW_DAEMON=""
		NOW=`date +%s`
		ACT_UPDATE_STATUS=no
		
		if [ $NOW -gt $ALIVE_STATUS ]; then
			ALIVE_STATUS=$[ $NOW + $UPDATE_STATUS_TIMER ]
			ACT_UPDATE_STATUS="yes"
		fi

		for foo in $DAEMON; do
			D_NAME=`echo "$foo" | cut -d'|' -f1`
			D_START=`echo "$foo" | cut -d'|' -f2`
			D_PID=`echo "$foo" | cut -d'|' -f3`
			UPTIME=$[ $NOW - $D_START ]
			if [ x$D_PID = x ]; then
				# start daemon !
				nezumi_res_log "Starting $D_NAME ..."
				./$D_NAME >/dev/null 2>&1 &
				D_PID=$!
				D_START=`date +%s`
				UPTIME=0
			else
				# check if it's still running !
				if kill -0 $D_PID 2>/dev/null; then
					# ok, good !
					:
				else
					# argh !!
					nezumi_res_log "Warning: $D_NAME down after $UPTIME secs - restarting !"
					./$D_NAME >/dev/null 2>&1 &
					D_PID=$!
					D_START=$NOW
					UPTIME=0
				fi
			fi
			NEW_DAEMON="$NEW_DAEMON ${D_NAME}|${D_START}|${D_PID}"
		done
		# copy & strip spaces
		DAEMON=`echo $NEW_DAEMON`
		if [ x"$ACT_UPDATE_STATUS" != x"no" ]; then
			echo "$DAEMON" >"$STFILE_PATH"
		fi
		# sleep a bit before re-running the tests....
		sleep 10s
	done
}

case "$1" in
start)
	nezumi_start
	;;
restart)
	nezumi_restart
	;;
stop)
	nezumi_stop
	;;
status)
	nezumi_status
	;;
resident)
	nezumi_resident
	;;
check)
	nezumi_check
	;;
*)
	echo "Usage: $0 ( start | stop | restart | status | check | resident )"
	exit 1
	;;
esac
exit 0

