#!/bin/bash
set -x

# System control will return either "active" or "inactive".
mudder_running=$(systemctl is-active mudder)
if [ "$mudder_running" == "active" ]; then
    service mudder stop
fi