# NGINX Reverse Proxy Configuration

This directory contains NGINX configuration files for reverse proxying zatterad WebSocket and HTTP API endpoints.

## Configuration Files

### zatterad.nginx.conf

Global NGINX configuration file that replaces `/etc/nginx/nginx.conf`.

**Features:**
- High-performance worker configuration (64 workers, 10000 connections per worker)
- WebSocket-optimized settings (keepalive_timeout 0, proxy_ignore_client_abort on)
- Real IP detection for load balancers (X-Forwarded-For)
- Gzip compression
- Includes sites-enabled and conf.d directories

**Key Settings:**
```nginx
user www-data;
worker_processes 64;
worker_rlimit_nofile 10000;

events {
    worker_connections 10000;
}

http {
    sendfile on;
    keepalive_timeout 0;
    server_tokens off;
    gzip on;
    proxy_ignore_client_abort on;

    include /etc/nginx/sites-enabled/*;
}
```

**Deployment:**
This file is copied to `/etc/nginx/nginx.conf` during PaaS bootstrap:
```bash
# Backup original config
mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf

# Install zatterad nginx config as global config
cp /etc/nginx/zatterad.nginx.conf /etc/nginx/nginx.conf
```

### zatterad-proxy.conf.template

NGINX site configuration template for proxying requests to zatterad backend.

**Features:**
- Listens on port 8090 for incoming connections
- WebSocket connection proxy support
- Health check endpoint at `/health` (via fcgiwrap + zatterad-healthcheck.sh)
- CORS headers for cross-origin requests
- HSTS security headers
- Upstream configuration (dynamically completed during startup)

**Template Structure:**
```nginx
server {
    listen 0.0.0.0:8090;

    location / {
        proxy_pass http://ws;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        # CORS and security headers
    }

    location /health {
        fastcgi_pass unix:/var/run/fcgiwrap.socket;
        fastcgi_param SCRIPT_FILENAME /usr/local/bin/zatterad-healthcheck.sh;
    }
}

upstream ws {
    # Dynamically added during bootstrap
}
```

**Dynamic Configuration:**

The template is incomplete by design. During PaaS bootstrap, the upstream block is completed:

```bash
# Copy template
cp /etc/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf

# Add upstream server(s)
echo "server 127.0.0.1:8091;" >> /etc/nginx/zatterad-proxy.conf
echo "}" >> /etc/nginx/zatterad-proxy.conf

# Install as default site
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/zatterad-proxy.conf /etc/nginx/sites-enabled/default
```

For multicore readonly mode, add multiple upstream servers:
```bash
echo "server 127.0.0.1:8091;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8092;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8093;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8094;" >> /etc/nginx/zatterad-proxy.conf
echo "}" >> /etc/nginx/zatterad-proxy.conf
```

## Deployment Scenarios

### PaaS Environment (AWS Elastic Beanstalk)

This is the primary use case. The [zatterad-paas-bootstrap.sh](../zatterad-paas-bootstrap.sh) script automatically:

1. Backs up original `/etc/nginx/nginx.conf`
2. Copies `zatterad.nginx.conf` to `/etc/nginx/nginx.conf` (global config)
3. Generates site config from `zatterad-proxy.conf.template`
4. Adds upstream server(s) dynamically
5. Installs as `/etc/nginx/sites-enabled/default`
6. Restarts fcgiwrap and nginx services

**Example flow from bootstrap script:**
```bash
# Replace global nginx config
mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/zatterad.nginx.conf /etc/nginx/nginx.conf

# Generate site config with upstream
cp /etc/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8091;" >> /etc/nginx/zatterad-proxy.conf
echo "}" >> /etc/nginx/zatterad-proxy.conf

# Install and restart
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/zatterad-proxy.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
```

### Manual Deployment

For non-PaaS environments, manually configure:

1. **Install dependencies:**
```bash
apt-get install nginx fcgiwrap
```

2. **Install global config:**
```bash
mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp deploy/nginx/zatterad.nginx.conf /etc/nginx/nginx.conf
```

3. **Create site config from template:**
```bash
cp deploy/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8091;" >> /etc/nginx/zatterad-proxy.conf
echo "}" >> /etc/nginx/zatterad-proxy.conf
```

4. **Install site config:**
```bash
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/zatterad-proxy.conf /etc/nginx/sites-enabled/default
```

5. **Test and reload:**
```bash
nginx -t
systemctl restart fcgiwrap
systemctl reload nginx
```

### Multicore Readonly Mode

For high-traffic read-only API nodes, add multiple upstream servers:

```bash
# Generate config with multiple backends
cp deploy/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8091;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8092;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8093;" >> /etc/nginx/zatterad-proxy.conf
echo "server 127.0.0.1:8094;" >> /etc/nginx/zatterad-proxy.conf
echo "}" >> /etc/nginx/zatterad-proxy.conf

# Install and reload
cp /etc/nginx/zatterad-proxy.conf /etc/nginx/sites-enabled/default
nginx -t && systemctl reload nginx
```

NGINX will automatically load balance across all backend instances.

## Health Check Endpoint

The `/health` endpoint provides container orchestration and load balancer health checks.

**Request:**
```bash
curl http://localhost/health
```

**Response:**
- `200 OK` - Node is healthy and synced
- `503 Service Unavailable` - Node is behind or unhealthy

**Integration:**
- AWS ELB/ALB target health checks
- Kubernetes liveness/readiness probes
- Docker health checks
- Monitoring systems (Prometheus, Datadog, etc.)

## Performance Tuning

### Connection Limits

```nginx
# Increase worker connections
events {
    worker_connections 4096;
}

# Upstream keepalive
upstream zatterad {
    server 127.0.0.1:8090;
    keepalive 64;
}
```

### Rate Limiting

```nginx
# Define rate limit zone
limit_req_zone $binary_remote_addr zone=api:10m rate=10r/s;

# Apply to location
location / {
    limit_req zone=api burst=20 nodelay;
}
```

### Caching (GET Requests Only)

```nginx
# Cache configuration
proxy_cache_path /var/cache/nginx levels=1:2 keys_zone=api_cache:10m max_size=1g;

location / {
    proxy_cache api_cache;
    proxy_cache_valid 200 1m;
    proxy_cache_methods GET HEAD;
}
```

## Troubleshooting

### WebSocket Connection Issues

Ensure HTTP/1.1 upgrade headers are set:
```nginx
proxy_http_version 1.1;
proxy_set_header Upgrade $http_upgrade;
proxy_set_header Connection "upgrade";
```

### 502 Bad Gateway

Check zatterad backend is running:
```bash
curl http://127.0.0.1:8090
netstat -tlnp | grep 8090
```

### Health Check Failing

Test health check script directly:
```bash
/deploy/zatterad-healthcheck.sh
echo $?  # Should be 0 for healthy
```

## Additional Resources

- [NGINX Documentation](https://nginx.org/en/docs/)
- [WebSocket Proxying](https://nginx.org/en/docs/http/websocket.html)
- [Main Deploy README](../README.md)
- [DDoS Protection Guide](../../docs/operations/ddos-protection.md)
- [Reverse Proxy Guide](../../docs/operations/reverse-proxy-guide.md)
