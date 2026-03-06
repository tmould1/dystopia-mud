#!/bin/sh
# download.sh — Check for latest GitHub release and download to ingest/

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
LIVE_DIR="$BASE_DIR/live"
INGEST_DIR="$BASE_DIR/ingest"
CONF="$BASE_DIR/server.conf"

log() { echo "[download] $*"; }
die() { log "ERROR: $*" >&2; exit 1; }

# Extract a JSON string value by key (simple grep/sed, no jq needed)
json_value() {
    grep -o "\"$1\" *: *\"[^\"]*\"" | head -1 | sed 's/.*: *"//;s/"$//'
}

# Portable HTTP fetch — tries curl, falls back to wget
# Usage: fetch_url <url> <output_file> [auth_token]
fetch_url() {
    _url="$1"
    _out="$2"
    _token="${3:-}"

    if command -v curl >/dev/null 2>&1; then
        if [ -n "$_token" ]; then
            curl -sL -H "Accept: application/vnd.github+json" \
                -H "Authorization: Bearer $_token" \
                -o "$_out" -w "%{http_code}" "$_url" || echo "000"
        else
            curl -sL -H "Accept: application/vnd.github+json" \
                -o "$_out" -w "%{http_code}" "$_url" || echo "000"
        fi
    elif command -v wget >/dev/null 2>&1; then
        if [ -n "$_token" ]; then
            wget -q --header="Accept: application/vnd.github+json" \
                --header="Authorization: Bearer $_token" \
                -O "$_out" "$_url" 2>/dev/null && echo "200" || echo "000"
        else
            wget -q --header="Accept: application/vnd.github+json" \
                -O "$_out" "$_url" 2>/dev/null && echo "200" || echo "000"
        fi
    else
        die "Neither curl nor wget is available"
    fi
}

# Portable file download with progress
# Usage: download_file <url> <output_file> [auth_token]
download_file() {
    _url="$1"
    _out="$2"
    _token="${3:-}"

    if command -v curl >/dev/null 2>&1; then
        if [ -n "$_token" ]; then
            curl -L --fail --progress-bar \
                -H "Authorization: Bearer $_token" \
                -o "$_out" "$_url"
        else
            curl -L --fail --progress-bar -o "$_out" "$_url"
        fi
    elif command -v wget >/dev/null 2>&1; then
        if [ -n "$_token" ]; then
            wget --header="Authorization: Bearer $_token" \
                -O "$_out" "$_url"
        else
            wget -O "$_out" "$_url"
        fi
    fi
}

# Load configuration
[ -f "$CONF" ] || die "Config file not found: $CONF"
. "$CONF"
[ -n "${REPO:-}" ] || die "REPO not set in $CONF"
[ -n "${ASSET_NAME:-}" ] || die "ASSET_NAME not set in $CONF"

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

HTTP_CODE=$(fetch_url "$API_URL" "$INGEST_DIR/.release.json" "${GITHUB_TOKEN:-}")

if [ "$HTTP_CODE" = "000" ]; then
    die "Could not connect to GitHub API — check DNS and network"
fi
if [ "$HTTP_CODE" = "404" ]; then
    die "No releases found for $REPO"
fi
if [ "$HTTP_CODE" != "200" ]; then
    die "GitHub API returned HTTP $HTTP_CODE"
fi

log "Got release info (HTTP $HTTP_CODE)"

# Parse release info
TAG=$(json_value "tag_name" < "$INGEST_DIR/.release.json")
ASSET_URL=$(grep -o "\"browser_download_url\" *: *\"[^\"]*${ASSET_NAME}\"" \
    "$INGEST_DIR/.release.json" | head -1 | sed 's/.*: *"//;s/"$//') || true

if [ -z "$TAG" ]; then
    die "Could not parse tag_name from release JSON"
fi
log "Latest release: $TAG"

# Check if already up to date
if [ "$TAG" = "$CURRENT" ]; then
    log "Already up to date ($TAG)"
    rm -f "$INGEST_DIR/.release.json"
    exit 0
fi

if [ -z "$ASSET_URL" ]; then
    die "Asset '$ASSET_NAME' not found in release $TAG"
fi

# Clean ingest directory (preserve .release.json briefly)
find "$INGEST_DIR" -mindepth 1 ! -name '.release.json' -exec rm -rf {} + 2>/dev/null || true

# Download
log "Downloading $ASSET_NAME..."
download_file "$ASSET_URL" "$INGEST_DIR/$ASSET_NAME" "${GITHUB_TOKEN:-}" || die "Download failed"

# Extract
log "Extracting..."
tar -xzf "$INGEST_DIR/$ASSET_NAME" -C "$INGEST_DIR" || die "Extraction failed"

# Record pending version
echo "$TAG" > "$INGEST_DIR/.pending_version"

# Clean up
rm -f "$INGEST_DIR/$ASSET_NAME" "$INGEST_DIR/.release.json"

log "Download complete: $CURRENT -> $TAG"
log "Next step: run validate.sh"
