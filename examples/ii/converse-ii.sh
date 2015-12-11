#!/bin/sh

: ${TERMINAL:="evilvte"}

if [ $# -lt 1 ]; then
	echo "usage: $0 CHANNEL_DIR"
	exit
fi

if [ ! -d "$1" -o ! -e "$1/in" -o ! -e "$1/out" ]; then
	echo "Directory $1 or ii files (in/out) do not exist"
	exit
fi

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

# Kill child processes on exit
trap 'kill $(jobs -pr)' SIGINT SIGTERM EXIT

# Clean ii output
tail -n +1 -f "$1/out" | clean > "$1/out-clean" &

# Launch converse
$TERMINAL -title "$ - ii" -e converse "$1/out-clean" "$1/in"
