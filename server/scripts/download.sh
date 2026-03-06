#!/bin/bash
# download.sh — Check for latest GitHub release and download to ingest/
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LIVE_DIR="$BASE_DIR/live"
INGEST_DIR="$BASE_DIR/ingest"
CONF="$BASE_DIR/server.conf"

log() { echo "[download] $*"; }
die() { log "ERROR: $*" >&2; exit 1; }

# Load configuration
[ -f "$CONF" ] || die "Config file not found: $CONF"
source "$CONF"
[ -n "${REPO:-}" ] || die "REPO not set in $CONF"
[ -n "${ASSET_NAME:-}" ] || die "ASSET_NAME not set in $CONF"

# Auth header if token provided
AUTH_HEADER=""
if [ -n "${GITHUB_TOKEN:-}" ]; then
    AUTH_HEADER="Authorization: Bearer $GITHUB_TOKEN"
fi

# Ensure directories exist
mkdir -p "$LIVE_DIR" "$INGEST_DIR"

# Current installed version
CURRENT="none"
if [ -f "$LIVE_DIR/.version" ]; then
    CURRENT="$(cat "$LIVE_DIR/.version")"
fi
log "Current version: $CURRENT"

# Query GitHub API for latest release
log "Checking for latest release of $REPO..."
API_URL="https://api.github.com/repos/${REPO}/releases/latest"

CURL_ARGS=(-sL -H "Accept: application/vnd.github+json")
if [ -n "$AUTH_HEADER" ]; then
    CURL_ARGS+=(-H "$AUTH_HEADER")
fi

HTTP_CODE=$(curl "${CURL_ARGS[@]}" -o "$INGEST_DIR/.release.json" -w "%{http_code}" "$API_URL")

if [ "$HTTP_CODE" = "404" ]; then
    die "No releases found for $REPO"
fi
if [ "$HTTP_CODE" != "200" ]; then
    die "GitHub API returned HTTP $HTTP_CODE"
fi

# Parse release info
TAG=$(jq -r '.tag_name' "$INGEST_DIR/.release.json")
ASSET_URL=$(jq -r --arg name "$ASSET_NAME" '.assets[] | select(.name == $name) | .browser_download_url' "$INGEST_DIR/.release.json")

[ -n "$TAG" ] && [ "$TAG" != "null" ] || die "Could not parse tag_name from release"
log "Latest release: $TAG"

# Check if already up to date
if [ "$TAG" = "$CURRENT" ]; then
    log "Already up to date ($TAG)"
    rm -f "$INGEST_DIR/.release.json"
    exit 0
fi

[ -n "$ASSET_URL" ] && [ "$ASSET_URL" != "null" ] || die "Asset '$ASSET_NAME' not found in release $TAG"

# Clean ingest directory (preserve .release.json briefly)
find "$INGEST_DIR" -mindepth 1 ! -name '.release.json' -exec rm -rf {} + 2>/dev/null || true

# Download
log "Downloading $ASSET_NAME..."
CURL_DL_ARGS=(-L --fail --progress-bar)
if [ -n "$AUTH_HEADER" ]; then
    CURL_DL_ARGS+=(-H "$AUTH_HEADER")
fi
curl "${CURL_DL_ARGS[@]}" -o "$INGEST_DIR/$ASSET_NAME" "$ASSET_URL"

# Extract
log "Extracting..."
tar -xzf "$INGEST_DIR/$ASSET_NAME" -C "$INGEST_DIR"

# Record pending version
echo "$TAG" > "$INGEST_DIR/.pending_version"

# Clean up
rm -f "$INGEST_DIR/$ASSET_NAME" "$INGEST_DIR/.release.json"

log "Download complete: $CURRENT -> $TAG"
log "Next step: run validate.sh"
