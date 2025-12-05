# Reverse Proxy Guide for Zattera Nodes

Complete guide for configuring reverse proxies and load balancers in front of `zatterad` nodes.

## Overview

A **reverse proxy** sits between clients and your `zatterad` node(s), forwarding client requests to the backend and returning responses. This architectural pattern provides:

- **Load balancing** - Distribute traffic across multiple node instances
- **Health checking** - Automatic detection and removal of unhealthy backends
- **Security** - Request filtering, rate limiting, and DDoS protection
- **TLS/SSL termination** - Centralized HTTPS certificate management
- **Performance** - Connection pooling, caching, and compression
- **High availability** - Failover to backup nodes
- **Observability** - Centralized logging and metrics

This guide covers NGINX (recommended) and alternative reverse proxy solutions including HAProxy, Traefik, Caddy, and cloud-native load balancers.

## Architecture

### Without Reverse Proxy

```
Client → zatterad:8090 (WebSocket/HTTP)
```

### With Reverse Proxy

```
Client → Reverse Proxy:8090 → zatterad:8091 (WebSocket/HTTP)
                            ↓
                            /health endpoint
```

### Multi-Instance (Load Balanced)

```
Client → Reverse Proxy:8090 → zatterad-1:8091
                            → zatterad-2:8092
                            → zatterad-3:8093
                            → zatterad-4:8094
```

## NGINX Configuration

### Built-in NGINX Configuration

Zattera includes production-ready NGINX configuration files in the `deploy/nginx/` directory. These configurations are battle-tested and optimized for production use.

#### Features

- **High performance**: 64 worker processes, 10,000 connections per worker
- **WebSocket support**: Proxy upgrade headers
- **CORS headers**: Cross-origin API access
- **HSTS**: HTTP Strict Transport Security
- **Real IP forwarding**: For AWS/cloud environments

#### Docker Deployment with NGINX

Enable the built-in NGINX reverse proxy via environment variable:

```bash
docker run -d \
  --name zattera-node \
  -e USE_NGINX_PROXY=true \
  -p 8090:8090 \
  -p 2001:2001 \
  zatterahub/zattera:latest
```

**Behavior**:
- `zatterad` listens on `127.0.0.1:8091` (internal)
- NGINX listens on `0.0.0.0:8090` (public)
- Health check available at `http://localhost:8090/health`

#### Configuration Files

| File | Purpose |
|------|---------|
| [deploy/nginx/zatterad.nginx.conf](../../deploy/nginx/zatterad.nginx.conf) | Main NGINX configuration |
| [deploy/nginx/zatterad-proxy.conf.template](../../deploy/nginx/zatterad-proxy.conf.template) | NGINX site configuration with reverse proxy and health check endpoint |
| [deploy/zatterad-healthcheck.sh](../../deploy/zatterad-healthcheck.sh) | Health check script (CGI) |

### Docker Compose with NGINX

Run NGINX as a separate container alongside `zatterad`.

#### Basic Setup

`docker-compose.yml`:

```yaml
version: '3.8'

services:
  zatterad:
    image: zatterahub/zattera:latest
    container_name: zatterad-backend
    volumes:
      - zatterad-data:/var/lib/zatterad
    environment:
      - HOME=/var/lib/zatterad
    # Expose only to nginx, not publicly
    expose:
      - "8091"
    networks:
      - zattera-network

  nginx:
    image: nginx:alpine
    container_name: zatterad-nginx
    ports:
      - "8090:8090"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
      - ./zatterad-healthcheck.sh:/usr/local/bin/zatterad-healthcheck.sh:ro
    depends_on:
      - zatterad
    networks:
      - zattera-network

volumes:
  zatterad-data:
    driver: local

networks:
  zattera-network:
    driver: bridge
```

#### NGINX Configuration for Docker

`nginx.conf`:

```nginx
events {
    worker_connections 1024;
}

http {
    upstream zatterad_backend {
        # Use Docker service name
        server zatterad-backend:8091;
    }

    server {
        listen 8090;

        location / {
            proxy_pass http://zatterad_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;

            # WebSocket support
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";

            # CORS
            add_header Access-Control-Allow-Origin "*";
        }

        location /health {
            return 200 "OK\n";
            add_header Content-Type text/plain;
        }
    }
}
```

#### Start Services

```bash
docker-compose up -d

# View logs
docker-compose logs -f nginx
docker-compose logs -f zatterad

# Test API
curl http://localhost:8090

# Check health
curl http://localhost:8090/health
```

#### Multi-Instance Load Balancing with Docker Compose

`docker-compose.yml` with multiple `zatterad` instances:

