#!/bin/bash
set -x

echo "=== ApplicationStop: Preparing for deployment ==="

# Do NOT stop the running server. The new binary and data files will be
# deployed in-place, and an admin can run 'copyover' in-game to pick up
# the changes without disconnecting players.

if systemctl is-active --quiet mudder; then
    echo "Mudder service is running - will continue running during deployment"
    echo "New files will be picked up on next copyover"
else
    echo "Mudder service is not running (or not installed yet)"
fi

exit 0
