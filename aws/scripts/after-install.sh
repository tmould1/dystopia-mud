#!/bin/bash
set -ex

echo "=== AfterInstall: Configuring deployment ==="

# Get AWS region from instance metadata (IMDSv2 requires a token)
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60")
REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region)
echo "AWS Region: $REGION"

# Derive instance name from deployment group name
INSTANCE="${DEPLOYMENT_GROUP_NAME##*-}"
if [ -z "$INSTANCE" ]; then
    INSTANCE="main"
fi
echo "Instance: $INSTANCE"

BASE_DIR="/usr/local/mudder/${INSTANCE}"
BIN_DIR="${BASE_DIR}/bin"
GAMEDATA_DIR="${BASE_DIR}/gamedata"
SERVICE_NAME="mudder-${INSTANCE}"

# Verify binary was deployed
if [ ! -f "${BIN_DIR}/dystopia" ]; then
    echo "ERROR: Binary not found at ${BIN_DIR}/dystopia"
    exit 1
fi

# Set ownership and permissions for the binary
chown mudder:mudder "${BIN_DIR}/dystopia"
chmod 755 "${BIN_DIR}/dystopia"

# Verify binary is executable
if [ ! -x "${BIN_DIR}/dystopia" ]; then
    echo "ERROR: Binary is not executable"
    exit 1
fi

echo "Binary installed at: ${BIN_DIR}/dystopia"

# Create symlink in gamedata directory for argv[0] path resolution
cd "$GAMEDATA_DIR"

if [ -L dystopia ]; then
    echo "Removing existing symlink..."
    rm dystopia
fi

echo "Creating symlink: dystopia -> ${BIN_DIR}/dystopia"
ln -s "${BIN_DIR}/dystopia" dystopia
chown -h mudder:mudder dystopia

# Stage binary for copyover (dystopia_new)
echo "Staging binary for copyover..."
cp "${BIN_DIR}/dystopia" "${GAMEDATA_DIR}/dystopia_new"
chown mudder:mudder "${GAMEDATA_DIR}/dystopia_new"
chmod 755 "${GAMEDATA_DIR}/dystopia_new"

# Get the game port from SSM Parameter Store
GAME_PORT=$(aws ssm get-parameter --name "/mudder/${INSTANCE}/game-port" --query 'Parameter.Value' --output text --region "$REGION")
echo "Game port for ${INSTANCE}: $GAME_PORT"

# Generate systemd service file for this instance
cat > "/etc/systemd/system/${SERVICE_NAME}.service" << EOF
[Unit]
Description=Dystopia MUD Server (${INSTANCE})
After=syslog.target network.target remote-fs.target
Requires=remote-fs.target

[Service]
Type=simple
User=mudder
Group=mudder
WorkingDirectory=${GAMEDATA_DIR}
ExecStart=${GAMEDATA_DIR}/dystopia ${GAME_PORT}
ExecStop=/bin/kill -TERM \$MAINPID
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable "${SERVICE_NAME}.service"

# Set ownership of all gamedata (preserving EFS data)
chown -R mudder:mudder "$GAMEDATA_DIR"

echo "Game data directory structure:"
ls -la "$GAMEDATA_DIR/"
echo "Database directories:"
ls -la "$GAMEDATA_DIR/db/" 2>/dev/null || echo "(no db directory yet)"

echo "=== AfterInstall: Complete ==="