```yaml
version: '3.8'

services:
  zatterad-1:
    image: zatterahub/zattera:latest
    container_name: zatterad-1
    volumes:
      - zatterad-data-1:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  zatterad-2:
    image: zatterahub/zattera:latest
    container_name: zatterad-2
    volumes:
      - zatterad-data-2:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  zatterad-3:
    image: zatterahub/zattera:latest
    container_name: zatterad-3
    volumes:
      - zatterad-data-3:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  nginx:
    image: nginx:alpine
    container_name: zatterad-nginx
    ports:
      - "8090:8090"
    volumes:
      - ./nginx-lb.conf:/etc/nginx/nginx.conf:ro
    depends_on:
      - zatterad-1
      - zatterad-2
      - zatterad-3
    networks:
      - zattera-network

volumes:
  zatterad-data-1:
  zatterad-data-2:
  zatterad-data-3:

networks:
  zattera-network:
    driver: bridge
```

`nginx-lb.conf`:

```nginx
events {
    worker_connections 1024;
}

http {
    upstream zatterad_cluster {
        least_conn;  # Use least connections algorithm

        server zatterad-1:8090 max_fails=3 fail_timeout=30s;
        server zatterad-2:8090 max_fails=3 fail_timeout=30s;
        server zatterad-3:8090 max_fails=3 fail_timeout=30s;
    }

    server {
        listen 8090;

        location / {
            proxy_pass http://zatterad_cluster;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;

            # WebSocket support
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";

            # Timeouts
            proxy_connect_timeout 60s;
            proxy_send_timeout 60s;
            proxy_read_timeout 60s;
        }
    }
}
```

### Custom NGINX Setup

For non-Docker deployments or custom configurations, set up NGINX manually.

#### Installation

**Ubuntu/Debian**:
```bash
sudo apt-get update
sudo apt-get install -y nginx fcgiwrap
```

**macOS**:
```bash
brew install nginx fcgiwrap
```

**RHEL/CentOS**:
```bash
sudo yum install -y nginx fcgiwrap
```

#### Basic Configuration

Create `/etc/nginx/sites-available/zatterad`:

```nginx
upstream zatterad_backend {
    # Single instance
    server 127.0.0.1:8091;

    # For load balancing, add multiple servers:
    # server 127.0.0.1:8091;
    # server 127.0.0.1:8092;
    # server 127.0.0.1:8093;
    # server 127.0.0.1:8094;
}

server {
    listen 8090;
    server_name _;

    # Main API endpoint
    location / {
        # Disable access log for high traffic
        access_log off;

        # Proxy to zatterad
        proxy_pass http://zatterad_backend;

        # Headers
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;

        # HTTP/1.1 required for WebSocket
        proxy_http_version 1.1;

        # Disable buffering for streaming responses
        proxy_buffering off;

        # WebSocket upgrade
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # CORS headers
        add_header Access-Control-Allow-Origin "*";
        add_header Access-Control-Allow-Methods "GET, POST, OPTIONS";
        add_header Access-Control-Allow-Headers "DNT,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Content-Range,Range";

        # Security headers
        add_header Strict-Transport-Security "max-age=31557600; includeSubDomains; preload" always;
    }

    # Health check endpoint
    location /health {
        access_log off;
        fastcgi_pass unix:/var/run/fcgiwrap.socket;
        fastcgi_param SCRIPT_FILENAME /usr/local/bin/zatterad-healthcheck.sh;
        include fastcgi_params;
    }
}
```

#### Enable Site

```bash
# Create symlink
sudo ln -s /etc/nginx/sites-available/zatterad /etc/nginx/sites-enabled/

# Test configuration
sudo nginx -t

# Reload NGINX
sudo systemctl reload nginx
```

### Health Check Script

Copy [deploy/zatterad-healthcheck.sh](../../deploy/zatterad-healthcheck.sh) to `/usr/local/bin/zatterad-healthcheck.sh`:

```bash
sudo cp deploy/zatterad-healthcheck.sh /usr/local/bin/zatterad-healthcheck.sh
sudo chmod +x /usr/local/bin/zatterad-healthcheck.sh
```

**How it works**:
1. Queries `database_api.get_dynamic_global_properties` from `zatterad`
2. Checks if latest block timestamp is within 60 seconds of current time
3. Returns HTTP status:
   - `200`: Healthy (block age ≤ 60 seconds)
   - `502`: Node not responding
   - `503`: Node responding but blockchain stale (block age > 60 seconds)

**Usage**:
```bash
# Check node health
curl http://localhost:8090/health

# Output examples:
# Healthy: "Block age is less than 60 seconds old, this node is considered healthy."
# Stale: "The node is responding but block chain age is 125 seconds old"
# Down: "The node is currently not responding."
```

### Advanced NGINX Configuration

#### Rate Limiting

Protect against API abuse:

