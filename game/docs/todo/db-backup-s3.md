# AWS S3 Database Backup System for Dystopia MUD

## Goal
Enable off-site backups of all SQLite databases to AWS S3 for disaster recovery, with both scheduled (cron) and manual (in-game admin command) triggers.

## Architecture
```
MUD Server (Linux)
├── gamedata/db/areas/      ─┐
├── gamedata/db/players/     ├──► backup_to_s3.sh ──► [AWS S3 Bucket]
└── gamedata/db/game/       ─┘         │
                                       │
Triggers:                              │
├── Cron job (daily at 3am) ──────────┘
└── In-game `backup` command ─────────┘
```

**Backup flow:**
1. SQLite `.backup` command creates safe copies (handles WAL mode)
2. Files compressed into timestamped `.tar.gz` archive
3. Archive uploaded to S3 with configurable prefix
4. Old backups pruned based on retention policy

---

## Current Database Structure

| Category | Location | Contents | Size |
|----------|----------|----------|------|
| **Area DBs** | `gamedata/db/areas/` | 71 SQLite files (mobs, objects, rooms) | ~5.7MB |
| **Player DBs** | `gamedata/db/players/` | Per-player character data | ~1.4MB |
| **Game DBs** | `gamedata/db/game/` | Help, config, leaderboards, kingdoms | ~1.1MB |

**Current backup status:**
- Player DBs have basic local backup (`players/backup/` subfolder, no rotation)
- Area and Game DBs have no backup mechanism
- No off-site backup exists

---

## Prerequisites

### Step 1: Install AWS CLI

```bash
# Debian/Ubuntu
sudo apt install awscli

# RHEL/CentOS/Fedora
sudo yum install awscli
# or
sudo dnf install awscli

# Verify installation
aws --version
```

### Step 2: Configure AWS Credentials

```bash
# Interactive setup
aws configure

# Enter:
# AWS Access Key ID: [your-access-key]
# AWS Secret Access Key: [your-secret-key]
# Default region: us-east-1 (or your preferred region)
# Default output format: json
```

### Step 3: Create S3 Bucket

```bash
# Create bucket (name must be globally unique)
aws s3 mb s3://dystopia-mud-backups

# Enable versioning (optional but recommended)
aws s3api put-bucket-versioning \
  --bucket dystopia-mud-backups \
  --versioning-configuration Status=Enabled
```

### Step 4: IAM Permissions

Create an IAM user/role with this policy:
```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": [
                "s3:PutObject",
                "s3:GetObject",
                "s3:ListBucket",
                "s3:DeleteObject"
            ],
            "Resource": [
                "arn:aws:s3:::dystopia-mud-backups",
                "arn:aws:s3:::dystopia-mud-backups/*"
            ]
        }
    ]
}
```

---

## Implementation Steps

### Step 1: Create Configuration File

Create `gamedata/backup.conf`:
```ini
# AWS S3 Backup Configuration for Dystopia MUD

# S3 bucket name (must exist and have appropriate permissions)
S3_BUCKET=dystopia-mud-backups

# Prefix for backup files (e.g., "prod" or "staging")
S3_PREFIX=backups

# Number of days to retain backups (older backups are deleted)
RETENTION_DAYS=30

# AWS CLI profile (leave empty for default or instance role)
AWS_PROFILE=

# Optional: specify database directory (auto-detected if empty)
DB_DIR=
```

### Step 2: Create Backup Script

