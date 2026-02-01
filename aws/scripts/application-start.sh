#!/bin/bash
set -ex

echo "=== ApplicationStart: Starting MUD server ==="

# Get AWS region from instance metadata (IMDSv2 requires a token)
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60")
REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region)
echo "AWS Region: $REGION"

# Derive instance name from deployment group name
INSTANCE="${DEPLOYMENT_GROUP_NAME##*-}"
if [ -z "$INSTANCE" ]; then
    INSTANCE="main"
fi
SERVICE_NAME="mudder-${INSTANCE}"

echo "Instance: $INSTANCE"
echo "Service: $SERVICE_NAME"

# Get the game port from SSM Parameter Store
GAME_PORT=$(aws ssm get-parameter --name "/mudder/${INSTANCE}/game-port" --query 'Parameter.Value' --output text --region "$REGION")
echo "Game port: $GAME_PORT"

# Reload systemd in case service file changed
systemctl daemon-reload

# Check if service is already running
if systemctl is-active --quiet "$SERVICE_NAME"; then
    echo "$SERVICE_NAME is already running"
    echo "New binary deployed - will be picked up on next copyover"
    systemctl status "$SERVICE_NAME" --no-pager
else
    # Service not running, start it
    echo "Starting $SERVICE_NAME..."
    systemctl start "$SERVICE_NAME"

    # Give it a moment to start
    sleep 3

    # Check if it started successfully
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        echo "$SERVICE_NAME started successfully"
        systemctl status "$SERVICE_NAME" --no-pager
    else
        echo "ERROR: $SERVICE_NAME failed to start"
        echo "=== Service logs ==="
        journalctl -u "$SERVICE_NAME" --no-pager -n 50
        exit 1
    fi
fi

echo "=== ApplicationStart: Complete ==="
