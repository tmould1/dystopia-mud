#!/bin/bash
set -ex

echo "=== BeforeInstall: Preparing for deployment ==="

# Get AWS region from instance metadata
REGION=$(curl -s http://169.254.169.254/latest/meta-data/placement/region)
echo "AWS Region: $REGION"

# Ensure the mudder user exists
if ! id -u mudder > /dev/null 2>&1; then
    echo "Creating mudder user..."
    useradd -r -s /sbin/nologin mudder
fi

# Create bin directory if it doesn't exist
mkdir -p /usr/local/mudder/bin

# Create gamedata directory (mount point) if it doesn't exist
mkdir -p /usr/local/mudder/gamedata

# Verify EFS is mounted
if ! mountpoint -q /usr/local/mudder/gamedata; then
    echo "WARNING: EFS is not mounted at /usr/local/mudder/gamedata"
    echo "Attempting to mount EFS..."

    # Get EFS ID from SSM Parameter Store
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

    # Mount EFS
    mount -t efs -o tls,iam "${EFS_ID}:/" /usr/local/mudder/gamedata

    if ! mountpoint -q /usr/local/mudder/gamedata; then
        echo "FATAL: Could not mount EFS"
        exit 1
    fi
fi

echo "EFS mounted successfully at /usr/local/mudder/gamedata"
echo "Contents of gamedata directory:"
ls -la /usr/local/mudder/gamedata/

echo "=== BeforeInstall: Complete ==="