Create `game/tools/backup_to_s3.sh`:
```bash
#!/bin/bash
#
# Dystopia MUD - Database Backup to AWS S3
#
# Usage:
#   ./backup_to_s3.sh              # Full backup of all databases
#   ./backup_to_s3.sh --dry-run    # Test without uploading
#   ./backup_to_s3.sh --verbose    # Show detailed progress
#

set -e

# Determine script and project directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
GAMEDATA_DIR="$PROJECT_DIR/gamedata"
CONFIG_FILE="$GAMEDATA_DIR/backup.conf"
LOG_FILE="$GAMEDATA_DIR/log/backup.log"

# Defaults
S3_BUCKET=""
S3_PREFIX="backups"
RETENTION_DAYS=30
AWS_PROFILE=""
DRY_RUN=false
VERBOSE=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

log() {
    local msg="[$(date '+%Y-%m-%d %H:%M:%S')] $1"
    echo "$msg"
    echo "$msg" >> "$LOG_FILE"
}

log_verbose() {
    if $VERBOSE; then
        log "$1"
    fi
}

# Load configuration
if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
else
    log "ERROR: Configuration file not found: $CONFIG_FILE"
    exit 1
fi

if [[ -z "$S3_BUCKET" ]]; then
    log "ERROR: S3_BUCKET not configured in $CONFIG_FILE"
    exit 1
fi

# Set up AWS profile if specified
AWS_CMD="aws"
if [[ -n "$AWS_PROFILE" ]]; then
    AWS_CMD="aws --profile $AWS_PROFILE"
fi

# Create temporary directory for backup
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
BACKUP_DIR=$(mktemp -d)
BACKUP_NAME="dystopia_backup_${TIMESTAMP}"
ARCHIVE_NAME="${BACKUP_NAME}.tar.gz"

log "Starting backup: $BACKUP_NAME"
log_verbose "Temporary directory: $BACKUP_DIR"

# Function to safely backup a SQLite database
backup_sqlite_db() {
    local src="$1"
    local dst="$2"

    if [[ -f "$src" ]]; then
        # Use SQLite's .backup command for safe hot backup
        sqlite3 "$src" ".backup '$dst'" 2>/dev/null || cp "$src" "$dst"
        log_verbose "Backed up: $(basename "$src")"
    fi
}

# Backup areas
log "Backing up area databases..."
mkdir -p "$BACKUP_DIR/areas"
for db in "$GAMEDATA_DIR"/db/areas/*.db; do
    if [[ -f "$db" ]]; then
        backup_sqlite_db "$db" "$BACKUP_DIR/areas/$(basename "$db")"
    fi
done

# Backup players
log "Backing up player databases..."
mkdir -p "$BACKUP_DIR/players"
for db in "$GAMEDATA_DIR"/db/players/*.db; do
    if [[ -f "$db" ]]; then
        backup_sqlite_db "$db" "$BACKUP_DIR/players/$(basename "$db")"
    fi
done

# Backup game databases
log "Backing up game databases..."
mkdir -p "$BACKUP_DIR/game"
for db in "$GAMEDATA_DIR"/db/game/*.db; do
    if [[ -f "$db" ]]; then
        backup_sqlite_db "$db" "$BACKUP_DIR/game/$(basename "$db")"
    fi
done

# Create compressed archive
log "Creating archive..."
cd "$BACKUP_DIR"
tar -czf "/tmp/$ARCHIVE_NAME" .
ARCHIVE_SIZE=$(du -h "/tmp/$ARCHIVE_NAME" | cut -f1)
log "Archive created: $ARCHIVE_NAME ($ARCHIVE_SIZE)"

# Upload to S3
S3_PATH="s3://${S3_BUCKET}/${S3_PREFIX}/${ARCHIVE_NAME}"
if $DRY_RUN; then
    log "DRY RUN: Would upload to $S3_PATH"
else
    log "Uploading to S3..."
    $AWS_CMD s3 cp "/tmp/$ARCHIVE_NAME" "$S3_PATH"
    log "Upload complete: $S3_PATH"
fi

# Cleanup old backups
if [[ $RETENTION_DAYS -gt 0 ]] && ! $DRY_RUN; then
    log "Cleaning up backups older than $RETENTION_DAYS days..."
    CUTOFF_DATE=$(date -d "-${RETENTION_DAYS} days" '+%Y-%m-%d')

    $AWS_CMD s3 ls "s3://${S3_BUCKET}/${S3_PREFIX}/" | while read -r line; do
        FILE_DATE=$(echo "$line" | awk '{print $1}')
        FILE_NAME=$(echo "$line" | awk '{print $4}')

        if [[ "$FILE_DATE" < "$CUTOFF_DATE" ]] && [[ "$FILE_NAME" == dystopia_backup_* ]]; then
            log_verbose "Deleting old backup: $FILE_NAME"
            $AWS_CMD s3 rm "s3://${S3_BUCKET}/${S3_PREFIX}/${FILE_NAME}"
        fi
    done
fi

# Cleanup temporary files
rm -rf "$BACKUP_DIR"
rm -f "/tmp/$ARCHIVE_NAME"

log "Backup complete!"
exit 0
```