```nginx
# Define rate limit zones (outside server block)
limit_req_zone $binary_remote_addr zone=api_limit:10m rate=10r/s;
limit_req_zone $binary_remote_addr zone=health_limit:1m rate=1r/s;

server {
    listen 8090;

    location / {
        # Allow burst of 20 requests, delay excess
        limit_req zone=api_limit burst=20 nodelay;

        proxy_pass http://zatterad_backend;
        # ... (rest of config)
    }

    location /health {
        # Rate limit health checks
        limit_req zone=health_limit burst=5;

        fastcgi_pass unix:/var/run/fcgiwrap.socket;
        fastcgi_param SCRIPT_FILENAME /usr/local/bin/zatterad-healthcheck.sh;
    }
}
```

#### IP Whitelisting

Restrict access to specific IPs:

```nginx
# Whitelist configuration
geo $allowed_ip {
    default 0;
    192.168.1.0/24 1;
    10.0.0.0/8 1;
}

server {
    listen 8090;

    location / {
        if ($allowed_ip = 0) {
            return 403;
        }

        proxy_pass http://zatterad_backend;
        # ... (rest of config)
    }
}
```

#### TLS/SSL Configuration

Enable HTTPS:

```nginx
server {
    listen 443 ssl http2;
    server_name api.example.com;

    # SSL certificate
    ssl_certificate /etc/letsencrypt/live/api.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/api.example.com/privkey.pem;

    # SSL configuration
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;

    # SSL session cache
    ssl_session_cache shared:SSL:10m;
    ssl_session_timeout 10m;

    location / {
        proxy_pass http://zatterad_backend;
        # ... (rest of config)
    }
}

# Redirect HTTP to HTTPS
server {
    listen 80;
    server_name api.example.com;
    return 301 https://$server_name$request_uri;
}
```

#### Load Balancing Strategies

**Round Robin** (default):
```nginx
upstream zatterad_backend {
    server 127.0.0.1:8091;
    server 127.0.0.1:8092;
    server 127.0.0.1:8093;
}
```

**Least Connections**:
```nginx
upstream zatterad_backend {
    least_conn;
    server 127.0.0.1:8091;
    server 127.0.0.1:8092;
    server 127.0.0.1:8093;
}
```

**IP Hash** (session persistence):
```nginx
upstream zatterad_backend {
    ip_hash;
    server 127.0.0.1:8091;
    server 127.0.0.1:8092;
    server 127.0.0.1:8093;
}
```

**Weighted**:
```nginx
upstream zatterad_backend {
    server 127.0.0.1:8091 weight=3;  # 60% of traffic
    server 127.0.0.1:8092 weight=2;  # 40% of traffic
}
```

**Health Checks** (NGINX Plus only):
```nginx
upstream zatterad_backend {
    server 127.0.0.1:8091;
    server 127.0.0.1:8092 max_fails=3 fail_timeout=30s;
    server 127.0.0.1:8093 backup;  # Only used if others fail
}
```

#### Logging Configuration

```nginx
# Custom log format
log_format zatterad_log '$remote_addr - $remote_user [$time_local] '
                      '"$request" $status $body_bytes_sent '
                      '"$http_referer" "$http_user_agent" '
                      'rt=$request_time uct="$upstream_connect_time" '
                      'uht="$upstream_header_time" urt="$upstream_response_time"';

server {
    listen 8090;

    # Access log with timing
    access_log /var/log/nginx/zatterad_access.log zatterad_log;

    # Error log
    error_log /var/log/nginx/zatterad_error.log warn;

    location / {
        proxy_pass http://zatterad_backend;
        # ...
    }
}
```

## Alternative Reverse Proxy Solutions

### HAProxy

HAProxy is a high-performance, open-source TCP/HTTP load balancer and reverse proxy known for reliability and efficiency.

#### Docker Deployment

`docker-compose.yml`:

```yaml
version: '3.8'

services:
  zatterad-1:
    image: zatterahub/zattera:latest
    container_name: zatterad-1
    volumes:
      - zatterad-data-1:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  zatterad-2:
    image: zatterahub/zattera:latest
    container_name: zatterad-2
    volumes:
      - zatterad-data-2:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  haproxy:
    image: haproxy:2.8-alpine
    container_name: zatterad-haproxy
    ports:
      - "8090:8090"
      - "8404:8404"  # Stats page
    volumes:
      - ./haproxy.cfg:/usr/local/etc/haproxy/haproxy.cfg:ro
    depends_on:
      - zatterad-1
      - zatterad-2
    networks:
      - zattera-network

volumes:
  zatterad-data-1:
  zatterad-data-2:

networks:
  zattera-network:
    driver: bridge
```

`haproxy.cfg`:

