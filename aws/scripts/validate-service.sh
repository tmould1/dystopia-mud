#!/bin/bash
set -x

echo "=== ValidateService: Checking MUD server health ==="

MAX_ATTEMPTS=10
SLEEP_TIME=3

# Get AWS region from instance metadata
REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)

# Get the game port from SSM Parameter Store
GAME_PORT=$(aws ssm get-parameter --name /mudder/game-port --query 'Parameter.Value' --output text --region "$REGION")
echo "Validating mudder service on port $GAME_PORT..."

# Check if service is running
if ! systemctl is-active --quiet mudder; then
    echo "ERROR: Mudder service is not running"
    echo "=== Service status ==="
    systemctl status mudder --no-pager || true
    echo "=== Service logs ==="
    journalctl -u mudder --no-pager -n 50
    exit 1
fi

echo "Service is running, checking port connectivity..."

# Try to connect to the port
for i in $(seq 1 $MAX_ATTEMPTS); do
    if nc -z localhost "$GAME_PORT" 2>/dev/null; then
        echo "SUCCESS: Service is accepting connections on port $GAME_PORT"
        echo "=== Service status ==="
        systemctl status mudder --no-pager
        echo "=== ValidateService: Complete ==="
        exit 0
    fi
    echo "Attempt $i/$MAX_ATTEMPTS: Port $GAME_PORT not responding, waiting ${SLEEP_TIME}s..."
    sleep $SLEEP_TIME
done

echo "ERROR: Service did not respond on port $GAME_PORT after $MAX_ATTEMPTS attempts"
echo "=== Service status ==="
systemctl status mudder --no-pager || true
echo "=== Service logs ==="
journalctl -u mudder --no-pager -n 50
exit 1
