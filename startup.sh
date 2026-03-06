#!/bin/bash
# Written by Furey.
# With additions from Tony.
# Converted to bash by Todd.

# Get the directory where this script lives
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Load port from server.conf if available
port=8888
if [ -f "$SCRIPT_DIR/../server.conf" ]; then
  source "$SCRIPT_DIR/../server.conf"
  port="${PORT:-$port}"
elif [ -f "$SCRIPT_DIR/server/server.conf" ]; then
  source "$SCRIPT_DIR/server/server.conf"
  port="${PORT:-$port}"
fi
if [ "$1" != "" ]; then
  port="$1"
fi

# Define paths - gamedata is relative to script location
# Script can be in project root or in gamedata/ itself
if [ -d "$SCRIPT_DIR/gamedata" ]; then
  # Script is in project root
  GAMEDATA_DIR="$SCRIPT_DIR/gamedata"
else
  # Script might be in gamedata/
  GAMEDATA_DIR="$SCRIPT_DIR"
fi

# Raise limits if the shell allows it (silently ignored otherwise).
ulimit -c unlimited 2>/dev/null  # Allow core dumps for debugging
ulimit -f unlimited 2>/dev/null  # No file size cap

if [ -e "$GAMEDATA_DIR/txt/shutdown.txt" ]; then
  rm -f "$GAMEDATA_DIR/txt/shutdown.txt"
fi

while true; do
  # Run the MUD (executable is in gamedata/)
  # Logs are written internally to gamedata/log/ with timestamped filenames
  "$GAMEDATA_DIR/dystopia" "$port"

  # Restart, giving old connections a chance to die.
  if [ -e "$GAMEDATA_DIR/txt/shutdown.txt" ]; then
    rm -f "$GAMEDATA_DIR/txt/shutdown.txt"
    exit 0
  fi

  sleep 2
done
