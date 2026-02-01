#!/bin/bash
set -ex

echo "=== BeforeInstall: Preparing for deployment ==="

# Get AWS region from instance metadata (IMDSv2 requires a token)
TOKEN=$(curl -s -X PUT "http://169.254.169.254/latest/api/token" -H "X-aws-ec2-metadata-token-ttl-seconds: 60")
REGION=$(curl -s -H "X-aws-ec2-metadata-token: $TOKEN" http://169.254.169.254/latest/meta-data/placement/region)
echo "AWS Region: $REGION"

# Derive instance name from deployment group name (e.g., MudderDeploymentGroup-5e -> 5e)
INSTANCE="${DEPLOYMENT_GROUP_NAME##*-}"
if [ -z "$INSTANCE" ]; then
    INSTANCE="main"
fi
echo "Instance: $INSTANCE"

BASE_DIR="/usr/local/mudder/${INSTANCE}"
BIN_DIR="${BASE_DIR}/bin"
EFS_MOUNT="/usr/local/mudder/efs"
GAMEDATA_DIR="${EFS_MOUNT}/${INSTANCE}"

# Ensure the mudder user exists
if ! id -u mudder > /dev/null 2>&1; then
    echo "Creating mudder user..."
    useradd -r -s /sbin/nologin mudder
fi

# Create bin directory for this instance
mkdir -p "$BIN_DIR"

# Create EFS mount point if it doesn't exist
mkdir -p "$EFS_MOUNT"

# Get EFS ID from SSM Parameter Store (needed for mount and migration)
EFS_ID=$(aws ssm get-parameter --name /mudder/efs-filesystem-id --query 'Parameter.Value' --output text --region "$REGION")
if [ -z "$EFS_ID" ]; then
    echo "ERROR: Could not retrieve EFS ID from SSM"
    exit 1
fi
echo "EFS ID: $EFS_ID"

# Ensure EFS mount helper is installed
if ! command -v mount.efs &> /dev/null; then
    echo "Installing amazon-efs-utils..."
    dnf install -y amazon-efs-utils || yum install -y amazon-efs-utils
fi

# Check if EFS is still mounted at the old single-instance location
OLD_GAMEDATA="/usr/local/mudder/gamedata"
if [ -d "$OLD_GAMEDATA" ] && [ ! -L "$OLD_GAMEDATA" ] && mountpoint -q "$OLD_GAMEDATA" 2>/dev/null; then
    echo "=== Detected old single-instance layout, migrating to multi-instance ==="

    # Stop old service if running
    if systemctl is-active --quiet mudder 2>/dev/null; then
        echo "Stopping old mudder service..."
        systemctl stop mudder
    fi

    # Unmount EFS from old location
    echo "Unmounting EFS from $OLD_GAMEDATA..."
    umount "$OLD_GAMEDATA"

    # Mount EFS at new location
    echo "Mounting EFS at $EFS_MOUNT..."
    mount -t efs -o tls,iam "${EFS_ID}:/" "$EFS_MOUNT"

    # Move existing data into main/ subdirectory on EFS
    if [ ! -d "$EFS_MOUNT/main/db" ]; then
        echo "Moving existing game data into efs/main/..."
        mkdir -p "$EFS_MOUNT/main"
        cd "$EFS_MOUNT"
        for item in db log doc run txt; do
            if [ -e "$item" ] && [ "$item" != "main" ] && [ "$item" != "5e" ]; then
                echo "  Moving $item -> main/$item"
                mv "$item" main/
            fi
        done
        # Clean up old symlinks from EFS root
        for item in dystopia dystopia_new; do
            if [ -e "$item" ] || [ -L "$item" ]; then
                echo "  Removing old $item from EFS root"
                rm -f "$item"
            fi
        done
    else
        echo "efs/main/db already exists, skipping data move"
    fi

    # Move old binary to new location
    OLD_BIN="/usr/local/mudder/bin/dystopia"
    if [ -f "$OLD_BIN" ]; then
        echo "Moving binary to /usr/local/mudder/main/bin/"
        mkdir -p /usr/local/mudder/main/bin
        mv "$OLD_BIN" /usr/local/mudder/main/bin/
        rmdir /usr/local/mudder/bin 2>/dev/null || true
    fi

    # Remove old gamedata directory (was a mount point, now empty)
    rmdir "$OLD_GAMEDATA" 2>/dev/null || true

    # Update fstab to use new mount point
    echo "Updating /etc/fstab..."
    sed -i 's|/usr/local/mudder/gamedata|/usr/local/mudder/efs|g' /etc/fstab

    # Disable and remove old service, replace with instance-named service
    if [ -f /etc/systemd/system/mudder.service ]; then
        echo "Migrating mudder.service -> mudder-main.service..."
        systemctl disable mudder 2>/dev/null || true
        rm /etc/systemd/system/mudder.service
    fi

    echo "=== Migration complete ==="
fi

# Verify EFS is mounted at the new location
if ! mountpoint -q "$EFS_MOUNT"; then
    echo "EFS is not mounted at $EFS_MOUNT, mounting..."
    mount -t efs -o tls,iam "${EFS_ID}:/" "$EFS_MOUNT"

    if ! mountpoint -q "$EFS_MOUNT"; then
        echo "FATAL: Could not mount EFS"
        exit 1
    fi
fi

echo "EFS mounted successfully at $EFS_MOUNT"

# Create instance game data directories on EFS
echo "Ensuring game data directories exist for instance: $INSTANCE..."
mkdir -p "$GAMEDATA_DIR/db/areas"
mkdir -p "$GAMEDATA_DIR/db/game"
mkdir -p "$GAMEDATA_DIR/db/players/backup"
mkdir -p "$GAMEDATA_DIR/log"
mkdir -p "$GAMEDATA_DIR/doc"
mkdir -p "$GAMEDATA_DIR/run"
mkdir -p "$GAMEDATA_DIR/txt"

# Create symlink from instance gamedata to EFS subdirectory
if [ -L "${BASE_DIR}/gamedata" ]; then
    rm "${BASE_DIR}/gamedata"
elif [ -d "${BASE_DIR}/gamedata" ]; then
    # If it's a real directory (shouldn't happen), remove it
    rmdir "${BASE_DIR}/gamedata" 2>/dev/null || true
fi
ln -s "$GAMEDATA_DIR" "${BASE_DIR}/gamedata"

# Set ownership
chown -R mudder:mudder "$BASE_DIR"
chown -R mudder:mudder "$GAMEDATA_DIR"

echo "Instance directory structure:"
ls -la "$BASE_DIR/"
echo "Game data directories:"
ls -la "$GAMEDATA_DIR/"

echo "=== BeforeInstall: Complete ==="