```haproxy
global
    log stdout format raw local0
    maxconn 4096

defaults
    log global
    mode http
    option httplog
    option dontlognull
    timeout connect 5s
    timeout client 50s
    timeout server 50s

# Statistics
listen stats
    bind *:8404
    stats enable
    stats uri /stats
    stats refresh 10s

# Frontend
frontend zatterad_frontend
    bind *:8090
    default_backend zatterad_backend

# Backend with health checks
backend zatterad_backend
    balance roundrobin
    option httpchk GET /
    http-check expect status 200

    server zatterad-1 zatterad-1:8090 check inter 5s rise 2 fall 3
    server zatterad-2 zatterad-2:8090 check inter 5s rise 2 fall 3
```

Start services:

```bash
docker-compose up -d

# View stats
open http://localhost:8404/stats

# Test API
curl http://localhost:8090
```

#### Installation (Standalone)

```bash
sudo apt-get install -y haproxy
```

#### Configuration (Standalone)

`/etc/haproxy/haproxy.cfg`:

```haproxy
global
    log /dev/log local0
    log /dev/log local1 notice
    maxconn 4096
    user haproxy
    group haproxy
    daemon

defaults
    log global
    mode http
    option httplog
    option dontlognull
    timeout connect 5000
    timeout client 50000
    timeout server 50000

# Statistics page
listen stats
    bind *:8404
    stats enable
    stats uri /stats
    stats refresh 30s
    stats auth admin:password

# Frontend
frontend zatterad_frontend
    bind *:8090
    mode http
    default_backend zatterad_backend

# Backend
backend zatterad_backend
    mode http
    balance roundrobin
    option httpchk GET /health
    http-check expect status 200

    server node1 127.0.0.1:8091 check inter 5s rise 2 fall 3
    server node2 127.0.0.1:8092 check inter 5s rise 2 fall 3
    server node3 127.0.0.1:8093 check inter 5s rise 2 fall 3
    server node4 127.0.0.1:8094 check inter 5s rise 2 fall 3 backup
```

#### Start HAProxy

```bash
sudo systemctl enable haproxy
sudo systemctl start haproxy

# Check status
sudo systemctl status haproxy

# View stats
open http://localhost:8404/stats
```

### Traefik

Traefik is a modern, cloud-native reverse proxy and load balancer with automatic service discovery, perfect for containerized environments.

#### Docker Compose Configuration

`docker-compose.yml`:

```yaml
version: '3.8'

services:
  traefik:
    image: traefik:v2.10
    command:
      - "--api.insecure=true"
      - "--providers.docker=true"
      - "--providers.docker.exposedbydefault=false"
      - "--entrypoints.web.address=:8090"
    ports:
      - "8090:8090"
      - "8080:8080"  # Dashboard
    volumes:
      - /var/run/docker.sock:/var/run/docker.sock:ro
    networks:
      - zattera-network

  zatterad-1:
    image: zatterahub/zattera:latest
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.zatterad.rule=PathPrefix(`/`)"
      - "traefik.http.routers.zatterad.entrypoints=web"
      - "traefik.http.services.zatterad.loadbalancer.server.port=8090"
      - "traefik.http.services.zatterad.loadbalancer.healthcheck.path=/health"
      - "traefik.http.services.zatterad.loadbalancer.healthcheck.interval=10s"
    networks:
      - zattera-network

  zatterad-2:
    image: zatterahub/zattera:latest
    labels:
      - "traefik.enable=true"
      - "traefik.http.routers.zatterad.rule=PathPrefix(`/`)"
      - "traefik.http.routers.zatterad.entrypoints=web"
      - "traefik.http.services.zatterad.loadbalancer.server.port=8090"
    networks:
      - zattera-network

networks:
  zattera-network:
    driver: bridge
```

Start services:

```bash
docker-compose up -d

# View dashboard
open http://localhost:8080
```

### Caddy

Caddy is a modern web server with automatic HTTPS via Let's Encrypt, offering simple configuration and powerful reverse proxy features.

#### Docker Deployment

`docker-compose.yml`:

```yaml
version: '3.8'

services:
  zatterad-1:
    image: zatterahub/zattera:latest
    container_name: zatterad-1
    volumes:
      - zatterad-data-1:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  zatterad-2:
    image: zatterahub/zattera:latest
    container_name: zatterad-2
    volumes:
      - zatterad-data-2:/var/lib/zatterad
    expose:
      - "8090"
    networks:
      - zattera-network

  caddy:
    image: caddy:2-alpine
    container_name: zatterad-caddy
    ports:
      - "8090:8090"
      - "443:443"   # HTTPS (if using domain)
      - "2019:2019" # Admin API
    volumes:
      - ./Caddyfile:/etc/caddy/Caddyfile:ro
      - caddy-data:/data
      - caddy-config:/config
    depends_on:
      - zatterad-1
      - zatterad-2
    networks:
      - zattera-network

volumes:
  zatterad-data-1:
  zatterad-data-2:
  caddy-data:
  caddy-config:

networks:
  zattera-network:
    driver: bridge
```

