#!/bin/sh

#
# A hacky script that 
#
# Authors:
# 	Yousef Amar <yousef@amar.io>
#

#if [ $# -lt 1 ]; then
#	echo "usage: $0 [OPTIONS]"
#	exit
#fi

TERMINAL="evilvte"
II_DIR=~/ii
II_USERNAME="paraknight"
II_FULLNAME="Yousef Amar"
PART_MSG="So long, and thanks for all the fish!"

# Default servers and channels
typeset -A SERVERS
SERVERS=(
	[chat.freenode.net]=\#gamedev
)

# Kill child processes on exit
trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

last_date=""

clean () {
	while read line; do
		date=$(echo "$line" | cut -d' ' -f1)
		if [ "$date" != "$last_date" ]; then
			last_date="$date"
			echo "$date"
		fi
		echo "$line" | cut -d' ' -f2-
	done
}

intercept () {
	while read line; do
		cmd=$(echo "$line" | cut -d' ' -f1)
		if [ "$cmd" == "/join" ]; then
			(
				channel=$(echo "$line" | cut -d' ' -f2)
				echo "Connecting to $channel on $server"

				# Wait until channel directory is created
				while [ ! -d "$II_DIR/$server/$channel" ]; do inotifywait -qqt 1 -e create "$II_DIR/$server"; done

				# Wait until ii files are created
				while [ ! -e "$II_DIR/$server/$channel/in" -o ! -e "$II_DIR/$server/$channel/out" ]; do inotifywait -qqt 1 -e create "$II_DIR/$server/$channel"; done

				# Clean ii output
				tail -n +1 -f "$II_DIR/$server/$channel/out" | clean > "$II_DIR/$server/$channel/out-clean" &
				out_pid=$!

				# Intercept input
				if [ ! -p "$II_DIR/$server/$channel/in-intercept" ]; then mkfifo "$II_DIR/$server/$channel/in-intercept"; fi
				cat "$II_DIR/$server/$channel/in-intercept" | tee "$II_DIR/$server/$channel/in" | intercept &
				in_pid=$!

				# Launch converse
				$TERMINAL -title "$ - ii" -e converse "$II_DIR/$server/$channel/out-clean" "$II_DIR/$server/$channel/in-intercept"

				# Leave channel
				echo "/part $channel $PART_MSG" > "$II_DIR/$server/in";

				# Kill child processes
				kill $in_pid
				kill $out_pid
			)
		fi
	done
}

for server in "${!SERVERS[@]}"; do
	(
		echo "Connecting to $server ${SERVERS[$server]}"

		# Ask for password
		read -s -p "Enter $server password: " pass
		echo

		# Launch ii
		ii -i "$II_DIR" -s "$server" -n "$II_USERNAME" -k "$pass" -f "$II_FULLNAME" &
		ii_pid=$!

		# Wait until server directory is created
		while [ ! -d "$II_DIR/$server" ]; do inotifywait -qqt 1 -e create "$II_DIR"; done

		# Wait until ii files are created
		while [ ! -e "$II_DIR/$server/in" -o ! -e "$II_DIR/$server/out" ]; do inotifywait -qqt 1 -e create "$II_DIR/$server"; done

		# Clean ii output
		tail -n +1 -f "$II_DIR/$server/out" | clean > "$II_DIR/$server/out-clean" &
		out_pid=$!

		# Intercept input
		if [ ! -p "$II_DIR/$server/in-intercept" ]; then mkfifo "$II_DIR/$server/in-intercept"; fi

		cat "$II_DIR/$server/in-intercept" | tee "$II_DIR/$server/in" | intercept &
		in_pid=$!

		# Launch converse
		$TERMINAL -title "$ - ii" -e converse "$II_DIR/$server/out-clean" "$II_DIR/$server/in-intercept"

		# Leave all channels
		echo "/partall $PART_MSG" > "$II_DIR/$server/in";

		# Kill child processes
		kill $in_pid
		kill $out_pid
		kill $ii_pid
	)
done

# Connect to some channels
#for channel in \#reddit-gamedev \#gamedev \#/r/webdev \#\#webdev \#archlinux \#\#electronics \#uzbl \#ledger \#2f30
#do
#	echo "/join $channel" 
#done
#
#
#
#for chan in "$II_DIR"/*/ "$II_DIR"/*/*/; do
#	dir=$(basename "$chan")
#	tail -n +1 -f "$chan"out | clean > "$chan"out-clean &
#	$TERMINAL -title "$dir - ii" -e converse "$chan"out-clean "$chan"in &
#done
#
#wait
