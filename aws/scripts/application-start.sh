#!/bin/bash
set -ex

echo "=== ApplicationStart: Starting MUD server ==="

# Get AWS region from instance metadata (IMDSv2 requires a token)
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60")
REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region)
echo "AWS Region: $REGION"

# Get the game port from SSM Parameter Store
GAME_PORT=$(aws ssm get-parameter --name /mudder/game-port --query 'Parameter.Value' --output text --region "$REGION")
echo "Game port: $GAME_PORT"

# Reload systemd in case service file changed
systemctl daemon-reload

# Check if service is already running
if systemctl is-active --quiet mudder; then
    echo "Mudder service is already running"
    echo "New binary deployed - will be picked up on next copyover"
    systemctl status mudder --no-pager
else
    # Service not running, start it
    echo "Starting mudder service..."
    systemctl start mudder

    # Give it a moment to start
    sleep 3

    # Check if it started successfully
    if systemctl is-active --quiet mudder; then
        echo "Mudder service started successfully"
        systemctl status mudder --no-pager
    else
        echo "ERROR: Mudder service failed to start"
        echo "=== Service logs ==="
        journalctl -u mudder --no-pager -n 50
        exit 1
    fi
fi

echo "=== ApplicationStart: Complete ==="