`Caddyfile`:

```caddy
# HTTP load balancing
:8090 {
    reverse_proxy zatterad-1:8090 zatterad-2:8090 {
        lb_policy round_robin
        health_uri /
        health_interval 10s
        health_timeout 5s
    }

    # CORS headers
    header Access-Control-Allow-Origin "*"
    header Access-Control-Allow-Methods "GET, POST, OPTIONS"

    # Logging
    log {
        output stdout
        format json
    }
}

# HTTPS with automatic certificates (if using domain)
# api.example.com {
#     reverse_proxy zatterad-1:8090 zatterad-2:8090 {
#         lb_policy least_conn
#     }
# }
```

Start services:

```bash
docker-compose up -d

# View logs
docker-compose logs -f caddy

# Test API
curl http://localhost:8090

# View Caddy admin API
curl http://localhost:2019/config/
```

#### Installation (Standalone)

```bash
# Ubuntu/Debian
sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list
sudo apt update
sudo apt install caddy
```

#### Configuration

`/etc/caddy/Caddyfile`:

```caddy
# Basic reverse proxy
:8090 {
    reverse_proxy localhost:8091
}

# With automatic HTTPS
api.example.com {
    reverse_proxy localhost:8091
}

# Load balancing
:8090 {
    reverse_proxy localhost:8091 localhost:8092 localhost:8093 {
        lb_policy round_robin
        health_uri /health
        health_interval 10s
        health_timeout 5s
    }
}

# Advanced configuration
:8090 {
    # CORS
    header Access-Control-Allow-Origin "*"
    header Access-Control-Allow-Methods "GET, POST, OPTIONS"

    # Logging
    log {
        output file /var/log/caddy/zatterad.log
        format json
    }

    # Rate limiting (requires plugin)
    # rate_limit {
    #     zone dynamic {
    #         rate 100r/s
    #     }
    # }

    reverse_proxy localhost:8091
}
```

#### Start Caddy

```bash
sudo systemctl enable caddy
sudo systemctl start caddy

# Reload after config changes
sudo systemctl reload caddy
```

## Cloud-Native Load Balancers

Cloud providers offer managed load balancing services that eliminate the need to maintain reverse proxy infrastructure.

### AWS Elastic Load Balancer (ELB)

#### Application Load Balancer (ALB)

Best choice for HTTP/WebSocket traffic with advanced routing capabilities.

**Key Features**:
- Layer 7 (HTTP/HTTPS) load balancing with content-based routing
- Native WebSocket and HTTP/2 support
- Path-based and host-based routing
- Integrated health checks with custom endpoints
- Auto Scaling group integration
- AWS WAF integration for DDoS protection

**Setup** (AWS CLI):

```bash
# Create target group
aws elbv2 create-target-group \
    --name zatterad-targets \
    --protocol HTTP \
    --port 8090 \
    --vpc-id vpc-xxxxx \
    --health-check-path /health \
    --health-check-interval-seconds 30 \
    --healthy-threshold-count 2 \
    --unhealthy-threshold-count 3

# Create load balancer
aws elbv2 create-load-balancer \
    --name zatterad-alb \
    --subnets subnet-xxxxx subnet-yyyyy \
    --security-groups sg-xxxxx

# Register targets
aws elbv2 register-targets \
    --target-group-arn arn:aws:elasticloadbalancing:... \
    --targets Id=i-instance1 Id=i-instance2
```

#### Network Load Balancer (NLB)

Optimized for high-performance TCP traffic with ultra-low latency.

**Key Features**:
- Layer 4 (TCP/UDP) load balancing
- Ultra-low latency (<100μs)
- Static IP addresses per Availability Zone
- Extreme performance (millions of requests/second)
- Preserves source IP address

**Best Use Case**: P2P endpoint load balancing (port 2001) for witness nodes

### Google Cloud Load Balancing

```bash
# Create health check
gcloud compute health-checks create http zatterad-health \
    --port 8090 \
    --request-path /health \
    --check-interval 10s

# Create backend service
gcloud compute backend-services create zatterad-backend \
    --protocol HTTP \
    --health-checks zatterad-health \
    --global

# Add instances to backend
gcloud compute backend-services add-backend zatterad-backend \
    --instance-group zatterad-instances \
    --instance-group-zone us-central1-a \
    --global

# Create load balancer
gcloud compute url-maps create zatterad-lb \
    --default-service zatterad-backend

gcloud compute target-http-proxies create zatterad-proxy \
    --url-map zatterad-lb

gcloud compute forwarding-rules create zatterad-rule \
    --global \
    --target-http-proxy zatterad-proxy \
    --ports 80
```

### Azure Load Balancer

