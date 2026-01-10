#!/bin/bash
set -x

echo "=== ApplicationStop: Stopping MUD server ==="

# Check if service exists and is running
if systemctl is-active --quiet mudder; then
    echo "Stopping mudder service..."
    systemctl stop mudder

    # Wait for the service to fully stop (give players time to save)
    echo "Waiting for service to stop..."
    sleep 5

    # Verify it stopped
    if systemctl is-active --quiet mudder; then
        echo "WARNING: Service did not stop cleanly, sending SIGKILL..."
        systemctl kill mudder
        sleep 2
    fi

    echo "Mudder service stopped"
else
    echo "Mudder service is not running (or not installed yet)"
fi

# Always exit successfully - we don't want to fail deployment if service wasn't running
exit 0
