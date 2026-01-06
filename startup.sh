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

# Define paths - gamedata is relative to script location
# Script can be in project root or in gamedata/ itself
if [ -d "$SCRIPT_DIR/gamedata" ]; then
  # Script is in project root
  GAMEDATA_DIR="$SCRIPT_DIR/gamedata"
else
  # Script might be in gamedata/
  GAMEDATA_DIR="$SCRIPT_DIR"
fi

# Set limits.
# nohup
nice
# ulimit -s 1024k  # Stack size limit (in kbytes), commented out similar to original.
ulimit -c $((8128 * 1024))  # Core dump size in bytes
ulimit -f $((16256 * 1024))  # File size limit in bytes

if [ -e "$GAMEDATA_DIR/area/shutdown.txt" ]; then
  rm -f "$GAMEDATA_DIR/area/shutdown.txt"
fi

while true; do
  # Run the MUD (executable is in gamedata/)
  # Logs are written internally to gamedata/log/ with timestamped filenames
  "$GAMEDATA_DIR/dystopia" "$port"

  # Restart, giving old connections a chance to die.
  if [ -e "$GAMEDATA_DIR/area/shutdown.txt" ]; then
    rm -f "$GAMEDATA_DIR/area/shutdown.txt"
    exit 0
  fi

  sleep 2
done