```bash
# Create load balancer
az network lb create \
    --resource-group myResourceGroup \
    --name zatterad-lb \
    --sku Standard \
    --frontend-ip-name zatterad-frontend

# Create health probe
az network lb probe create \
    --resource-group myResourceGroup \
    --lb-name zatterad-lb \
    --name zatterad-health \
    --protocol http \
    --port 8090 \
    --path /health

# Create backend pool
az network lb address-pool create \
    --resource-group myResourceGroup \
    --lb-name zatterad-lb \
    --name zatterad-backend-pool

# Create load balancing rule
az network lb rule create \
    --resource-group myResourceGroup \
    --lb-name zatterad-lb \
    --name zatterad-rule \
    --protocol tcp \
    --frontend-port 8090 \
    --backend-port 8090 \
    --frontend-ip-name zatterad-frontend \
    --backend-pool-name zatterad-backend-pool \
    --probe-name zatterad-health
```

## Kubernetes Ingress

Kubernetes Ingress provides HTTP/HTTPS routing to services within a cluster, acting as a reverse proxy at the cluster edge.

### NGINX Ingress Controller

The most popular Ingress controller, based on NGINX.

`zatterad-ingress.yaml`:

```yaml
apiVersion: v1
kind: Service
metadata:
  name: zatterad-service
spec:
  selector:
    app: zatterad
  ports:
    - protocol: TCP
      port: 8090
      targetPort: 8090
  type: ClusterIP

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: zatterad-ingress
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
    nginx.ingress.kubernetes.io/websocket-services: zatterad-service
    nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
    # CORS
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "*"
    # Rate limiting
    nginx.ingress.kubernetes.io/limit-rps: "100"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: zatterad-service
            port:
              number: 8090
  tls:
  - hosts:
    - api.example.com
    secretName: zatterad-tls
```

### Health Check Configuration

`zatterad-deployment.yaml`:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: zatterad
spec:
  replicas: 3
  selector:
    matchLabels:
      app: zatterad
  template:
    metadata:
      labels:
        app: zatterad
    spec:
      containers:
      - name: zatterad
        image: zatterahub/zattera:latest
        ports:
        - containerPort: 8090
        livenessProbe:
          httpGet:
            path: /health
            port: 8090
          initialDelaySeconds: 300
          periodSeconds: 30
          timeoutSeconds: 5
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /health
            port: 8090
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
```

Apply configuration:

```bash
kubectl apply -f zatterad-deployment.yaml
kubectl apply -f zatterad-ingress.yaml

# Check status
kubectl get ingress
kubectl get pods
kubectl describe ingress zatterad-ingress
```

## Reverse Proxy Performance Tuning

### NGINX Optimization

Fine-tune NGINX for high-traffic API nodes.

#### Worker Configuration

```nginx
# Auto-detect CPU cores
worker_processes auto;

# Increase file descriptor limit
worker_rlimit_nofile 65535;

events {
    # Connections per worker
    worker_connections 10000;

    # Use efficient event model
    use epoll;  # Linux
    # use kqueue;  # BSD/macOS

    # Accept multiple connections
    multi_accept on;
}
```

#### Connection Settings

```nginx
http {
    # Disable keepalive for API traffic (stateless)
    keepalive_timeout 0;

    # Or set short timeout
    # keepalive_timeout 65;
    # keepalive_requests 100;

    # Send file optimization
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;

    # Buffer sizes
    client_body_buffer_size 128k;
    client_max_body_size 10m;
    client_header_buffer_size 1k;
    large_client_header_buffers 4 4k;
    output_buffers 1 32k;
    postpone_output 1460;
}
```

#### Proxy Buffers

```nginx
location / {
    # Disable buffering for WebSocket and streaming
    proxy_buffering off;

    # Or configure buffers for non-streaming
    # proxy_buffer_size 4k;
    # proxy_buffers 8 4k;
    # proxy_busy_buffers_size 8k;

    # Timeouts
    proxy_connect_timeout 60s;
    proxy_send_timeout 60s;
    proxy_read_timeout 60s;

    proxy_pass http://zatterad_backend;
}
```

#### Caching (for specific endpoints)

```nginx
# Cache configuration
proxy_cache_path /var/cache/nginx/zatterad levels=1:2 keys_zone=zatterad_cache:10m max_size=1g inactive=60m;

server {
    location / {
        # Cache GET requests for specific methods
        proxy_cache zatterad_cache;
        proxy_cache_methods GET;
        proxy_cache_key "$request_uri";
        proxy_cache_valid 200 5m;

        # Cache headers
        add_header X-Cache-Status $upstream_cache_status;

        proxy_pass http://zatterad_backend;
    }
}
```

### Operating System Tuning

#### Linux Kernel Parameters

Optimize kernel settings for high-concurrency reverse proxy workloads.

`/etc/sysctl.conf`:

```conf
# Increase connection tracking
net.netfilter.nf_conntrack_max = 1048576

