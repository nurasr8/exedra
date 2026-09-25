#!/bin/bash
# Bootstrap: fetch upstream toolchain into /venim/sources, no custom kernel/libc.
set -e
mkdir -p venim/sources venim/build venim/cache venim/db venim/packages
echo "bootstrap: using host kernel/gcc/glibc/systemd as upstream."
which gcc cc make tar xz gzip readelf patchelf 2>/dev/null || true
echo "layout ready: venim/{packages,build,cache,db,sources}"
