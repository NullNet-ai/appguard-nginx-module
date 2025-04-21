# appguard-nginx-module

## Getting Started

To build and use the `appguard-nginx-module`, ensure the following dependencies are installed on your system.

### 1. Install Required Build Tools

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  gcc \
  g++ \
  make \
  cmake \
  libpcre3 \
  libpcre3-dev \
  zlib1g \
  zlib1g-dev \
  libssl-dev
```

### 2. Download NGINX Source Code

You need the NGINX source code to compile the module alongside it.

[Official NGINX Source Downloads](https://nginx.org/en/download.html)

Example (latest stable as of writing):

```bash
wget https://nginx.org/download/nginx-1.25.3.tar.gz
tar -xzf nginx-1.25.3.tar.gz
cd nginx-1.25.3
```

### 3. Install gRPC and Protobuf Development Libraries

These packages provide the required headers and tools to compile gRPC-based modules in C++.

```bash
sudo apt install -y \
  libgrpc++-dev \
  libprotobuf-dev \
  protobuf-compiler-grpc
```

## Build

This module must be compiled alongside the NGINX source. You can build it either as a **dynamic** or **static** module.

### 1. Clone the Module
```bash
git clone https://github.com/NullNet-ai/appguard-nginx-module
cd appguard-nginx-module
```

### 2. Build as a Dynamic Module

```bash
cd /path/to/nginx-source

./configure \
  --add-dynamic-module=/full/path/to/appguard-nginx-module \
  --with-compat

make modules
```

The compiled `.so` module will be located in `objs/appguard_nginx_module.so`.
You can load it in your NGINX config like this:
```bash
load_module modules/appguard_nginx_module.so;
```

### 3. (Optional) Build as a Static Module
If you prefer a statically linked module:
```bash
cd /path/to/nginx-source

./configure \
  --add-module=/full/path/to/appguard-nginx-module \
  --with-http_ssl_module

make
sudo make install
```
This will compile NGINX with the module built-in.

## Directives
The `appguard-nginx-module` introduces custom directives that can be used in the  `server` context.

| Directive               | Syntax                                      | Default             | Description |
|-------------------------|---------------------------------------------|---------------------|-------------|
| `appguard_enabled`        | `appguard_enabled on \| off`                  | `off`               | Enables or disables AppGuard processing for requests. When enabled, HTTP requests will be evaluated by the AppGuard service. |
| `appguard_tls`           | `appguard_tls on \| off`                     | `off`               | Enables or disables TLS (Transport Layer Security) for gRPC communication with the backend server. When enabled, all communication with the backend will be encrypted. |
| `appguard_server_addr`   | `appguard_server_addr <host>:<port>`        | `""`                | Specifies the address of the gRPC backend server that handles policy decisions. Default is empty, meaning no server is defined until configured. |
| `appguard_app_id`        | `appguard_app_id <id>`                      | `""`                | A unique identifier for your application used for authentication or tracking purposes with the backend server. |
| `appguard_app_secret`    | `appguard_app_secret <secret>`              | `""`                | A secret key associated with the `appguard_app_id` used for authentication with the backend server. |
| `appguard_default_policy`| `appguard_default_policy <allow\|deny>`      | `deny`              | Defines the default policy when no explicit rule matches the request. If set to `allow`, requests that don't match any rules will be allowed; otherwise, they are denied. |
| `appguard_server_cert_path` | `appguard_server_cert_path <path>`        | `""`                | Specifies the file path to the server's certificate (e.g., CA certificate) used for TLS verification when `appguard_tls` is enabled. If left empty, the system's default root CAs will be used for verification. |

---

### Example Configuraiton
```nginx
http {
    server {
        listen 80;

        appguard_enabled on;
        appguard_server_addr localhost:50051;
        appguard_app_id qwerty;
        appguard_app_secret ytrewq;
        appguard_tls on;
        appguard_default_policy allow;
        appguard_server_cert_path /path/to/ca.pem;

        location /secure/ {
            proxy_pass http://backend;
        }
    }
}
```

## Licence
[MIT](https://github.com/NullNet-ai/appguard-nginx-module/tree/main?tab=MIT-1-ov-file#readme)