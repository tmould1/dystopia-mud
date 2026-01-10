#!/bin/bash
set -ex

echo "=== ApplicationStart: Starting MUD server ==="

# Get AWS region from instance metadata
REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)

# Get the game port from SSM Parameter Store
GAME_PORT=$(aws ssm get-parameter --name /mudder/game-port --query 'Parameter.Value' --output text --region "$REGION")
echo "Game port: $GAME_PORT"

# Reload systemd in case service file changed
systemctl daemon-reload

# Start the service
echo "Starting mudder service..."
systemctl start mudder

# Give it a moment to start
sleep 3

# Check if it's running
if systemctl is-active --quiet mudder; then
    echo "Mudder service started successfully"
    systemctl status mudder --no-pager
else
    echo "ERROR: Mudder service failed to start"
    echo "=== Service logs ==="
    journalctl -u mudder --no-pager -n 50
    exit 1
fi

echo "=== ApplicationStart: Complete ==="