# TCP settings
net.ipv4.tcp_max_syn_backlog = 8192
net.core.somaxconn = 8192
net.core.netdev_max_backlog = 5000

# File descriptors
fs.file-max = 2097152

# Ephemeral ports
net.ipv4.ip_local_port_range = 1024 65535

# TCP keepalive
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 60
net.ipv4.tcp_keepalive_probes = 3
```

Apply changes:

```bash
sudo sysctl -p
```

#### File Descriptor Limits

`/etc/security/limits.conf`:

```conf
* soft nofile 65535
* hard nofile 65535
nginx soft nofile 65535
nginx hard nofile 65535
```

## Reverse Proxy Monitoring and Observability

### NGINX Status Module

Enable real-time metrics collection from NGINX.

Enable stub_status module:

```nginx
server {
    listen 8091;
    server_name localhost;

    location /nginx_status {
        stub_status on;
        access_log off;
        allow 127.0.0.1;
        deny all;
    }
}
```

Query status:

```bash
curl http://localhost:8091/nginx_status

# Output:
# Active connections: 291
# server accepts handled requests
#  16630948 16630948 31070465
# Reading: 6 Writing: 179 Waiting: 106
```

### Prometheus Integration

Export NGINX metrics to Prometheus for monitoring and alerting.

Install NGINX Prometheus exporter:

```bash
docker run -d \
  --name nginx-exporter \
  -p 9113:9113 \
  nginx/nginx-prometheus-exporter:0.11 \
  -nginx.scrape-uri=http://nginx:8091/nginx_status
```

`prometheus.yml`:

```yaml
scrape_configs:
  - job_name: 'nginx'
    static_configs:
      - targets: ['localhost:9113']
```

### Centralized Logging with ELK Stack

Ship NGINX logs to Elasticsearch for analysis and visualization.

**Pipeline**: NGINX → Filebeat → Logstash → Elasticsearch → Kibana

`filebeat.yml`:

```yaml
filebeat.inputs:
- type: log
  enabled: true
  paths:
    - /var/log/nginx/zatterad_access.log
  fields:
    type: nginx-access

- type: log
  enabled: true
  paths:
    - /var/log/nginx/zatterad_error.log
  fields:
    type: nginx-error

output.logstash:
  hosts: ["logstash:5044"]
```

## Troubleshooting Reverse Proxy Issues

### Connection Problems

**Error**: `502 Bad Gateway`

**Causes**:
- `zatterad` not running or crashed
- Wrong backend port
- Firewall blocking connection

**Debug**:
```bash
# Check zatterad is listening
netstat -tlnp | grep 8091
# or
ss -tlnp | grep 8091

# Test direct connection
curl -X POST http://localhost:8091 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}'

# Check NGINX error log
tail -f /var/log/nginx/error.log

# Test NGINX config
sudo nginx -t
```

**Error**: `504 Gateway Timeout`

**Causes**:
- `zatterad` responding slowly
- Timeout values too low
- Network latency

**Solution**:
```nginx
location / {
    proxy_connect_timeout 300s;
    proxy_send_timeout 300s;
    proxy_read_timeout 300s;
    proxy_pass http://zatterad_backend;
}
```

### WebSocket Connection Failures

**Symptoms**: WebSocket upgrade fails, falls back to polling

**Check**:
```bash
# Test WebSocket connection
wscat -c ws://localhost:8090

# Check upgrade headers in NGINX logs
tail -f /var/log/nginx/access.log | grep Upgrade
```

**Fix**:
```nginx
location / {
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    # ...
}
```

### High CPU/Memory Usage

**NGINX consuming high resources**:

```bash
# Check worker processes
ps aux | grep nginx

# Monitor connections
watch -n 1 'netstat -an | grep :8090 | wc -l'

# Check logs for errors
tail -f /var/log/nginx/error.log
```

**Solutions**:
- Reduce `worker_processes` if over-provisioned
- Enable access log only for errors: `access_log off;`
- Implement rate limiting
- Check for DDoS/abuse

### Health Check Failures

**Symptom**: `/health` endpoint returns 502/503

**Debug**:
```bash
# Test health check directly
/usr/local/bin/zatterad-healthcheck.sh

# Check dependencies
which curl jq

