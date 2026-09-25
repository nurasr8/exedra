#!/bin/bash
# DESTDIR-install calamares build into a pacman-style tarball for seeding.
set -e
[ -f "$HOME/aurbuild/build/calamares" ] || { echo "binary missing"; exit 1; }
rm -rf "$HOME/aurbuild/pkg"
DESTDIR="$HOME/aurbuild/pkg" ninja -C "$HOME/aurbuild/build" install >/dev/null
rm -rf "$HOME/aurbuild/pkg/usr/share/doc"
rm -f ~/pkgcache/calamares-3.4.2-1.pkg.tar.zst
bsdtar -C "$HOME/aurbuild/pkg" -cf ~/pkgcache/calamares-3.4.2-1.pkg.tar .
zstd -q --rm ~/pkgcache/calamares-3.4.2-1.pkg.tar
zstd -t -q ~/pkgcache/calamares-3.4.2-1.pkg.tar.zst
ls -la ~/pkgcache/calamares-3.4.2-1.pkg.tar.zst
