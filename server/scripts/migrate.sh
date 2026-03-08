#!/bin/sh
# migrate.sh — Deploy validated release from ingest/ to a target directory
#
# Usage:
#   migrate.sh              Dry-run: deploy to dryrun/ (ingest NOT cleaned)
#   migrate.sh TARGET_DIR   Deploy to TARGET_DIR (ingest cleaned after)
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
INGEST_DIR="$BASE_DIR/ingest"
STAGING="$INGEST_DIR/dystopia"
MAX_BACKUPS=3

# Determine target directory
if [ $# -ge 1 ]; then
    TARGET_DIR="$1"
    IS_DRYRUN=false
    log() { echo "[migrate] $*"; }
else
    TARGET_DIR="$BASE_DIR/dryrun"
    IS_DRYRUN=true
    log() { echo "[migrate:dry-run] $*"; }
fi

mkdir -p "$TARGET_DIR"
TARGET_DIR="$(cd "$TARGET_DIR" && pwd)"
BACKUP_DIR="$TARGET_DIR/backups"

die() { echo "[migrate] ERROR: $*" >&2; exit 1; }

# Verify ingest has a validated release
[ -f "$INGEST_DIR/.pending_version" ] || die "No pending version. Run validate.sh first."
[ -d "$STAGING/gamedata" ] || die "Staging directory missing."
[ -f "$STAGING/gamedata/dystopia" ] || die "Binary not found in staging."

NEW_VERSION="$(cat "$INGEST_DIR/.pending_version")"
CURRENT="none"
if [ -f "$TARGET_DIR/.version" ]; then
    CURRENT="$(cat "$TARGET_DIR/.version")"
fi

log "Migrating: $CURRENT -> $NEW_VERSION"
log "Target: $TARGET_DIR"

# Ensure target directory structure exists
mkdir -p "$TARGET_DIR/gamedata/db/areas"
mkdir -p "$TARGET_DIR/gamedata/db/game"
mkdir -p "$TARGET_DIR/gamedata/db/players"
mkdir -p "$TARGET_DIR/gamedata/doc"
mkdir -p "$TARGET_DIR/gamedata/log"
mkdir -p "$TARGET_DIR/gamedata/run"

# --- Versioned backup ---
SAFE_GAME_DBS="base_help.db class.db classeq.db tables.db quest.db"

if [ "$CURRENT" != "none" ]; then
    BACKUP="$BACKUP_DIR/$CURRENT"
    mkdir -p "$BACKUP" "$BACKUP/db/areas" "$BACKUP/db/game"

    # Back up current binary
    if [ -f "$TARGET_DIR/gamedata/dystopia" ]; then
        cp "$TARGET_DIR/gamedata/dystopia" "$BACKUP/dystopia"
    fi

    # Back up area databases
    for db in "$TARGET_DIR/gamedata/db/areas/"*.db; do
        [ -f "$db" ] && cp "$db" "$BACKUP/db/areas/"
    done 2>/dev/null || true

    # Back up safe game databases (only the ones we'll overwrite)
    for db in $SAFE_GAME_DBS; do
        if [ -f "$TARGET_DIR/gamedata/db/game/$db" ]; then
            cp "$TARGET_DIR/gamedata/db/game/$db" "$BACKUP/db/game/$db"
        fi
    done

    log "Backed up $CURRENT to backups/$CURRENT/"
fi

# --- Rotate old backups (keep last $MAX_BACKUPS) ---
if [ -d "$BACKUP_DIR" ]; then
    BACKUP_COUNT=$(ls -1d "$BACKUP_DIR"/v* 2>/dev/null | wc -l)
    if [ "$BACKUP_COUNT" -gt "$MAX_BACKUPS" ]; then
        REMOVE_COUNT=$((BACKUP_COUNT - MAX_BACKUPS))
        ls -1d "$BACKUP_DIR"/v* 2>/dev/null | head -n "$REMOVE_COUNT" | while read -r old; do
            rm -rf "$old"
            log "Rotated old backup: $(basename "$old")"
        done
    fi
fi

# --- Deploy new release ---

# Copy new binary
cp "$STAGING/gamedata/dystopia" "$TARGET_DIR/gamedata/dystopia"
chmod +x "$TARGET_DIR/gamedata/dystopia"
log "Installed new binary"

# Copy area databases (all safe to overwrite)
if [ -d "$STAGING/gamedata/db/areas" ]; then
    cp "$STAGING/gamedata/db/areas/"*.db "$TARGET_DIR/gamedata/db/areas/"
    log "Updated area databases"
fi

# Copy ONLY safe game databases — never touch game.db or live_help.db
for db in $SAFE_GAME_DBS; do
    if [ -f "$STAGING/gamedata/db/game/$db" ]; then
        cp "$STAGING/gamedata/db/game/$db" "$TARGET_DIR/gamedata/db/game/$db"
    fi
done
log "Updated game databases (safe files only)"

# Copy documentation
if [ -d "$STAGING/gamedata/doc" ]; then
    cp -r "$STAGING/gamedata/doc/." "$TARGET_DIR/gamedata/doc/"
    log "Updated documentation"
fi

# Copy startup script
if [ -f "$STAGING/startup.sh" ]; then
    cp "$STAGING/startup.sh" "$TARGET_DIR/startup.sh"
    chmod +x "$TARGET_DIR/startup.sh"
    log "Updated startup.sh"
fi

# Record new version
echo "$NEW_VERSION" > "$TARGET_DIR/.version"

# --- Clean up ingest (only for real deployments) ---
if ! $IS_DRYRUN; then
    rm -rf "$STAGING"
    rm -f "$INGEST_DIR/.pending_version" "$INGEST_DIR/.ready" "$INGEST_DIR/.release.json"
    log "Cleaned ingest directory"
fi

log ""
log "=== Migration complete: $CURRENT -> $NEW_VERSION ==="
log ""
if $IS_DRYRUN; then
    log "Dry-run deploy to: $TARGET_DIR"
    log "Ingest directory was NOT cleaned."
    log "To deploy for real, re-run with the live directory as an argument."
else
    log "The running server is still using the old binary in memory."
    log "To activate the update:"
    log "  1. Connect to the MUD as an admin (level 7+)"
    log "  2. Run the 'copyover' command in-game"
    log "  3. The server will re-exec with the new binary"
    if [ "$CURRENT" != "none" ]; then
        log ""
        log "Previous version backed up to: $TARGET_DIR/backups/$CURRENT/"
    fi
fi
