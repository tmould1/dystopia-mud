#!/bin/bash
set -x

echo "=== ApplicationStop: Preparing for deployment ==="

# Derive instance name from deployment group name
INSTANCE="${DEPLOYMENT_GROUP_NAME##*-}"
if [ -z "$INSTANCE" ]; then
    INSTANCE="main"
fi
SERVICE_NAME="mudder-${INSTANCE}"

echo "Instance: $INSTANCE"
echo "Service: $SERVICE_NAME"

# Do NOT stop the running server. The new binary and data files will be
# deployed in-place, and an admin can run 'copyover' in-game to pick up
# the changes without disconnecting players.

if systemctl is-active --quiet "$SERVICE_NAME"; then
    echo "$SERVICE_NAME is running - will continue running during deployment"
    echo "New files will be picked up on next copyover"
else
    echo "$SERVICE_NAME is not running (or not installed yet)"
fi

exit 0
