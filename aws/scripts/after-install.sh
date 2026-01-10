#!/bin/bash
set -ex

echo "=== AfterInstall: Configuring deployment ==="

# Verify binary was deployed
if [ ! -f /usr/local/mudder/bin/dystopia ]; then
    echo "ERROR: Binary not found at /usr/local/mudder/bin/dystopia"
    exit 1
fi

# Set ownership and permissions for the binary
chown mudder:mudder /usr/local/mudder/bin/dystopia
chmod 755 /usr/local/mudder/bin/dystopia

# Verify binary is executable
if [ ! -x /usr/local/mudder/bin/dystopia ]; then
    echo "ERROR: Binary is not executable"
    exit 1
fi

echo "Binary installed at: /usr/local/mudder/bin/dystopia"

# Create symlink in gamedata directory
# The MUD binary uses argv[0] to locate data directories relative to itself
# By creating a symlink in gamedata/, the binary finds area/, player/, etc. as siblings
cd /usr/local/mudder/gamedata

if [ -L dystopia ]; then
    echo "Removing existing symlink..."
    rm dystopia
fi

echo "Creating symlink: dystopia -> ../bin/dystopia"
ln -s ../bin/dystopia dystopia
chown -h mudder:mudder dystopia

# Ensure all required directories exist on EFS
echo "Ensuring game data directories exist..."
mkdir -p /usr/local/mudder/gamedata/area
mkdir -p /usr/local/mudder/gamedata/player/backup
mkdir -p /usr/local/mudder/gamedata/player/store
mkdir -p /usr/local/mudder/gamedata/notes
mkdir -p /usr/local/mudder/gamedata/log
mkdir -p /usr/local/mudder/gamedata/txt
mkdir -p /usr/local/mudder/gamedata/doc

# Set ownership of all gamedata (preserving EFS data)
chown -R mudder:mudder /usr/local/mudder/gamedata

echo "Game data directory structure:"
ls -la /usr/local/mudder/gamedata/

echo "=== AfterInstall: Complete ==="
