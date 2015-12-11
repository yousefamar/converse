#!/bin/sh

if [ $# -lt 2 ]; then
	echo "$0 watches every /out file in your ii directory and notifies on pattern matches."
	echo
	echo "Usage: $0 II_DIR PATTERN"
	echo "  II_DIR:  Path to ii root directory"
	echo "  PATTERN: Regex that when matched, triggers a notification"
	echo "    Case insensitive and excludes timestamps and sender usernames"
	echo "    e.g. '.' for everything, '$(whoami)' for username mentions, etc"
	exit
fi

II_DIR=$1
PATTERN=$2

if [ ! -d "$II_DIR" ]; then
	echo "Directory $II_DIR does not exist"
	exit
fi

while true; do
	# This part is based on uii/iiinotify
	notification=$(inotifywait -q -r "$1" -e modify)
	file=$(echo "$notification" | cut -f3 -d ' ')
	directory=$(echo "$notification" | cut -f1 -d ' ')
	if [ "$file" = "out" ]; then
		server=$(echo "$directory" | awk -F/ '{print $(NF-2)}')
		channel=$(echo "$directory" | awk -F/ '{print $(NF-1)}')
		line=$(tail -n1 "$directory/$file") #| sed -e 's/&/\&amp;/g' | sed -e 's/</\&lt;/g')
		user=$(echo "$line" | cut -f3 -d ' ')
		message=$(echo "$line" | cut -f4- -d ' ')
		[[ "$message" =~ $PATTERN ]] && notify-send -t 3000 -- "$channel @ $server" "$user $message"
	fi
done