# Check fcgiwrap
systemctl status fcgiwrap
ls -la /var/run/fcgiwrap.socket
```

**Common issues**:
- `curl` or `jq` not installed
- `fcgiwrap` not running
- Incorrect socket path
- Health check script not executable

## Reverse Proxy Best Practices

### Security

1. **Implement rate limiting** - Prevent API abuse and DDoS attacks with per-IP request limits
2. **Use IP whitelisting** - Restrict access to trusted networks when possible
3. **Enable TLS/SSL** - Always use HTTPS in production with strong cipher suites
4. **Hide server information** - Set `server_tokens off;` in NGINX to prevent version disclosure
5. **Configure firewall rules** - Only expose necessary ports (80, 443, 8090)
6. **Implement WAF** - Use Web Application Firewall for advanced threat protection
7. **Regular security updates** - Keep reverse proxy software patched

### Reliability and High Availability

1. **Configure health checks** - Automatically detect and remove unhealthy backends
2. **Implement graceful degradation** - Use backup servers and fallback strategies
3. **Enable circuit breakers** - Fail fast on unhealthy backends to prevent cascading failures
4. **Use multiple availability zones** - Distribute backends across regions/zones
5. **Deploy redundant proxies** - Run multiple reverse proxy instances (e.g., keepalived for HA)
6. **Implement connection draining** - Gracefully shutdown backends without dropping requests
7. **Test failover scenarios** - Regular disaster recovery drills

### Performance Optimization

1. **Enable connection pooling** - Reuse upstream connections to reduce latency
2. **Implement caching** - Cache immutable responses (historical blocks, static data)
3. **Enable compression** - Use gzip for JSON responses to reduce bandwidth
4. **Use CDN** - CloudFlare/Fastly for global distribution and edge caching
5. **Tune buffer sizes** - Match buffer configuration to traffic patterns
6. **Optimize keepalive** - Balance between connection reuse and resource consumption
7. **Monitor performance metrics** - Track latency, throughput, and error rates

### Operational Excellence

1. **Use configuration management** - Ansible/Terraform/Chef for infrastructure as code
2. **Version control configs** - Track all configuration changes in Git
3. **Automated testing** - Validate configs before deployment (e.g., `nginx -t`)
4. **Implement rolling updates** - Zero-downtime deployments with gradual rollout
5. **Maintain runbooks** - Document procedures for common operations and incidents
6. **Enable observability** - Comprehensive logging, metrics, and tracing
7. **Regular capacity planning** - Monitor growth and scale proactively

## Reverse Proxy Solution Comparison

Choose the right reverse proxy based on your deployment environment and requirements.

| Feature | NGINX | HAProxy | Traefik | Caddy | Cloud LB |
|---------|-------|---------|---------|-------|----------|
| **Ease of Setup** | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Performance** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **WebSocket Support** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Load Balancing** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Health Checks** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Auto HTTPS** | ❌ | ❌ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Service Discovery** | ❌ | ❌ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Cost** | Free (OSS) | Free (OSS) | Free (OSS) | Free (OSS) | Pay per use |
| **Maturity** | 20+ years | 20+ years | 8 years | 8 years | Varies |
| **Community** | Very Large | Very Large | Large | Medium | N/A |
| **Best Use Case** | General purpose, high traffic | TCP/HTTP HA, observability | Containers, K8s, microservices | Simple setup, auto HTTPS | Cloud-native, fully managed |
| **Learning Curve** | Medium | Medium | Low | Very Low | Low |

### Recommendations by Scenario

**High-traffic production API node**:
- **1st choice**: NGINX (battle-tested, high performance)
- **2nd choice**: HAProxy (excellent observability and health checks)

**Docker/Kubernetes deployment**:
- **1st choice**: Traefik (automatic service discovery)
- **2nd choice**: Kubernetes Ingress with NGINX controller

**Simple setup with automatic HTTPS**:
- **1st choice**: Caddy (zero-config HTTPS)
- **2nd choice**: Traefik

**Cloud-native deployment**:
- **1st choice**: Cloud provider load balancer (AWS ALB/NLB, GCP LB, Azure LB)
- **2nd choice**: NGINX on cloud VMs

**P2P witness node (Layer 4)**:
- **1st choice**: HAProxy (TCP mode with health checks)
- **2nd choice**: AWS NLB (if on AWS)

**Budget-constrained**:
- **1st choice**: NGINX (free, low resource usage)
- **2nd choice**: Caddy (free, easy to maintain)

## Additional Resources

- [Quick Start Guide](../getting-started/quick-start.md) - Get started quickly with Docker
- [Building Guide](../getting-started/building.md) - Build instructions for all platforms
- [Node Modes Guide](node-modes-guide.md) - Different node types and configurations
- [Genesis Launch Guide](genesis-launch.md) - Launch your own Zattera network
- [Configuration Examples](../../configs/README.md) - Ready-to-use configuration files
- [Deployment Scripts](../../deploy/README.md) - Docker and deployment configurations
- [NGINX Documentation](https://nginx.org/en/docs/) - Official NGINX documentation
- [HAProxy Documentation](https://docs.haproxy.org/) - Official HAProxy documentation

## License

See [LICENSE.md](../LICENSE.md)
