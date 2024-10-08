#!/bin/bash
# Written by Furey.
# With additions from Tony.
# Converted to bash by Todd.

# Set the port number.
port=8888
if [ "$1" != "" ]; then
  port="$1"
fi

# Change to area directory.
# autokill 60 >> ../area/autokill.txt &
cd ../area

# Set limits.
# nohup
nice
# ulimit -s 1024k  # Stack size limit (in kbytes), commented out similar to original.
ulimit -c $((8128 * 1024))  # Core dump size in bytes
ulimit -f $((16256 * 1024))  # File size limit in bytes
if [ -e shutdown.txt ]; then
  rm -f shutdown.txt
fi

while true; do
  # If you want to have logs in a different directory,
  # change the 'logfile' line to reflect the directory name.
  index=1000
  while true; do
    logfile="../log/$index.log"
    if [ ! -e "$logfile" ]; then
      break
    fi
    ((index++))
  done

  # Run merc.
  cd ../src
  # cp dystopia ../area

  cd ../area
  ../src/dystopia "$port" >& "$logfile"

  # Restart, giving old connections a chance to die.
  if [ -e shutdown.txt ]; then
    rm -f shutdown.txt
    exit 0
  fi

  sleep 2
done
