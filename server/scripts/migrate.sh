#!/bin/bash
# migrate.sh — Deploy validated release from ingest/ to live/
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LIVE_DIR="$BASE_DIR/live"
INGEST_DIR="$BASE_DIR/ingest"
STAGING="$INGEST_DIR/dystopia"

log() { echo "[migrate] $*"; }
die() { log "ERROR: $*" >&2; exit 1; }

# Verify ingest has a validated release
[ -f "$INGEST_DIR/.pending_version" ] || die "No pending version. Run download.sh and validate.sh first."
[ -d "$STAGING/gamedata" ] || die "Staging directory missing. Run download.sh first."
[ -f "$STAGING/gamedata/dystopia" ] || die "Binary not found in staging."

NEW_VERSION="$(cat "$INGEST_DIR/.pending_version")"
CURRENT="none"
if [ -f "$LIVE_DIR/.version" ]; then
    CURRENT="$(cat "$LIVE_DIR/.version")"
fi

log "Migrating: $CURRENT -> $NEW_VERSION"

# Ensure live directory structure exists
mkdir -p "$LIVE_DIR/gamedata/db/areas"
mkdir -p "$LIVE_DIR/gamedata/db/game"
mkdir -p "$LIVE_DIR/gamedata/db/players"
mkdir -p "$LIVE_DIR/gamedata/doc"
mkdir -p "$LIVE_DIR/gamedata/log"
mkdir -p "$LIVE_DIR/gamedata/run"

# Back up current binary
if [ -f "$LIVE_DIR/gamedata/dystopia" ]; then
    cp "$LIVE_DIR/gamedata/dystopia" "$LIVE_DIR/gamedata/dystopia.bak"
    log "Backed up current binary to dystopia.bak"
fi

# Copy new binary
cp "$STAGING/gamedata/dystopia" "$LIVE_DIR/gamedata/dystopia"
chmod +x "$LIVE_DIR/gamedata/dystopia"
log "Installed new binary"

# Copy area databases (all safe to overwrite)
if [ -d "$STAGING/gamedata/db/areas" ]; then
    cp "$STAGING/gamedata/db/areas/"*.db "$LIVE_DIR/gamedata/db/areas/"
    log "Updated area databases"
fi

# Copy ONLY safe game databases — never touch game.db or live_help.db
SAFE_GAME_DBS="base_help.db class.db classeq.db tables.db quest.db"
for db in $SAFE_GAME_DBS; do
    if [ -f "$STAGING/gamedata/db/game/$db" ]; then
        cp "$STAGING/gamedata/db/game/$db" "$LIVE_DIR/gamedata/db/game/$db"
    fi
done
log "Updated game databases (safe files only)"

# Copy documentation
if [ -d "$STAGING/gamedata/doc" ]; then
    cp -r "$STAGING/gamedata/doc/." "$LIVE_DIR/gamedata/doc/"
    log "Updated documentation"
fi

# Copy startup script
if [ -f "$STAGING/startup.sh" ]; then
    cp "$STAGING/startup.sh" "$LIVE_DIR/startup.sh"
    chmod +x "$LIVE_DIR/startup.sh"
    log "Updated startup.sh"
fi

# Record new version
echo "$NEW_VERSION" > "$LIVE_DIR/.version"

log ""
log "=== Migration complete: $CURRENT -> $NEW_VERSION ==="
log ""
log "The running server is still using the old binary in memory."
log "To activate the update:"
log "  1. Connect to the MUD as an admin (level 7+)"
log "  2. Run the 'copyover' command in-game"
log "  3. The server will re-exec with the new binary"
log ""
log "Previous binary backed up to: live/gamedata/dystopia.bak"
