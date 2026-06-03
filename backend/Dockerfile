FROM debian:bookworm-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    libmysqlclient-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libmicrohttpd-dev \
    libnlohmann-json3-dev \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY app /app/app

RUN g++ -std=c++17 -O2 -Wall /app/app/main.cpp -o /app/backend_app \
    -lmicrohttpd -lmysqlclient -lcurl -lssl -lcrypto -pthread

EXPOSE 8000

CMD ["/app/backend_app"]
