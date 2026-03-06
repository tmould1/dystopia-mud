#!/bin/sh
# validate.sh — Verify the downloaded release in ingest/ is valid
set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
INGEST_DIR="$BASE_DIR/ingest"
STAGING="$INGEST_DIR/dystopia"

PASS=0
FAIL=0
TOTAL=0

check() {
    TOTAL=$((TOTAL + 1))
    desc="$1"
    shift
    if "$@" >/dev/null 2>&1; then
        echo "  PASS  $desc"
        PASS=$((PASS + 1))
    else
        echo "  FAIL  $desc"
        FAIL=$((FAIL + 1))
    fi
}

echo "[validate] Validating release in $STAGING"
echo ""

# 1. Staging directory exists
check "Staging directory exists" test -d "$STAGING"

# 2. Binary exists
check "Binary exists" test -f "$STAGING/gamedata/dystopia"

# 3. Binary is ELF 64-bit
check "Binary is ELF 64-bit executable" sh -c "file '$STAGING/gamedata/dystopia' | grep -q 'ELF 64-bit'"

# 4. Binary has execute permission (fix if not)
if [ -f "$STAGING/gamedata/dystopia" ] && [ ! -x "$STAGING/gamedata/dystopia" ]; then
    chmod +x "$STAGING/gamedata/dystopia"
fi
check "Binary is executable" test -x "$STAGING/gamedata/dystopia"

# 5. Area databases present (at least 10)
AREA_COUNT=0
if [ -d "$STAGING/gamedata/db/areas" ]; then
    AREA_COUNT=$(find "$STAGING/gamedata/db/areas" -name '*.db' -type f | wc -l)
fi
check "Area databases present ($AREA_COUNT found, need 10+)" test "$AREA_COUNT" -ge 10

# 6. Required game databases
check "base_help.db exists" test -f "$STAGING/gamedata/db/game/base_help.db"
check "class.db exists" test -f "$STAGING/gamedata/db/game/class.db"
check "tables.db exists" test -f "$STAGING/gamedata/db/game/tables.db"
check "quest.db exists" test -f "$STAGING/gamedata/db/game/quest.db"

# 7. Startup script
check "startup.sh exists" test -f "$STAGING/startup.sh"

# 8. Pending version file
check "Pending version file exists" test -f "$INGEST_DIR/.pending_version"

# Summary
echo ""
if [ "$FAIL" -eq 0 ]; then
    echo "[validate] Validation passed: $PASS/$TOTAL checks OK"
    if [ -f "$INGEST_DIR/.pending_version" ]; then
        echo "[validate] Version: $(cat "$INGEST_DIR/.pending_version")"
    fi
    echo "[validate] Next step: run migrate.sh"
    exit 0
else
    echo "[validate] Validation FAILED: $PASS passed, $FAIL failed out of $TOTAL"
    exit 1
fi
