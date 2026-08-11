# AFRIH Deployment Guide

## Option 1: Docker (Recommended for VPS)

```bash
# Clone the repo
git clone https://github.com/asiimwe3/afrih-website.git
cd afrih-website

# Build and run with Docker Compose
docker-compose up -d

# The site will be available on port 80
# Check: curl http://localhost
```

## Option 2: Direct Compilation on VPS

```bash
# Install gcc
apt update && apt install -y gcc

# Clone and build
git clone https://github.com/asiimwe3/afrih-website.git
cd afrih-website
gcc -O2 -o afrih_server afrih_server.c -lpthread
./afrih_server 8080
```

## Option 3: Behind Nginx Reverse Proxy

```nginx
server {
    listen 80;
    server_name afrih.yourdomain.com;
    
    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

Then with SSL via Certbot:
```bash
certbot --nginx -d afrih.yourdomain.com
```

## Systemd Service

```ini
[Unit]
Description=AFRIH C Web Server
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/afrih
ExecStart=/opt/afrih/afrih_server 8080
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

## Form Submissions

All form submissions are saved to:
- `submissions/` directory as individual CSV files
- `submissions/all_submissions.log` as a single log

Submissions persist across container restarts when using the volume mount in docker-compose.yml.

## Health Check

```bash
curl http://localhost:8080/
```

Returns the home page (HTTP 200) if the server is running.
