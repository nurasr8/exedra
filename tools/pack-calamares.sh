#!/bin/bash
# DESTDIR-install calamares build into a pacman-style tarball for seeding.
set -e
[ -f "$HOME/aurbuild/build/src/calamares/calamares" ] || { echo "binary missing"; exit 1; }
rm -rf "$HOME/aurbuild/pkg"
DESTDIR="$HOME/aurbuild/pkg" ninja -C "$HOME/aurbuild/build" install >/dev/null
rm -rf "$HOME/aurbuild/pkg/usr/share/doc"
tar -C "$HOME/aurbuild/pkg" -cf ~/pkgcache/calamares-3.4.2-1.pkg.tar --zstd .
ls -la ~/pkgcache/calamares-3.4.2-1.pkg.tar.zst
