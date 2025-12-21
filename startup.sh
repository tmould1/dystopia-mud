#!/bin/bash
# Written by Furey.
# With additions from Tony.
# Converted to bash by Todd.

# Set the port number.
port=8888
if [ "$1" != "" ]; then
  port="$1"
fi

# Get the directory where this script lives
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Set limits.
# nohup
nice
# ulimit -s 1024k  # Stack size limit (in kbytes), commented out similar to original.
ulimit -c $((8128 * 1024))  # Core dump size in bytes
ulimit -f $((16256 * 1024))  # File size limit in bytes
if [ -e area/shutdown.txt ]; then
  rm -f area/shutdown.txt
fi

while true; do
  # If you want to have logs in a different directory,
  # change the 'logfile' line to reflect the directory name.
  index=1000
  while true; do
    logfile="log/$index.log"
    if [ ! -e "$logfile" ]; then
      break
    fi
    ((index++))
  done

  # Run the MUD (executable is in project root)
  ./dystopia "$port" >& "$logfile"

  # Restart, giving old connections a chance to die.
  if [ -e area/shutdown.txt ]; then
    rm -f area/shutdown.txt
    exit 0
  fi

  sleep 2
done
