# Server Deployment

This directory contains the deployment pipeline for self-hosted Dystopia MUD servers. When a new version is tagged on GitHub, the release workflow builds it and pushes it to your server automatically via SCP.

## Directory Structure

```
server/
├── server.conf          # Server configuration (port)
├── scripts/
│   ├── validate.sh      # Verify a staged release is valid
│   └── migrate.sh       # Deploy staged release to live/
├── live/                # Production server (created on first deploy)
│   ├── .version         # Current version tag
│   ├── gamedata/        # Binary, databases, logs, player files
│   ├── startup.sh       # Server startup script
│   └── backups/         # Versioned backups from previous migrations
│       ├── v1.0.0/
│       └── v1.1.0/
└── ingest/              # Staging area (populated by GitHub Actions)
    ├── dystopia/        # Extracted release
    ├── .pending_version # Version being staged
    └── .ready           # Signal that upload is complete
```

## Setup

### 1. Generate an SSH Deploy Key

On your local machine, generate a dedicated keypair:

```bash
ssh-keygen -t ed25519 -f ~/.ssh/dystopia_deploy -N "" -C "dystopia-deploy"
```

This creates two files:
- `~/.ssh/dystopia_deploy` — private key (goes into GitHub Secrets)
- `~/.ssh/dystopia_deploy.pub` — public key (goes on your server)

### 2. Add the Public Key to Your Server

Copy the public key to the server account that will receive deployments:

```bash
ssh-copy-id -i ~/.ssh/dystopia_deploy.pub youruser@yourserver
```

Or manually append it to `~/.ssh/authorized_keys` on the server.

### 3. Create the Server Directory

On your server, create the `server/` directory structure:

```bash
mkdir -p /path/to/server/ingest
mkdir -p /path/to/server/live
mkdir -p /path/to/server/scripts
```

Copy `scripts/validate.sh` and `scripts/migrate.sh` to the server's `scripts/` directory, and `server.conf` to the server's root.

### 4. Add GitHub Secrets

In your GitHub repository, go to **Settings > Secrets and variables > Actions** and add these repository secrets:

| Secret | Description | Example |
|--------|-------------|---------|
| `DEPLOY_SSH_KEY` | Contents of the private key file | (paste entire file including BEGIN/END lines) |
| `DEPLOY_HOST` | Server hostname or IP address | `mud.example.com` |
| `DEPLOY_USER` | SSH username on the server | `fatu` |
| `DEPLOY_PATH` | Absolute path to `server/` directory | `/home/fatu/dystopia/server` |

If these secrets are not set, the deploy step is skipped — forks without a server are unaffected.

## Deployment Flow

### Automatic (GitHub Actions)

1. **Tag a release**: `git tag v1.2.0 && git push origin v1.2.0`
2. **GitHub builds** the Linux release and publishes it
3. **GitHub SCPs** the tarball to your server's `ingest/` directory and writes `.ready`
4. **In-game**, an admin (level 12) runs:
   - `deploybot check` — confirms the new version is staged
   - `deploybot validate` — runs integrity checks
   - `deploybot migrate` — backs up current version, deploys new files, cleans staging
   - `copyover` — hot-swaps the running binary

### Manual (No GitHub Secrets)

If you prefer manual deployment or your server setup differs:

1. Download the release tarball from [GitHub Releases](../../releases)
2. Copy it to the server: `scp dystopia-linux-amd64.tar.gz user@server:/path/to/server/ingest/`
3. On the server:
   ```bash
   cd /path/to/server/ingest
   tar -xzf dystopia-linux-amd64.tar.gz
   rm dystopia-linux-amd64.tar.gz
   echo "v1.2.0" > .pending_version
   echo "ready" > .ready
   ```
4. Then use `deploybot` in-game or run the scripts directly:
   ```bash
   sh scripts/validate.sh
   sh scripts/migrate.sh
   ```

## Backups

Each migration creates a versioned backup in `live/backups/`:

```
live/backups/v1.1.0/
├── dystopia           # Previous binary
├── db/areas/*.db      # Previous area databases
└── db/game/*.db       # Previous safe game databases
```

The last 3 backups are kept. Older backups are rotated out automatically.

To manually roll back, copy the backup files back to `live/gamedata/` and run `copyover` in-game.

## Configuration

Edit `server/server.conf` to set the game server port:

```bash
PORT=8000
```

The startup script (`startup.sh` / `startup.bat`) reads this value automatically.
