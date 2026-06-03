#!/bin/bash

echo "=== Buscando libmicrohttpd ==="
find /usr -name "*microhttpd*" 2>/dev/null

echo -e "\n=== Buscando libmariadb ==="
find /usr -name "*mariadb*" 2>/dev/null

echo -e "\n=== Buscando libcurl ==="
find /usr -name "*curl*" 2>/dev/null

echo -e "\n=== Buscando libssl ==="
find /usr -name "*ssl*" 2>/dev/null

echo -e "\n=== Verificando pkg-config ==="
pkg-config --list-all | grep -E "microhttpd|mariadb|curl|openssl"

echo -e "\n=== Intentando compilar ==="
g++ -std=c++17 -O2 -Wall /app/app/main.cpp -o /app/backend_app \
    -lmicrohttpd -lmariadb -lcurl -lssl -lcrypto -pthread -v

echo -e "\n=== Compilación completada ==="
