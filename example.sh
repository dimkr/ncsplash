#!/bin/sh

# the delay between messages, in seconds
DELAY=2

# create a FIFO
fifo="$(mktemp -u)"
mkfifo $fifo

# update_status
# updates the status message in ncsplash
# input: a message
# output: -
update_status() {
	echo "$@" >> $fifo
	sleep $DELAY
}

# actual_code
# runs the actual flow of the script - this is a dummy function for
# demonstration purposes, of course
# input: -
# output: -
actual_code() {
	update_status "Recognizing devices"
	update_status "Mounting file systems"
	update_status "Setting up swap"
	update_status "Configuring network"
	update_status "Starting daemons"
	update_status "Starting desktop environment"

	# signal ncsplash to terminate
	echo "exit" >> $fifo
}

# run the actual code and redirect any output; the first writing to the FIFO
# will be blocking until ncsplash gets notified about it
actual_code > /dev/null 2>&1 &

# run ncsplash; it will block the shell until EXIT_TEXT is received
ncsplash $fifo "Starting Ultimate Operating System 1.0 Beta ..."

# delete the FIFO
rm -f $fifo