Make it executable:
```bash
chmod +x game/tools/backup_to_s3.sh
```

### Step 3: Create Cron Job

Create `game/tools/backup_cron.example`:
```cron
# Dystopia MUD - Database Backup Schedule
#
# Install with: crontab -e
# Then add the line below (adjust path as needed)

# Daily backup at 3:00 AM server time
0 3 * * * /path/to/dystopia-mud/game/tools/backup_to_s3.sh >> /path/to/dystopia-mud/gamedata/log/backup.log 2>&1

# Optional: Weekly full backup on Sunday at 4:00 AM
# 0 4 * * 0 /path/to/dystopia-mud/game/tools/backup_to_s3.sh --verbose >> /path/to/dystopia-mud/gamedata/log/backup.log 2>&1
```

Install the cron job:
```bash
# Edit crontab
crontab -e

# Add the backup line with correct path
0 3 * * * /home/mud/dystopia-mud/game/tools/backup_to_s3.sh >> /home/mud/dystopia-mud/gamedata/log/backup.log 2>&1
```

### Step 4: Add In-Game Admin Command

**Add to `game/src/core/merc.h`** (with other command prototypes):
```c
DECLARE_DO_FUN( do_backup );
```

**Add to `game/src/core/interp.c`** (in `cmd_table[]`):
```c
{ "backup",      do_backup,      POS_DEAD,    MAX_LEVEL, LOG_ALWAYS, TRUE  },
```

**Add to `game/src/commands/wizutil.c`**:
```c
/*
 * do_backup - Trigger off-site database backup to S3
 * Admin-only command (MAX_LEVEL) to manually initiate backup
 */
void do_backup( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char script_path[MAX_STRING_LENGTH];
    pid_t pid;

    /* Build path to backup script */
    snprintf( script_path, sizeof(script_path),
              "%s/../game/tools/backup_to_s3.sh", mud_base_dir );

    /* Check if backup script exists */
    if ( access( script_path, X_OK ) != 0 )
    {
        send_to_char( "Backup script not found or not executable.\n\r", ch );
        return;
    }

    /* Check for argument */
    if ( argument[0] != '\0' && !str_cmp( argument, "status" ) )
    {
        /* Show recent backup log entries */
        send_to_char( "Recent backup activity:\n\r", ch );
        snprintf( buf, sizeof(buf), "tail -20 %s/log/backup.log 2>/dev/null || echo 'No backup log found.'", mud_base_dir );
        /* Would need to implement log reading - for now just indicate feature */
        send_to_char( "Use 'tail gamedata/log/backup.log' from shell to view logs.\n\r", ch );
        return;
    }

    send_to_char( "Initiating database backup to S3...\n\r", ch );

    /* Fork and execute backup script */
    pid = fork();

    if ( pid < 0 )
    {
        send_to_char( "Failed to start backup process.\n\r", ch );
        return;
    }
    else if ( pid == 0 )
    {
        /* Child process - execute backup script */
        execl( "/bin/sh", "sh", "-c", script_path, (char *)NULL );
        exit( 1 );  /* If execl fails */
    }
    else
    {
        /* Parent process */
        snprintf( buf, sizeof(buf), "Backup process started (PID: %d).\n\r", pid );
        send_to_char( buf, ch );
        send_to_char( "Check 'gamedata/log/backup.log' for progress.\n\r", ch );
    }
}
```

---

## Files to Create

| File | Purpose |
|------|---------|
| `game/tools/backup_to_s3.sh` | Main backup script |
| `gamedata/backup.conf` | Backup configuration (copy from template) |
| `gamedata/backup.conf.example` | Configuration template (safe to commit) |
| `game/tools/backup_cron.example` | Cron job template |

