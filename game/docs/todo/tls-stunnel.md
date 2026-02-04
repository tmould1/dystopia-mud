# Stunnel TLS Proxy Setup for Dystopia MUD (Linux)

## Goal
Enable secure (TLS-encrypted) connections for Mudlet and other TLS-capable MUD clients.

## Architecture
```
Mudlet (Secure) → [TLS Port 8887] → Stunnel → [localhost:8888] → MUD
                   (encrypted)                   (plaintext)
```

Players can connect either way:
- **Port 8888** - Plain telnet (legacy clients)
- **Port 8887** - TLS encrypted (Mudlet with "Secure" checked)

---

## Implementation Steps

### Step 1: Install Stunnel

```bash
# Debian/Ubuntu
sudo apt install stunnel4

# RHEL/CentOS/Fedora
sudo yum install stunnel
# or
sudo dnf install stunnel
```

### Step 2: Generate TLS Certificate

**Option A: Self-signed (for testing/private servers)**
```bash
sudo openssl req -new -x509 -days 365 -nodes \
  -out /etc/stunnel/stunnel.pem \
  -keyout /etc/stunnel/stunnel.pem \
  -subj "/CN=dystopia-mud"

sudo chmod 600 /etc/stunnel/stunnel.pem
```

**Option B: Let's Encrypt (for production with domain name)**
```bash
# Install certbot
sudo apt install certbot

# Get certificate (requires port 80 open, domain pointing to server)
sudo certbot certonly --standalone -d mud.yourdomain.com

# Certificates will be at:
# /etc/letsencrypt/live/mud.yourdomain.com/fullchain.pem
# /etc/letsencrypt/live/mud.yourdomain.com/privkey.pem
```

### Step 3: Create Stunnel Configuration

Create `/etc/stunnel/dystopia.conf`:
```ini
; Stunnel configuration for Dystopia MUD

; Run as daemon
foreground = no
pid = /var/run/stunnel/dystopia.pid

; Certificate (self-signed)
cert = /etc/stunnel/stunnel.pem

; OR for Let's Encrypt (uncomment and adjust domain):
; cert = /etc/letsencrypt/live/mud.yourdomain.com/fullchain.pem
; key = /etc/letsencrypt/live/mud.yourdomain.com/privkey.pem

; Service definition
[dystopia-tls]
accept = 8887
connect = 127.0.0.1:8888
```

### Step 4: Start Stunnel

```bash
# Create pid directory if needed
sudo mkdir -p /var/run/stunnel
sudo chown stunnel:stunnel /var/run/stunnel

# Test configuration manually
sudo stunnel /etc/stunnel/dystopia.conf

# Or enable as systemd service (recommended)
sudo systemctl enable stunnel@dystopia
sudo systemctl start stunnel@dystopia

# Check status
sudo systemctl status stunnel@dystopia
```

### Step 5: Update Firewall

```bash
# UFW (Ubuntu/Debian)
sudo ufw allow 8887/tcp

# firewalld (RHEL/CentOS/Fedora)
sudo firewall-cmd --permanent --add-port=8887/tcp
sudo firewall-cmd --reload

# iptables (direct)
sudo iptables -A INPUT -p tcp --dport 8887 -j ACCEPT
```

### Step 6: Test Connection

```bash
# From the server itself
openssl s_client -connect localhost:8887

# Should see TLS handshake info, then MUD welcome screen
```

**From Mudlet:**
1. Create new profile or edit existing
2. Server: `your-server-ip` or `mud.yourdomain.com`
3. Port: `8887`
4. Check **"Secure"** checkbox
5. Connect

---

## Files to Create

| File | Purpose |
|------|---------|
| `/etc/stunnel/dystopia.conf` | Stunnel configuration |
| `/etc/stunnel/stunnel.pem` | TLS certificate (if self-signed) |

---

## Optional: Integrate with startup.sh

Add to `startup.sh`:
```bash
# Start stunnel if not running
if ! pgrep -f "stunnel.*dystopia" > /dev/null; then
    echo "Starting stunnel for TLS..."
    stunnel /etc/stunnel/dystopia.conf
fi
```

---

## Verification Checklist

- [ ] Stunnel installed (`which stunnel`)
- [ ] Certificate generated (`ls -la /etc/stunnel/stunnel.pem`)
- [ ] Configuration file created (`cat /etc/stunnel/dystopia.conf`)
- [ ] Stunnel running (`ps aux | grep stunnel`)
- [ ] Port 8887 open in firewall (`sudo ufw status` or `ss -tlnp | grep 8887`)
- [ ] `openssl s_client -connect localhost:8887` shows MUD welcome
- [ ] Mudlet connects with "Secure" checkbox enabled
- [ ] GMCP/MCCP still work over TLS

---

## Troubleshooting

**"Connection refused" on port 8887:**
```bash
# Check stunnel is running
ps aux | grep stunnel

# Check it's listening on the right port
ss -tlnp | grep 8887

# Check stunnel logs
sudo journalctl -u stunnel@dystopia
# or
sudo tail /var/log/stunnel/dystopia.log
```

**"Certificate verify failed" in Mudlet:**
- Self-signed certs will show a warning - this is expected
- Mudlet should still connect after accepting the warning
- For production, use Let's Encrypt to avoid warnings

**MUD protocols not working:**
- TLS is transparent - GMCP/MCCP should work identically
- Stunnel just wraps the TCP stream, doesn't modify content

---

## Let's Encrypt Certificate Renewal

If using Let's Encrypt, set up auto-renewal:
```bash
# Test renewal
sudo certbot renew --dry-run

# Certbot usually sets up a cron job automatically
# To reload stunnel after renewal, create a deploy hook:
sudo mkdir -p /etc/letsencrypt/renewal-hooks/deploy
sudo cat > /etc/letsencrypt/renewal-hooks/deploy/stunnel-reload.sh << 'EOF'
#!/bin/bash
systemctl restart stunnel@dystopia
EOF
sudo chmod +x /etc/letsencrypt/renewal-hooks/deploy/stunnel-reload.sh
```