## Files to Modify

| File | Change |
|------|--------|
| `game/src/core/merc.h` | Add `DECLARE_DO_FUN( do_backup );` |
| `game/src/core/interp.c` | Add `backup` command to `cmd_table[]` |
| `game/src/commands/wizutil.c` | Add `do_backup()` function |

---

## Verification Checklist

### Prerequisites
- [ ] AWS CLI installed (`aws --version`)
- [ ] AWS credentials configured (`aws sts get-caller-identity`)
- [ ] S3 bucket exists (`aws s3 ls s3://your-bucket-name`)
- [ ] sqlite3 CLI available (`which sqlite3`)

### Backup Script
- [ ] Configuration file created (`cat gamedata/backup.conf`)
- [ ] Script is executable (`ls -la game/tools/backup_to_s3.sh`)
- [ ] Dry run works (`./game/tools/backup_to_s3.sh --dry-run`)
- [ ] Actual backup works (`./game/tools/backup_to_s3.sh --verbose`)
- [ ] Backup appears in S3 (`aws s3 ls s3://your-bucket/backups/`)

### Cron Job
- [ ] Cron job installed (`crontab -l`)
- [ ] Log file created after scheduled time (`cat gamedata/log/backup.log`)
- [ ] Backup appears in S3 after scheduled time

### In-Game Command
- [ ] MUD compiles with new command
- [ ] `backup` command available to MAX_LEVEL admins
- [ ] Command triggers backup and reports status

### Restore Test
- [ ] Download backup from S3 (`aws s3 cp s3://your-bucket/backups/dystopia_backup_*.tar.gz .`)
- [ ] Extract archive (`tar -xzf dystopia_backup_*.tar.gz`)
- [ ] SQLite files open correctly (`sqlite3 areas/midgaard.db ".tables"`)

---

## Troubleshooting

**"aws: command not found":**
```bash
# Install AWS CLI
sudo apt install awscli

# Or install via pip
pip install awscli
```

**"Unable to locate credentials":**
```bash
# Configure AWS credentials
aws configure

# Verify credentials work
aws sts get-caller-identity
```

**"Access Denied" when uploading:**
```bash
# Check bucket policy allows your IAM user
aws s3api get-bucket-policy --bucket your-bucket-name

# Verify IAM permissions
aws iam get-user-policy --user-name your-user --policy-name your-policy
```

**"sqlite3: command not found":**
```bash
# Install SQLite CLI
sudo apt install sqlite3
```

**Backup script fails silently:**
```bash
# Run with verbose output
./game/tools/backup_to_s3.sh --verbose

# Check log file
tail -50 gamedata/log/backup.log
```

**Cron job not running:**
```bash
# Check cron service
sudo systemctl status cron

# Check cron logs
grep CRON /var/log/syslog | tail -20

# Verify crontab entry
crontab -l
```

---

## Restore Procedure

To restore from backup:

```bash
# 1. List available backups
aws s3 ls s3://dystopia-mud-backups/backups/

# 2. Download desired backup
aws s3 cp s3://dystopia-mud-backups/backups/dystopia_backup_20240115_030000.tar.gz .

# 3. Stop the MUD
./shutdown.sh  # or kill the process

# 4. Backup current databases (just in case)
mv gamedata/db gamedata/db.old

# 5. Create new db directory
mkdir -p gamedata/db/{areas,players,game}

# 6. Extract backup
tar -xzf dystopia_backup_20240115_030000.tar.gz -C gamedata/db/

# 7. Restart the MUD
./startup.sh
```

---

## Cost Estimation

Based on ~8MB total database size:
- **Compressed backup size**: ~2-3MB per backup
- **Daily backups, 30-day retention**: ~60-90MB stored
- **S3 Standard pricing** (~$0.023/GB/month): ~$0.002/month
- **PUT requests** (~30/month at $0.005/1000): ~$0.0002/month
- **Total estimated cost**: < $0.01/month

For larger databases or longer retention, consider S3 Intelligent-Tiering or Glacier for additional savings.
